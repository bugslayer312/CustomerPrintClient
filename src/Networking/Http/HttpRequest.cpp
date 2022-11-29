#include "HttpRequest.h"
#include "TcpException.h"

#include <ostream>

std::ostream& operator<<(std::ostream& ost, HttpMethod method) {
    switch (method)
    {
    case HttpMethod::Get:
        ost << "GET";
        break;
    case HttpMethod::Post:
        ost << "POST";
        break;
    case HttpMethod::Put:
        ost << "PUT";
        break;
    case HttpMethod::Delete:
        ost << "DELETE";
        break;
    default:
        throw TcpException("HTTP client: Unsupported method", false);
    }
    return ost;
}

HttpRequest::HttpRequest(HttpMethod method, std::string const& path, ParamList const& params)
    : m_id(std::time(nullptr))
    , m_method(method)
    , m_path(path)
    , m_params(params)
{
}

RequestIdType HttpRequest::GetId() const {
    return m_id;
}

HttpRequest& HttpRequest::AddHeader(std::string const& name, std::string const& value)
{
    m_headers[name] = value;
    return *this;
}

HttpRequest& HttpRequest::SetContent(jsonxx::Object const& json) {
    SetContent(HttpContentUPtr(new HttpJsonContent(json)));
    return *this;
}

HttpRequest& HttpRequest::SetContent(HttpContentUPtr content) {
    m_content = std::move(content);
    m_headers[HttpHeaders::ContentLength] = std::to_string(m_content->GetLength());
    m_headers[HttpHeaders::ContentType] = m_content->GetContentTypeString();
    return *this;
}

std::ostream& operator<<(std::ostream& ost, HttpRequest const& request) {
    ost << request.m_method << ' ' << request.m_path;
    if (!request.m_params.empty()) {
        auto it = request.m_params.begin();
        ost << '?' << it->first << '=' << it->second;
        while (++it != request.m_params.end()) {
            ost << '&' << it->first << '=' << it->second;
        }
    }
    ost << ' ' << HttpVersion_1_1 << "\r\n";
    for (auto const& header : request.m_headers) {
        ost << header.first << ": " << header.second << "\r\n"; 
    }
    ost << "\r\n";
    if (request.m_content) {
        ost << *request.m_content;
    }

    return ost;
}