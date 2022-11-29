#include "CoreManager.h"
#include "../Core/OS.h"
#include "../Core/Config.h"
#include "../Core/CredentialsCache.h"
#include "../Core/Format.h"
#include "../Core/Log.h"
#include "../Printing/Pdf/PdfLibrary.h"
#include "../Printing/ImageBundleRenderer.h"
#include "../Printing/PdfDocumentRenderer.h"
#include "AsyncResultGeneric.h"
#include "CloudPrintClient.h"
#include "PrintOffice.h"
#include "JobProcessor.h"
#include "ConvertPrintProfile.h"
#include "UpdateClient.h"
#include "TitledException.h"

#include <boost/filesystem.hpp>

#include <chrono>
#include <mutex>

typedef std::shared_ptr<std::promise<void>> PromiseVoidPtr;

namespace fs = boost::filesystem;

char const* const ServerName = "test.cloudprinter.com";
char const* const UpdateServerName = "cloudprinter.com";
char const* const UpdateServerInstallerPath = "/downloads/win/current/";
char const* const ConfigName = "config.json";
char const* const CredentialsCacheName = "credentials.cache";

namespace Cfg {

namespace SelectedOffice {
char const* const Tag = "SelectedOffice";
char const* const Id = "Id";
char const* const Latitude = "Latitude";
char const* const Longitude = "Longitude";
} // namespace SelectedOffice

char const* const SelectedProfileId = "SelectedProfileId";

} // namespace Cfg

std::exception_ptr ProcessException(std::exception_ptr ex, char const* localError, ExceptionCategory category) {
    try {
        std::rethrow_exception(ex);
    }
    catch(std::exception& _ex) {
        ex = std::make_exception_ptr(TitledException(localError, _ex.what(), category));
    }
    return ex;
}

IDocumentRenderer::PrintProfile CreatePrintProfile(PrintOfficePtr printOffice) {
    PrintProfilePtr profile;
    if (printOffice) {
        profile = printOffice->GetSelectedPrintProfile();
    }
    return ConvertPrintProfile(profile);
}

class AppDataFolderManager;
AppDataFolderManager& GetAppDataFolderManager();

class AppDataFolderManager {
private:
    AppDataFolderManager() {
        fs::path path = GetPath();
        boost::system::error_code ec;
        if (!fs::exists(path)) {
            if (!fs::create_directories(GetPath(), ec)) {
                throw std::runtime_error(Format("Failed to create app data folder: %s", ec.message().c_str()));
            }
        }
    }
public:
    fs::path GetPath() {
        fs::path path(OS::GetProgramDataDir());
        path /= CoreManager::GetAppName();
        return path;
    }
    AppDataFolderManager(AppDataFolderManager const&) = delete;
    AppDataFolderManager(AppDataFolderManager&&) = delete;
    AppDataFolderManager& operator=(AppDataFolderManager const&) = delete;
    AppDataFolderManager& operator=(AppDataFolderManager&&) = delete;
    friend AppDataFolderManager& GetAppDataFolderManager();
};

AppDataFolderManager& GetAppDataFolderManager() {
    static AppDataFolderManager appDataFolderManager;
    return appDataFolderManager;
}

CoreManager& GetCoreManager() {
    static CoreManager coreManager;
    return coreManager;
}

CoreManager::CoreManager()
    : m_cloudPrintClient(new CloudPrintClient(ServerName))
    , m_config(new Config(GetConfigPath()))
    , m_credCache(new CredentialsCache(GetCredentialsCachePath()))
    , m_jobProcessor(new JobProcessor(*m_cloudPrintClient, ImageFormat::Png))
{
    if (!m_config->Load()) {
        Log("Failed to load config\n");
    }
}

void CoreManager::InitPdfLibrary(std::function<void(int)>&& unsupportedFeatureCallback,
                                 std::unique_ptr<Pdf::IPasswordProvider> passwordProvider) {
    m_pdfLibrary.reset(new Pdf::Library(std::move(unsupportedFeatureCallback), std::move(passwordProvider)));
}

CoreManager::~CoreManager() {
}

Config& CoreManager::GetConfig() {
    return *m_config;
}

