#pragma once

#include "ImageFormat.h"

#include <memory>
#include <iosfwd>

struct GDIObjDeleter {
    void operator()(void* gdiObj);
};

typedef std::unique_ptr<void, GDIObjDeleter> HBITMAPUPtr;

HBITMAPUPtr CreateHBITMAPPtr(void* hBitmap);

void SaveHBITMAPToStdStream(HBITMAPUPtr const& hBmp, ImageFormat outFormat, std::ostream& outStream, std::size_t* outSize = nullptr);