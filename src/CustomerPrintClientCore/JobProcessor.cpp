#include "JobProcessor.h"

#include "AsyncResultGeneric.h"
#include "CloudPrintClient.h"
#include "../Core/StringUtilities.h"
#include "../Core/Log.h"
#include "../Core/Format.h"
#include "../Printing/PrintSettings.h"
#include "ConvertPrintProfile.h"
#include "../Networking/Http/HttpDefinitions.h"
#include "../Networking/Http/TcpException.h"
#include "PendingJobProgressCallback.h"
#include "TitledException.h"

#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <queue>
#include <condition_variable>
#include <unordered_map>

struct RunPrintingResult {
    PrintJobInfoPtr Job;
    std::size_t PendingJobId;
};
typedef std::shared_ptr<RunPrintingResult> RunPrintingResultPtr;
typedef std::function<void(RunPrintingResultPtr)> RunPrintingCallback;

struct PendingJobInfo {
    PendingJobInfo() : Cancelling(false), CurrentHttpRequestId(0) {}
    bool Cancelling;
    RequestIdType CurrentHttpRequestId;
};

class PendingJobList {
public:
    virtual ~PendingJobList() = default;
    virtual void AddJobToPendingList(std::size_t pendingJobId) = 0;
    virtual void RemoveJobToPendingList(std::size_t pendingJobId) = 0;
};

typedef std::shared_ptr<PendingJobList> PendingJobListPtr;
typedef std::weak_ptr<PendingJobList> PendingJobListWPtr;

class PendingJob {
public:
    PendingJob(PendingJobListPtr pendingJobList, IPendingJobProgressCallback& callback, std::string const& jobName)
        : m_pendingJobList(pendingJobList)
        , m_callback(callback)
        , m_id(s_lastId++)
        , m_name(jobName)
    {
        if (std::shared_ptr<PendingJobList> listShPtr = m_pendingJobList.lock()) {
            listShPtr->AddJobToPendingList(m_id);
        }
    }

    ~PendingJob() {
        if (std::shared_ptr<PendingJobList> listShPtr = m_pendingJobList.lock()) {
            listShPtr->RemoveJobToPendingList(m_id);
        }
        m_callback.OnJobDestroy(m_id);
    }

    std::size_t GetId() const {
        return m_id;
    } 

    std::string const& GetName() const {
        return m_name;
    }

    void OnRenderPage(std::size_t pageNum, std::string const& pageName) {
        m_callback.OnJobProgress(m_id, PendingJobStatus::Rendering, static_cast<int>(pageNum), pageName);
    }

    void OnError(std::string const& msg) {
        m_callback.OnJobProgress(m_id, PendingJobStatus::Error, 0, msg);
    }

    void OnUploadPage(std::size_t pageNum, std::string const& pageName) {
        m_callback.OnJobProgress(m_id, PendingJobStatus::Uploading, static_cast<int>(pageNum), pageName);
    }

    void OnComplete() {
        m_callback.OnJobProgress(m_id, PendingJobStatus::Complete, 0, s_emptyStr);
    }

    void OnCancel() {
        m_callback.OnJobProgress(m_id, PendingJobStatus::Canceled, 0, m_name);
    }

private:
    static std::size_t s_lastId;
    static const std::string s_emptyStr;
    PendingJobListWPtr m_pendingJobList;
    IPendingJobProgressCallback& m_callback;
    std::size_t m_id;
    std::string m_name;
};

std::size_t PendingJob::s_lastId = 0;
std::string const PendingJob::s_emptyStr;

typedef std::shared_ptr<PendingJob> PendingJobPtr;

struct ProgressCallbackWrapper {
    ProgressCallbackWrapper(IPendingJobProgressCallback& callback) : Callback(callback) {}
    IPendingJobProgressCallback& Callback;
};
typedef std::shared_ptr<ProgressCallbackWrapper> ProgressCallbackPtr;

struct FileStream {
    std::string Name;
    std::unique_ptr<std::istream> Content;
    std::size_t ContentSize;

    FileStream()
        : ContentSize(0)
    {
    }
    FileStream(std::string&& name)
        : Name(std::move(name))
        , Content(new std::stringstream(std::ios::in|std::ios::out|std::ios::binary))
        , ContentSize(0)
    {
    };
    FileStream(FileStream const& rhs) = delete;
    FileStream& operator=(FileStream const& rhs) = delete;
    FileStream(FileStream&& rhs) = default;
    FileStream& operator=(FileStream&& rhs) = default;
};

