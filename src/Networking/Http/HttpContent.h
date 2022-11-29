#pragma once

#include "HttpDefinitions.h"

#include <iosfwd>
#include <memory>

namespace jsonxx {
class Object;
}

class HttpContent {
public:
    HttpContent(ContentType m_type);
    virtual ~HttpContent() = default;
    virtual std::string GetContentTypeString() const;
    virtual void Save(std::ostream& ost) const = 0;
    virtual std::size_t GetLength() const = 0;

protected:
    ContentType m_type;
};
typedef std::unique_ptr<HttpContent> HttpContentUPtr;


// HttpStringContent

class HttpStringContent : public HttpContent {
public:
    HttpStringContent(std::string&& str);
    virtual void Save(std::ostream& ost) const override;
    virtual std::size_t GetLength() const override;

private:
    std::string const m_str;
};


// HttpJsonContent

class HttpJsonContent : public HttpStringContent {
public:
    HttpJsonContent(jsonxx::Object const& json);
};


// HttpStreamContent

class HttpStreamContent : public HttpContent {
public:
    HttpStreamContent(std::unique_ptr<std::istream> ist);
    virtual void Save(std::ostream& ost) const override;
    virtual std::size_t GetLength() const override;

private:
    std::unique_ptr<std::istream> m_ist;
};


// HttpMultipartFormDataContent

class HttpMultipartFormDataContentImpl;

class HttpMultipartFormDataContent : public HttpContent {
public:
    HttpMultipartFormDataContent();
    virtual ~HttpMultipartFormDataContent() override;
    virtual std::string GetContentTypeString() const override;
    virtual void Save(std::ostream& ost) const override;
    virtual std::size_t GetLength() const override;

    void AddValue(std::string const& key, std::string const& value);
    void AddFile(std::string const& key, std::string const& filename, std::unique_ptr<std::istream> content,
                 std::size_t contentSize, ContentType contentType);

private:
    std::unique_ptr<HttpMultipartFormDataContentImpl> m_impl;
};

std::ostream& operator<<(std::ostream& ost, HttpContent const& content);