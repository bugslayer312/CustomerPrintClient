#include "ImageFormat.h"

ImageFormat ImageFormatFromWPath(std::wstring const& path) {
    ImageFormat result = ImageFormat::Invalid;
    std::size_t dotPos = path.find_last_of(L'.');
    if (dotPos != std::string::npos) {
        std::wstring const ext = path.substr(dotPos + 1);
        if (ext == L"bmp") {
            result = ImageFormat::Bmp;
        } else if (ext == L"png") {
            result = ImageFormat::Png;
        } else if (ext == L"jpg" || ext == L"jpeg") {
            result = ImageFormat::Jpeg;
        } else if (ext == L"gif") {
            result = ImageFormat::Gif;
        } else if (ext == L"tif" || ext == L"tiff") {
            result = ImageFormat::Tiff;
        } else if (ext == L"emf") {
            result = ImageFormat::Emf;
        }
    }
    return result;
}

std::string ImageFormatToString(ImageFormat imageFormat) {
    std::string result;
    switch (imageFormat)
    {
    case ImageFormat::Bmp:
        result = "bmp";
        break;
    case ImageFormat::Png:
        result = "png";
        break;
    case ImageFormat::Jpeg:
        result = "jpeg";
        break;
    case ImageFormat::Gif:
        result = "gif";
        break;
    case ImageFormat::Tiff:
        result = "tiff";
        break;
    case ImageFormat::Emf:
        result = "emf";
        break;
    }
    return result;
}

std::wstring ImageFormatToWString(ImageFormat imageFormat) {
    std::wstring result;
    switch (imageFormat)
    {
    case ImageFormat::Bmp:
        result = L"bmp";
        break;
    case ImageFormat::Png:
        result = L"png";
        break;
    case ImageFormat::Jpeg:
        result = L"jpeg";
        break;
    case ImageFormat::Gif:
        result = L"gif";
        break;
    case ImageFormat::Tiff:
        result = L"tiff";
        break;
    case ImageFormat::Emf:
        result = L"emf";
        break;
    }
    return result;
}