typedef std::list<FileStream> FileStreamList;
typedef std::shared_ptr<FileStreamList> FileStreamListPtr;

typedef std::function<FileStreamListPtr()> FileRenderTask;
typedef std::function<void(FileStreamListPtr, std::exception_ptr)> FileRenderedCallback;

typedef std::list<UploadPageResult> UploadPageResultList;
typedef std::shared_ptr<UploadPageResultList> UploadPageResultListPtr;

typedef std::promise<RenderAndUploadResultPtr> RenderAndUploadPromise;
typedef std::shared_ptr<RenderAndUploadPromise> RenderAndUploadPromisePtr;

struct UploadPagesContext {
    CreateJobResultPtr JobInfo;
    FileStreamListPtr InputStreams;
    UploadPageResultListPtr UploadResults;
    RenderAndUploadPromisePtr Promise;
    PendingJobPtr Job;
};
typedef std::shared_ptr<UploadPagesContext> UploadPagesContextPtr;

// JobProcessorImpl

class JobProcessorImpl : public std::enable_shared_from_this<JobProcessorImpl>
                       , public PendingJobList
{
    struct RenderJob {
        FileRenderTask task;
        FileRenderedCallback resultCallback;
        operator bool() const {
            return task && resultCallback;
        }
    };
public:
    JobProcessorImpl(CloudPrintClient& cloudPrintClient, ImageFormat outFormat)
        : m_cloudPrintClient(cloudPrintClient)
        , m_outFormat(outFormat)
        , m_isClosing(false)
        , m_thread(&JobProcessorImpl::RenderLoop, this)
    {
    }

    ~JobProcessorImpl() {
        m_isClosing = true;
        CancelAllPendingJobs();
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            RenderJob emptyJob;
            m_queue.push(emptyJob);
        }
        m_cv.notify_one();
        m_thread.join();
    }

    IAsyncResultPtr RenderAndUpload(IPendingJobProgressCallback& progressCallback,
                                    IDocumentRendererUPtr documentRenderer,
                                    std::string const& officeId,
                                    std::string const& printProfileId,
                                    PrintSettings const& printSettings,
                                    int copies,
                                    RenderAndUploadCallback callback) {
        
        RenderAndUploadPromisePtr promise(new RenderAndUploadPromise());
        IAsyncResultPtr result(new AsyncResultGeneric<RenderAndUploadResultPtr>(promise->get_future(), callback));
        PendingJobPtr job(new PendingJob(shared_from_this(), progressCallback, documentRenderer->GetJobName()));
        progressCallback.OnJobCreate(job->GetId(), job->GetName(), documentRenderer->GetPageCount(),
            static_cast<std::size_t>(copies), officeId, printProfileId);
        auto renderCallback = [this, promise, job, officeId, printProfileId, copies]
                                    (FileStreamListPtr streams, std::exception_ptr ex) {
            if (ex) {
                char const* errMsg = "Failed to render files";
                job->OnError(errMsg);
                AppendErrorTitle(ex, errMsg);
                promise->set_exception(ex);
                return;
            }
            if (streams->empty()) {
                // canceled
                return HandleJobCancelling(promise, job);
            }
            CreateJobAndUploadPages(promise, job, officeId, printProfileId, streams, copies);
        };

        Render(job, std::move(documentRenderer), printSettings, renderCallback);

        return result;
    }

    void CancelPendingJob(std::size_t pendingJobId) {
        std::unique_lock<std::shared_mutex> lock(m_pendingJobsMutex);
        auto found = m_pendingJobs.find(pendingJobId);
        if (found != m_pendingJobs.end()) {
            found->second.Cancelling = true;
            if (found->second.CurrentHttpRequestId) {
                m_cloudPrintClient.CancelRequest(found->second.CurrentHttpRequestId);
            }
        }
    }

    void CancelAllPendingJobs() {
        std::unique_lock<std::shared_mutex> lock(m_pendingJobsMutex);
        for (auto& pair : m_pendingJobs) {
            pair.second.Cancelling = true;
            if (pair.second.CurrentHttpRequestId) {
                RequestIdType const cancelRequestId = pair.second.CurrentHttpRequestId;
                pair.second.CurrentHttpRequestId = 0;
                m_cloudPrintClient.CancelRequest(cancelRequestId);
            }
        }
    }

    std::size_t GetPendingJobsCount() const {
        std::shared_lock<std::shared_mutex> lock(m_pendingJobsMutex);
        return m_pendingJobs.size();
    }

