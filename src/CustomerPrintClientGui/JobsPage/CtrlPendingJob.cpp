#include "CtrlPendingJob.h"

#include "IJobListItemCallback.h"

#include <wx/bitmap.h>
#include <wx/stattext.h>
#include <wx/statbmp.h>
#include <wx/gbsizer.h>
#include <wx/gauge.h>
#include <wx/button.h>

wxSize const CtrlPendingJob::m_imgSize(32, 32);
wxString const PageRenderingStatusFmt(L"Rendering %s [%d / %d]");
wxString const PageUploadingStatusFmt(L"Uploading %s [%d / %d]");
wxString const PrintStatusComplete(L"Complete");
wxString const PrintStatusCancelling(L"Cancelling...");
wxString const PrintStatusCancelled(L"Cancelled");
wxString const PageErrorStatusFmt(L"Error: %s");

wxColor const colorGray(100, 100, 100);

CtrlPendingJob::CtrlPendingJob(wxWindow* parent, IJobListPendingItemCallback& callback, std::size_t pendingJobId,
                               PendingJobCtrlInitInfo const& initInfo)
    : wxWindow(parent, wxID_ANY)
    , m_callback(callback)
    , m_pendingJobId(pendingJobId)
    , m_pageCount(initInfo.PagesCount)
    , m_canCancel(true)
{
    bool const isPdf = wxString::FromUTF8(initInfo.Name).EndsWith(L".pdf");
    wxGridBagSizer* sizer = new wxGridBagSizer(0, 0);
    wxBitmap const bmpContent(isPdf ? L"CONTENT_PDF" : L"CONTENT_IMAGE", wxBITMAP_TYPE_PNG_RESOURCE);
    wxStaticBitmap* ctrlImages = new wxStaticBitmap(this, wxID_ANY, bmpContent, wxDefaultPosition, m_imgSize);
    sizer->Add(ctrlImages, wxGBPosition(0, 0), wxGBSpan(3, 1), wxALIGN_CENTER|wxALL, 5);
    m_lblStatus = new wxStaticText(this, wxID_ANY, wxEmptyString);
    wxStaticText* lblCost = new wxStaticText(this, wxID_ANY, initInfo.TotalCost);
    wxFont font = lblCost->GetFont();
    font.MakeBold();
    lblCost->SetFont(font);
    sizer->Add(m_lblStatus, wxGBPosition(0, 1), wxGBSpan(1, 2), wxALIGN_LEFT|wxALIGN_BOTTOM|wxTOP, 5);
    sizer->Add(lblCost, wxGBPosition(0, 3), wxDefaultSpan, wxALIGN_RIGHT|wxALIGN_BOTTOM|wxTOP|wxRIGHT, 5);
    m_gaugeProgress = new wxGauge(this, wxID_ANY, 100);
    //m_gaugeProgress->SetValue(100);
    m_btnCancel = new wxButton(this, wxID_ANY, L"Cancel", wxDefaultPosition, wxSize(-1, 18));
    m_btnCancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        m_lblStatus->SetLabel(PrintStatusCancelling);
        m_callback.OnJobCancelClick(m_pendingJobId, m_canCancel);
    });
    wxStaticText* lblDate = new wxStaticText(this, wxID_ANY, initInfo.CreateDate);
    lblDate->SetForegroundColour(colorGray);
    sizer->Add(m_gaugeProgress, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    sizer->Add(m_btnCancel, wxGBPosition(1, 2), wxDefaultSpan, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);
    sizer->Add(lblDate,  wxGBPosition(1, 3), wxDefaultSpan, wxALIGN_RIGHT|wxRIGHT, 5);
    wxStaticText* lblOfficeName = new wxStaticText(this, wxID_ANY, initInfo.OfficeName);
    wxStaticText* lblJobName = new wxStaticText(this, wxID_ANY, initInfo.Name);
    wxBoxSizer* thirdLineSizer = new wxBoxSizer(wxHORIZONTAL);
    thirdLineSizer->Add(lblOfficeName, wxSizerFlags(1).Border(wxRIGHT).Expand());
    thirdLineSizer->Add(lblJobName, wxSizerFlags(1).Expand());
    sizer->Add(thirdLineSizer, wxGBPosition(2, 1), wxGBSpan(1, 3), wxEXPAND|wxBOTTOM|wxRIGHT, 5);
    sizer->AddGrowableCol(1);
    SetSizerAndFit(sizer);
}

std::size_t CtrlPendingJob::GetPendingJobId() const {
    return m_pendingJobId;
}

void CtrlPendingJob::UpdateStatusRendering(int pageNum, wxString const& pageName) {
    m_lblStatus->SetLabel(wxString::Format(PageRenderingStatusFmt, pageName, pageNum, m_pageCount));
    m_gaugeProgress->SetValue((2*pageNum-1)*50/(2*m_pageCount));
}

void CtrlPendingJob::UpdateStatusUploading(int pageNum, wxString const& pageName) {
    m_lblStatus->SetLabel(wxString::Format(PageUploadingStatusFmt, pageName, pageNum, m_pageCount));
    m_gaugeProgress->SetValue(50 + (2*pageNum-1)*50/(2*m_pageCount));
}

void CtrlPendingJob::UpdateStatusCompete() {
    m_lblStatus->SetForegroundColour(*wxGREEN);
    m_lblStatus->SetLabel(PrintStatusComplete);
    m_gaugeProgress->SetValue(100);
    m_btnCancel->SetLabel(L"Close");
    m_canCancel = false;
}

void CtrlPendingJob::UpdateStatusError(wxString const& error) {
    m_lblStatus->SetForegroundColour(*wxRED);
    m_lblStatus->SetLabel(wxString::Format(PageErrorStatusFmt, error));
    m_btnCancel->SetLabel(L"Close");
    m_canCancel = false;
}

void CtrlPendingJob::UpdateStatusCancelled() {
    m_lblStatus->SetForegroundColour(*wxRED);
    m_lblStatus->SetLabel(PrintStatusCancelled);
    m_btnCancel->SetLabel(L"Close");
    m_canCancel = false;
}