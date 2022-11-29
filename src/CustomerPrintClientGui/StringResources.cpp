#include "StringResources.h"

namespace Gui {

wxString const PrintOffice(L"Print office");
wxString const PrintServices(L"Print services");
wxString const JobDetails(L"Job details");
wxString const PaperFmt(L"%s (%dx%dmm)");
wxString const PriceFmt(L"%.2f%s per page");
wxString const TotalCostFmt(L"Total: %.2f%s");
wxString const CostFmt(L"%.2f%s");
wxString const Paid(L"Paid");
wxString const Unpaid(L"Unpaid");
wxString const PagesCopiesFmt(L"Pages: %d, Copies: %d");
wxString const JobNameFmt(L"Name: %s");
wxString const Loading(L"Loading...");
wxString const PendingJobsFmt(L" (Pending jobs:%d)");
wxString const UpdateAvailableFmt(L"There's newer version available (%d.%d.%d).\n"
    "Do you want to download and install it right now?");
wxString const UpdateNotAvailableFmt(L"The current version is %d.%d.%d.\n"
    "There's no update available");
wxString const StartDownloadUpdateCaption(L"Start downloading update");
wxString const StartDownloadUpdateFmt(L"Version: %d.%d.%d\nFile: %s");
wxString const FinishDownloadUpdateCaption(L"The update is downloaded");
wxString const FailedLaunchUpdateCaption(L"Failed to launch installer");

std::string const StdEmptyString;

wxString const& GetPaidString(bool isPaid) {
    return isPaid ? Paid : Unpaid;
}

} // namespace Gui