#include "PanelPrintPreview.h"

#include "../../Core/Log.h"
#include "../../CustomerPrintClientCore/PrintOffice.h"
#include "../../CustomerPrintClientCore/CoreManager.h"
#include "../../CustomerPrintClientCore/ConvertPrintProfile.h"
#include "../StringResources.h"
#include "CtrlCloseHeader.h"
#include "CtrlPrintProfiles.h"
#include "CtrlPrintPreviewCanvas.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/artprov.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/spinctrl.h>
#include <wx/wupdlock.h>

#include <string>

namespace PrintPreview {
enum CtrlIds {
    ID_SELECT_PRINT_PROFILE = wxID_HIGHEST + 150,
    ID_SELECT_ORIENTATION,
    ID_SELECT_MODE,
    ID_PAGE_ALIGN,
    ID_COPIES,
    ID_RUN_PRINT,
    ID_CLOSE_HEADER,
    ID_PREVIEW_CANVAS
};
} // namespace PrintPreview

PanelPrintPreview::PanelPrintPreview(wxWindow* parent, ICallback& callback, JobContentType contentType,
                                     wxArrayString const& files, PrintOfficePtr printOffice, PrintSettings& printSettings,
                                     int& copies, wxWindowID id, wxPoint const& pos, wxSize const& size, long style)
    : wxWindow(parent, id, pos, size, style)
    , m_callback(callback)
    , m_printOffice(printOffice)
    , m_printSettings(printSettings)
    , m_copies(copies)
    , m_pageAlignPopup(nullptr)
{
    std::vector<std::wstring> vfiles;
    vfiles.reserve(files.GetCount());
    for (wxString const& file : files) {
        vfiles.push_back(file.ToStdWstring());
    }
    m_previewRenderer = GetCoreManager().CreatePreviewRenderer(contentType, std::move(vfiles));
    CtrlCloseHeader* ctrlCloseHeader = new CtrlCloseHeader(this, PrintPreview::ID_CLOSE_HEADER);
    m_ctrlPreviewCanvas = new CtrlPrintPreviewCanvas(this, m_printOffice->GetSelectedPrintProfile(),
        m_printSettings, *m_previewRenderer, PrintPreview::ID_PREVIEW_CANVAS);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(ctrlCloseHeader, wxSizerFlags(0).Border(wxBOTTOM).Top().Expand());
    wxBoxSizer* centerSizer = new wxBoxSizer(wxHORIZONTAL);
    centerSizer->Add(m_ctrlPreviewCanvas, wxSizerFlags(1).Expand());
    if (contentType == JobContentType::Images) {
        centerSizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL),
            wxSizerFlags(0).Border(wxRIGHT).Expand());
        wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
        rightSizer->Add(new wxStaticText(this, wxID_ANY, L"Orientation:"), wxSizerFlags(0).Border(wxTOP));
        wchar_t const* orientations[] = {L"Portrait", L"Landscape"};
        wxComboBox* cmbOrientation = new wxComboBox(this, PrintPreview::ID_SELECT_ORIENTATION,
            orientations[static_cast<int>(m_printSettings.Orientation())], wxDefaultPosition, wxDefaultSize,
            wxArrayString(2, orientations), wxCB_READONLY);
        rightSizer->Add(cmbOrientation, wxSizerFlags(0).Expand());
        rightSizer->Add(new wxStaticText(this, wxID_ANY, L"Mode:"), wxSizerFlags(0).DoubleBorder(wxTOP));
        wchar_t const* modes[] = {L"Original size", L"Screen size", L"Stretch to page"};
        wxComboBox* cmbMode = new wxComboBox(this, PrintPreview::ID_SELECT_MODE,
            modes[static_cast<int>(m_printSettings.Mode())], wxDefaultPosition, wxDefaultSize,
            wxArrayString(3, modes), wxCB_READONLY);
        rightSizer->Add(cmbMode, wxSizerFlags(0).Expand());
        wxButton* btnPageAlign = new wxButton(this, PrintPreview::ID_PAGE_ALIGN, wxT("Align"));
        btnPageAlign->SetBitmap(wxArtProvider::GetBitmap(wxART_MISSING_IMAGE, wxART_TOOLBAR, wxSize(16, 16)));
        rightSizer->Add(btnPageAlign, wxSizerFlags(0).DoubleBorder(wxTOP).Expand());
        centerSizer->Add(rightSizer, wxSizerFlags(0).Border(wxRIGHT).Expand());

        Bind(wxEVT_COMBOBOX, &PanelPrintPreview::OnOrientationSelected, this, PrintPreview::ID_SELECT_ORIENTATION);
        Bind(wxEVT_COMBOBOX, &PanelPrintPreview::OnModeChanged, this, PrintPreview::ID_SELECT_MODE);
        Bind(wxEVT_BUTTON, &PanelPrintPreview::OnPageAlignClick, this, PrintPreview::ID_PAGE_ALIGN);
    }
    sizer->Add(centerSizer, wxSizerFlags(1).Expand());
    sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL),
        wxSizerFlags(0).Border(wxBOTTOM).Expand());
    wxBoxSizer* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
    m_ctrlPrintProfiles = new CtrlPrintProfiles(this, PrintPreview::ID_SELECT_PRINT_PROFILE, m_printOffice.get());
    bottomSizer->Add(m_ctrlPrintProfiles, wxSizerFlags(0).Border(wxLEFT));
    wxBoxSizer* copiesSizer = new wxBoxSizer(wxVERTICAL);
    copiesSizer->AddSpacer(wxSizerFlags::GetDefaultBorder()/2);
    copiesSizer->Add(new wxStaticText(this, wxID_ANY, L"Copies:"), wxSizerFlags(0));
    bottomSizer->Add(copiesSizer, wxSizerFlags(0).DoubleBorder(wxLEFT));
    bottomSizer->Add(new wxSpinCtrl(this, PrintPreview::ID_COPIES, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        wxSP_ARROW_KEYS, 1, 100, m_copies), wxSizerFlags(0).Border(wxLEFT));
    wxBoxSizer* runPrintSizer = new wxBoxSizer(wxVERTICAL);
    m_totalCost = new wxStaticText(this, wxID_ANY, GetTotalCostString(), wxDefaultPosition, wxSize(120, -1), wxST_NO_AUTORESIZE);
    m_totalCost->SetFont(m_totalCost->GetFont().MakeBold());
    wxButton* btnPrint = new wxButton(this, PrintPreview::ID_RUN_PRINT, wxT("PRINT"), wxDefaultPosition,
        wxSize(120, 30));
    runPrintSizer->AddSpacer(wxSizerFlags::GetDefaultBorder()/2);
    runPrintSizer->Add(m_totalCost, wxSizerFlags(0).Border(wxLEFT).Left());
    runPrintSizer->Add(btnPrint, wxSizerFlags(0).Border().Left().Expand());
    bottomSizer->AddStretchSpacer();
    bottomSizer->Add(runPrintSizer, wxSizerFlags(0));
    sizer->Add(bottomSizer, wxSizerFlags(0).Expand());
    SetSizer(sizer);
    
    Bind(wxEVT_BUTTON, &PanelPrintPreview::OnHeaderCloseClick, this, PrintPreview::ID_CLOSE_HEADER);
    Bind(wxEVT_PRINT_PROFILES, &PanelPrintPreview::OnPrintProfileSelect, this, PrintPreview::ID_SELECT_PRINT_PROFILE);
    Bind(wxEVT_SPINCTRL, &PanelPrintPreview::OnCopiesChanged, this, PrintPreview::ID_COPIES);
    Bind(wxEVT_BUTTON, &PanelPrintPreview::OnPrintClick, this, PrintPreview::ID_RUN_PRINT);

    m_ctrlPreviewCanvas->SetFocus();
}

