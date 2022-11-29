#include "MainWindow.h"

#include <wx/app.h>
#include <wx/sysopt.h>

class RenderPdfApp : public wxApp
{
public:
    virtual bool OnInit() {
        if (!wxApp::OnInit()) {
            return false;
        }

#ifdef __WINDOWS__
        wxSystemOptions::SetOption("msw.remap", 2);
#endif
        MainWindow *wnd = new MainWindow();
        wnd->Center();
        wnd->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(RenderPdfApp);
