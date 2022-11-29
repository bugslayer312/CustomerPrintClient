#pragma once

#include <string>

enum class ImageFormat {
    Invalid,
    Bmp,
    Png,
    Jpeg,
    Gif,
    Tiff,
    Emf
};

ImageFormat ImageFormatFromWPath(std::wstring const& path);
std::string ImageFormatToString(ImageFormat imageFormat);
std::wstring ImageFormatToWString(ImageFormat imageFormat);