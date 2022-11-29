#pragma once

#include <wx/window.h>

class wxToolBar;
class wxSizeEvent;

class CtrlWithToolBar : public wxWindow {
public:
    CtrlWithToolBar(wxWindow *parent, wxWindowID id, wxPoint const& pos = wxDefaultPosition,
                     wxSize const& size = wxDefaultSize, long style = 0);
    wxToolBar* CreateToolBar(long style, wxWindowID id = wxID_ANY);
    wxToolBar* GetToolBar() const;
    void SetToolBar(wxToolBar* toolBar);

protected:
    virtual wxPoint GetClientAreaOrigin() const override;
    virtual void DoGetClientSize(int* width, int* height) const override;
    virtual void DoSetClientSize(int width, int height) override;

private:
    void OnSize(wxSizeEvent& event);
    void PositionToolBar();

private:
    wxToolBar* m_toolBar;
};