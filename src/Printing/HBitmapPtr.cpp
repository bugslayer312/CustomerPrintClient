#include "HBitmapPtr.h"

#include "GdiplusUtilities.h"

#include <windows.h>
#include <gdiplus.h>

void GDIObjDeleter::operator()(void* gdiObj) {
    if (gdiObj) {
        ::DeleteObject(gdiObj);
    }
}

HBITMAPUPtr CreateHBITMAPPtr(void* hBitmap) {
    return HBITMAPUPtr(hBitmap);
}

void SaveHBITMAPToStdStream(HBITMAPUPtr const& hBmp, ImageFormat outFormat, std::ostream& outStream, std::size_t* outSize) {
    std::unique_ptr<Gdiplus::Bitmap> bmp(Gdiplus::Bitmap::FromHBITMAP(static_cast<HBITMAP>(hBmp.get()), NULL));
    GdiplusUtils::SaveBitmapToStdStream(*bmp, outFormat, outStream, outSize);
}