#pragma once

#include "ImageFormat.h"

// #include <guiddef.h>

#include <memory>
#include <iosfwd>

class GdiplusInitImpl;

namespace Gdiplus {

class Bitmap;

} // namespace Gdiplus

class GdiplusInit {
public:
    GdiplusInit();
    ~GdiplusInit();
private:
    std::unique_ptr<GdiplusInitImpl> m_impl;
};

namespace GdiplusUtils {

// bool GetEncoderClsid(ImageFormat imageFormat, CLSID* pClsid);

void SaveBitmapToStdStream(Gdiplus::Bitmap& bmp, ImageFormat outFormat, std::ostream& outStream, std::size_t* outSize = nullptr);

} // namespace GdiplusUtils