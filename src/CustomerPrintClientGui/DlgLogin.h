#pragma once

#include "AsyncResultQueue.h"
#include "../CustomerPrintClientCore/IAsyncResult.h"

#include <wx/dialog.h>

class wxTextCtrl;
class wxSizer;
class wxHyperlinkCtrl;
class wxHyperlinkEvent;
class wxButton;
class wxCommandEvent;
class ILoginManager;

enum class DlgLoginMode {
    SignIn = 0,
    SignUp,
    ForgotPassword
};

class DlgLogin : public wxDialog
               , public AsyncResultQueue {
public:
    DlgLogin(wxWindow* parent, ILoginManager& loginManager,
            DlgLoginMode mode = DlgLoginMode::SignIn,
            wxString const& email = wxEmptyString);

private:
    void SetMode(DlgLoginMode mode);
    bool ValidateEmail();
    bool ValidatePassword(bool withRepeat);
    void OnHyperlinkClick(wxHyperlinkEvent& event);
    void OnApply(wxCommandEvent&);
    void OnSkip(wxCommandEvent&);
    void OnIdle(wxIdleEvent& ev);

    enum CtrlIds {
        ID_DlgLoginCmdFirst = wxID_HIGHEST+50,
        ID_RefSignIn = ID_DlgLoginCmdFirst,
        ID_RefSignUp,
        ID_RefForgotPassword,
        ID_Apply,
        ID_Skip
    };

private:
    ILoginManager& m_loginManager;
    DlgLoginMode m_mode;
    wxSizer* m_sizerForgotMsg;
    wxTextCtrl* m_editEmail;
    wxTextCtrl* m_editName;
    wxTextCtrl* m_editPassword;
    wxTextCtrl* m_editPasswordRepeat;
    wxSizer* m_sizerEmail;
    wxSizer* m_sizerName;
    wxSizer* m_sizerPassword;
    wxSizer* m_sizerPasswordRepeat;
    wxHyperlinkCtrl* m_refForgotPassword;
    wxSizer* m_refSignUp;
    wxSizer* m_refSignIn;
    wxHyperlinkCtrl* m_refGoBackToSignIn;
    wxButton* m_btnOk;
    wxButton* m_btnSkip;
};