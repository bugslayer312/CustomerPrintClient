#pragma once

#include <string>
#include <ctime>

namespace Strings {

std::string ToUTF8(std::wstring const& wideString);
std::wstring FromUTF8(std::string const& utf8String);
bool UTCDateFromUTCDateString(std::string const& str, std::time_t& outTime);
std::string UTCDateTimeToLocalTimeString(std::time_t time);
std::wstring ExtractFileName(std::wstring const& path);
void ReplaceFileExtension(std::wstring& fileName, std::wstring const& newExt);

} // namespace Strings