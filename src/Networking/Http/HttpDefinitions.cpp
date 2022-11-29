#include "HttpDefinitions.h"

std::string ContentTypeToString(ContentType contentType) {
    switch (contentType) {
    case ContentType::ApplicationJson:
        return "application/json";
    case ContentType::ApplicationOctetStream:
        return "application/octet-stream";
    case ContentType::MultipartFormData:
        return "multipart/form-data";
    case ContentType::ImageBmp:
        return "image/bmp";
    case ContentType::ImagePng:
        return "image/png";
    case ContentType::ImageJpeg:
        return "image/jpeg";
    }
    return "text/plain";
}

const char* const HttpVersion_1_1 = "HTTP/1.1";
const char* const HttpVersion_1_0 = "HTTP/1.0";

namespace HttpHeaders {
const char* const ContentLength = "Content-Length";
const char* const ContentType = "Content-Type";
const char* const ContentDisposition = "Content-Disposition";
const char* const Authorization = "Authorization";
} // namespace HttpHeaders