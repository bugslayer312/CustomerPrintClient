#pragma once

#include <wx/string.h>

#include <string>

namespace Gui {

extern wxString const PrintOffice;
extern wxString const PrintServices;
extern wxString const JobDetails;
extern wxString const PaperFmt;
extern wxString const PriceFmt;
extern wxString const TotalCostFmt;
extern wxString const CostFmt;
extern wxString const PagesCopiesFmt;
extern wxString const JobNameFmt;
extern wxString const Loading;
extern wxString const PendingJobsFmt;
extern wxString const UpdateAvailableFmt;
extern wxString const UpdateNotAvailableFmt;
extern wxString const StartDownloadUpdateCaption;
extern wxString const StartDownloadUpdateFmt;
extern wxString const FinishDownloadUpdateCaption;
extern wxString const FailedLaunchUpdateCaption;

wxString const& GetPaidString(bool isPaid);

extern std::string const StdEmptyString;

} // namespace Gui