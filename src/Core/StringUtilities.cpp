#include "StringUtilities.h"

#include <locale>
#include <codecvt>
#include <sstream>
#include <iomanip>

namespace Strings {

std::string ToUTF8(std::wstring const& wideString) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wideString);
}

std::wstring FromUTF8(std::string const& utf8String) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(utf8String);
}

char const* ServerDateFmt = "%Y-%m-%dT%H:%M:%S";

/*
const int SecondsPerMinute = 60;
const int SecondsPerHour = 3600;
const int SecondsPerDay = 86400;
const int DaysOfMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


bool IsLeapYear(int year) {
    if (year % 4 != 0) return false;
    if (year % 100 != 0) return true;
    return (year % 400) == 0;
}

std::time_t timegm(std::tm* t)
{
    time_t secs = 0;
    for (short y = 1970; y < t->tm_year; ++y)
        secs += (IsLeapYear(y)? 366: 365) * SecondsPerDay;
    for (short m = 1; m < t->tm_mon; ++m) {
        secs += DaysOfMonth[m - 1] * SecondsPerDay;
        if (m == 2 && IsLeapYear(t->tm_year)) secs += SecondsPerDay;
    }
    secs += (t->tm_mday - 1) * SecondsPerDay;
    secs += t->tm_hour * SecondsPerHour;
    secs += t->tm_min * SecondsPerMinute;
    secs += t->tm_sec;
    return secs;
} */

std::time_t timegm(std::tm* t) {
    std::tm* tmpTm;
    std::time_t sec, localSec, gmtSec;
    std::time(&sec);
    tmpTm = std::localtime(&sec);
    localSec = std::mktime(tmpTm);
    tmpTm = std::gmtime(&sec);
    gmtSec = std::mktime(tmpTm);
    return std::mktime(t) + localSec - gmtSec;
}

bool UTCDateFromUTCDateString(std::string const& str, std::time_t& outTime) {
    std::istringstream ist(str);
    std::tm t{0};
    if(ist >> std::get_time(&t, ServerDateFmt)) {
        outTime = timegm(&t);
        return true;
    }
    return false;
}

std::string UTCDateTimeToLocalTimeString(std::time_t time) {
    std::ostringstream ost;
    std::tm t = *std::localtime(&time);
    ost << std::put_time(&t, "%c");
    return ost.str();
}

std::wstring ExtractFileName(std::wstring const& path) {
    std::size_t const pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

void ReplaceFileExtension(std::wstring& fileName, std::wstring const& newExt) {
    std::size_t const pos = fileName.rfind(L'.');
    if (pos != std::wstring::npos) {
        fileName.replace(pos + 1, fileName.size() - pos - 1, newExt);
    }
    else {
        fileName += L'.';
        fileName += newExt;
    }
}

} // namespace Strings