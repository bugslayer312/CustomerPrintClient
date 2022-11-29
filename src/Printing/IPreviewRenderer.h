#pragma once

#include "HBitmapPtr.h"

#include <string>

namespace Drawing {
struct Rect;
struct Point;
}

class PrintSettings;

class IPreviewRenderer {
public:
    struct PrintProfile {
        int DpiX;
        int DpiY;
        int PaperWidthMM;
        int PaperHeightMM;
        bool IsColor;
    };

    virtual ~IPreviewRenderer() = default;
    virtual void SetPrintProfile(PrintProfile const& printProfile) = 0;
    virtual std::size_t GetPageCount() const = 0;
    virtual HBITMAPUPtr RenderForPreview(std::size_t pageNum, PrintSettings const& printSettings,
        Drawing::Rect const& pageRect, Drawing::Point& outOrigin) const = 0;
};
typedef std::unique_ptr<IPreviewRenderer> IPreviewRendererUPtr;