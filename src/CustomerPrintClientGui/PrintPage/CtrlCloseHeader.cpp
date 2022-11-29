#include "CtrlCloseHeader.h"

#include <wx/renderer.h>
#include <wx/dcclient.h>

CtrlCloseHeader::CtrlCloseHeader(wxWindow *parent,
                                wxWindowID id,
                                const wxPoint& pos,
                                const wxSize& size,
                                long style,
                                const wxString& name)
    : wxWindow(parent, id, pos, size, style|wxFULL_REPAINT_ON_RESIZE, name)
    , m_btnIsPressed(false)
{
}

BEGIN_EVENT_TABLE(CtrlCloseHeader, wxWindow)
    EVT_PAINT(CtrlCloseHeader::OnPaint)
    EVT_ERASE_BACKGROUND(CtrlCloseHeader::OnEraseBackground)
    EVT_LEFT_DOWN(CtrlCloseHeader::OnLMouse)
    EVT_LEAVE_WINDOW(CtrlCloseHeader::OnMouseLeave)
    EVT_LEFT_UP(CtrlCloseHeader::OnLMouse)
END_EVENT_TABLE()

wxSize CtrlCloseHeader::DoGetBestSize() const {
    return wxSize(18, 18);
}

wxRect CtrlCloseHeader::GetButtonRect() const {
    int right(0), bottom(0);
    GetClientSize(&right, &bottom);
    return wxRect(right - 17, 1, 16, 16);
}

void CtrlCloseHeader::OnPaint(wxPaintEvent& evt) {
    wxPaintDC dc(this);
    
    dc.SetBrush(GetBackgroundColour());
    dc.SetPen(*wxTRANSPARENT_PEN);
    wxRegionIterator upd(GetUpdateRegion()); // get the update rect list
    while (upd)
    {
        dc.DrawRectangle(upd.GetRect());
        upd++;
    }
    wxRendererNative& renderer = wxRendererNative::Get();
    renderer.DrawTitleBarBitmap(this, dc, GetButtonRect(), wxTITLEBAR_BUTTON_CLOSE, m_btnIsPressed ? wxCONTROL_PRESSED : 0);
}

void CtrlCloseHeader::OnEraseBackground(wxEraseEvent& evt) {
    // Don't remove!
}

void CtrlCloseHeader::OnLMouse(wxMouseEvent& evt) {
    bool changed(false);
    if (evt.LeftDown() && GetButtonRect().Contains(evt.GetPosition())) {
        if(!m_btnIsPressed) {
            m_btnIsPressed = true;
            Refresh(false);
        }
    }
    else if (evt.LeftUp()) {
        if (m_btnIsPressed) {
            m_btnIsPressed = false;
            Refresh(false);
            wxCommandEvent event(wxEVT_BUTTON, GetId());
            wxPostEvent(this, event);
        }       
    }
    evt.Skip();
}

void CtrlCloseHeader::OnMouseLeave(wxMouseEvent& evt) {
    if (m_btnIsPressed) {
        m_btnIsPressed = false;
        Refresh();
    }
    evt.Skip();
}