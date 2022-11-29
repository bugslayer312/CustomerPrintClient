#pragma once

#include "../Printing/HBitmapPtr.h"

#include <wx/bitmap.h>

class wxWindow;

typedef std::unique_ptr<wxBitmap> wxBitmapUPtr;

wxBitmapUPtr CreateWxBitmapFromHBitmap(HBITMAPUPtr hBitmap);

HBITMAPUPtr CreateHBitmapFromWxBitmap(wxBitmapUPtr wxBmp);