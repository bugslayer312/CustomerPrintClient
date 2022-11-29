#include "StringResources.h"

namespace Http {

const char* const OperationCanceled = "Operation canceled";
const char* const ConnectionFailed = "Failed to connect to %s(%s:%d)";
const char* const ResolveFailed = "Failed to resolve %s";
const char* const SslHandShakeFailed = "SSL handshake failed";
const char* const SendRequestFailed = "Failed send request";
const char* const ReadResponseFailed = "Failed read response";
const char* const ResponseInvalidHttpVersion = "Invalid response. HTTP version: %s";
const char* const ResponseInvalidStatusCode = "Invalid response. Failed parse status code";
const char* const ResponseInvalidStatusMsg = "Invalid response. Failed parse status message";
const char* const ResponseHeadersReadFailed = "Failed read response headers";
const char* const ResponseInvalidHeader = "Invalid response. Failed parse header";
const char* const ResponseBodyReadFailed = "Failed read response body";
char const* const FailedParseResponseBody = "Failed parse response body";

} // namespace Http

const std::string EmptyString;