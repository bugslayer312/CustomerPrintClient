#include "DlgJobCard.h"

#include "../../Core/StringUtilities.h"
#include "../StringResources.h"
#include "../../CustomerPrintClientCore/PrintJobInfo.h"
#include "../../CustomerPrintClientCore/CoreManager.h"
#include "../../CustomerPrintClientCore/PrintOffice.h"
#include "../MapPage/CtrlRating.h"
#include "Styles.h"

#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/statbmp.h>
#include <wx/log.h>
#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/wupdlock.h>

DlgJobCard::DlgJobCard(wxWindow* parent, wxSize const& size, PrintJobInfoPtr job)
    : wxDialog(parent, wxID_ANY, Gui::JobDetails, wxDefaultPosition, size, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
    , m_job(job)
{
    SetMinSize(size);
    int const border = wxSizerFlags::GetDefaultBorder();
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxGridBagSizer* headSizer = new wxGridBagSizer(0, 0);
    headSizer->AddGrowableCol(0);

    wxStaticText* lblAccessCode = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(m_job->AccessCode));
    wxFont bigBoldFont = lblAccessCode->GetFont();
    bigBoldFont.MakeBold();
    bigBoldFont.SetPointSize(bigBoldFont.GetPointSize()+2);
    lblAccessCode->SetFont(bigBoldFont);
    wxStaticText* lblPayStatus = new wxStaticText(this, wxID_ANY, Gui::GetPaidString(m_job->PayedOff));
    wxFont mediumFont = lblPayStatus->GetFont();
    mediumFont.SetPointSize(mediumFont.GetPointSize()+1);
    lblPayStatus->SetFont(mediumFont);
    wxStaticText* lblCost = new wxStaticText(this, wxID_ANY, wxString::Format(Gui::CostFmt, m_job->Price,
        wxString::FromUTF8(m_job->CurrencySymbol)));
    lblPayStatus->SetFont(mediumFont);
    wxFont mediumBoldFont = mediumFont.Bold();
    lblCost->SetFont(mediumBoldFont);
    headSizer->Add(lblAccessCode, wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_LEFT|wxALIGN_BOTTOM|wxLEFT, border);
    headSizer->Add(lblPayStatus, wxGBPosition(0, 1), wxDefaultSpan, wxALIGN_LEFT|wxALIGN_BOTTOM);
    headSizer->Add(lblCost, wxGBPosition(0, 2), wxDefaultSpan, wxALIGN_RIGHT|wxALIGN_BOTTOM|wxRIGHT, border);
    
    wxStaticText* lblStatus = new wxStaticText(this, wxID_ANY, PrintJobStatusToString(m_job->Status));
    lblStatus->SetFont(mediumFont);
    lblStatus->SetForegroundColour(GetColorByStatus(m_job->Status));
    wxStaticText* lblUpdateTime = new wxStaticText(this, wxID_ANY, Strings::UTCDateTimeToLocalTimeString(m_job->UpdateTime));
    lblUpdateTime->SetFont(mediumFont);
    lblUpdateTime->SetForegroundColour(colorGray);
    headSizer->Add(lblStatus, wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_LEFT|wxALIGN_BOTTOM|wxLEFT|wxTOP, border);
    headSizer->Add(lblUpdateTime, wxGBPosition(1, 1), wxGBSpan(1, 2), wxALIGN_RIGHT|wxALIGN_BOTTOM|wxRIGHT|wxTOP, border);

    sizer->Add(headSizer, wxSizerFlags(0).Expand().Border(wxTOP));

    wxStaticText* lblPagesCopies = new wxStaticText(this, wxID_ANY,
        wxString::Format(Gui::PagesCopiesFmt, static_cast<int>(m_job->PagesCount), static_cast<int>(m_job->Copies)));
    sizer->Add(lblPagesCopies, wxSizerFlags(0).Expand().Border(wxLEFT|wxRIGHT|wxTOP));

    wxStaticText* lblJobName = new wxStaticText(this, wxID_ANY, wxString::Format(Gui::JobNameFmt, m_job->Name));
    sizer->Add(lblJobName, wxSizerFlags(0).Expand().Border(wxLEFT|wxRIGHT));

    sizer->Add(new wxStaticLine(this, wxID_ANY), wxSizerFlags(0).Expand().Border());

    m_lblProfileName = new wxStaticText(this, wxID_ANY, Gui::Loading);
    m_lblProfileName->SetFont(bigBoldFont);
    m_lblPaper = new wxStaticText(this, wxID_ANY, Gui::Loading);
    m_lblPagePrice = new wxStaticText(this, wxID_ANY, Gui::Loading);
    wxFont mediumNarrowFont = mediumFont.Italic();
    m_lblPaper->SetFont(mediumNarrowFont);
    m_lblPagePrice->SetFont(mediumNarrowFont);
    m_lblPaper->SetForegroundColour(colorGray);
    m_lblPagePrice->SetForegroundColour(colorGray);
    sizer->Add(m_lblProfileName, wxSizerFlags(0).Expand().Border(wxLEFT|wxRIGHT|wxTOP));
    sizer->Add(m_lblPaper, wxSizerFlags(0).Expand().Border(wxLEFT|wxRIGHT|wxTOP));
    sizer->Add(m_lblPagePrice, wxSizerFlags(0).Expand().Border(wxLEFT|wxRIGHT|wxTOP));

    sizer->Add(new wxStaticLine(this, wxID_ANY), wxSizerFlags(0).Expand().Border());

    wxGridBagSizer* officeHeadSizer = new wxGridBagSizer(0, 0);
    officeHeadSizer->AddGrowableCol(0);
    wxStaticText* lblOfficeName = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(m_job->OfficeName));
    lblOfficeName->SetFont(bigBoldFont);
    m_ctrlRating = new CtrlRating(this, wxID_ANY);
    officeHeadSizer->Add(lblOfficeName, wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_LEFT|wxALIGN_BOTTOM|wxLEFT, border);
    officeHeadSizer->Add(m_ctrlRating, wxGBPosition(0, 1), wxDefaultSpan, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxRIGHT, border);

    sizer->Add(officeHeadSizer, wxSizerFlags(0).Expand().Border(wxTOP));

    m_lblOfficeAddress = new wxStaticText(this, wxID_ANY, Gui::Loading);
    m_lblOfficePhone = new wxStaticText(this, wxID_ANY, Gui::Loading);
    m_lblOfficePhone->SetFont(mediumFont);
    sizer->Add(m_lblOfficeAddress, wxSizerFlags(0).Expand().Border(wxLEFT|wxRIGHT|wxTOP));
    sizer->Add(m_lblOfficePhone, wxSizerFlags(0).Expand().Border(wxLEFT|wxRIGHT|wxTOP));

    SetSizer(sizer);

    if (!UpdateOfficeAndProfile()) {
        IAsyncResultPtr asyncResult = GetCoreManager().AsyncGetPrintOffice(m_job->OfficeId,
            std::bind(&DlgJobCard::UpdateOfficeAndProfile, this));
        StoreAsyncResult(asyncResult);
    }

    SetSizer(sizer);
    Bind(wxEVT_IDLE, &DlgJobCard::OnIdle, this);

    Center();
}

