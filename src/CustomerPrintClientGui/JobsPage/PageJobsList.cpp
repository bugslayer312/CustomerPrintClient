#include "PageJobsList.h"

#include "../../Core/Log.h"
#include "../../CustomerPrintClientCore/PrintJobInfo.h"
#include "../../CustomerPrintClientCore/CoreManager.h"
#include "CtrlJobDetails.h"
#include "CtrlPendingJob.h"

#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/wupdlock.h>

#include <unordered_map>
#include <unordered_set>
#include <algorithm>

wxColour const SelectionColour(138, 191, 244);

struct JobUiItem {
    PrintJobInfoPtr Job;
    CtrlJobDetails* Wnd;

    JobUiItem(PrintJobInfoPtr job, CtrlJobDetails* wnd)
        : Job(job)
        , Wnd(wnd)
    {
    }
};

PageJobsList::PageJobsList(wxWindow *parent, IJobListPendingItemCallback& callback, wxWindowID winid,
                           const wxPoint& pos, const wxSize& size, long style)
    : wxScrolledCanvas(parent, winid, pos, size, style)
    , m_callback(callback)
    , m_pendingCount(0)
    , m_needUpdateContent(true)
    , m_jobs(new PrintJobInfoList())
    , m_selectedChild(nullptr)
{
    SetScrollRate(10, 10);
    SetSizer(new wxBoxSizer(wxVERTICAL));
}

bool PageJobsList::NeedUpdateContent() const {
    return m_needUpdateContent;
}

void PageJobsList::UpdateContent(PrintJobInfoListPtr jobs) {
    m_needUpdateContent = false;
    std::unordered_map<std::string, JobUiItem> curJobs;
    wxSizer* sizer = GetSizer();
    int itemIdx(0);
    wxWindowUpdateLocker layoutUpdater(this);
    for (PrintJobInfoPtr job: *m_jobs) {
        curJobs.emplace(std::make_pair(job->Id, JobUiItem(job,
            dynamic_cast<CtrlJobDetails*>(sizer->GetItem(itemIdx++)->GetWindow()))));
    }
    for (auto it = jobs->rbegin(); it != jobs->rend(); ++it) {
        PrintJobInfoPtr job = *it;
        auto found = curJobs.find(job->Id);
        if (found == curJobs.end()) {
            m_jobs->push_front(job);
            sizer->Insert(m_pendingCount, new CtrlJobDetails(this, *this, job), wxSizerFlags(0).Expand());
        }
        else {
            if (JobsDiffers(*found->second.Job, *job)) {
                UpdateJobFromAnother(*found->second.Job, *job);
                found->second.Wnd->UpdateJob();
            }
            curJobs.erase(found);
        }
    }
    if (!curJobs.empty()) {
        std::unordered_set<std::string> keysToErase;
        for(auto const& item: curJobs) {
            if (m_selectedChild == item.second.Wnd) {
                m_selectedChild = nullptr;
            }
            item.second.Wnd->Destroy();
            keysToErase.insert(item.first);
        }
        m_jobs->remove_if([&keysToErase](PrintJobInfoPtr const& job) {
            auto found = keysToErase.find(job->Id);
            if (found != keysToErase.end()) {
                keysToErase.erase(found);
                return true;
            }
            return false;
        });
    }
    sizer->Layout();
    FitInside();
    //AdjustScrollbars();
}

void PageJobsList::CreatePendingJob(std::size_t id, PendingJobCtrlInitInfo const& initInfo) {
    ++m_pendingCount;
    wxSizer* sizer = GetSizer();
    wxWindowUpdateLocker layoutUpdater(this);
    sizer->Insert(0, new CtrlPendingJob(this, m_callback, id, initInfo), wxSizerFlags(0).Expand());
    sizer->Layout();
    FitInside();
}

void PageJobsList::DeletePendingJob(std::size_t id) {
    if (CtrlPendingJob* pendingJobCtrl = GetPendingJobCtrl(id)) {
        wxWindowUpdateLocker layoutUpdater(this);
        pendingJobCtrl->Destroy();
        --m_pendingCount;
        GetSizer()->Layout();
        FitInside();
    }
}

void PageJobsList::UpdatePendingJobStatusRendering(std::size_t id, int pageNum, wxString const& pageName) {
    if (CtrlPendingJob* pendingJobCtrl = GetPendingJobCtrl(id)) {
        pendingJobCtrl->UpdateStatusRendering(pageNum, pageName);
    }
}

void PageJobsList::UpdatePendingJobStatusUploading(std::size_t id, int pageNum, wxString const& pageName) {
    if (CtrlPendingJob* pendingJobCtrl = GetPendingJobCtrl(id)) {
        pendingJobCtrl->UpdateStatusUploading(pageNum, pageName);
    }
}

void PageJobsList::UpdatePendingJobStatusComplete(std::size_t id) {
    if (CtrlPendingJob* pendingJobCtrl = GetPendingJobCtrl(id)) {
        pendingJobCtrl->UpdateStatusCompete();
    }
}

void PageJobsList::UpdatePendingJobStatusError(std::size_t id, wxString const& error) {
    if (CtrlPendingJob* pendingJobCtrl = GetPendingJobCtrl(id)) {
        pendingJobCtrl->UpdateStatusError(error);
    }
}

void PageJobsList::UpdatePendingJobStatusCancelled(std::size_t id) {
    if (CtrlPendingJob* pendingJobCtrl = GetPendingJobCtrl(id)) {
        pendingJobCtrl->UpdateStatusCancelled();
    }
}

void PageJobsList::ReplacePendingJobWithComplete(std::size_t id, PrintJobInfoPtr jobInfo) {
    if (CtrlPendingJob* pendingJobCtrl = GetPendingJobCtrl(id)) {
        wxSizer* sizer = GetSizer();
        wxWindowUpdateLocker layoutUpdater(this);
        m_jobs->push_front(jobInfo);
        CtrlJobDetails* newJobCtr = new CtrlJobDetails(this, *this, jobInfo);
        if (m_selectedChild == pendingJobCtrl) {
            newJobCtr->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
            m_selectedChild = newJobCtr;
        }
        sizer->Insert(m_pendingCount, newJobCtr, wxSizerFlags(0).Expand());
        pendingJobCtrl->Destroy();
        --m_pendingCount;
        sizer->Layout();
        FitInside();
    }
}

void PageJobsList::OnJobClicked(wxWindow* wnd) {
    if (m_selectedChild) {
        m_selectedChild->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
        m_selectedChild->Refresh();
    }
    wnd->SetBackgroundColour(SelectionColour);
    wnd->Refresh();
    m_selectedChild = wnd;
}

CtrlPendingJob* PageJobsList::GetPendingJobCtrl(std::size_t id) const {
    wxSizer* sizer = GetSizer();
    for (std::size_t i(0); i < m_pendingCount; ++i) {
        if (CtrlPendingJob* pendingJobCtrl = dynamic_cast<CtrlPendingJob*>(sizer->GetItem(i)->GetWindow())) {
            if (pendingJobCtrl->GetPendingJobId() == id) {
                return pendingJobCtrl;
            }
        }
    }
    return nullptr;
}