private:
    static void AppendErrorTitle(std::exception_ptr& ex, char const* title) {
        try {
            std::rethrow_exception(ex);
        }
        catch(std::exception& _ex)
        {
            ex = std::make_exception_ptr(TitledException(title, _ex.what(), ExceptionCategory::Job));
        }
    }

    static void HandleJobCancelling(RenderAndUploadPromisePtr promise, PendingJobPtr job) {
        Log("Cancelling job (jobId:%d name:%s)\n", job->GetId(), job->GetName().c_str());
        job->OnCancel();
        promise->set_value(std::make_shared<RenderAndUploadResult>(nullptr, job->GetId()));
    }

    void ProcessServerException(UploadPagesContextPtr uploadCtx, std::exception_ptr ex, char const* localError,
                                bool needCancelJob) {
        try {
            std::rethrow_exception(ex);
        }
        catch(TcpException& _ex) {
            if (_ex.WasCancel()) {
                if (needCancelJob) {
                    CancelServerJob(uploadCtx, nullptr);
                }
                else {
                    HandleJobCancelling(uploadCtx->Promise, uploadCtx->Job);
                }
                return;
            }
            ex = std::make_exception_ptr(TitledException(localError, _ex.what(), ExceptionCategory::Job));
        }
        catch(std::exception& _ex) {
            ex = std::make_exception_ptr(TitledException(localError, _ex.what(), ExceptionCategory::Job));
        }
        uploadCtx->Job->OnError(localError);
        uploadCtx->Promise->set_exception(ex);
    }

    ContentType GetContentType() {
        switch (m_outFormat)
        {
        case ImageFormat::Bmp:
            return ContentType::ImageBmp;
        case ImageFormat::Png:
            return ContentType::ImagePng;
        case ImageFormat::Jpeg:
            return ContentType::ImageJpeg;
        default:
            return ContentType::ApplicationOctetStream;
        }
    }

    void CancelServerJob(UploadPagesContextPtr uploadCtx, std::exception_ptr ex) {
        auto cancelJobCallback = [this, uploadCtx, ex](std::exception_ptr _ex) {
            Log("%s (jobId:%d juid:%s)\n", _ex ? "Failed to cancel job on server" : "Cancel job on server succeded",
                uploadCtx->Job->GetId(), uploadCtx->JobInfo->JobId.c_str());
            if (ex) {
                uploadCtx->Promise->set_exception(ex);
                return;
            }
            HandleJobCancelling(uploadCtx->Promise, uploadCtx->Job);
        };

        SetPendingJobHttpCancelId(uploadCtx->Job->GetId(), 0);
        m_cloudPrintClient.AsyncCancelJob(uploadCtx->JobInfo->JobId, cancelJobCallback);
    }

    void GetNewJobInfo(UploadPagesContextPtr uploadCtx) {
        auto getJobCallback = [this, uploadCtx](PrintJobInfoPtr jobInfo, std::exception_ptr ex) {
            if (ex) {
                return ProcessServerException(uploadCtx, ex, "Failed to retrieve new job info", true);
            }
            Log("Retrieve new job info succeded (jobId:%d juid:%s)\n", uploadCtx->Job->GetId(),
                uploadCtx->JobInfo->JobId.c_str());
            uploadCtx->Job->OnComplete();
            uploadCtx->Promise->set_value(std::make_shared<RenderAndUploadResult>(jobInfo, uploadCtx->Job->GetId()));
        };

        RequestIdType cancelId = m_cloudPrintClient.AsyncGetJob(uploadCtx->JobInfo->JobId, getJobCallback);
        SetPendingJobHttpCancelId(uploadCtx->Job->GetId(), cancelId);
    }

    void CompleteSuccessfulJob(UploadPagesContextPtr uploadCtx) {
        auto completeJobCallback = [this, uploadCtx](std::exception_ptr ex) {
            if (ex) {
                return ProcessServerException(uploadCtx, ex, "Failed to complete job", true);
            }
            Log("Complete job succeded (jobId:%d juid:%s)\n", uploadCtx->Job->GetId(), uploadCtx->JobInfo->JobId.c_str());
            if (IsJobCanceling(uploadCtx->Job->GetId())) {
                return CancelServerJob(uploadCtx, nullptr);
            }
            GetNewJobInfo(uploadCtx);
        };

        RequestIdType cancelId = m_cloudPrintClient.AsyncCompleteJob(uploadCtx->JobInfo->JobId, completeJobCallback);
        SetPendingJobHttpCancelId(uploadCtx->Job->GetId(), cancelId);
    }

    void UploadNextPage(UploadPagesContextPtr uploadCtx, UploadPageResultPtr prevUploadPageResult, std::exception_ptr ex) {
        std::size_t pageNum = uploadCtx->UploadResults->size() + 1;
        if (ex) {
            return ProcessServerException(uploadCtx, ex, Format("Failed to upload page %d", pageNum).c_str(), true);
        }
        if (prevUploadPageResult) {
            Log("Page has been uploaded (jobId:%d num:%d pguid:%s url:%s)\n", uploadCtx->Job->GetId(), pageNum,
                prevUploadPageResult->PageId.c_str(), prevUploadPageResult->Url.c_str());
            uploadCtx->UploadResults->push_back(std::move(*prevUploadPageResult));
            if (uploadCtx->InputStreams->empty()) {
                Log("All pages are uploaded (jobId:%d, juid:%s)\n", uploadCtx->Job->GetId(), uploadCtx->JobInfo->JobId.c_str());
                if (IsJobCanceling(uploadCtx->Job->GetId())) {
                    return CancelServerJob(uploadCtx, nullptr);
                }
                return CompleteSuccessfulJob(uploadCtx);
            }
        }

        if (IsJobCanceling(uploadCtx->Job->GetId())) {
            return CancelServerJob(uploadCtx, nullptr);
        }

        FileStream fs(std::move(uploadCtx->InputStreams->front()));
        uploadCtx->InputStreams->pop_front();
        pageNum = uploadCtx->UploadResults->size() + 1;
        uploadCtx->Job->OnUploadPage(pageNum, fs.Name);
        RequestIdType cancelId = m_cloudPrintClient.AsyncUploadPage(uploadCtx->JobInfo->JobId, pageNum, fs.Name,
            std::move(fs.Content), fs.ContentSize, GetContentType(),
            std::bind(&JobProcessorImpl::UploadNextPage, this, uploadCtx, std::placeholders::_1, std::placeholders::_2));
        SetPendingJobHttpCancelId(uploadCtx->Job->GetId(), cancelId);
    }

    void CreateJobAndUploadPages(RenderAndUploadPromisePtr promise, PendingJobPtr job,
                                 std::string const& officeId, std::string const& profileId,
                                 FileStreamListPtr streams, int copies) {
        
        if (IsJobCanceling(job->GetId())) {
            return HandleJobCancelling(promise, job);
        }
        
        UploadPagesContextPtr uploadCtx(new UploadPagesContext());
        uploadCtx->InputStreams = streams;
        uploadCtx->Promise = promise;
        uploadCtx->UploadResults.reset(new UploadPageResultList());
        uploadCtx->Job = job;
        
        auto createJobCallback = [this, uploadCtx] (CreateJobResultPtr createJobResult, std::exception_ptr ex)
        {    
            if (ex) {
                return ProcessServerException(uploadCtx, ex, "Error create job", false);
            }
            uploadCtx->JobInfo = createJobResult;
            Log("Job has been created on server (jobId:%d juid:%s accessCode:%s)\n", uploadCtx->Job->GetId(),
                createJobResult->JobId.c_str(), createJobResult->AccessCode.c_str());
            UploadNextPage(uploadCtx, nullptr, nullptr);
        };

        RequestIdType cancelId = m_cloudPrintClient.AsyncCreateJob(officeId, profileId, job->GetName(),
            ImageFormatToString(m_outFormat), uploadCtx->InputStreams->size(), copies, createJobCallback);
        SetPendingJobHttpCancelId(uploadCtx->Job->GetId(), cancelId);
    }

    void Render(PendingJobPtr job, IDocumentRendererUPtr documentRenderer,
                PrintSettings printSettings, FileRenderedCallback callback) {
        
        std::shared_ptr<IDocumentRenderer> docRenderer = std::shared_ptr<IDocumentRenderer>(std::move(documentRenderer));
        FileRenderTask renderTask = [this, job, docRenderer, printSettings]()
        {
            FileStreamListPtr result(new FileStreamList());
            std::size_t const pageCount = docRenderer->GetPageCount();
            std::wstring const outExt = ImageFormatToWString(m_outFormat);
            for (size_t pageNum(0); pageNum < pageCount; ++pageNum) {
                if (IsJobCanceling(job->GetId())) {
                    result->clear();
                    Log("Rendering is canceled (jobId:%d)\n", job->GetId());
                    return result;
                }
                std::wstring fileName = docRenderer->GetPageFileName(pageNum);
                job->OnRenderPage(pageNum + 1, Strings::ToUTF8(fileName));
                Strings::ReplaceFileExtension(fileName, outExt);
                FileStream fileStream(Strings::ToUTF8(fileName));
                docRenderer->RenderForPrinting(pageNum, printSettings, m_outFormat,
                    dynamic_cast<std::iostream&>(*fileStream.Content), fileStream.ContentSize);
                result->push_back(std::move(fileStream));
            }
            Log("Rendering is complete (jobId:%d)\n", job->GetId());
            return result;
        };

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_queue.push({renderTask, callback});
        }
        m_cv.notify_one();
    }

    void RenderLoop() {
        while (!m_isClosing) {
            std::unique_lock<std::mutex> cvlock(m_cvMutex);
            std::queue<RenderJob> jobs;
            m_cv.wait(cvlock, [this, &jobs]{
                std::lock_guard<std::mutex> qlock(m_queueMutex);
                if (m_queue.empty()) {
                    return false;
                }
                else {
                    jobs.swap(m_queue);
                    return true;
                }
            });
            while (!jobs.empty()) {
                RenderJob& job = jobs.front();
                try {
                    if (job) {
                        FileStreamListPtr result = job.task();
                        job.resultCallback(result, nullptr);
                    }
                }
                catch (...) {
                    job.resultCallback(nullptr, std::current_exception());
                }
                jobs.pop();
            }
        }
        Log("Finish render thread\n");
    }

    virtual void AddJobToPendingList(std::size_t pendingJobId) override {
        std::unique_lock<std::shared_mutex> lock(m_pendingJobsMutex);
        m_pendingJobs.insert({pendingJobId, PendingJobInfo()});
    }

    virtual void RemoveJobToPendingList(std::size_t pendingJobId) override {
        std::unique_lock<std::shared_mutex> lock(m_pendingJobsMutex);
        m_pendingJobs.erase(pendingJobId);
    }

    void SetPendingJobHttpCancelId(std::size_t pendingJobId, RequestIdType cancelId) {
        std::unique_lock<std::shared_mutex> lock(m_pendingJobsMutex);
        auto found = m_pendingJobs.find(pendingJobId);
        if(found != m_pendingJobs.end()) {
            found->second.CurrentHttpRequestId = cancelId;
        }
    }

    bool IsJobCanceling(std::size_t pendingJobId) const {
        std::shared_lock<std::shared_mutex> lock(m_pendingJobsMutex);
        auto found = m_pendingJobs.find(pendingJobId);
        if(found == m_pendingJobs.end()) {
            return false;
        }
        return found->second.Cancelling;
    }

