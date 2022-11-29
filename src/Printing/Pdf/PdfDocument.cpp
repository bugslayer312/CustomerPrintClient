#include "PdfDocument.h"

#include "../../Core/Log.h"
#include "IPdfPasswordProvider.h"
#include "PdfError.h"

#include <fpdfview.h>
#include <fpdf_dataavail.h>
#include <fpdf_formfill.h>
#include <fpdf_edit.h>

#include <gdiplus.h>

#include <fstream>
#include <cstring>
#include <exception>

namespace Pdf {

// FileAccessInfo

struct FileAccessInfo {
    FileAccessInfo(Document* document)
        : Doc(document)
    {
        std::memset(&SdkData, 0, sizeof(SdkData));
        SdkData.m_FileLen = 0;
        SdkData.m_GetBlock = GetBlock;
        SdkData.m_Param = this;
        if (std::istream* pIst = document->GetStream()) {
            if (*pIst) {
                pIst->seekg(0, std::ios::end);
                SdkData.m_FileLen = static_cast<unsigned long>(pIst->tellg());
                pIst->seekg(0);
            }
        }
    }

    static int GetBlock(void* param, unsigned long position, unsigned char* pBuf, unsigned long size) {
        Document* doc = static_cast<FileAccessInfo*>(param)->Doc;
        if (std::istream* pIstr = doc->GetStream()) {
            if (*pIstr) {
                pIstr->seekg(position);
                pIstr->read(reinterpret_cast<char*>(pBuf), size);
                if (pIstr->gcount() == size && !pIstr->fail()) {
                    return size;
                }
            }
        }
        return 0;
    }

    Document* Doc;
    FPDF_FILEACCESS SdkData;
};

// DocumentAvailabilityProvider

struct DocumentAvailabilityProvider {
    DocumentAvailabilityProvider(FileAccessInfo* fileAccess)
        : SdkHandle(nullptr)
    {
        memset(&SdkFileAvail, 0, sizeof(SdkFileAvail));
        SdkFileAvail.version = 1;
	    SdkFileAvail.IsDataAvail = IsDataAvail;
        memset(&SdkDownloadHints, 0, sizeof(SdkDownloadHints));
        SdkDownloadHints.version = 1;
        SdkDownloadHints.AddSegment = AddSegment;
        SdkHandle = FPDFAvail_Create(&SdkFileAvail, &fileAccess->SdkData);
    }

    ~DocumentAvailabilityProvider() {
        FPDFAvail_Destroy(SdkHandle);
    }

    bool IsDocAvailable() const {
        return FPDFAvail_IsDocAvail(SdkHandle, const_cast<FX_DOWNLOADHINTS*>(&SdkDownloadHints)) == PDF_DATA_AVAIL;
    }

    bool IsPageAvailable(int pageNum) const {
        return FPDFAvail_IsPageAvail(SdkHandle, pageNum,
            const_cast<FX_DOWNLOADHINTS*>(&SdkDownloadHints)) == PDF_DATA_AVAIL;
    }

    bool IsLinearized() const {
        return FPDFAvail_IsLinearized(SdkHandle) == PDF_LINEARIZED;
    }

    operator bool() const {
        return SdkHandle != nullptr;
    }

    static FPDF_BOOL IsDataAvail(_FX_FILEAVAIL* /* this */, size_t /* offset */, size_t /* size */) {
        return true;
    }

    static void AddSegment(FX_DOWNLOADHINTS* /* this */, size_t /* offset */, size_t /* size */)
    {
    }

    FX_FILEAVAIL SdkFileAvail;
    FX_DOWNLOADHINTS SdkDownloadHints;
    FPDF_AVAIL SdkHandle;
};

// InternalData

struct InternalData {
    InternalData(Document* doc)
        : FileAccess(doc)
        , DocAvailProvider(&FileAccess)
        , SdkDocumentHandle(nullptr)
    {
    }

