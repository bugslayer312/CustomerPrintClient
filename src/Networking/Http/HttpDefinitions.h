#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <list>
#include <ctime>

typedef std::unordered_map<std::string, std::string> ParamMap;
typedef std::list<std::pair<std::string, std::string>> ParamList;
typedef std::time_t RequestIdType;

enum class HttpMethod {
    Get,
    Post,
    Put,
    Delete
};

enum class Protocol {
    Http,
    Https
};

enum class ContentType {
    TextPlain,
    ApplicationJson,
    ApplicationOctetStream,
    MultipartFormData,
    ImagePng,
    ImageBmp,
    ImageJpeg
};

std::string ContentTypeToString(ContentType contentType);

extern const char* const HttpVersion_1_1;
extern const char* const HttpVersion_1_0;

namespace HttpHeaders {
extern const char* const ContentLength;
extern const char* const ContentType;
extern const char* const ContentDisposition;
extern const char* const Authorization;
} // namespace HttpHeaders