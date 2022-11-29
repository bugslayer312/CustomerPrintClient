#pragma once

#include <string>

namespace Http {

extern const char* const OperationCanceled;
extern const char* const ConnectionFailed;
extern const char* const ResolveFailed;
extern const char* const SslHandShakeFailed;
extern const char* const SendRequestFailed;
extern const char* const ReadResponseFailed;
extern const char* const ResponseInvalidHttpVersion;
extern const char* const ResponseInvalidStatusCode;
extern const char* const ResponseInvalidStatusMsg;
extern const char* const ResponseHeadersReadFailed;
extern const char* const ResponseInvalidHeader;
extern const char* const ResponseBodyReadFailed;
extern char const* const FailedParseResponseBody;

} // namespace Http

extern const std::string EmptyString;