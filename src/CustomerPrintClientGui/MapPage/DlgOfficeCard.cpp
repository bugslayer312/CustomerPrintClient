#include "DlgOfficeCard.h"

#include "../../Core/Format.h"
#include "../../CustomerPrintClientCore/PrintOffice.h"
#include "../StringResources.h"
#include "CtrlRating.h"

#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/statbmp.h>
#include <wx/image.h>
#include <wx/stattext.h>
#include <wx/scrolwin.h>

DlgOfficeCard::DlgOfficeCard(wxWindow* parent, wxSize const& size, PrintOffice const& office)
    : wxDialog(parent, wxID_ANY, Gui::PrintOffice, wxDefaultPosition, size, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
    , m_office(office)
{
    SetMinSize(wxSize(300, 350));
    int const locBmpWidth = 32;
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* captionSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticBitmap* bmpCtrlLocation = new wxStaticBitmap(this, wxID_ANY, wxBitmap(L"OFFICE_LOCATION", wxBITMAP_TYPE_PNG_RESOURCE),
        wxDefaultPosition, wxSize(locBmpWidth, locBmpWidth));
    wxStaticText* txtCaption = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(office.Name)
        , wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
    wxFont smallFonf = txtCaption->GetFont();
    wxFont midFonf(smallFonf);
    wxFont bigFonf(smallFonf);
    wxColor colorGray(100, 100, 100);
    bigFonf.SetPointSize(bigFonf.GetPointSize()+3);
    midFonf.SetPointSize(midFonf.GetPointSize()+2);
    bigFonf.SetNumericWeight(600);
    txtCaption->SetFont(bigFonf);
    txtCaption->Wrap(300);
    CtrlRating* ctrlRating = new CtrlRating(this, wxID_ANY, office.Rating);
    ctrlRating->SetPadding(wxTOP, 6);
    captionSizer->Add(bmpCtrlLocation, wxSizerFlags(0).Left().Border(wxRIGHT));
    captionSizer->Add(txtCaption, wxSizerFlags(1).Expand());
    captionSizer->Add(ctrlRating, wxSizerFlags(0).Align(wxRIGHT).DoubleBorder(wxLEFT));
    mainSizer->Add(captionSizer, wxSizerFlags(0).Top().DoubleBorder(wxLEFT|wxRIGHT|wxTOP).Expand());
    if (!office.Description.empty()) {
        wxBoxSizer* descSizer = new wxBoxSizer(wxHORIZONTAL);
        descSizer->AddSpacer(locBmpWidth + wxSizerFlags::GetDefaultBorder());
        wxStaticText* txtDesc = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(office.Description)
            , wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
        txtDesc->Wrap(300);
        txtDesc->SetFont(midFonf);
        txtDesc->SetForegroundColour(colorGray);
        descSizer->Add(txtDesc, wxSizerFlags(0).Left());
        mainSizer->Add(descSizer, wxSizerFlags(0).Top().Expand().DoubleBorder(wxLEFT|wxRIGHT|wxTOP));
    }
    wxBoxSizer* addressSizer = new wxBoxSizer(wxHORIZONTAL);
    addressSizer->AddSpacer(locBmpWidth + wxSizerFlags::GetDefaultBorder());
    wxStaticText* txtAddress = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(office.Address));
    txtAddress->Wrap(300);
    addressSizer->Add(txtAddress, wxSizerFlags(0).Left());
    mainSizer->Add(addressSizer, wxSizerFlags(0).Top().Expand().DoubleBorder(wxLEFT|wxRIGHT|wxTOP));

    const int phoneBmpWidth = 20;
    if (!office.Phone.empty()) {
        wxBoxSizer* phoneSizer = new wxBoxSizer(wxHORIZONTAL);
        wxStaticBitmap* bmpCtrlPhone = new wxStaticBitmap(this, wxID_ANY, wxBitmap(L"OFFICE_PHONE", wxBITMAP_TYPE_PNG_RESOURCE),
            wxDefaultPosition, wxSize(phoneBmpWidth, phoneBmpWidth));
        wxStaticText* txtPhone = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(office.Phone));
        phoneSizer->AddSpacer(locBmpWidth - phoneBmpWidth);
        phoneSizer->Add(bmpCtrlPhone, wxSizerFlags(0).Left().Border(wxRIGHT));
        phoneSizer->Add(txtPhone, wxSizerFlags(1).Expand());
        mainSizer->Add(phoneSizer, wxSizerFlags(0).Top().Expand().DoubleBorder(wxLEFT|wxRIGHT|wxTOP));
    }

    wxStaticText* txtServices = new wxStaticText(this, wxID_ANY, Gui::PrintServices);
    txtServices->SetFont(bigFonf);
    mainSizer->Add(txtServices, wxSizerFlags(0).Top().Expand().DoubleBorder(wxLEFT|wxRIGHT|wxTOP));

    wxScrolledWindow* panProfiles = new wxScrolledWindow(this);
    panProfiles->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT);
    wxBoxSizer* profileSizer = new wxBoxSizer(wxVERTICAL);
    wxFont profileNameFont(midFonf);
    wxFont priceFont = smallFonf.Italic();
    profileNameFont.SetNumericWeight(600);
    for (std::size_t i(0); i < office.GetPrintProfileCount(); ++i) {
        PrintProfilePtr profile = office.GetPrintProfile(i);
        wxStaticText* txtProfileName = new wxStaticText(panProfiles, wxID_ANY, wxString::FromUTF8(profile->Name)
            , wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
        txtProfileName->SetFont(profileNameFont);
        profileSizer->Add(txtProfileName, wxSizerFlags(0).Border(wxTOP));
        
        wxStaticText* txtPaper = new wxStaticText(panProfiles, wxID_ANY, wxString::Format(Gui::PaperFmt,
            wxString::FromUTF8(profile->Paper.Name), profile->Paper.Width/100, profile->Paper.Height/100));
        txtPaper->SetForegroundColour(colorGray);
        profileSizer->Add(txtPaper);

        wxStaticText* txtPrice = new wxStaticText(panProfiles, wxID_ANY, wxString::Format(Gui::PriceFmt,
            profile->Price, wxString::FromUTF8(office.CurrencySymbol)));
        txtPrice->SetForegroundColour(colorGray);
        txtPrice->SetFont(priceFont);
        profileSizer->Add(txtPrice);
    }

    panProfiles->SetSizer(profileSizer);
    panProfiles->FitInside();
    panProfiles->SetScrollRate(5, 5);
    mainSizer->Add(panProfiles, wxSizerFlags(1).Expand().DoubleBorder(wxLEFT|wxRIGHT));

    wxButton* btnOk = new wxButton(this, wxID_OK, L"Print here", wxDefaultPosition, wxSize(200, wxDefaultCoord));
    mainSizer->Add(btnOk, wxSizerFlags(0).CenterHorizontal().DoubleBorder());

    SetEscapeId(wxID_NONE);
    // SetAutoLayout(true);
    SetSizer(mainSizer);
    // mainSizer->SetSizeHints(this);
    // mainSizer->Fit(this);
    Center();
}