PanelPrintPreview::~PanelPrintPreview() {
    Log("PanelPrintPreview::~PanelPrintPreview()\n");
}

void PanelPrintPreview::OnPageAlignPopupClose(HorizontalPageAlign horAlign, VerticalPageAlign vertAlign) {
    if (m_printSettings.HorAlign() != horAlign || m_printSettings.VertAlign() != vertAlign) {
        m_printSettings.HorAlign(horAlign).VertAlign(vertAlign);
        m_ctrlPreviewCanvas->NotifyPrintSettingsChanged();
    }
}

void PanelPrintPreview::SetPrintOffice(PrintOfficePtr printOffice) {
    if (printOffice->Id == m_printOffice->Id) {
        return;
    }
    {
        wxWindowUpdateLocker noUpdates(this);
        m_printOffice = printOffice;
        CtrlPrintProfiles* ctrlPrintProfiles = new CtrlPrintProfiles(this, PrintPreview::ID_SELECT_PRINT_PROFILE, m_printOffice.get());
        GetSizer()->Replace(m_ctrlPrintProfiles, ctrlPrintProfiles, true);
        delete m_ctrlPrintProfiles;
        m_ctrlPrintProfiles = ctrlPrintProfiles;
        GetSizer()->Layout();
    }
    PrintProfilePtr profile = m_printOffice->GetSelectedPrintProfile();
    m_previewRenderer->SetPrintProfile(ConvertPrintProfile(profile));
    m_ctrlPreviewCanvas->SetPrintProfile(profile);
    UpdateTotalCostText();
}

