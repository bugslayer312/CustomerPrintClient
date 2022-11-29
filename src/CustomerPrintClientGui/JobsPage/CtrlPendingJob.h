#pragma once

#include <wx/window.h>

class IJobListPendingItemCallback;
class wxStaticText;
class wxButton;
class wxGauge;

struct PendingJobCtrlInitInfo {
    wxString Name;
    wxString OfficeName;
    wxString TotalCost;
    wxString CreateDate;
    int PagesCount;
};

class CtrlPendingJob : public wxWindow {
public:
    CtrlPendingJob(wxWindow* parent, IJobListPendingItemCallback& callback, std::size_t pendingJobId,
        PendingJobCtrlInitInfo const& initInfo);
    
    std::size_t GetPendingJobId() const;
    void UpdateStatusRendering(int pageNum, wxString const& pageName);
    void UpdateStatusUploading(int pageNum, wxString const& pageName);
    void UpdateStatusCompete();
    void UpdateStatusError(wxString const& error);
    void UpdateStatusCancelled();

private:
    IJobListPendingItemCallback& m_callback;
    std::size_t const m_pendingJobId;
    int m_pageCount;
    bool m_canCancel;
    wxStaticText* m_lblStatus;
    wxButton* m_btnCancel;
    wxGauge* m_gaugeProgress;
    static wxSize const m_imgSize;
};