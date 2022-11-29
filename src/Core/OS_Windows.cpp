#include "OS.h"

#include <shlobj_core.h>
#include <versionhelpers.h>

namespace OS {

/*std::string const& GetConfigDir() {
    static std::string const configDir("C:\\ProgramData");
    return configDir;
} */

std::wstring GetProgramDataDir() {
    std::wstring result;
    PWSTR path = NULL;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &path);
    if (SUCCEEDED(hr)) {
        result = path;
    }
    CoTaskMemFree(path);
    return result;
}

std::wstring GetUserDocumentDir() {
    std::wstring result;
    PWSTR path = NULL;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);
    if (SUCCEEDED(hr)) {
        result = path;
    }
    CoTaskMemFree(path);
    return result;
}

std::wstring GetDownloadsDir() {
    std::wstring result;
    PWSTR path = NULL;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &path);
    if (SUCCEEDED(hr)) {
        result = path;
    }
    CoTaskMemFree(path);
    return result;
}

Version GetVersion() {
    NTSTATUS(WINAPI * RtlGetVersion)(LPOSVERSIONINFOEXW);
	OSVERSIONINFOEXW osInfo;
	*(FARPROC*)& RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");
    if (RtlGetVersion)
	{
		osInfo.dwOSVersionInfoSize = sizeof(osInfo);
		RtlGetVersion(&osInfo);
		return {static_cast<int>(osInfo.dwMajorVersion), static_cast<int>(osInfo.dwMinorVersion)};
	}

    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    return {static_cast<int>(osvi.dwMajorVersion), static_cast<int>(osvi.dwMinorVersion)};
}

} // namespace OS