    FileAccessInfo FileAccess;
    DocumentAvailabilityProvider DocAvailProvider;
    FPDF_DOCUMENT SdkDocumentHandle;
};

class DCResourceGuard {
public:
    DCResourceGuard(Gdiplus::Graphics& g, HDC dc)
        : m_g(g), m_dc(dc)
    {}
    ~DCResourceGuard() {
        m_g.ReleaseHDC(m_dc);
    }
    HDC GetHDC() const {
        return m_dc;
    }

private:
    Gdiplus::Graphics& m_g;
    HDC m_dc;
};

// Page

class Page {
public:
    Page(FPDF_DOCUMENT hDoc, int index)
        : m_sdkDocumentHandle(hDoc)
        , m_index(index)
        , m_sdkPageHandle(nullptr)
    {
    }

    ~Page() {
        if (m_sdkPageHandle) {
            FPDF_ClosePage(m_sdkPageHandle);
            m_sdkPageHandle = nullptr;
        }
    }

    FPDF_PAGE GetHandle() const {
        if (!m_sdkPageHandle) {
            const_cast<Page*>(this)->m_sdkPageHandle = FPDF_LoadPage(m_sdkDocumentHandle, m_index);
            if (!m_sdkPageHandle) {
                Log("Failed to load page %d. Error code %d\n", m_index, FPDF_GetLastError());
            }
        }
        return m_sdkPageHandle;
    }

    Drawing::Size GetSize() const {
        double width(0), height(0);
        FPDF_GetPageSizeByIndex(m_sdkDocumentHandle, m_index, &width, &height);
        return Drawing::Size(static_cast<int>(width), static_cast<int>(height));
    }

    /* HBITMAPUPtr RenderToBitmap(Drawing::Size const& size) {
        HBITMAPUPtr outBitmap;
        std::unique_ptr<Gdiplus::Bitmap> bmp(new Gdiplus::Bitmap(size.Width, size.Height, PixelFormat24bppRGB));
        {
            Gdiplus::Graphics g(bmp.get());
            g.Clear(Gdiplus::Color::White);
            {
                DCResourceGuard dc(g, g.GetHDC());
                FPDF_RenderPage(dc.GetHDC(), GetHandle(), 0, 0, size.Width, size.Height, 0, FPDF_LCD_TEXT);
            }
        }
        HBITMAP hBitmap;
        if (bmp->GetHBITMAP(Gdiplus::Color::White, &hBitmap) == Gdiplus::Status::Ok) {
            outBitmap.reset(hBitmap);
        }
        return outBitmap;
    } */

