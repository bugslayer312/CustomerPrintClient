#include "PdfDocumentRenderer.h"

#include "Pdf/PdfLibrary.h"
#include "Pdf/PdfDocument.h"
#include "../Core/StringUtilities.h"

#include <windows.h>
#include <gdiplus.h>

PdfDocumentRenderer::PdfDocumentRenderer(Pdf::Library& pdfLib, std::wstring const& filePath, PrintProfile const& printProfile)
    : DocumentRendererBase(printProfile)
    , m_document(pdfLib.OpenDocument(Strings::ToUTF8(filePath)))
    , m_fileName(Strings::ExtractFileName(filePath))
{
}

PdfDocumentRenderer::~PdfDocumentRenderer() {
}

std::size_t PdfDocumentRenderer::GetPageCount() const {
    return m_document->GetPageCount();
}

HBITMAPUPtr PdfDocumentRenderer::RenderForPreview(std::size_t pageNum, PrintSettings const& printSettings,
                                                  Drawing::Rect const& pageRect, Drawing::Point& outOrigin) const {

    HBITMAPUPtr outBitmap;
    Drawing::Size const srcSize = m_document->GetPageSize(pageNum);
    if (srcSize.Width > 0 && srcSize.Height > 0 && pageRect.Width > 0 && pageRect.Height > 0) {
        Gdiplus::Rect destRect(0, 0, srcSize.Width, srcSize.Height);
        float const srcW_h = static_cast<float>(srcSize.Width) / srcSize.Height;
        float const pageW_h = static_cast<float>(pageRect.Width) / pageRect.Height;
        if (srcW_h < pageW_h) {
            destRect.Width = static_cast<INT>(pageRect.Height * srcW_h);
            destRect.Height = pageRect.Height;
        } else {
            destRect.Width = pageRect.Width;
            destRect.Height = static_cast<INT>(pageRect.Width / srcW_h);
        }
        destRect.X = (pageRect.Width - destRect.Width) / 2;
        destRect.Y = (pageRect.Height - destRect.Height) / 2;
        outOrigin.X = destRect.X < 0 ? 0 : destRect.X;
        outOrigin.Y = destRect.Y < 0 ? 0 : destRect.Y;
        outBitmap = m_document->RenderPage(pageNum, Drawing::Size(destRect.Width, destRect.Height), !m_printProfile.IsColor);
    }
    return outBitmap;
}

std::string PdfDocumentRenderer::GetJobName() const {
    return Strings::ToUTF8(m_fileName);
}

std::wstring PdfDocumentRenderer::GetPageFileName(std::size_t /*pageNum*/) const {
    return m_fileName;
}

void PdfDocumentRenderer::RenderForPrinting(std::size_t pageNum, PrintSettings const& printSettings, ImageFormat outFormat,
                                            std::ostream& outStream, std::size_t& outStreamSize) const {

    int dpiX(m_printProfile.DpiX), dpiY(m_printProfile.DpiY);
    dpiX = dpiY = 300; // we downscale dpi to 300 intentionally; remove if need to use original dpi
    Drawing::Size const pageSizePx(static_cast<int>(dpiX/25.4*m_printProfile.PaperWidthMM),
        static_cast<int>(dpiY/25.4*m_printProfile.PaperHeightMM));
    if (HBITMAPUPtr outBitmap = m_document->RenderPage(pageNum, pageSizePx, !m_printProfile.IsColor)) {
        SaveHBITMAPToStdStream(outBitmap, outFormat, outStream, &outStreamSize);
    }
}