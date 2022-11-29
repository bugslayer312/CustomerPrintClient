#include "ConvertPrintProfile.h"

IPreviewRenderer::PrintProfile ConvertPrintProfile(PrintProfilePtr printProfile) {
    if (printProfile) {
        return {
            printProfile->PrinterSettings.DpiX,
            printProfile->PrinterSettings.DpiY,
            static_cast<int>(printProfile->Paper.Width / 100),
            static_cast<int>(printProfile->Paper.Height / 100),
            printProfile->ProfileSettings.IsColor()
        };
    }
    return {300, 300, 210, 297, true};  // default
}