#include "ImageBundleRenderer.h"

#include "../Core/Log.h"
#include "../Core/StringUtilities.h"
#include "GdiplusUtilities.h"
#include "Primitives.h"
#include "PrintSettings.h"

// #define NOMINMAX
#include <windows.h>
//using std::min;
//using std::max;
#include <gdiplus.h>

void DrawImageGrayscale(Gdiplus::Graphics& graphics, Gdiplus::Image* img, Gdiplus::RectF const& destRect,
                        Gdiplus::REAL srcX, Gdiplus::REAL srcY, Gdiplus::REAL srcWidth, Gdiplus::REAL srcHeight) {
    static Gdiplus::ColorMatrix const grayScaleMatrix =
    {
        0.299f, 0.299f, 0.299f, 0, 0,
        0.587f, 0.587f, 0.587f, 0, 0,
        0.114f, 0.114f, 0.114f, 0, 0,
        0,      0,      0,      1, 0,
        0,      0,      0,      0, 1
    };
    Gdiplus::ImageAttributes attr;
    attr.SetColorMatrix(&grayScaleMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
    graphics.DrawImage(img, destRect, srcX, srcY, srcWidth, srcHeight, Gdiplus::UnitPixel, &attr);
}

ImageBundleRenderer::ImageBundleRenderer(std::vector<std::wstring>&& imagePaths, PrintProfile const& printProfile)
    : DocumentRendererBase(printProfile)
    , m_imagePaths(std::move(imagePaths))
{
}

std::size_t ImageBundleRenderer::GetPageCount() const {
    return m_imagePaths.size();
}

HBITMAPUPtr ImageBundleRenderer::RenderForPreview(std::size_t pageNum, PrintSettings const& printSettings,
                                                  Drawing::Rect const& pageRect, Drawing::Point& outOrigin) const {

    HBITMAPUPtr result;
    std::unique_ptr<Gdiplus::Bitmap> srcBmp(Gdiplus::Bitmap::FromFile(m_imagePaths[pageNum].c_str()));
    if (!srcBmp || srcBmp->GetLastStatus() != Gdiplus::Status::Ok) {
        Log("Failed to load image\n");
        return result;
    }
    int dpiX(m_printProfile.DpiX), dpiY(m_printProfile.DpiY);
    dpiX = dpiY = 300; // we downscale dpi to 300 intentionally; remove if need to use original dpi
    Gdiplus::SizeF const srcSize(srcBmp->GetWidth(), srcBmp->GetHeight());
    if (srcSize.Width > 0 && srcSize.Height > 0 && pageRect.Width > 0 && pageRect.Height > 0) {
        Gdiplus::RectF destRect(0, 0, srcSize.Width, srcSize.Height);
        switch (printSettings.Mode()) {
            case PrintMode::OriginalSize:
            {
                Gdiplus::SizeF printerPagePx(dpiX/25.4*m_printProfile.PaperWidthMM,
                    dpiY/25.4*m_printProfile.PaperHeightMM);
                if (printSettings.Orientation() == PrintOrientation::Landscape) {
                    std::swap(printerPagePx.Width, printerPagePx.Height);
                }
                destRect.Width = pageRect.Width * srcSize.Width / printerPagePx.Width;
                destRect.Height = pageRect.Height * srcSize.Height / printerPagePx.Height;
                break;
            }
            case PrintMode::DisplaySize:
            {
                Gdiplus::SizeF paperSizeMM(m_printProfile.PaperWidthMM, m_printProfile.PaperHeightMM);
                if (printSettings.Orientation() == PrintOrientation::Landscape) {
                    std::swap(paperSizeMM.Width, paperSizeMM.Height);
                }
                Gdiplus::SizeF const srcSizeMM(srcSize.Width*25.4f/96.0f, srcSize.Height*25.4f/96.0f);
                destRect.Width = pageRect.Width * srcSizeMM.Width / paperSizeMM.Width;
                destRect.Height = pageRect.Height * srcSizeMM.Height / paperSizeMM.Height;
                break;
            }
            case PrintMode::StretchToPage:
            {
                float const srcW_h = srcSize.Width / srcSize.Height;
                float const pageW_h = pageRect.Width / pageRect.Height;
                if (srcW_h < pageW_h) {
                    destRect.Width = pageRect.Height * srcW_h;
                    destRect.Height = pageRect.Height;
                } else {
                    destRect.Width = pageRect.Width;
                    destRect.Height = pageRect.Width / srcW_h;
                }
                break;
            }
        }

        switch (printSettings.HorAlign()) {
        case HorizontalPageAlign::Center:
            destRect.X = (pageRect.Width - destRect.Width) / 2.0f;
            break;
        case HorizontalPageAlign::Right:
            destRect.X = pageRect.Width - destRect.Width;
            break;
        }
        switch (printSettings.VertAlign()) {
        case VerticalPageAlign::Center:
            destRect.Y = (pageRect.Height - destRect.Height) / 2.0f;
            break;
        case VerticalPageAlign::Bottom:
            destRect.Y = pageRect.Height - destRect.Height;
            break;
        }

        std::unique_ptr<Gdiplus::Bitmap> destBmp(new Gdiplus::Bitmap(
            destRect.Width > pageRect.Width ? pageRect.Width : destRect.Width,
            destRect.Height > pageRect.Height ? pageRect.Height : destRect.Height,
            PixelFormat24bppRGB));
        {
            Gdiplus::Graphics graphics(destBmp.get());
            Gdiplus::RectF const drawRect(destRect.X > 0 ? 0 : destRect.X, destRect.Y > 0 ? 0 : destRect.Y,
                destRect.Width, destRect.Height);
            if (m_printProfile.IsColor) {
                graphics.DrawImage(srcBmp.get(), drawRect);
            }
            else {
                DrawImageGrayscale(graphics, srcBmp.get(), drawRect, 0, 0, srcSize.Width, srcSize.Height);
            }
        }
        srcBmp.reset(nullptr);
        HBITMAP hBitmap;
        if (destBmp->GetHBITMAP(Gdiplus::Color::White, &hBitmap) == Gdiplus::Status::Ok) {
            result.reset(hBitmap);
            outOrigin.X = destRect.X < 0 ? 0 : destRect.X;
            outOrigin.Y = destRect.Y < 0 ? 0 : destRect.Y;
        }
    }
    return result;
}

std::string ImageBundleRenderer::GetJobName() const {
    return GetPageCount() > 1 ? std::string("Images") : Strings::ToUTF8(GetPageFileName(0));
}

std::wstring ImageBundleRenderer::GetPageFileName(std::size_t pageNum) const {
    return Strings::ExtractFileName(m_imagePaths[pageNum]);
}

void ImageBundleRenderer::RenderForPrinting(std::size_t pageNum, PrintSettings const& printSettings, ImageFormat outFormat,
                                            std::ostream& outStream, std::size_t& outStreamSize) const {

    outStreamSize = 0;

    std::unique_ptr<Gdiplus::Bitmap> srcBmp(Gdiplus::Bitmap::FromFile(m_imagePaths[pageNum].c_str()));
    int dpiX(m_printProfile.DpiX), dpiY(m_printProfile.DpiY);
    dpiX = dpiY = 300; // we downscale dpi to 300 intentionally; remove if need to use original dpi
    Gdiplus::SizeF pageSizePx(dpiX/25.4*m_printProfile.PaperWidthMM, dpiY/25.4*m_printProfile.PaperHeightMM);
    Gdiplus::Size const outPageSizePx(static_cast<INT>(pageSizePx.Width), static_cast<INT>(pageSizePx.Height));
    if (printSettings.Orientation() == PrintOrientation::Landscape) {
        std::swap(pageSizePx.Width, pageSizePx.Height);
        std::swap(dpiX, dpiY);
    }
    Gdiplus::SizeF const srcSize(srcBmp->GetWidth(), srcBmp->GetHeight());
    Gdiplus::RectF drawRect(0, 0, srcSize.Width, srcSize.Height);
    switch (printSettings.Mode()) {
        case PrintMode::DisplaySize:
        {
            drawRect.Width = srcSize.Width * dpiX / 96.0f;
            drawRect.Height = srcSize.Height * dpiY / 96.0f;
            break;
        }
        case PrintMode::StretchToPage:
        {
            float const srcW_h = srcSize.Width / srcSize.Height;
            float const pageW_h = pageSizePx.Width / pageSizePx.Height;
            if (srcW_h < pageW_h) {
                drawRect.Width = pageSizePx.Height * srcW_h;
                drawRect.Height = pageSizePx.Height;
            } else {
                drawRect.Width = pageSizePx.Width;
                drawRect.Height = pageSizePx.Width / srcW_h;
            }
        }
    }
    switch (printSettings.HorAlign())
    {
    case HorizontalPageAlign::Center:
        drawRect.X = (pageSizePx.Width - drawRect.Width) / 2.0f;
        break;
    case HorizontalPageAlign::Right:
        drawRect.X = pageSizePx.Width - drawRect.Width;
        break;
    }
    switch (printSettings.VertAlign())
    {
    case VerticalPageAlign::Center:
        drawRect.Y = (pageSizePx.Height - drawRect.Height) / 2.0f;
        break;
    case VerticalPageAlign::Bottom:
        drawRect.Y = pageSizePx.Height - drawRect.Height;
        break;
    }
    std::unique_ptr<Gdiplus::Bitmap> destBmp(new Gdiplus::Bitmap(outPageSizePx.Width, outPageSizePx.Height,
        PixelFormat24bppRGB));
    {
        Gdiplus::Graphics graphics(destBmp.get());
        //std::unique_ptr<Gdiplus::Graphics> graphics(Gdiplus::Graphics::FromImage(destBmp.get()));
        graphics.Clear(Gdiplus::Color::White);
        if (printSettings.Orientation() == PrintOrientation::Landscape) {
            graphics.RotateTransform(-90.0f);
            graphics.TranslateTransform(-pageSizePx.Width, 0);
        }
        if (m_printProfile.IsColor) {
            graphics.DrawImage(srcBmp.get(), drawRect);
        } else {
            DrawImageGrayscale(graphics, srcBmp.get(), drawRect, 0, 0, srcSize.Width, srcSize.Height);
        }
    }
    GdiplusUtils::SaveBitmapToStdStream(*destBmp, outFormat, outStream, &outStreamSize);
}