/*void CoreManager::AsyncConnect() {
    m_cloudPrintClient->AsyncConnect([](std::exception_ptr ex) {
        Log("Async connect result: %s\n", (ex ? "fail" : "success"));
    });
} */

IAsyncResultPtr CoreManager::AsyncLogin(std::string const& email, std::string const& pass, VoidCallback callback) {
    PromiseVoidPtr promise(new std::promise<void>());
    IAsyncResultPtr result(new AsyncResultGeneric<void>(promise->get_future(), callback));

    auto getUserInfoCallback = [this, email, pass, promise](UserInfo const& ui, std::exception_ptr ex) {
        if (ex) {
            promise->set_exception(ex);
        }
        else {
            m_credCache->SetCredentials({email, pass});
            m_userInfo = ui;
            promise->set_value();
        }
    };

    auto loginCallback = [this, promise, getUserInfoCallback](std::exception_ptr ex) {
        if (ex) {
            promise->set_exception(ex);
        }
        else {
            m_cloudPrintClient->AsyncGetUserInfo(getUserInfoCallback);
        }
    };
    m_cloudPrintClient->AsyncLogin(email, pass, loginCallback);

    return result;
}

IAsyncResultPtr CoreManager::AsyncSignUp(std::string const& email, std::string const& pass,
                                        std::string const& name, VoidCallback callback) {
    PromiseVoidPtr promise(new std::promise<void>());
    IAsyncResultPtr result(new AsyncResultGeneric<void>(promise->get_future(), callback));

    auto signUpCallback = [this, promise] (std::exception_ptr ex) {
        if (ex) {
            promise->set_exception(ex);
        }
        else {
            promise->set_value();
        }
    };
    m_cloudPrintClient->AsyncSignUp(email, pass, name, signUpCallback);

    return result;
}

IAsyncResultPtr CoreManager::AsyncResetPassword(std::string const& email, VoidCallback callback) {
    PromiseVoidPtr promise(new std::promise<void>());
    IAsyncResultPtr result(new AsyncResultGeneric<void>(promise->get_future(), callback));

    auto resetPasswordCallback = [this, promise] (std::exception_ptr ex) {
        if (ex) {
            promise->set_exception(ex);
        }
        else {
            promise->set_value();
        }
    };
    m_cloudPrintClient->AsyncResetPassword(email, resetPasswordCallback);

    return result;
}

IAsyncResultPtr CoreManager::AsyncLoginAnonymously(VoidCallback callback) {
    PromiseVoidPtr promise(new std::promise<void>());
    IAsyncResultPtr result(new AsyncResultGeneric<void>(promise->get_future(), callback));
    
    auto loginCallback = [this, promise](std::exception_ptr ex) {
        if (ex) {
            promise->set_exception(ex);
        }
        else {
            promise->set_value();
        }
    };
    m_cloudPrintClient->AsyncLoginAnonymously(loginCallback);

    return result;
}

IAsyncResultPtr CoreManager::AsyncLoginWithStoredCreds(BoolCallback callback) {
    std::shared_ptr<std::promise<bool>> promise(new std::promise<bool>());
    IAsyncResultPtr result(new AsyncResultGeneric<bool>(promise->get_future(), callback));

    auto getUserInfoCallback = [this, promise](UserInfo const& uifo, std::exception_ptr ex) {
        if (ex) {
            promise->set_value(false);
        }
        else {
            m_userInfo = uifo;
            promise->set_value(true);
        }
    };

    auto loginCallback = [this, promise, getUserInfoCallback](std::exception_ptr ex) {
        if (ex) {
            promise->set_value(false);
        }
        else {
            m_cloudPrintClient->AsyncGetUserInfo(getUserInfoCallback);
        }
    };

    Credentials creds;
    if (m_credCache->GetCredentials(creds)) {
        m_cloudPrintClient->AsyncLogin(creds.Login, creds.Password, loginCallback);
    }
    else {
        promise->set_value(false);
    }

    return result;
}

