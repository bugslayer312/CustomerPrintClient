#include "HttpContent.h"

#include "../../Core/jsonxx.h"

#include <ostream>
#include <sstream>

// HttpContent

HttpContent::HttpContent(ContentType type)
    : m_type(type)
{
}

std::string HttpContent::GetContentTypeString() const {
    return ContentTypeToString(m_type);
}


// HttpStringContent

HttpStringContent::HttpStringContent(std::string&& str)
    : HttpContent(ContentType::TextPlain)
    , m_str(std::move(str))
{
}

void HttpStringContent::Save(std::ostream& ost) const {
    ost << m_str;
}

std::size_t HttpStringContent::GetLength() const {
    return m_str.size();
}


// HttpJsonContent

HttpJsonContent::HttpJsonContent(jsonxx::Object const& json)
    : HttpStringContent(json.json())
{
    m_type = ContentType::ApplicationJson;
}

// HttpStreamContent

HttpStreamContent::HttpStreamContent(std::unique_ptr<std::istream> ist)
    : HttpContent(ContentType::ApplicationOctetStream)
    , m_ist(std::move(ist))
{
}

void HttpStreamContent::Save(std::ostream& ost) const {
    ost << m_ist->rdbuf();
}

std::size_t HttpStreamContent::GetLength() const {
    std::streamoff const pos = m_ist->tellg();
    m_ist->seekg(0, std::ios::end);
    std::size_t const result = static_cast<std::size_t>(m_ist->tellg());
    m_ist->seekg(pos);
    return result;
}


// HttpMultipartFormDataContentImpl

class HttpMultipartFormDataContentImpl {
    class Part {
    public:
        Part(std::string const& key, std::string const& value)
            : m_headers(BuildContentDispositionHeader(key, ""))
            , m_content(new std::istringstream(value))
            , m_contentSize(value.size())
        {
        }

        Part(std::string const& key, std::string const& filename, std::unique_ptr<std::istream> content,
             std::size_t constentSize, ContentType contentType)
            : m_headers(BuildContentDispositionHeader(key, filename))
            , m_content(std::move(content))
            , m_contentSize(constentSize)
        {
            m_headers += "\r\n";
            m_headers += HttpHeaders::ContentType;
            m_headers += ": ";
            m_headers += ContentTypeToString(contentType);
        }

        std::size_t CalcSize() const {
            return m_headers.size() + 4 /* \r\n\r\n */ + m_contentSize;
        }

        void Save(std::ostream& ost) const {
            m_content->seekg(0);
            ost << m_headers << "\r\n\r\n" << m_content->rdbuf();
        }

    private:
        static std::string BuildContentDispositionHeader(std::string const& name, std::string const& filename) {
            std::string result(HttpHeaders::ContentDisposition);
            result += ": form-data; name=\"";
            result += name;
            result += "\"";
            if (!filename.empty()) {
                result += "; filename=\"";
                result += filename;
                result += "\"";
            }
            return result;
        }

    private:
        std::string m_headers;
        std::unique_ptr<std::istream> m_content;
        std::size_t const m_contentSize;
    };

public:
    std::string GetContentTypeString() const {
        std::string result = ContentTypeToString(ContentType::MultipartFormData);
        result += "; boundary=";
        result += m_boundary;
        return result;
    }

    void Save(std::ostream& ost) const {
        if (m_parts.empty()) {
            return;
        }

        for (auto const& part: m_parts) {
            ost << "--" << m_boundary << "\r\n";
            part.Save(ost);
            ost << "\r\n";
        }
        ost << "--" << m_boundary << "--\r\n";
    }
    
    std::size_t GetLength() const {
        if (m_parts.empty()) {
            return 0;
        }
        std::size_t const bndSize = m_boundary.size();
        std::size_t result(0);
        for (auto const& part: m_parts) {
            result += bndSize + 4 /* --boundary\r\n */ + part.CalcSize() + 2 /* \r\n */;
        }
        result += bndSize + 6 /* --boundary--\r\n */;
        return result;
    }

    void AddValue(std::string const& key, std::string const& value) {
        m_parts.emplace_back(key, value);
    }

    void AddFile(std::string const& key, std::string const& filename, std::unique_ptr<std::istream> content,
                 std::size_t contentSize, ContentType contentType) {

        m_parts.emplace_back(key, filename, std::move(content), contentSize, contentType);
    }

private:
    static std::string const m_boundary;
    std::list<Part> m_parts;
};

std::string const HttpMultipartFormDataContentImpl::m_boundary = "------------------------a452714f3cf68d90";

// HttpMultipartFormDataContent

HttpMultipartFormDataContent::HttpMultipartFormDataContent()
    : HttpContent(ContentType::MultipartFormData)
    , m_impl(new HttpMultipartFormDataContentImpl())
{

}

HttpMultipartFormDataContent::~HttpMultipartFormDataContent()
{
}

std::string HttpMultipartFormDataContent::GetContentTypeString() const {
    return m_impl->GetContentTypeString();
}

void HttpMultipartFormDataContent::Save(std::ostream& ost) const {
    m_impl->Save(ost);
}

std::size_t HttpMultipartFormDataContent::GetLength() const {
    return m_impl->GetLength();
}

void HttpMultipartFormDataContent::AddValue(std::string const& key, std::string const& value) {
    m_impl->AddValue(key, value);
}

void HttpMultipartFormDataContent::AddFile(std::string const& key, std::string const& filename,
                                           std::unique_ptr<std::istream> content,
                                           std::size_t contentSize, ContentType contentType) {

    m_impl->AddFile(key, filename, std::move(content), contentSize, contentType);
}

// free functions
std::ostream& operator<<(std::ostream& ost, HttpContent const& content) {
    content.Save(ost);
    return ost;
}