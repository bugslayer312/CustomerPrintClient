#pragma once

#include <wx/window.h>

class CtrlCloseHeader : public wxWindow {
public:
    CtrlCloseHeader(wxWindow *parent, wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize, long style = 0,
                const wxString& name = wxPanelNameStr);
    
    wxDECLARE_EVENT_TABLE();

protected:
    virtual wxSize DoGetBestSize() const override;
private:
    wxRect GetButtonRect() const;
    void OnPaint(wxPaintEvent& evt);
    void OnEraseBackground(wxEraseEvent& evt);
    void OnLMouse(wxMouseEvent& evt);
    void OnMouseLeave(wxMouseEvent& evt);

    bool m_btnIsPressed;
};