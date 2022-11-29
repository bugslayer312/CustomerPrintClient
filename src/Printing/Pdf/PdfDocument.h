#pragma once

#include "../HBitmapPtr.h"
#include "../Primitives.h"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace Pdf {

class Library;
struct InternalData;
class IPasswordProvider;
class Page;
typedef std::unique_ptr<Page> PageUPtr;

class Document {
private:
    Document(std::string const& filePath, IPasswordProvider& passwordProvider);
public:
    Document(Document const&) = delete;
    Document(Document&&) = delete;
    Document& operator=(Document const&) = delete;
    Document& operator=(Document&&) = delete;
    ~Document();
    std::istream* GetStream() const;
    operator bool() const;
    std::size_t GetPageCount() const;
    Drawing::Size GetPageSize(std::size_t pageNum) const;
    HBITMAPUPtr RenderPage(std::size_t pageNum, Drawing::Size const& size, bool grayScale);

    friend class Library;

private:
    std::unique_ptr<std::istream> m_stream;
    std::unique_ptr<InternalData> m_data;
    bool m_linearized;
    std::vector<PageUPtr> m_pages;
};

} // namespace Pdf