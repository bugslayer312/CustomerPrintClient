#include "../../CustomerPrintClientCore/PrintJobInfo.h"

#include <wx/colour.h>

extern wxColour const colorGray;

wxColour const& GetColorByStatus(PrintJobStatus status);