#pragma once

#include "IDocumentRenderer.h"

namespace Pdf {

class Library;
class Document;

} // namespace Pdf

class PdfDocumentRenderer : public DocumentRendererBase {
public:
    PdfDocumentRenderer(Pdf::Library& pdfLib, std::wstring const& filePath, PrintProfile const& printProfile);
    virtual ~PdfDocumentRenderer() override;
    virtual std::size_t GetPageCount() const override;
    virtual HBITMAPUPtr RenderForPreview(std::size_t pageNum, PrintSettings const& printSettings, Drawing::Rect const& pageRect,
        Drawing::Point& outOrigin) const override;
    virtual std::string GetJobName() const override;
    virtual std::wstring GetPageFileName(std::size_t pageNum) const override;
    virtual void RenderForPrinting(std::size_t pageNum, PrintSettings const& printSettings, ImageFormat outFormat,
        std::ostream& outStream, std::size_t& outStreamSize) const override;

private:
    std::unique_ptr<Pdf::Document> m_document;
    std::wstring m_fileName;
};