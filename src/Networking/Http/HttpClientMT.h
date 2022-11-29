#pragma once

#include "HttpDefinitions.h"

#include <string>
#include <memory>
#include <exception>
#include <functional>

class HttpResponse;
class HttpRequest;
class HttpClientMTImplBase;
typedef std::shared_ptr<HttpResponse> ResponsePtr;
typedef std::function<void(ResponsePtr, std::exception_ptr)> HttpRequestCallback;

class HttpClientMT {
public:
    HttpClientMT(std::string const& serverName, int port, Protocol proto, std::size_t threadCount);
    ~HttpClientMT();

    HttpRequest CreateRequest(HttpMethod method, std::string const& path, ParamList const& params);
    void PostRequest(HttpRequest&& request, HttpRequestCallback callback);
    void CancelRequest(RequestIdType id);

private:
    std::unique_ptr<HttpClientMTImplBase> m_impl;
};