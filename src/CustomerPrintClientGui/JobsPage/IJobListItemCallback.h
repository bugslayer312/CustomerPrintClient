#pragma once

class wxWindow;

class IJobListItemCallback {
public:
    virtual ~IJobListItemCallback() = default;
    virtual void OnJobClicked(wxWindow* wnd) = 0;
};

class IJobListPendingItemCallback {
public:
    virtual ~IJobListPendingItemCallback() = default;
    virtual void OnJobCancelClick(std::size_t pendingJobId, bool doCancel) = 0;
};