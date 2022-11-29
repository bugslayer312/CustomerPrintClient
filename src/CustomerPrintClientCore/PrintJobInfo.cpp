#include "PrintJobInfo.h"

#include "../Core/jsonxx.h"
#include "../Core/StringUtilities.h"

#include <cstdio>

std::string PrintJobStatusToString(PrintJobStatus status) {
    switch (status)
    {
    case PrintJobStatus::Created:
        return "created";
    case PrintJobStatus::Canceled:
        return "canceled";
    case PrintJobStatus::Uploaded:
        return "uploaded";
    case PrintJobStatus::Printing:
        return "printing";
    case PrintJobStatus::Rejected:
        return "rejected";
    case PrintJobStatus::Done:
        return "done";
    default:
        return "error";
    }
}

PrintJobStatus PrintJobStatusFromString(std::string const& status) {
    if (status == "created") {
        return PrintJobStatus::Created;
    } else if (status == "canceled") {
        return PrintJobStatus::Canceled;
    } else if (status == "uploaded") {
        return PrintJobStatus::Uploaded;
    } else if (status == "printing") {
        return PrintJobStatus::Printing;
    } else if (status == "rejected") {
        return PrintJobStatus::Rejected;
    } else if (status == "done") {
        return PrintJobStatus::Done;
    } else {
        return PrintJobStatus::Error;
    }
}


// PrintJobInfo

PrintJobInfo::PrintJobInfo()
    : IsLoaded(false)
{
}

PrintJobInfo::PrintJobInfo(jsonxx::Object const& json)
    : PrintJobInfo()
{
    std::string updateTime, status;
    if (ReadString(json, "juid", Id) &&
        ReadString(json, "accessCode", AccessCode) &&
        ReadString(json, "name", Name) &&
        ReadString(json, "timeUpdated", updateTime) && Strings::UTCDateFromUTCDateString(updateTime, UpdateTime) &&
        ReadString(json, "ouid", OfficeId) &&
        ReadString(json, "puid", ProfileId) &&
        ReadString(json, "status", status) &&
        ReadNumber(json, "copies", Copies) &&
        ReadNumber(json, "amount", PagesCount)) {
        
        Status = PrintJobStatusFromString(status);
        PayedOff = json.get<jsonxx::Boolean>("payedOff", false);
        IsLoaded = true;
        ReadString(json, "officeName", OfficeName);
        ReadString(json, "symbol", CurrencySymbol);
        ReadNumber(json, "fixedPrice", Price);
    }
}

PrintJobInfo::operator bool() const {
    return IsLoaded;
}

bool JobsDiffers(PrintJobInfo const& job1, PrintJobInfo const& job2) {
    return job1.UpdateTime != job2.UpdateTime ||
        job1.Status != job2.Status ||
        job1.PayedOff != job2.PayedOff;
}

void UpdateJobFromAnother(PrintJobInfo& job1, PrintJobInfo const& job2) {
    job1.UpdateTime = job2.UpdateTime;
    job1.Status = job2.Status;
    job1.PayedOff = job2.PayedOff;
}