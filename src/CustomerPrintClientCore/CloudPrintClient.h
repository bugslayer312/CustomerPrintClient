#pragma once

#include "Types.h"
#include "IAsyncResult.h"
#include "UserInfo.h"
#include "../Printing/ImageFormat.h"
#include "../Networking/Http/HttpDefinitions.h"
#include "../Core/OS.h"

#include <string>
#include <memory>
#include <future>
#include <list>
#include <unordered_set>
#include <iosfwd>

class HttpClient;
class HttpClientMT;
class HttpResponse;

typedef std::function<void(std::exception_ptr)> VoidCallbackEx;

typedef std::shared_ptr<HttpResponse> HttpResponsePtr;

typedef std::function<void(UserInfo const&, std::exception_ptr)> GetUserInfoCallbackEx;

typedef std::function<void(PrintOfficePtr, std::exception_ptr)> GetPrintOfficeCallbackEx;

typedef std::unordered_set<std::string> SetOfString;
typedef std::shared_ptr<SetOfString> SetOfStringPtr;
typedef std::list<PrintOfficePtr> PrintOfficeList;
typedef std::shared_ptr<PrintOfficeList> PrintOfficeListPtr;
typedef std::function<void(PrintOfficeListPtr, std::exception_ptr)> SearchPrintOfficesCallbackEx;

struct CreateJobResult {
    std::string JobId;
    std::string AccessCode;
};
typedef std::shared_ptr<CreateJobResult> CreateJobResultPtr;
typedef std::function<void(CreateJobResultPtr, std::exception_ptr)> CreateJobCallbackEx;

struct UploadPageResult {
    std::string Url;
    std::string PageId;
};
typedef std::shared_ptr<UploadPageResult> UploadPageResultPtr;
typedef std::function<void(UploadPageResultPtr, std::exception_ptr)> UploadPageResultCallbackEx;

typedef std::function<void(PrintJobInfoListPtr, std::exception_ptr)> GetJobsListCallbackEx;
typedef std::function<void(PrintJobInfoPtr, std::exception_ptr)> GetJobCallbackEx;

class CloudPrintClient {
public:
    struct LoginContext;

    CloudPrintClient(std::string const& serverName);
    ~CloudPrintClient();
    //void AsyncConnect(VoidCallbackEx callback);
    //void WaitConnected();
    //bool IsConnected() const;
    void AsyncLogin(std::string const& email, std::string const& pass, VoidCallbackEx callback);
    void AsyncSignUp(std::string const& email, std::string const& pass, std::string const& name,
                     VoidCallbackEx callback);
    void AsyncResetPassword(std::string const& email, VoidCallbackEx callback);
    void AsyncLoginAnonymously(VoidCallbackEx callback);
    void AsyncLogout(VoidCallbackEx callback);
    void AsyncGetUserInfo(GetUserInfoCallbackEx callback);
    void AsyncGetPrintOffice(std::string const& officeId, GetPrintOfficeCallbackEx callback);
    void AsyncSearchPrintOffices(float latitude, float longitude, std::uint32_t radiusMeters,
        SetOfStringPtr excludeKeys, SearchPrintOfficesCallbackEx callback);
    RequestIdType AsyncCreateJob(std::string const& officeId, std::string const& profileId, std::string const& jobName,
        std::string const& fmt, std::size_t pageCount, int copies, CreateJobCallbackEx callback);
    RequestIdType AsyncUploadPage(std::string const& jobId, std::size_t pageNum, std::string const& filename,
        std::unique_ptr<std::istream> fileContent, std::size_t fileSize, ContentType contentType,
        UploadPageResultCallbackEx callback);
    RequestIdType AsyncCompleteJob(std::string const& jobId, VoidCallbackEx callback);
    RequestIdType AsyncCancelJob(std::string const& jobId, VoidCallbackEx callback);
    RequestIdType AsyncGetJob(std::string const& jobId, GetJobCallbackEx callback);
    void AsyncGetJobsList(GetJobsListCallbackEx callback, int offset = 0, int limit = 50);

    void CancelRequest(RequestIdType cancelId);

private:
    std::unique_ptr<HttpClientMT> m_httpClient;
    std::unique_ptr<std::future<HttpResponsePtr>> m_connectFuture;
    std::unique_ptr<LoginContext> m_loginCtx;
};