IAsyncResultPtr CoreManager::AsyncLogout(VoidCallback callback) {
    PromiseVoidPtr promise(new std::promise<void>());
    IAsyncResultPtr result(new AsyncResultGeneric<void>(promise->get_future(), callback));

    auto loginAnonCallback = [this, promise] (std::exception_ptr ex) {
        if (ex) {
            promise->set_exception(ex);
        }
        else {
            m_userInfo.Status = UserInfoStatus::LoggedAnonymous;
            promise->set_value();
        }
    };

    auto logoutCallback = [this, promise, loginAnonCallback](std::exception_ptr ex) {
        if (ex) {
            promise->set_exception(ex);
        }
        else {
            m_userInfo = UserInfo();
            m_credCache->ClearCredentials();
            m_cloudPrintClient->AsyncLoginAnonymously(loginAnonCallback);
        }
    };
    m_cloudPrintClient->AsyncLogout(logoutCallback);

    return result;
}

IAsyncResultPtr CoreManager::AsyncSearchPrintOffices(float latitude, float longitude, std::uint32_t radiusMeters,
                                                    bool newFound, SearchPrintOfficesPreviewCallback callback) {
    SetOfStringPtr excludeKeys(new SetOfString());
    if (newFound) {
        for (auto const& p: m_printOffices) {
            excludeKeys->insert(p.first);
        }
    }
    std::shared_ptr<std::promise<PrintOfficePreviewListPtr>> promise(new std::promise<PrintOfficePreviewListPtr>());
    IAsyncResultPtr result(new AsyncResultGeneric<PrintOfficePreviewListPtr>(promise->get_future(), callback));

    auto callbackWrapper = [this, promise](PrintOfficeListPtr offices, std::exception_ptr ex) {
        if (ex) {
            promise->set_exception(ex);
            return;
        }
        PrintOfficePreviewListPtr result(new PrintOfficePreviewList());
        {
            std::unique_lock<std::shared_mutex> lock(m_officeMutex);
            for (PrintOfficePtr office: *offices) {
                result->push_back(office);
                m_printOffices[office->Id] = office;
            }
        }
        promise->set_value(result);
    };
    m_cloudPrintClient->AsyncSearchPrintOffices(latitude, longitude, radiusMeters, excludeKeys, callbackWrapper);
    
    return result;
}

IAsyncResultPtr CoreManager::AsyncGetPrintOffice(std::string const& officeId, VoidCallback callback) {
    PromiseVoidPtr promise(new std::promise<void>());
    IAsyncResultPtr result(new AsyncResultGeneric<void>(promise->get_future(), callback));

    auto getPrintOfficeCallback = [this, promise](PrintOfficePtr office, std::exception_ptr ex) {
        if (ex) {
            promise->set_exception(ex);
            return;
        }
        {
            std::unique_lock<std::shared_mutex> lock(m_officeMutex);
            m_printOffices[office->Id] = office;
        }
        promise->set_value();
    };

    m_cloudPrintClient->AsyncGetPrintOffice(officeId, getPrintOfficeCallback);

    return result;
}

IAsyncResultPtr CoreManager::AsyncRunPrinting(IPendingJobProgressCallback& progressCallback, JobContentType contentType,
                                              std::vector<std::wstring>&& files, PrintSettings const& printSettings,
                                              int copies, RunPrintingCallback callback) {

    Log("Printing %d files...\n", files.size());
    if (!m_selectedOffice) {
        throw std::runtime_error("Print office not selected");
    }
    std::string const printProfileId = m_selectedOffice->GetSelectedPrintProfile()->Id;
    IDocumentRendererUPtr documentRenderer;
    switch (contentType)
    {
    case JobContentType::Images:
        documentRenderer.reset(new ImageBundleRenderer(std::move(files), CreatePrintProfile(m_selectedOffice)));
        break;
    case JobContentType::Pdf:
        if (!m_pdfLibrary) {
            throw std::runtime_error("Pdf library not initialized");
        }
        documentRenderer.reset(new PdfDocumentRenderer(*m_pdfLibrary, files.front(), CreatePrintProfile(m_selectedOffice)));
        break;
    default:
        throw std::runtime_error("Unknown renderer type");
    }

    return m_jobProcessor->RenderAndUpload(progressCallback, std::move(documentRenderer), m_selectedOffice->Id, printProfileId,
        printSettings, copies,
        [callback](RenderAndUploadResultPtr result) {
            callback(result->Job, result->PendingJobId);
        });
}

