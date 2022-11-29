#pragma once

#include <string>

namespace OS {

//std::string const& GetConfigDir();

std::wstring GetProgramDataDir();

std::wstring GetUserDocumentDir();

std::wstring GetDownloadsDir();

struct Version {
    int Major;
    int Minor;
    bool operator>=(Version const& rhs) const {
        return Major > rhs.Major || Major == rhs.Major && Minor >= rhs.Minor;
    }
};

Version GetVersion();

} // OS