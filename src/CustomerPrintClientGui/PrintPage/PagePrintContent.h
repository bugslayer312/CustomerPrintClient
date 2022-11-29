#pragma once

// #include "PanelSelectContentType.h"
#include "PanelBrowseFile.h"
#include "PanelPrintPreview.h"

class wxShowEvent;
class PrintSettings;

class PagePrintContent : public wxWindow
                       , public PanelBrowseFile::ICallback
                       , public PanelPrintPreview::ICallback {
public:
    class ICallback {
    public:
        virtual ~ICallback() = default;
        virtual void RunPrinting(JobContentType contentType, wxArrayString const& files,
            PrintSettings const& printSettings, int copies) = 0;
    };

    PagePrintContent(wxWindow* parent, ICallback& callback, wxWindowID id, wxPoint const& pos = wxDefaultPosition,
                     wxSize const& size = wxDefaultSize, long style = 0);
    virtual ~PagePrintContent() override;
    void UpdatePrintOffice();

    virtual void OnContentTypeSelected(JobContentType contentType) override;
    virtual void OnBrowseResult() override;
    virtual void OnPreviewCloseClick() override;
    virtual void OnPrintClick() override;

private:
    ICallback& m_callback;
    JobContentType m_contentType;
    wxArrayString m_selectedFiles;
    std::unique_ptr<PrintSettings> m_printSettings;
    int m_copies;
    // PanelSelectContentType* m_selectContentTypePanel;
    PanelBrowseFile* m_browsePanel;
    PanelPrintPreview* m_previewPanel;
};