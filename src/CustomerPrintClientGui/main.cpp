#include "MainWindow.h"

#include <wx/app.h>
#include <wx/sysopt.h>
#include <wx/image.h>
#include <wx/iconbndl.h>
#include <wx/snglinst.h>

WXUINT AlreadyRunningBroadcast(0);

class RemotePrintApp : public wxApp
{
public:
    virtual bool OnInit() override {
        AlreadyRunningBroadcast = RegisterWindowMessage(wxApp::GetAppName().wc_str());
        m_checker.reset(new wxSingleInstanceChecker());

        if (m_checker->IsAnotherRunning()) {
            PostMessage(HWND_BROADCAST, AlreadyRunningBroadcast, 0, 0);
            return false;
        }
        if (!wxApp::OnInit()) {
            return false;
        }

#ifdef __WINDOWS__
        wxSystemOptions::SetOption("msw.remap", 2);
#endif
        wxImage::AddHandler(new wxPNGHandler());
        MainWindow *wnd = new MainWindow();
        wxIconBundle iconBundle(wxICON(APP_ICON_SMALL));
        iconBundle.AddIcon(wxICON(APP_ICON_BIG));
        wnd->SetIcons(iconBundle);
        wnd->Center();
        wnd->Show(true);
        return true;
    }

    virtual int OnExit() override {
        m_checker.reset();
        return 0;
    }
private:
    std::unique_ptr<wxSingleInstanceChecker> m_checker;
};

wxIMPLEMENT_APP(RemotePrintApp);
