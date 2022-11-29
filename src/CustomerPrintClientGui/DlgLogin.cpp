#include "DlgLogin.h"
#include "../CustomerPrintClientCore/ILoginManager.h"
#include "../Core/Log.h"

#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/hyperlink.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/log.h>
#include <wx/notifmsg.h>

#include <exception>
#include <tuple>

static wxString strSignIn(L"Sign in");
static wxString strSignUp(L"Sign up");
static wxString strResetPassword(L"Reset password");
static wxString strForgotHint(L"Enter your email address, and we'll send you a password reset email");

static const int EditCtrlWidth = 250;

DlgLogin::DlgLogin(wxWindow* parent, ILoginManager& loginManager
                    , DlgLoginMode mode /*DlgLoginMode::SignIn*/
                    , wxString const& email /*wxEmptyString*/)
        : wxDialog(parent, wxID_ANY, strSignIn, wxDefaultPosition,
                   wxDefaultSize, wxCAPTION)
        , m_loginManager(loginManager)
{
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

    m_sizerForgotMsg = CreateTextSizer(strForgotHint, EditCtrlWidth);
    topSizer->Add(m_sizerForgotMsg, wxSizerFlags().DoubleBorder().Expand());

    std::tuple<wxSizer**, wchar_t const*, wxTextCtrl**, long> editBlocks[] = {
        {&m_sizerName, L"Name:", &m_editName, 0},
        {&m_sizerEmail, L"E-mail:", &m_editEmail, 0},
        {&m_sizerPassword, L"Password:", &m_editPassword, wxTE_PASSWORD},
        {&m_sizerPasswordRepeat, L"Repeat Password:", &m_editPasswordRepeat, wxTE_PASSWORD}
    };
    for (auto block: editBlocks) {
        auto sizer = *std::get<0>(block) = new wxBoxSizer(wxVERTICAL);
        sizer->Add(CreateTextSizer(std::get<1>(block)), wxSizerFlags().DoubleBorder(wxTOP|wxLEFT|wxRIGHT));
        sizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
        auto edit = *std::get<2>(block) = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition
            , wxSize(EditCtrlWidth, wxDefaultCoord), std::get<3>(block));
        sizer->Add(edit, wxSizerFlags().TripleBorder(wxLEFT|wxRIGHT).Expand());
        topSizer->Add(sizer, wxSizerFlags().Expand());
    }
    m_editEmail->SetValue(email);

    m_refSignIn = new wxBoxSizer(wxHORIZONTAL);
    m_refSignIn->Add(new wxStaticText(this, wxID_ANY, L"Already have an account? "));
    m_refSignIn->Add(new wxHyperlinkCtrl(this, ID_RefSignIn, strSignIn, wxEmptyString));
    topSizer->Add(m_refSignIn, wxSizerFlags().Border(wxTOP).Center());

    m_refGoBackToSignIn = new wxHyperlinkCtrl(this, ID_RefSignIn, L"Go back to Login", wxEmptyString);
    topSizer->Add(m_refGoBackToSignIn, wxSizerFlags().Border(wxTOP).Center());

    m_refForgotPassword = new wxHyperlinkCtrl(this, ID_RefForgotPassword, L"Forgot Password ?", wxEmptyString);
    topSizer->Add(m_refForgotPassword, wxSizerFlags().Border(wxTOP).Center());

    m_refSignUp = new wxBoxSizer(wxHORIZONTAL);
    m_refSignUp->Add(new wxStaticText(this, wxID_ANY, L"Don't have an account? "));
    m_refSignUp->Add(new wxHyperlinkCtrl(this, ID_RefSignUp, strSignUp, wxEmptyString));
    topSizer->Add(m_refSignUp, wxSizerFlags().Border(wxTOP).Center());

    topSizer->Add(new wxStaticLine(this), wxSizerFlags().Expand().DoubleBorder(wxLEFT|wxRIGHT|wxTOP));
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_btnOk = new wxButton(this, ID_Apply, strSignIn);
    m_btnSkip = new wxButton(this, ID_Skip, L"Skip");
    buttonSizer->Add(m_btnOk, wxSizerFlags().DoubleBorder(wxRIGHT|wxTOP|wxBOTTOM));
    buttonSizer->Add(m_btnSkip, wxSizerFlags().DoubleBorder(wxRIGHT|wxTOP|wxBOTTOM));
    topSizer->Add(buttonSizer, wxSizerFlags().Right().Border(wxRIGHT));
    
    Bind(wxEVT_HYPERLINK, &DlgLogin::OnHyperlinkClick, this, ID_RefSignIn);
    Bind(wxEVT_HYPERLINK, &DlgLogin::OnHyperlinkClick, this, ID_RefSignUp);
    Bind(wxEVT_HYPERLINK, &DlgLogin::OnHyperlinkClick, this, ID_RefForgotPassword);
    Bind(wxEVT_BUTTON, &DlgLogin::OnApply, this, ID_Apply);
    Bind(wxEVT_BUTTON, &DlgLogin::OnSkip, this, ID_Skip);
    Bind(wxEVT_IDLE, &DlgLogin::OnIdle, this);

    SetEscapeId(wxID_NONE);
    SetAutoLayout(true);
    SetSizer(topSizer);
    SetMode(mode);
    Center();
}

