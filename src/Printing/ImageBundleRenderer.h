#pragma once

#include "IDocumentRenderer.h"

#include <string>
#include <vector>

class ImageBundleRenderer : public DocumentRendererBase {
public:
    ImageBundleRenderer(std::vector<std::wstring>&& imagePaths, PrintProfile const& printProfile);
    virtual std::size_t GetPageCount() const override;
    virtual HBITMAPUPtr RenderForPreview(std::size_t pageNum, PrintSettings const& printSettings, Drawing::Rect const& pageRect,
        Drawing::Point& outOrigin) const override;
    virtual std::string GetJobName() const override;
    virtual std::wstring GetPageFileName(std::size_t pageNum) const override;
    virtual void RenderForPrinting(std::size_t pageNum, PrintSettings const& printSettings, ImageFormat outFormat,
        std::ostream& outStream, std::size_t& outStreamSize) const override;

private:
    std::vector<std::wstring> m_imagePaths;
};