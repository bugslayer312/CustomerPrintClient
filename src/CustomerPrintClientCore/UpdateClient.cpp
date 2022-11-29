#include "UpdateClient.h"

#include "../Core/jsonxx.h"
#include "../Core/OS.h"
#include "../Core/Log.h"
#include "../Core/StringUtilities.h"
#include "../Networking/Http/HttpClientMT.h"
#include "../Networking/Http/HttpRequest.h"
#include "../Networking/Http/HttpResponse.h"
#include "../Networking/Http/HttpException.h"
#include "../Networking/StringResources.h"

#include <cstdlib>
#include <fstream>

char const * const VersionsFileName = "versions.json";

char const* Versions = "versions";
namespace Ver {
    char const* MinimalWinVer = "minimal-winver";
    namespace Installer {
        char const* Tag = "installer";
        char const* Version = "version";
        char const* File = "file";
    } // namespace Installer
} // namespace Ver

bool ParseTwoHexDigitStrNum(char const* str, int& outInt) {
    char* end = nullptr;
    outInt = std::strtol(str, &end, 16);
    bool res = (str + 2) == end;
    return res;
}

bool ParseOSVersion(std::string const& str, OS::Version& version) {
    return str.size() == 4 && ParseTwoHexDigitStrNum(str.substr(0, 2).c_str(), version.Major) &&
        ParseTwoHexDigitStrNum(str.substr(2, 2).c_str(), version.Minor);
}

UpdateClient::UpdateClient(std::string const& serverName, std::string const& installerPath)
    : m_installerPath(installerPath)
    , m_httpClient(new HttpClientMT(serverName, 80, Protocol::Http, 1))
{
}

UpdateClient::~UpdateClient() {
    Log("UpdateClient::~UpdateClient()\n");
}

void UpdateClient::AsyncGetUpdateList(GetUpdateListCallbackEx callback) {
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Get, m_installerPath + VersionsFileName, {});

    auto httpCallback = [this, callback](ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(nullptr, ex);
        }
        std::uint32_t const code = response->GetStatusCode();
        std::string versionsJson = response->GetContent();
        jsonxx::Object content;
        bool contentParsed = content.parse(versionsJson);
        if (code == 200) {
            if (contentParsed && content.has<jsonxx::Array>(Versions)) {
                UpdateInfoListPtr result(new UpdateInfoList());
                jsonxx::Array& versions = content.get<jsonxx::Array>(Versions);
                for (std::size_t i(0), cnt(versions.size()); i < cnt && contentParsed; ++i) {
                    jsonxx::Object const& jsonVersion = versions.get<jsonxx::Object>(i);
                    if(contentParsed = jsonVersion.has<jsonxx::Object>(Ver::Installer::Tag)) {
                        UpdateInfoPtr updateInfo(new UpdateInfo());
                        jsonxx::Object const& jsonInstaller = jsonVersion.get<jsonxx::Object>(Ver::Installer::Tag);
                        if (contentParsed = jsonVersion.has<jsonxx::String>(Ver::MinimalWinVer) &&
                            ParseOSVersion(jsonVersion.get<jsonxx::String>(Ver::MinimalWinVer), updateInfo->MinimumOSVersion) &&
                            jsonInstaller.has<jsonxx::String>(Ver::Installer::Version) &&
                            ParseVersion(jsonInstaller.get<jsonxx::String>(Ver::Installer::Version).c_str(), updateInfo->InstallerVersion) &&
                            jsonInstaller.has<jsonxx::String>(Ver::Installer::File))
                        {
                            updateInfo->InstallerFileName = jsonInstaller.get<jsonxx::String>(Ver::Installer::File);
                            result->push_back(updateInfo);
                        }
                    }
                }
                return callback(result, nullptr);
            }
            return callback(nullptr, std::make_exception_ptr(HttpException(500, Http::FailedParseResponseBody)));
        }
        callback(nullptr, std::make_exception_ptr(HttpException(code, contentParsed && content.has<jsonxx::String>("error") ?
            content.get<jsonxx::String>("error") : response->GetStatusMessage())));
    };

    m_httpClient->PostRequest(std::move(request), httpCallback);
}

void UpdateClient::AsyncDownloadInstaller(std::string const& fileName, AsyncDownloadInstallerCallback callback) {
    HttpRequest request = m_httpClient->CreateRequest(HttpMethod::Get, m_installerPath + fileName, {});

    auto httpCallback = [this, fileName, callback](ResponsePtr response, std::exception_ptr ex) {
        if (ex) {
            return callback(nullptr, ex);
        }
        std::uint32_t const code = response->GetStatusCode();
        if (code == 200) {
            std::wstring filePath = OS::GetDownloadsDir();
            filePath += L'\\';
            filePath += Strings::FromUTF8(fileName);
            {
                std::ofstream ost(filePath, std::ios::out|std::ios::ate|std::ios::binary);
                if (!ost) {
                    return callback(nullptr, std::make_exception_ptr(std::runtime_error("Failed to save file")));
                }
                response->SaveContentToStream(ost);
            }
            return callback(std::make_shared<std::wstring>(std::move(filePath)), nullptr);
        }
        callback(nullptr, std::make_exception_ptr(HttpException(code, response->GetStatusMessage())));
    };

    m_httpClient->PostRequest(std::move(request), httpCallback);
}