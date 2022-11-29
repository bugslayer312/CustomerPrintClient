#pragma once

#include "HttpDefinitions.h"

#include <string>
#include <memory>
#include <exception>
#include <functional>

class HttpResponse;
class HttpRequest;
class HttpClientImplBase;
typedef std::shared_ptr<HttpResponse> ResponsePtr;
typedef std::function<void(ResponsePtr, std::exception_ptr)> HttpRequestCallback;

class HttpClient {
public:
    HttpClient(std::string const& serverName, int port, Protocol proto, std::size_t threadCount);
    ~HttpClient();

    HttpRequest CreateRequest(HttpMethod method, std::string const& path, ParamList const& params);
    void PostRequest(HttpRequest&& request, HttpRequestCallback callback);
    void Cancel();

private:
    std::unique_ptr<HttpClientImplBase> m_impl;
};