bool DlgJobCard::UpdateOfficeAndProfile() {
    if (PrintOfficePtr printOffice = GetCoreManager().GetPrintOffice(m_job->OfficeId)) {
        std::unique_ptr<wxWindow, void(*)(wxWindow*)> layoutUpdater(this, [](wxWindow* wnd) {
            wnd->Layout();
        });
        m_lblOfficeAddress->SetLabelText(wxString::FromUTF8(printOffice->Address));
        m_lblOfficeAddress->Wrap(GetClientSize().x - wxSizerFlags::GetDefaultBorder()*4);
        m_lblOfficePhone->SetLabelText(wxString::FromUTF8(printOffice->Phone));
        m_ctrlRating->SetRating(printOffice->Rating);
        if (PrintProfilePtr printProfile = printOffice->GetPrintProfile(m_job->ProfileId)) {
            m_lblProfileName->SetLabelText(wxString::FromUTF8(printProfile->Name));
            m_lblPaper->SetLabelText(wxString::Format(Gui::PaperFmt, wxString::FromUTF8(printProfile->Paper.Name),
                printProfile->Paper.Width/100, printProfile->Paper.Height/100));
            m_lblPagePrice->SetLabelText(wxString::Format(Gui::PriceFmt, printProfile->Price,
                wxString::FromUTF8(printOffice->CurrencySymbol)));
            return true;
        }
        return true;
    }
    return false;
}

void DlgJobCard::OnIdle(wxIdleEvent& /* ev */) {
    RetrieveAsyncResult([this](char const* /* title */, char const* errorMsg, ExceptionCategory /*category*/){
        wxLogError(errorMsg);
    });
}