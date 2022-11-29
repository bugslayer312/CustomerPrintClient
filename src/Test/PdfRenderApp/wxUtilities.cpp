#include "wxUtilities.h"

#include <wx/window.h>

#include <windows.h>

wxBitmapUPtr CreateWxBitmapFromHBitmap(HBITMAPUPtr hBitmap) {
    wxBitmapUPtr result;
    if (hBitmap) {
        HBITMAP hBmp = static_cast<HBITMAP>(hBitmap.get());
        BITMAP bm;
        if(::GetObject(hBmp, sizeof(bm), &bm)) {
            result.reset(new wxBitmap());
            if (result->InitFromHBITMAP(static_cast<WXHBITMAP>(hBmp), bm.bmWidth, bm.bmHeight, bm.bmBitsPixel)) {
                hBitmap.release();
            }
            else {
                return nullptr;
            }
        }
    }
    return result;
}

HBITMAPUPtr CreateHBitmapFromWxBitmap(wxBitmapUPtr wxBmp) {
    HBITMAPUPtr result;
    if (wxBmp->IsOk()) {
        result.reset(wxBmp->GetGDIImageData()->m_hBitmap);
        wxBmp->GetGDIImageData()->m_hBitmap = nullptr;
    }
    return result;
}

LayoutUpdater::LayoutUpdater(wxWindow* wnd) : m_wnd(wnd) {
    m_wnd->Freeze();
}

LayoutUpdater::~LayoutUpdater() {
    m_wnd->Thaw();
}