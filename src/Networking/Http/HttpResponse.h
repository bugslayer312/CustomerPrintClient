#pragma once

#include "HttpDefinitions.h"

#include <sstream>
#include <memory>

class HttpClientImplBase;
class HttpClientMTImplBase;

class HttpResponse {
public:
    std::string const& GetHttpVersion() const;
    std::uint32_t GetStatusCode() const;
    std::string const& GetStatusMessage() const;
    ParamMap const& GetHeaders() const;
    std::string GetContent() const;
    std::size_t GetContentLength() const;
    std::string const& GetContentType() const;
    void SaveContentToStream(std::ostream& ost);

    static const std::size_t InvalidContentLength = static_cast<std::size_t>(-1);

    friend class HttpClientImplBase;
    friend class HttpClientMTImplBase;

private:
    HttpResponse() = default;

private:
    std::string m_httpVersion;
    std::uint32_t m_statusCode;
    std::string m_statusMessage;
    ParamMap m_headers;
    std::stringstream m_content;
};

std::ostream& operator<<(std::ostream& ost, HttpResponse const& response);