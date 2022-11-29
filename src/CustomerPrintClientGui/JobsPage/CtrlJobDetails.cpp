#include "CtrlJobDetails.h"

#include "Styles.h"

#include "../../Core/StringUtilities.h"
#include "../../CustomerPrintClientCore/PrintJobInfo.h"
#include "../StringResources.h"
#include "DlgJobCard.h"
#include "IJobListItemCallback.h"

// #include <wx/dcclient.h>
#include <wx/bitmap.h>
#include <wx/stattext.h>
#include <wx/statbmp.h>
#include <wx/gbsizer.h>

int const CtrlJobDetails::m_margin = 6;
wxSize const CtrlJobDetails::m_imgSize(32, 32);

CtrlJobDetails::CtrlJobDetails(wxWindow* parent, IJobListItemCallback& callback, PrintJobInfoPtr job)
    : wxWindow(parent, wxID_ANY)
    , m_callback(callback)
    , m_job(job)
{
    bool const isPdf = wxString::FromUTF8(job->Name).EndsWith(L".pdf");
    wxGridBagSizer* sizer = new wxGridBagSizer(0, 0);
    wxBitmap const bmpContent(isPdf ? L"CONTENT_PDF" : L"CONTENT_IMAGE", wxBITMAP_TYPE_PNG_RESOURCE);
    wxStaticBitmap* ctrlImages = new wxStaticBitmap(this, wxID_ANY, bmpContent, wxDefaultPosition, m_imgSize);
    sizer->Add(ctrlImages, wxGBPosition(0, 0), wxGBSpan(3, 1), wxALIGN_CENTER|wxALL, 5);
    wxStaticText* lblAccessCode = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(m_job->AccessCode));
    wxFont font = lblAccessCode->GetFont();
    font.MakeBold();
    font.SetPointSize(font.GetPointSize()+1);
    lblAccessCode->SetFont(font);
    m_lblPayStatus = new wxStaticText(this, wxID_ANY, Gui::GetPaidString(m_job->PayedOff));
    wxStaticText* lblCost = new wxStaticText(this, wxID_ANY, wxString::Format(Gui::CostFmt, m_job->Price,
        wxString::FromUTF8(m_job->CurrencySymbol)));
    font = lblCost->GetFont();
    font.MakeBold();
    lblCost->SetFont(font);
    sizer->Add(lblAccessCode, wxGBPosition(0, 1), wxGBSpan(1, 1), wxALIGN_LEFT|wxALIGN_BOTTOM|wxTOP, 5);
    sizer->Add(m_lblPayStatus, wxGBPosition(0, 2), wxGBSpan(1, 1), wxALIGN_RIGHT|wxALIGN_BOTTOM|wxEXPAND|wxTOP, 5);
    sizer->Add(lblCost, wxGBPosition(0, 3), wxGBSpan(1, 1), wxALIGN_RIGHT|wxALIGN_BOTTOM|wxTOP|wxRIGHT, 5);
    m_lblStatus = new wxStaticText(this, wxID_ANY, PrintJobStatusToString(m_job->Status));
    m_lblStatus->SetForegroundColour(GetColorByStatus(m_job->Status));
    m_lblUpdateTime = new wxStaticText(this, wxID_ANY, Strings::UTCDateTimeToLocalTimeString(m_job->UpdateTime));
    m_lblUpdateTime->SetForegroundColour(colorGray);
    sizer->Add(m_lblStatus, wxGBPosition(1, 1), wxGBSpan(1, 1), wxALIGN_LEFT|wxALIGN_BOTTOM);
    sizer->Add(m_lblUpdateTime, wxGBPosition(1, 2), wxGBSpan(1, 2), wxALIGN_RIGHT|wxALIGN_BOTTOM|wxEXPAND|wxRIGHT, 5);
    wxStaticText* lblOfficeName = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(m_job->OfficeName));
    sizer->Add(lblOfficeName, wxGBPosition(2, 1), wxGBSpan(1, 3), wxALIGN_LEFT|wxBOTTOM, 5);
    sizer->AddGrowableCol(2);
    SetSizerAndFit(sizer);
    ctrlImages->Bind(wxEVT_LEFT_DOWN, &CtrlJobDetails::OnMouseLeftDown, this);
    ctrlImages->Bind(wxEVT_LEFT_DCLICK, &CtrlJobDetails::OnMouseLeftDoubleClick, this);
    lblAccessCode->Bind(wxEVT_LEFT_DOWN, &CtrlJobDetails::OnMouseLeftDown, this);
    lblAccessCode->Bind(wxEVT_LEFT_DCLICK, &CtrlJobDetails::OnMouseLeftDoubleClick, this);
    m_lblPayStatus->Bind(wxEVT_LEFT_DOWN, &CtrlJobDetails::OnMouseLeftDown, this);
    m_lblPayStatus->Bind(wxEVT_LEFT_DCLICK, &CtrlJobDetails::OnMouseLeftDoubleClick, this);
    lblCost->Bind(wxEVT_LEFT_DOWN, &CtrlJobDetails::OnMouseLeftDown, this);
    lblCost->Bind(wxEVT_LEFT_DCLICK, &CtrlJobDetails::OnMouseLeftDoubleClick, this);
    m_lblStatus->Bind(wxEVT_LEFT_DOWN, &CtrlJobDetails::OnMouseLeftDown, this);
    m_lblStatus->Bind(wxEVT_LEFT_DCLICK, &CtrlJobDetails::OnMouseLeftDoubleClick, this);
    m_lblUpdateTime->Bind(wxEVT_LEFT_DOWN, &CtrlJobDetails::OnMouseLeftDown, this);
    m_lblUpdateTime->Bind(wxEVT_LEFT_DCLICK, &CtrlJobDetails::OnMouseLeftDoubleClick, this);
    lblOfficeName->Bind(wxEVT_LEFT_DOWN, &CtrlJobDetails::OnMouseLeftDown, this);
    lblOfficeName->Bind(wxEVT_LEFT_DCLICK, &CtrlJobDetails::OnMouseLeftDoubleClick, this);
    Bind(wxEVT_LEFT_DOWN, &CtrlJobDetails::OnMouseLeftDown, this);
    Bind(wxEVT_LEFT_DCLICK, &CtrlJobDetails::OnMouseLeftDoubleClick, this);
}

void CtrlJobDetails::UpdateJob() {
    m_lblPayStatus->SetLabelText(Gui::GetPaidString(m_job->PayedOff));
    m_lblStatus->SetForegroundColour(GetColorByStatus(m_job->Status));
    m_lblStatus->SetLabelText(PrintJobStatusToString(m_job->Status));
    m_lblUpdateTime->SetLabelText(Strings::UTCDateTimeToLocalTimeString(m_job->UpdateTime));
}

void CtrlJobDetails::OnMouseLeftDown(wxMouseEvent& ev) {
    m_callback.OnJobClicked(this);
    ev.Skip();
}

void CtrlJobDetails::OnMouseLeftDoubleClick(wxMouseEvent& ev) {
    DlgJobCard dlg(this->GetParent(), wxSize(350, 400), m_job);
    dlg.ShowModal();
    ev.Skip();
}