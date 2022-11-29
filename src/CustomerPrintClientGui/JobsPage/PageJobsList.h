#pragma once

#include "../../CustomerPrintClientCore/Types.h"
#include "IJobListItemCallback.h"

#include "wx/scrolwin.h"

struct PendingJobCtrlInitInfo;
class CtrlPendingJob;
class IJobListPendingItemCallback;

class PageJobsList : public wxScrolledCanvas
                   , public IJobListItemCallback
{
public:
    PageJobsList(wxWindow *parent, IJobListPendingItemCallback& callback, wxWindowID winid = wxID_ANY,
                 const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxVSCROLL);
    
    bool NeedUpdateContent() const;
    void UpdateContent(PrintJobInfoListPtr jobs);
    void CreatePendingJob(std::size_t id, PendingJobCtrlInitInfo const& initInfo);
    void DeletePendingJob(std::size_t id);
    void UpdatePendingJobStatusRendering(std::size_t id, int pageNum, wxString const& pageName);
    void UpdatePendingJobStatusUploading(std::size_t id, int pageNum, wxString const& pageName);
    void UpdatePendingJobStatusComplete(std::size_t id);
    void UpdatePendingJobStatusError(std::size_t id, wxString const& error);
    void UpdatePendingJobStatusCancelled(std::size_t id);
    void ReplacePendingJobWithComplete(std::size_t id, PrintJobInfoPtr jobInfo);

    virtual void OnJobClicked(wxWindow* wnd) override;

private:
    CtrlPendingJob* GetPendingJobCtrl(std::size_t id) const;

private:
    IJobListPendingItemCallback& m_callback;
    std::size_t m_pendingCount;
    bool m_needUpdateContent;
    PrintJobInfoListPtr m_jobs;
    wxWindow* m_selectedChild;
};