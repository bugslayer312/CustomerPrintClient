#pragma once

#include "IAsyncResult.h"
#include "ILoginManager.h"
#include "Types.h"
#include "UserInfo.h"
#include "JobContentType.h"
#include "../Printing/IPreviewRenderer.h"
#include "../Core/BinaryVersion.h"

#include <unordered_map>
#include <shared_mutex>
#include <vector>

class CoreManager;
class CloudPrintClient;
class Config;
class CredentialsCache;
class IUiLogin;
class PrintSettings;
class JobProcessor;
class IPendingJobProgressCallback;
struct PrintOfficePreview;
struct BinaryVersion;

typedef std::shared_ptr<PrintOfficePreview> PrintOfficePreviewPtr;
typedef std::list<PrintOfficePreviewPtr> PrintOfficePreviewList;
typedef std::shared_ptr<PrintOfficePreviewList> PrintOfficePreviewListPtr;

namespace Pdf {

class IPasswordProvider;
class Library;

} // namespace Pdf

typedef std::function<void(PrintOfficePreviewListPtr)> SearchPrintOfficesPreviewCallback;
typedef std::function<void(PrintJobInfoListPtr)> GetJobsListCallback;
typedef std::function<void(PrintJobInfoPtr, std::size_t)> RunPrintingCallback;

struct LookForUpdateResult {
    BinaryVersion NewVersion;
    std::string InstallerName;
};
typedef std::shared_ptr<LookForUpdateResult> LookForUpdateResultPtr;
typedef std::function<void(LookForUpdateResultPtr)> LookForUpdateCallback;

typedef std::function<void(std::shared_ptr<std::wstring>)> DownloadUpdateCallback;

CoreManager& GetCoreManager();

struct ConfigSelectedOfficeInfo {
    std::string OfficeId;
    float Latitude;
    float Longitude;
    std::string ProfileId;
};

class CoreManager : public ILoginManager {
private:
    CoreManager();
    CoreManager(CoreManager const&) = delete;
    CoreManager(CoreManager&&) = delete;
    CoreManager& operator=(CoreManager const&) = delete;
    CoreManager& operator=(CoreManager&&) = delete;
public:
    ~CoreManager();
    void InitPdfLibrary(std::function<void(int)>&& unsupportedFeatureCallback,
        std::unique_ptr<Pdf::IPasswordProvider> passwordProvider);
    Config& GetConfig();
    // void AsyncConnect();

    // ILoginManager implementation
    IAsyncResultPtr AsyncLogin(std::string const& email, std::string const& pass, VoidCallback callback) override;
    IAsyncResultPtr AsyncSignUp(std::string const& email, std::string const& pass, std::string const& name, 
                                VoidCallback callback) override;
    IAsyncResultPtr AsyncResetPassword(std::string const& email, VoidCallback callback) override;

    IAsyncResultPtr AsyncLoginAnonymously(VoidCallback callback);
    IAsyncResultPtr AsyncLoginWithStoredCreds(BoolCallback callback);
    IAsyncResultPtr AsyncLogout(VoidCallback callback);
    IAsyncResultPtr AsyncSearchPrintOffices(float latitude, float longitude, std::uint32_t radiusMeters,
        bool newFound, SearchPrintOfficesPreviewCallback callback);
    IAsyncResultPtr AsyncGetPrintOffice(std::string const& officeId, VoidCallback callback);
    IAsyncResultPtr AsyncRunPrinting(IPendingJobProgressCallback& progressCallback, JobContentType constentType,
        std::vector<std::wstring>&& files, PrintSettings const& printSettings, int copies, RunPrintingCallback callback);
    IAsyncResultPtr AsyncGetJobsList(GetJobsListCallback callback);
    IAsyncResultPtr AsyncCheckForUpdates(BinaryVersion const& currentVersion, LookForUpdateCallback callback);
    IAsyncResultPtr AsyncDownloadUpdate(std::string const& installerName, DownloadUpdateCallback callback);
    
    UserInfo const& GetUserInfo() const;
    PrintOfficePtr GetPrintOffice(std::string const& ouid) const;
    PrintOfficePtr SelectOffice(std::string const& ouid);
    PrintOfficePtr GetSelectedOffice() const;
    void CancelPendingJob(std::size_t pendingJobId);
    void CancelAllPendingJobs();
    std::size_t GetPendingJobsCount() const;
    IPreviewRendererUPtr CreatePreviewRenderer(JobContentType contentType, std::vector<std::wstring>&& files);
    bool GetConfigSelectedOfficeAndProfile(ConfigSelectedOfficeInfo& officeInfo) const;
    void UpdateConfigSelectedProfile();

    friend CoreManager& GetCoreManager();

    static std::string const& GetAppName();
    static std::string GetConfigPath();
    static std::string GetCredentialsCachePath();

private:
    std::unique_ptr<CloudPrintClient> m_cloudPrintClient;
    std::unique_ptr<Config> m_config;
    std::unique_ptr<CredentialsCache> m_credCache;
    std::unique_ptr<JobProcessor> m_jobProcessor;
    std::unique_ptr<Pdf::Library> m_pdfLibrary;
    UserInfo m_userInfo;
    std::unordered_map<std::string, PrintOfficePtr> m_printOffices;
    PrintOfficePtr m_selectedOffice;
    mutable std::shared_mutex m_officeMutex;
};