void PanelPrintPreview::OnHeaderCloseClick(wxCommandEvent& /* ev */) {
    m_callback.OnPreviewCloseClick();
}

void PanelPrintPreview::OnPrintProfileSelect(wxCommandEvent& ev) {
    m_printOffice->SelectPrintProfile(ev.GetString().ToUTF8().data());
    GetCoreManager().UpdateConfigSelectedProfile();
    PrintProfilePtr profile = m_printOffice->GetSelectedPrintProfile();
    m_previewRenderer->SetPrintProfile(ConvertPrintProfile(profile));
    m_ctrlPreviewCanvas->SetPrintProfile(profile);
    UpdateTotalCostText();
}

void PanelPrintPreview::OnOrientationSelected(wxCommandEvent& ev) {
    PrintOrientation const newOrientation = static_cast<PrintOrientation>(ev.GetInt());
    if (m_printSettings.Orientation() != newOrientation) {
        m_printSettings.Orientation(newOrientation);
        m_ctrlPreviewCanvas->NotifyPrintSettingsChanged();
    }
}

void PanelPrintPreview::OnModeChanged(wxCommandEvent& ev) {
    PrintMode const newMode = static_cast<PrintMode>(ev.GetInt());
    if (m_printSettings.Mode() != newMode) {
        m_printSettings.Mode(newMode);
        m_ctrlPreviewCanvas->NotifyPrintSettingsChanged();
    }
}

void PanelPrintPreview::OnPageAlignClick(wxCommandEvent& ev) {
    if (!m_pageAlignPopup) {
        m_pageAlignPopup = new CtrlPageAlignPopup(this, *this);
    }
    wxWindow* btn = static_cast<wxWindow*>(ev.GetEventObject());
    m_pageAlignPopup->Position(btn->ClientToScreen(wxPoint(0, 0)), btn->GetSize());
    m_pageAlignPopup->SetDataAndPopup(m_printSettings.HorAlign(), m_printSettings.VertAlign());
}

void PanelPrintPreview::OnCopiesChanged(wxCommandEvent& ev) {
    m_copies = ev.GetInt();
    UpdateTotalCostText();
}

void PanelPrintPreview::OnPrintClick(wxCommandEvent& /* ev */) {
    m_callback.OnPrintClick();
}

int PanelPrintPreview::GetPageCount() const {
    return m_copies * m_previewRenderer->GetPageCount();
}

wxString PanelPrintPreview::GetTotalCostString() const {
    float cost(0);
    wxString currency;
    if (m_printOffice) {
        currency = wxString::FromUTF8(m_printOffice->CurrencySymbol);
        if (PrintProfilePtr pp = m_printOffice->GetSelectedPrintProfile()) {
            cost = pp->Price * GetPageCount();
        }
    }
    return wxString::Format(Gui::TotalCostFmt, cost, currency);
}

void PanelPrintPreview::UpdateTotalCostText() {
    m_totalCost->SetLabel(GetTotalCostString());
}