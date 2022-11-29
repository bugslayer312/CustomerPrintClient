#pragma once

#include <string>
#include <ctime>

namespace jsonxx {

class Object;

} // namespace jsonxx

enum class PrintJobStatus {
    Error,
    Created,
    Canceled,
    Uploaded,
    Rejected,
    Printing,
    Done
};

std::string PrintJobStatusToString(PrintJobStatus status);
PrintJobStatus PrintJobStatusFromString(std::string const& status);

struct PrintJobInfo {
    std::string Id;
    std::string AccessCode;
    std::string Name;
    std::time_t UpdateTime;
    std::size_t Copies;
    std::size_t PagesCount;
    std::string OfficeId;
    std::string ProfileId;
    std::string OfficeName;
    std::string CurrencySymbol;
    float Price;
    bool PayedOff;
    PrintJobStatus Status;
    bool IsLoaded;

    PrintJobInfo();
    PrintJobInfo(jsonxx::Object const& json);
    PrintJobInfo(PrintJobInfo const&) = default;
    PrintJobInfo(PrintJobInfo&&) = default;
    PrintJobInfo& operator=(PrintJobInfo const&) = default;
    PrintJobInfo& operator=(PrintJobInfo&&) = default;
    operator bool() const;
};

bool JobsDiffers(PrintJobInfo const& job1, PrintJobInfo const& job2);
void UpdateJobFromAnother(PrintJobInfo& job1, PrintJobInfo const& job2);