void DlgLogin::SetMode(DlgLoginMode mode) {
    m_mode = mode;
    wxSizer* topSizer = GetSizer();
    switch (mode) {
        case DlgLoginMode::SignIn:
        {
            topSizer->Hide(m_sizerForgotMsg, true);
            topSizer->Hide(m_sizerName, true);
            m_editName->Clear();
            topSizer->Show(m_sizerPassword, true);
            topSizer->Hide(m_sizerPasswordRepeat, true);
            m_editPasswordRepeat->Clear();
            topSizer->Hide(m_refSignIn, true);
            topSizer->Hide(m_refGoBackToSignIn);
            topSizer->Show(m_refForgotPassword);
            topSizer->Show(m_refSignUp, true);
            m_btnOk->SetLabel(strSignIn);
            m_btnSkip->Show();
            m_editEmail->SetFocus();
            break;
        }
        case DlgLoginMode::ForgotPassword:
        {
            topSizer->Show(m_sizerForgotMsg, true);
            topSizer->Hide(m_sizerName, true);
            m_editName->Clear();
            topSizer->Hide(m_sizerPassword, true);
            m_editPassword->Clear();
            topSizer->Hide(m_sizerPasswordRepeat, true);
            m_editPasswordRepeat->Clear();
            topSizer->Hide(m_refSignIn, true);
            topSizer->Show(m_refGoBackToSignIn);
            topSizer->Hide(m_refForgotPassword);
            topSizer->Hide(m_refSignUp, true);
            m_btnOk->SetLabel(strResetPassword);
            m_btnSkip->Hide();
            m_editEmail->SetFocus();
            break;
        }
        default: // DlgLoginMode::SignUp
        {
            topSizer->Hide(m_sizerForgotMsg, true);
            topSizer->Show(m_sizerName, true);
            m_editEmail->Clear();
            topSizer->Show(m_sizerPassword, true);
            topSizer->Show(m_sizerPasswordRepeat, true);
            topSizer->Show(m_refSignIn, true);
            topSizer->Hide(m_refGoBackToSignIn);
            topSizer->Hide(m_refForgotPassword);
            topSizer->Hide(m_refSignUp, true);
            m_btnOk->SetLabel(strSignUp);
            m_btnSkip->Hide();
            m_editName->SetFocus();
            break;
        }
    }
    topSizer->SetSizeHints(this);
    topSizer->Fit(this);
}

bool DlgLogin::ValidateEmail() {
    if (m_editEmail->IsEmpty()) {
        wxLogError(L"Invalid e-mail");
        return false;
    }
    return true;
}

bool DlgLogin::ValidatePassword(bool withRepeat) {
    if (m_editPassword->IsEmpty()) {
        wxLogError(L"Password can't be empty");
        return false;
    }
    if (withRepeat && m_editPassword->GetValue() != m_editPasswordRepeat->GetValue()) {
        wxLogError(L"Passwords are different");
        return false;
    }
    return true;
}

void DlgLogin::OnHyperlinkClick(wxHyperlinkEvent& event) {
    SetMode(static_cast<DlgLoginMode>(event.GetId() - ID_DlgLoginCmdFirst));
}

void DlgLogin::OnApply(wxCommandEvent&) {
    try {
        switch (m_mode)
        {
        case DlgLoginMode::SignIn:
            {
                if (ValidateEmail() && ValidatePassword(false)) {
                    IAsyncResultPtr asyncResult = m_loginManager.AsyncLogin(m_editEmail->GetValue().ToUTF8().data(),
                        m_editPassword->GetValue().ToUTF8().data(),
                        [this]() {
                            EndModal(wxID_OK);    
                        });
                    StoreAsyncResult(asyncResult);
                }
            }
            break;
        case DlgLoginMode::ForgotPassword:
            {
                IAsyncResultPtr asyncResult = m_loginManager.AsyncResetPassword(m_editEmail->GetValue().ToUTF8().data(),
                    [this]() {
                        wxNotificationMessage notifResetPwd(L"Password", L"Password has been reset. Check your e-mail", GetParent());
                        notifResetPwd.Show();
                    });
                StoreAsyncResult(asyncResult);
            }
            break;
        default: // DlgLoginMode::SignUp
            {
                if (ValidateEmail() && ValidatePassword(true)) {
                    IAsyncResultPtr asyncResult = m_loginManager.AsyncSignUp(m_editEmail->GetValue().ToUTF8().data(),
                        m_editPassword->GetValue().ToUTF8().data(), m_editName->GetValue().ToUTF8().data(),
                        [this](){
                            wxNotificationMessage notifAccCreated(L"Sign up", L"Your account has been created", GetParent());
                            notifAccCreated.Show();
                        });
                    StoreAsyncResult(asyncResult);
                }
            }
            break;
        }
    }
    catch (std::exception& e) {
        wxNotificationMessage notifError(L"Error", e.what(), GetParent(), wxICON_ERROR);
        notifError.Show();
    }
}

void DlgLogin::OnSkip(wxCommandEvent&) {
    EndModal(wxID_CANCEL);
}

void DlgLogin::OnIdle(wxIdleEvent& ev) {
    RetrieveAsyncResult([this](char const* title, char const* errorMsg, ExceptionCategory /*category*/){
        wxNotificationMessage notifError(title, errorMsg, this, wxICON_ERROR);
        notifError.Show();
    });
}