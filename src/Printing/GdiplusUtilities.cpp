#include "GdiplusUtilities.h"

#include "../Core/Log.h"
#include "../Core/Format.h"

#include <windows.h>
#include <gdiplus.h>

#include <map>
#include <stdexcept>
#include <ostream>

class GdiplusInitImpl {
public:
    GdiplusInitImpl() {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        m_success = Gdiplus::GdiplusStartup(&m_token, &gdiplusStartupInput, NULL) == Gdiplus::Status::Ok;
        Log("Start Gdiplus: %s\n", m_success ? "success" : "fail");
    }
    ~GdiplusInitImpl() {
        if (m_success) {
            Log("Shutdown Gdiplus\n");
            Gdiplus::GdiplusShutdown(m_token);
        }
    }
    operator bool() const {
        return m_success;
    }

private:
    ULONG_PTR m_token;
    bool m_success;
};

GdiplusInit::GdiplusInit() : m_impl(new GdiplusInitImpl()){
}

GdiplusInit::~GdiplusInit() {
}

namespace GdiplusUtils {

struct EncoderClsidCache {
    EncoderClsidCache() {
        UINT num(0), size(0);
        if (Gdiplus::GetImageEncodersSize(&num, &size) == Gdiplus::Status::Ok && size) {
            std::map<std::wstring, ImageFormat> const availableFormats = {
                {L"image/bmp", ImageFormat::Bmp},
                {L"image/png", ImageFormat::Png},
                {L"image/jpeg", ImageFormat::Jpeg},
                {L"image/tiff", ImageFormat::Tiff},
                {L"image/gif", ImageFormat::Gif}
            };
            std::unique_ptr<Gdiplus::ImageCodecInfo, void(*)(void*)> pImageCodecInfo(
                static_cast<Gdiplus::ImageCodecInfo*>(malloc(size)),
                [](void* ptr) {
                    free(ptr);
                }
            );
            if (pImageCodecInfo && Gdiplus::GetImageEncoders(num, size, pImageCodecInfo.get()) == Gdiplus::Status::Ok) {
                for (UINT i(0); i < num; ++i) {
                    Gdiplus::ImageCodecInfo const& imageCodecInfo = pImageCodecInfo.get()[i];
                    auto foundIt = availableFormats.find(imageCodecInfo.MimeType);
                    if (foundIt != availableFormats.end()) {
                        Data[foundIt->second] = imageCodecInfo.Clsid;
                    }
                }
            }
        }
    }

    std::map<ImageFormat, CLSID> Data;
};

bool GetEncoderClsid(ImageFormat imageFormat, CLSID* pClsid) {
    static EncoderClsidCache clsidCache;
    auto foundIt = clsidCache.Data.find(imageFormat);
    if (foundIt != clsidCache.Data.end()) {
        *pClsid = foundIt->second;
        return true;
    }
    return false;
}

void SaveBitmapToStdStream(Gdiplus::Bitmap& bmp, ImageFormat outFormat, std::ostream& outStream, std::size_t* outSize) {
    if (outSize) {
        *outSize = 0;
    }
    IStream* pIStream = nullptr;
    if(CreateStreamOnHGlobal(NULL, FALSE, (LPSTREAM*)&pIStream) != S_OK) {
        throw std::runtime_error("Failed to create stream\n");
    }
    std::unique_ptr<IStream, void(*)(IStream*)> streamOwner(pIStream, [](IStream* st) {
        st->Release();
    });
    CLSID clsid;
    if (!GdiplusUtils::GetEncoderClsid(outFormat, &clsid)) {
        throw std::runtime_error(Format("Failed to find encoder for %s", ImageFormatToString(outFormat).c_str()));
    }
    if (bmp.Save(pIStream, &clsid) == Gdiplus::Status::Ok) {
        LARGE_INTEGER streamOffset;
        memset(&streamOffset, 0, sizeof(streamOffset));
        pIStream->Seek(streamOffset, STREAM_SEEK_SET, NULL);
        ULONG buffSize = 4096;
        char buff[4096];
        ULONG bytesRead(0);
        bool readOk(true);
        while (readOk) {
            if (pIStream->Read(buff, buffSize, &bytesRead) == S_OK) {
                if (bytesRead) {
                    if (outStream.write(buff, bytesRead)) {
                        if (outSize) {
                            *outSize += bytesRead;
                        }
                        readOk = true;
                        continue;
                    }
                }
                else {
                    return;
                }
            }
            readOk = false;
        }
    }
    throw std::runtime_error("Failed to save image to stream");
}

} // namespace GdiplusUtils