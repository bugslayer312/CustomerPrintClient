#pragma once

#include "../Printing/ImageFormat.h"
#include "../Printing/IDocumentRenderer.h"
#include "IAsyncResult.h"
// #include "PrintProfile.h"
#include "Types.h"
// #include "JobContentType.h"

//#include <vector>

class CloudPrintClient;
class PrintSettings;
class JobProcessorImpl;
class IPendingJobProgressCallback;

struct RenderAndUploadResult {
    PrintJobInfoPtr Job;
    std::size_t PendingJobId;
    RenderAndUploadResult(PrintJobInfoPtr job, std::size_t pendingJobId) : Job(job), PendingJobId(pendingJobId) {}
};
typedef std::shared_ptr<RenderAndUploadResult> RenderAndUploadResultPtr;
typedef std::function<void(RenderAndUploadResultPtr)> RenderAndUploadCallback;

class JobProcessor {
public:
    JobProcessor(CloudPrintClient& cloudPrintClient, ImageFormat outFormat);
    ~JobProcessor();

    IAsyncResultPtr RenderAndUpload(IPendingJobProgressCallback& progressCallback, IDocumentRendererUPtr documentRenderer,
                                    std::string const& officeId, std::string const& printProfileId,
                                    PrintSettings const& printSettings, int copies, RenderAndUploadCallback callback);
    
    void CancelPendingJob(std::size_t pendingJobId);
    void CancelAllPendingJobs();
    std::size_t GetPendingJobsCount() const;

private:
    std::shared_ptr<JobProcessorImpl> m_impl;
};