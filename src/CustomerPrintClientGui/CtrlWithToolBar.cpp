#include "CtrlWithToolBar.h"

#include <wx/toolbar.h>

CtrlWithToolBar::CtrlWithToolBar(wxWindow *parent, wxWindowID id, wxPoint const& pos, wxSize const& size,
                                long style)
    : wxWindow(parent, id, pos, size, style)
    , m_toolBar(nullptr)
{
    Bind(wxEVT_SIZE, &CtrlWithToolBar::OnSize, this, this->GetId());
}

wxToolBar* CtrlWithToolBar::CreateToolBar(long style, wxWindowID id) {
    wxCHECK_MSG(!m_toolBar, nullptr, wxT("recreating toolbar in CtrlWithToolBar"));
    SetToolBar(new wxToolBar(this, id, wxDefaultPosition, wxDefaultSize, style));
    return m_toolBar;
}

wxToolBar* CtrlWithToolBar::GetToolBar() const {
    return m_toolBar;
}

void CtrlWithToolBar::SetToolBar(wxToolBar* toolBar) {
    if ((toolBar != nullptr) != (m_toolBar != nullptr)) {
        if (toolBar) {
            m_toolBar = toolBar;
            PositionToolBar();
            m_toolBar->Hide();
        }
        Layout();
        if (m_toolBar) {
            m_toolBar->Show();
        }
    }
    m_toolBar = toolBar;
}

void CtrlWithToolBar::OnSize(wxSizeEvent& ev) {
    PositionToolBar();
    ev.Skip();
}

void CtrlWithToolBar::PositionToolBar() {
    if (m_toolBar && m_toolBar->IsShown()) {
        int width, height;
        wxWindow::DoGetClientSize(&width, &height);
        
        int tx, ty, tw, th;
        m_toolBar->GetPosition( &tx, &ty );
        m_toolBar->GetSize( &tw, &th );
        
        int x(0), y(0);
        if (m_toolBar->HasFlag(wxTB_BOTTOM)) {
            y = height - th;
        }
        else if (m_toolBar->HasFlag(wxTB_RIGHT)) {
            x = width - tw;
        }

        if (m_toolBar->HasFlag(wxTB_BOTTOM)) {
            if (ty < 0 && -ty == th)
                ty = height - th;
            if (tx < 0 && -tx == tw)
                tx = 0;
        }
        else if (m_toolBar->HasFlag(wxTB_RIGHT)) {
            if(ty < 0 && -ty == th)
                ty = 0;
            if(tx < 0 && -tx == tw)
                tx = width - tw;
        }
        else { // left or top
            if (ty < 0 && -ty == th)
                ty = 0;
            if (tx < 0 && -tx == tw)
                tx = 0;
        }

        if (m_toolBar->IsVertical()) {
            th = height;
        }
        else {
            tw = width;
        }
        m_toolBar->SetSize(x, y, tw, th, wxSIZE_NO_ADJUSTMENTS);
    }
}

wxPoint CtrlWithToolBar::GetClientAreaOrigin() const {
    wxPoint pt = wxWindow::GetClientAreaOrigin();
    if (m_toolBar && m_toolBar->IsShown()) {
        const wxSize sizeTB = m_toolBar->GetSize();
        if (m_toolBar->HasFlag(wxTB_TOP))
        {
            pt.y += sizeTB.y;
        }
        else if (m_toolBar->HasFlag(wxTB_LEFT))
        {
            pt.x += sizeTB.x;
        }
    }
    return pt;
}

void CtrlWithToolBar::DoGetClientSize(int* width, int* height) const {
    wxWindow::DoGetClientSize(width, height);
    wxPoint const pt = GetClientAreaOrigin();
    if (width) {
        *width -= pt.x;
    }
    if (height) {
        *height -= pt.y;
    }
    if (m_toolBar && m_toolBar->IsShown() && m_toolBar->HasFlag(wxTB_RIGHT|wxTB_BOTTOM)) {
        wxSize const sizeTB = m_toolBar->GetSize();
        if (m_toolBar->HasFlag(wxTB_RIGHT)) {
            if (width) {
                *width -= sizeTB.x;
            }
        }
        else if (m_toolBar->HasFlag(wxTB_TOP|wxTB_BOTTOM)) {
            if (height) {
                *height -= sizeTB.y;
            }
        }
    }
}

void CtrlWithToolBar::DoSetClientSize(int width, int height) {
    wxPoint const pt = GetClientAreaOrigin();
    width += pt.x;
    height += pt.y;
    if (m_toolBar && m_toolBar->IsShown() && m_toolBar->HasFlag(wxTB_RIGHT|wxTB_BOTTOM)) {
        wxSize const sizeTB = m_toolBar->GetSize();
        if (m_toolBar->HasFlag(wxTB_RIGHT)) {
            width += sizeTB.x;
        }
        else {
            height += sizeTB.y;
        }
    }
    wxWindow::DoSetClientSize(width, height);
}