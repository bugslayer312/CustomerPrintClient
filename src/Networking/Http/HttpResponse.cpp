#include "HttpResponse.h"
#include "../StringResources.h"

#include <ostream>
#include <sstream>

std::string const& HttpResponse::GetHttpVersion() const {
    return m_httpVersion;
}

std::uint32_t HttpResponse::GetStatusCode() const {
    return m_statusCode;
}

std::string const& HttpResponse::GetStatusMessage() const {
    return m_statusMessage;
}

ParamMap const& HttpResponse::GetHeaders() const {
    return m_headers;
}

std::string HttpResponse::GetContent() const {
    return m_content.str();
}

std::size_t HttpResponse::GetContentLength() const {
    auto it = m_headers.find(HttpHeaders::ContentLength);
    if (it != m_headers.end()) {
        std::istringstream ist(it->second);
        std::size_t result;
        if (!(ist >> result).fail()) {
            return result;
        }
    }
    return InvalidContentLength;
}

std::string const& HttpResponse::GetContentType() const {
    auto const& it = m_headers.find(HttpHeaders::ContentType);
    if (it != m_headers.end()) {
        return it->second;
    }
    return EmptyString;
}

void HttpResponse::SaveContentToStream(std::ostream& ost) {
    ost << m_content.rdbuf();
}

std::ostream& operator<<(std::ostream& ost, HttpResponse const& response) {
    ost << response.GetHttpVersion() << " " << response.GetStatusCode() << " " << response.GetStatusMessage() << "\r\n";
    for (auto const& header : response.GetHeaders()) {
        ost << header.first << ": " << header.second << "\r\n";
    }
    ost << "\r\n" << response.GetContent();
    return ost;
}