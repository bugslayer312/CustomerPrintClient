#pragma once

#include <string>

enum class PendingJobStatus {
    Rendering,
    Uploading,
    Complete,
    Canceled,
    Error
};

class IPendingJobProgressCallback {
public:
    virtual ~IPendingJobProgressCallback() = default;
    virtual void OnJobCreate(std::size_t id, std::string const& name, std::size_t pages, std::size_t copies,
                             std::string const& officeId, std::string const& printProfileId) = 0;
    virtual void OnJobProgress(std::size_t id, PendingJobStatus status, int intParam, std::string const& strParam) = 0;
    virtual void OnJobDestroy(std::size_t id) = 0;
};