private:
    CloudPrintClient& m_cloudPrintClient;
    ImageFormat const m_outFormat;
    bool m_isClosing;
    std::mutex m_queueMutex, m_cvMutex;
    mutable std::shared_mutex m_pendingJobsMutex;
    std::condition_variable m_cv;
    std::queue<RenderJob> m_queue;
    std::unordered_map<std::size_t, PendingJobInfo> m_pendingJobs;
    std::thread m_thread;
};


// JobProcessor

JobProcessor::JobProcessor(CloudPrintClient& cloudPrintClient, ImageFormat outFormat)
    : m_impl(new JobProcessorImpl(cloudPrintClient, outFormat))
{
}

JobProcessor::~JobProcessor()
{
}

IAsyncResultPtr JobProcessor::RenderAndUpload(IPendingJobProgressCallback& progressCallback,
                                              IDocumentRendererUPtr documentRenderer,
                                              std::string const& officeId,
                                              std::string const& printProfileId,
                                              PrintSettings const& printSettings,
                                              int copies,
                                              RenderAndUploadCallback callback) {
    return m_impl->RenderAndUpload(progressCallback, std::move(documentRenderer), officeId, printProfileId,
        printSettings, copies, callback);
}

void JobProcessor::CancelPendingJob(std::size_t pendingJobId) {
    m_impl->CancelPendingJob(pendingJobId);
}

void JobProcessor::CancelAllPendingJobs() {
    m_impl->CancelAllPendingJobs();
}

std::size_t JobProcessor::GetPendingJobsCount() const {
    return m_impl->GetPendingJobsCount();
}