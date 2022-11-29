#pragma once

#include <wx/window.h>
#include <wx/bitmap.h>

class CtrlRating : public wxWindow {
public:
    CtrlRating(wxWindow *parent, wxWindowID id,
            int rating = 0, int symbolCount = 5,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize, long style = 0,
            const wxString& name = wxPanelNameStr);
    
    int GetRating() const;
    void SetRating(int value);
    void SetPadding(int direction, int value);

    wxDECLARE_EVENT_TABLE();

protected:
    virtual wxSize DoGetBestSize() const override;
    void OnPaint(wxPaintEvent& evt);
    int CalcRatingStars() const;

private:
    wxBitmap m_bitmap;
    wxBitmap m_bitmapPale;
    int const m_symbolCount;
    int m_rating;
    int m_paddingDirection;
    int m_paddingValue;
};