#pragma once

//#include "HBitmapPtr.h"
#include "IPreviewRenderer.h"
#include "ImageFormat.h"

class PrintSettings;

namespace Drawing {

struct Rect;
struct Point;

} // namespace Drawing

class IDocumentRenderer : public IPreviewRenderer {
public:
    virtual std::string GetJobName() const = 0;
    virtual std::wstring GetPageFileName(std::size_t pageNum) const = 0;
    virtual void RenderForPrinting(std::size_t pageNum, PrintSettings const& printSettings, ImageFormat outFormat,
        std::ostream& outStream, std::size_t& outStreamSize) const = 0;
};
typedef std::unique_ptr<IDocumentRenderer> IDocumentRendererUPtr;

class DocumentRendererBase : public IDocumentRenderer {
public:
    DocumentRendererBase(PrintProfile const& printProfile)
        : m_printProfile(printProfile)
    {
    }

    virtual void SetPrintProfile(PrintProfile const& printProfile) override {
        m_printProfile = printProfile;
    }

protected:
    PrintProfile m_printProfile;
};