IAsyncResultPtr CoreManager::AsyncGetJobsList(GetJobsListCallback callback) {
    std::shared_ptr<std::promise<PrintJobInfoListPtr>> promise(new std::promise<PrintJobInfoListPtr>());
    IAsyncResultPtr result(new AsyncResultGeneric<PrintJobInfoListPtr>(promise->get_future(), callback));

    auto getJobsListCallback = [promise](PrintJobInfoListPtr result, std::exception_ptr ex) {
        if (ex) {
            promise->set_exception(ex);
        } else {
            promise->set_value(result);
        }
    };
    
    m_cloudPrintClient->AsyncGetJobsList(getJobsListCallback);
    
    return result;
}

IAsyncResultPtr CoreManager::AsyncCheckForUpdates(BinaryVersion const& currentVersion, LookForUpdateCallback callback) {
    std::shared_ptr<std::promise<LookForUpdateResultPtr>> promise(new std::promise<LookForUpdateResultPtr>());
    std::shared_ptr<UpdateClient> updateClient = std::make_shared<UpdateClient>(UpdateServerName, UpdateServerInstallerPath);
    // here we capture updateClient into async result callback for prevent his destroying
    // while his httpClient doing requests in his own ios thread. it will be released in
    // gui thread after polling async result and executing his callback
    IAsyncResultPtr result(new AsyncResultGeneric<LookForUpdateResultPtr>(promise->get_future(),
        [updateClient, callback](LookForUpdateResultPtr result) {
            callback(result);
        }));

    auto getUpdateListCallback = [promise, currentVersion](UpdateInfoListPtr updateInfoList, std::exception_ptr ex) {
        if (ex) {
            return promise->set_exception(ProcessException(ex, "Failed to get update list", ExceptionCategory::Update));
        }
        updateInfoList->sort([](UpdateInfoPtr const& lhs, UpdateInfoPtr const& rhs) {
            return lhs->MinimumOSVersion >= rhs->MinimumOSVersion;
        });
        LookForUpdateResultPtr result;
        OS::Version const osVer = OS::GetVersion();
        for (UpdateInfoList::iterator it = updateInfoList->begin(); it != updateInfoList->end(); ++it) {
            if (osVer >= (*it)->MinimumOSVersion) {
                if (currentVersion < (*it)->InstallerVersion) {
                    result.reset(new LookForUpdateResult{(*it)->InstallerVersion, (*it)->InstallerFileName});
                }
                break;
            }
        }
        promise->set_value(result);
    };
    updateClient->AsyncGetUpdateList(getUpdateListCallback);

    return result;
}

IAsyncResultPtr CoreManager::AsyncDownloadUpdate(std::string const& installerName, DownloadUpdateCallback callback) {
    typedef std::shared_ptr<std::wstring> ResultType;
    std::shared_ptr<std::promise<ResultType>> promise(new std::promise<ResultType>());
    std::shared_ptr<UpdateClient> updateClient = std::make_shared<UpdateClient>(UpdateServerName, UpdateServerInstallerPath);
    IAsyncResultPtr result(new AsyncResultGeneric<ResultType>(promise->get_future(),
        [updateClient, callback](ResultType result) {
            callback(result);
        }));

    auto downloadInstallerCallback = [promise](ResultType result, std::exception_ptr ex) {
        if (ex) {
            promise->set_exception(ProcessException(ex, "Failed to get update list", ExceptionCategory::Update));
        }
        else {
            promise->set_value(result);
        }
    };
    updateClient->AsyncDownloadInstaller(installerName, downloadInstallerCallback);

    return result;
}

UserInfo const& CoreManager::GetUserInfo() const {
    return m_userInfo;
}

PrintOfficePtr CoreManager::GetPrintOffice(std::string const& ouid) const {
    std::shared_lock<std::shared_mutex> lock(m_officeMutex);
    auto found = m_printOffices.find(ouid);
    if (found == m_printOffices.end()) {
        return nullptr;
    }
    return found->second;
}