    HBITMAPUPtr RenderToBitmap(Drawing::Size const& size, bool grayScale) {
        HBITMAPUPtr outBitmap;
        double docWidth(0), docHeight(0);
        if (!FPDF_GetPageSizeByIndex(m_sdkDocumentHandle, m_index, &docWidth, &docHeight) || !docWidth || !docHeight) {
            return outBitmap;
        }
        double const docW_h = docWidth / docHeight;
        double const destW_h = static_cast<double>(size.Width) / size.Height;
        Drawing::Rect destRect;
        if (docW_h < destW_h) {
            destRect.Width = static_cast<int>(size.Height * docW_h);
            destRect.Height = size.Height;
        } else {
            destRect.Width = size.Width;
            destRect.Height = static_cast<int>(size.Width / docW_h);
        }
        destRect.X = (size.Width - destRect.Width) / 2;
        destRect.Y = (size.Height - destRect.Height) / 2;
        const WORD colorDepth = 24;
        const int stride = ((size.Width*colorDepth + 31) & ~31) >> 3;
        BITMAPINFO bmi;
        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = size.Width;
        bmi.bmiHeader.biHeight = -size.Height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = colorDepth;
        bmi.bmiHeader.biSizeImage = stride*size.Height;
        void* pBmpData = nullptr;
        if (HBITMAP hBmp = CreateDIBSection(0, &bmi, DIB_RGB_COLORS, &pBmpData, NULL, 0)) {
            if (FPDF_BITMAP pdfBmp = FPDFBitmap_CreateEx(size.Width, size.Height, FPDFBitmap_BGR, pBmpData, stride))
            {
                FPDFBitmap_FillRect(pdfBmp, 0, 0, size.Width, size.Height, 0xFFFFFFFF);
                int flags = FPDF_LCD_TEXT;
                if (grayScale) {
                    flags |= FPDF_GRAYSCALE;
                }
                FPDF_RenderPageBitmap(pdfBmp, GetHandle(), destRect.X, destRect.Y, destRect.Width, destRect.Height, 0, flags);
                FPDFBitmap_Destroy(pdfBmp);
                outBitmap.reset(hBmp);
                return outBitmap;
            }
        }
        return outBitmap;
    }

private:
    FPDF_DOCUMENT m_sdkDocumentHandle;
    int m_index;
    FPDF_PAGE m_sdkPageHandle;
};

// Document

Document::Document(std::string const& filePath, IPasswordProvider& passwordProvider)
    : m_stream(new std::ifstream(filePath, std::ios::in|std::ios::binary))
    , m_data(new InternalData(this))
    , m_linearized(false)
{
    if (m_data->DocAvailProvider && m_data->DocAvailProvider.IsDocAvailable()) {
        m_linearized = m_data->DocAvailProvider.IsLinearized();
        FPDF_BYTESTRING pdfPassword = passwordProvider.GetPassword().c_str();
        bool retryLoad = true;
        while (retryLoad) {
            if (m_linearized) {
                m_data->SdkDocumentHandle = FPDFAvail_GetDocument(m_data->DocAvailProvider.SdkHandle, pdfPassword);
            } else {
                m_data->SdkDocumentHandle = FPDF_LoadCustomDocument(&m_data->FileAccess.SdkData, pdfPassword);
            }
            
            if (!m_data->SdkDocumentHandle) {
                unsigned long error = FPDF_GetLastError();
                if (error == FPDF_ERR_PASSWORD) {
                    if (passwordProvider.RequestPassword()) {
                        pdfPassword = passwordProvider.GetPassword().c_str();
                        continue;
                    }
                }
                throw std::runtime_error(ErrorMsg(error));
            }
            retryLoad = false;
        }
        int firstPageNum = FPDFAvail_GetFirstPageNum(m_data->SdkDocumentHandle);
        bool isFirstPageAvailable = m_data->DocAvailProvider.IsPageAvailable(firstPageNum);
        Log("First page:%d isAvail:%d\n", firstPageNum, isFirstPageAvailable);
        int const pageCount = FPDF_GetPageCount(m_data->SdkDocumentHandle);
        //m_pages.resize(pageCount);
        m_pages.reserve(static_cast<std::size_t>(pageCount));
        for (int pageNum = 0; pageNum < pageCount; ++pageNum) {
            if (!m_linearized || m_data->DocAvailProvider.IsPageAvailable(pageNum)) {
                m_pages.push_back(std::make_unique<Page>(m_data->SdkDocumentHandle, pageNum));
            }
        }
    }
}

Document::~Document() {
    m_pages.clear();
    if (m_data->SdkDocumentHandle) {
        FPDF_CloseDocument(m_data->SdkDocumentHandle);
    }
}

std::istream* Document::GetStream() const {
    return m_stream.get();
}

Document::operator bool() const {
    return m_data->SdkDocumentHandle != nullptr;
}

std::size_t Document::GetPageCount() const {
    return m_pages.size();
}

Drawing::Size Document::GetPageSize(std::size_t pageNum) const {
    return m_pages[pageNum]->GetSize();
}

HBITMAPUPtr Document::RenderPage(std::size_t pageNum, Drawing::Size const& size, bool grayScale) {
    return m_pages[pageNum]->RenderToBitmap(size, grayScale);
}

} // namespace Pdf