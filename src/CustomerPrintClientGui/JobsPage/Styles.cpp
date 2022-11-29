#include "Styles.h"

wxColour const colorGray(100, 100, 100);
wxColour const colorRed(179, 11, 0);
wxColour const colorBlack(0, 0, 0);
wxColour const colorGreen(22, 160, 93);
wxColour const colorBlue(0, 0, 128);

wxColour const& GetColorByStatus(PrintJobStatus status) {
    switch (status)
    {
    case PrintJobStatus::Error:
    case PrintJobStatus::Canceled:
    case PrintJobStatus::Rejected:
        return colorRed;
    case PrintJobStatus::Uploaded:
        return colorGreen;
    case PrintJobStatus::Done:
        return colorBlue;
    default:
        return colorBlack;
    }
}