PrintOfficePtr CoreManager::SelectOffice(std::string const& ouid) {
    m_selectedOffice = GetPrintOffice(ouid);
    std::string puid;
    if (m_selectedOffice) {
        std::string tagOffice(Cfg::SelectedOffice::Tag);
        tagOffice += '/';
        m_config->SetString(tagOffice + Cfg::SelectedOffice::Id, m_selectedOffice->Id);
        m_config->SetNumber(tagOffice + Cfg::SelectedOffice::Latitude, m_selectedOffice->Latitude);
        m_config->SetNumber(tagOffice + Cfg::SelectedOffice::Longitude, m_selectedOffice->Longitude);
        if (PrintProfilePtr profile = m_selectedOffice->GetSelectedPrintProfile()) {
            puid = profile->Id;
        }
    }
    else {
        m_config->SetNull(Cfg::SelectedOffice::Tag);
    }
    if (puid.empty()) {
        m_config->SetNull(Cfg::SelectedProfileId);
    }
    else {
        m_config->SetString(Cfg::SelectedProfileId, puid);
    }
    m_config->Save();
    return m_selectedOffice;
}

PrintOfficePtr CoreManager::GetSelectedOffice() const {
    return m_selectedOffice;
}

void CoreManager::CancelPendingJob(std::size_t pendingJobId) {
    m_jobProcessor->CancelPendingJob(pendingJobId);
}

void CoreManager::CancelAllPendingJobs() {
    m_jobProcessor->CancelAllPendingJobs();
}

std::size_t CoreManager::GetPendingJobsCount() const {
    return m_jobProcessor->GetPendingJobsCount();
}

IPreviewRendererUPtr CoreManager::CreatePreviewRenderer(JobContentType contentType,
                                                        std::vector<std::wstring>&& files) {
    switch (contentType)
    {
    case JobContentType::Images:
        return IPreviewRendererUPtr(new ImageBundleRenderer(std::move(files), CreatePrintProfile(m_selectedOffice)));
    case JobContentType::Pdf:
        if (!m_pdfLibrary) {
            throw std::runtime_error("Pdf library not initialized");
        }
        return IPreviewRendererUPtr(new PdfDocumentRenderer(*m_pdfLibrary,
            files.empty() ? std::wstring() : files.front(), CreatePrintProfile(m_selectedOffice)));
    default:
        return IPreviewRendererUPtr();
    }
}

bool CoreManager::GetConfigSelectedOfficeAndProfile(ConfigSelectedOfficeInfo& officeInfo) const {
    double lat(0), lng(0);
    std::string officeId;
    std::string tagOffice(Cfg::SelectedOffice::Tag);
    tagOffice += '/';
    if (m_config->GetString(tagOffice + Cfg::SelectedOffice::Id, officeId) &&
        m_config->GetNumber(tagOffice + Cfg::SelectedOffice::Latitude, lat) &&
        m_config->GetNumber(tagOffice + Cfg::SelectedOffice::Longitude, lng)) {

        officeInfo.OfficeId = std::move(officeId);
        officeInfo.Latitude = static_cast<float>(lat);
        officeInfo.Longitude = static_cast<float>(lng);
        m_config->GetString(Cfg::SelectedProfileId, officeInfo.ProfileId);
        return true;
    }
    return false;
}

void CoreManager::UpdateConfigSelectedProfile() {
    std::string puid;
    if (m_selectedOffice) {
        if (PrintProfilePtr profile = m_selectedOffice->GetSelectedPrintProfile()) {
            puid = profile->Id;
        }
    }
    if (puid.empty()) {
        m_config->SetNull(Cfg::SelectedProfileId);
    }
    else {
        m_config->SetString(Cfg::SelectedProfileId, puid);
    }
    m_config->Save();
}

std::string const& CoreManager::GetAppName() {
    static std::string const appName("RemotePrint");
    return appName;
}

std::string CoreManager::GetConfigPath() {
    fs::path path(GetAppDataFolderManager().GetPath());
    path /= ConfigName;
    return path.string();
}

std::string CoreManager::GetCredentialsCachePath() {
    fs::path path(GetAppDataFolderManager().GetPath());
    path /= CredentialsCacheName;
    return path.string();
}