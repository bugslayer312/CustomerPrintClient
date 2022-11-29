#pragma once

#include "../Core/OS.h"
#include "../Core/BinaryVersion.h"

#include <memory>
#include <string>
#include <functional>
#include <list>

class HttpClientMT;

struct UpdateInfo {
    OS::Version MinimumOSVersion;
    BinaryVersion InstallerVersion;
    std::string InstallerFileName;
};
typedef std::shared_ptr<UpdateInfo> UpdateInfoPtr;
typedef std::list<UpdateInfoPtr> UpdateInfoList;
typedef std::shared_ptr<UpdateInfoList> UpdateInfoListPtr;
typedef std::function<void(UpdateInfoListPtr, std::exception_ptr)> GetUpdateListCallbackEx;

typedef std::function<void(std::shared_ptr<std::wstring>, std::exception_ptr)> AsyncDownloadInstallerCallback;

class UpdateClient {
public:
    UpdateClient(std::string const& serverName, std::string const& installerPath);
    ~UpdateClient();
    void AsyncGetUpdateList(GetUpdateListCallbackEx callback);
    void AsyncDownloadInstaller(std::string const& fileName, AsyncDownloadInstallerCallback callback);
private:
    std::string m_installerPath;
    std::unique_ptr<HttpClientMT> m_httpClient;
};