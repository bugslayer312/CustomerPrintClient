#include "PagePrintContent.h"

#include "../../Core/Log.h"
#include "../../Printing/PrintSettings.h"
#include "../../CustomerPrintClientCore/CoreManager.h"

#include <wx/sizer.h>
#include <wx/wupdlock.h>

PagePrintContent::PagePrintContent(wxWindow* parent, PagePrintContent::ICallback& callback,
                                   wxWindowID id, wxPoint const& pos, wxSize const& size, long style)
    : wxWindow(parent, id, pos, size, style)
    , m_callback(callback)
    , m_contentType(JobContentType::Images)
    , m_printSettings(new PrintSettings())
    , m_copies(1)
    , m_previewPanel(nullptr)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_browsePanel = new PanelBrowseFile(this, *this, m_contentType, m_selectedFiles);
    sizer->Add(m_browsePanel, wxSizerFlags(1).Expand());
    SetSizer(sizer);
}

PagePrintContent::~PagePrintContent() {
}

void PagePrintContent::UpdatePrintOffice() {
    if (m_previewPanel) {
        m_previewPanel->SetPrintOffice(GetCoreManager().GetSelectedOffice());
    }
}

void PagePrintContent::OnContentTypeSelected(JobContentType contentType) {
    m_contentType = contentType;
}

void PagePrintContent::OnBrowseResult() {
    if (m_contentType == JobContentType::Pdf) {
        m_printSettings->Orientation(PrintOrientation::Portrait);
    }
    m_previewPanel = new PanelPrintPreview(this, *this, m_contentType, m_selectedFiles, GetCoreManager().GetSelectedOffice(),
        *m_printSettings, m_copies, wxID_ANY, wxDefaultPosition, GetSize());
    m_previewPanel->Hide();
    GetSizer()->Add(m_previewPanel, wxSizerFlags(1).Expand());
    m_previewPanel->ShowWithEffect(wxSHOW_EFFECT_EXPAND);
    m_browsePanel->Hide();
    Layout();
}

void PagePrintContent::OnPreviewCloseClick() {
    if (m_previewPanel) {
        wxWindowUpdateLocker noUpdates(this);
        GetSizer()->Detach(m_previewPanel);
        m_previewPanel->Destroy();
        m_previewPanel = nullptr;
        m_browsePanel->Show();
        m_browsePanel->SetSize(GetSize());
    }
}

void PagePrintContent::OnPrintClick() {
    m_callback.RunPrinting(m_contentType, m_selectedFiles, *m_printSettings, m_copies);
}