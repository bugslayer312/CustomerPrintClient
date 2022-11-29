#include "CtrlRating.h"

#include <wx/dcclient.h>

int const SymbolGap = 2;
int const MaxRating = 100;

CtrlRating::CtrlRating(wxWindow *parent, wxWindowID id, int rating, int symbolCount, const wxPoint& pos,
                        const wxSize& size, long style, const wxString& name)
    : wxWindow(parent, id, pos, size, style, name)
    , m_bitmap(L"RATING_STAR", wxBITMAP_TYPE_PNG_RESOURCE)
    , m_bitmapPale(L"RATING_STAR_PALE", wxBITMAP_TYPE_PNG_RESOURCE)
    , m_symbolCount(symbolCount)
    , m_rating(rating)
    , m_paddingDirection(0)
    , m_paddingValue(0)
{
}
    
int CtrlRating::GetRating() const {
    return m_rating;
}

void CtrlRating::SetRating(int value) {
    if (value < 0) value = 0;
    if (value > MaxRating) value = MaxRating;
    if (m_rating != value) {
        m_rating = value;
        Refresh();
    }
}

void CtrlRating::SetPadding(int direction, int value) {
    m_paddingDirection = direction;
    m_paddingValue = value;
}

wxSize CtrlRating::DoGetBestSize() const {
    wxSize result(0, 0);
    if (m_bitmap.IsOk()) {
        result.x = m_bitmap.GetWidth()*m_symbolCount + SymbolGap*(m_symbolCount-1);
        result.y = m_bitmap.GetHeight();
    }
    if (m_paddingValue > 0) {
        if (m_paddingDirection & wxLEFT) {
            result.x += m_paddingValue;
        }
        if (m_paddingDirection & wxRIGHT) {
            result.x += m_paddingValue;
        }
        if (m_paddingDirection & wxTOP) {
            result.y += m_paddingValue;
        }
        if (m_paddingDirection & wxBOTTOM) {
            result.y += m_paddingValue;
        }
    }
    return result;
}

void CtrlRating::OnPaint(wxPaintEvent& evt) {
    wxPaintDC dc(this);
    int symbolWidth = m_bitmap.GetWidth() + SymbolGap;
    wxPoint pt(0, 0);
    if (m_paddingValue > 0) {
        if (m_paddingDirection & wxLEFT) {
            pt.x += m_paddingValue;
        }
        if (m_paddingDirection & wxTOP) {
            pt.y += m_paddingValue;
        }
    }
    for (int i(0); i < CalcRatingStars(); ++i) {
        dc.DrawBitmap(m_bitmap, pt);
        pt.x += symbolWidth;
    }
    for (int i(CalcRatingStars()); i < m_symbolCount; ++i) {
        dc.DrawBitmap(m_bitmapPale, pt);
        pt.x += symbolWidth;
    }
}

int CtrlRating::CalcRatingStars() const {
    float oneStar = static_cast<float>(MaxRating) / m_symbolCount;
    int starCount = static_cast<int>((m_rating-1) / oneStar + 0.5f);
    if (starCount > m_symbolCount) {
        starCount = m_symbolCount;
    }
    return starCount;
}

BEGIN_EVENT_TABLE(CtrlRating, wxWindow)
    EVT_PAINT(CtrlRating::OnPaint)
END_EVENT_TABLE()