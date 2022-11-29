#pragma once

#include "HttpDefinitions.h"
#include "HttpContent.h"

#include <iosfwd>

class HttpClientImplBase;
class HttpClientMTImplBase;

class HttpRequest {
public:
    RequestIdType GetId() const;
    HttpRequest& AddHeader(std::string const& name, std::string const& value);
    HttpRequest& SetContent(jsonxx::Object const& json);
    HttpRequest& SetContent(HttpContentUPtr content);
    
    friend std::ostream& operator<<(std::ostream& ost, HttpRequest const& request);
    friend class HttpClientImplBase;
    friend class HttpClientMTImplBase;

    HttpRequest(HttpRequest&&) = default;
    HttpRequest& operator=(HttpRequest&&) = default;

private:
    HttpRequest(HttpMethod method, std::string const& path, ParamList const& params);

private:
    RequestIdType m_id;
    HttpMethod const m_method;
    std::string const m_path;
    ParamList const m_params;
    ParamMap m_headers;
    HttpContentUPtr m_content;
};

std::ostream& operator<<(std::ostream& ost, HttpRequest const& request);