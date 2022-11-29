#include "MainWindow.h"
#include "DlgLogin.h"
#include "MapPage/DlgOfficeCard.h"
#include "PrintPage/PagePrintContent.h"
#include "JobsPage/PageJobsList.h"
#include "JobsPage/CtrlPendingJob.h"
#include "../CustomerPrintClientCore/CoreManager.h"
#include "../CustomerPrintClientCore/PrintOffice.h"
#include "../CustomerPrintClientCore/PrintJobInfo.h"
#include "../Core/Log.h"
#include "../Core/Format.h"
#include "../Core/StringUtilities.h"
#include "../Core/BinaryVersion.h"
#include "../Printing/Pdf/IPdfPasswordProvider.h"
#include "../Printing/Pdf/PdfError.h"
#include "StringResources.h"
#include "Version.h"

#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/bitmap.h>
#include <wx/button.h>
#include <wx/simplebook.h>
#include <wx/webview.h>
#include <wx/msgdlg.h>
#include <wx/statbmp.h>
#include <wx/image.h>
#include <wx/stattext.h>
#include <wx/log.h>
#include <wx/notifmsg.h>
#include <wx/textdlg.h>
#include <wx/taskbar.h>
#include <wx/aboutdlg.h>
#include <wx/utils.h>

#ifdef __WINDOWS__

#define WX_BITMAP(name) wxBitmap(#name)

#else
#include "Icons/ionicons-map.xpm"
#include "Icons/ionicons-send.xpm"
#include "Icons/ionicons-help-circle.xpm"
#include "Icons/ionicons-apps.xpm"
#include "Icons/ionicons-exit.xpm"

#define WX_BITMAP(name) wxBitmap(name)

#endif

int const ViewPortCheckMs = 1000;
std::chrono::hours const UpdateCheckPeriod(24);

extern WXUINT AlreadyRunningBroadcast;

class JobProgressEvent: public wxEvent {
public:
    JobProgressEvent(wxEventType eventType, int winid, std::size_t jobId, PendingJobStatus status, int intParam,
                     wxString const& strParam)
        : wxEvent(winid, eventType)
        , m_jobId(jobId)
        , m_status(status)
        , m_intParam(intParam)
        , m_strParam(strParam)
    {
    }
    JobProgressEvent(JobProgressEvent const& other)
        : wxEvent(other)
        , m_jobId(other.m_jobId)
        , m_status(other.m_status)
        , m_intParam(other.m_intParam)
        , m_strParam(other.m_strParam)
    {
    }
    std::size_t GetJobId() const {
        return m_jobId;
    }
    PendingJobStatus GetStatus() const {
        return m_status;
    }
    int GetIntParam() const {
        return m_intParam;
    }
    wxString const& GetStringParam() const {
        return m_strParam;
    }

    virtual wxEvent* Clone() const override {
        return new JobProgressEvent(*this);
    }

private:
    std::size_t m_jobId;
    PendingJobStatus m_status;
    int m_intParam;
    wxString m_strParam;
};

class ReceiveUpdatesInfoEvent : public wxEvent {
public:
    ReceiveUpdatesInfoEvent(wxEventType eventType, int winid, LookForUpdateResultPtr updatesInfo)
        : wxEvent(winid, eventType)
        , m_updatesInfo(updatesInfo)
    {
    }

    LookForUpdateResultPtr GetUpdatesInfo() const {
        return m_updatesInfo;
    }

    virtual wxEvent* Clone() const override {
        return new ReceiveUpdatesInfoEvent(*this);
    }

private:
    LookForUpdateResultPtr m_updatesInfo;
};

wxDEFINE_EVENT(usrEVT_INIT_LOAD_OFFICES, wxCommandEvent);
wxDEFINE_EVENT(usrEVT_LOGIN_UI, wxCommandEvent);
wxDEFINE_EVENT(usrEVT_JOB_PROGRESS, JobProgressEvent);
wxDEFINE_EVENT(usrEVT_RECEIVE_UPDATES_INFO, ReceiveUpdatesInfoEvent);
wxDEFINE_EVENT(usrEVT_CHECK_UPDATES_INFO, wxCommandEvent);

namespace CommandId {
    enum Commands {
    PrintWhere = wxID_HIGHEST+1,
    PrintWhat,
    JobList,
    Help,
    LogIn,
    Settings,
    Quit,
    About,
    CheckForUpdates,
    TaskBarIconShow,
    TaskBarIconHide,
    TaskBarIconExit
};    
} // namespace CommandId

namespace CtrlId {
enum Ctrls {
    ID_PAGE_PRINT_CONTENT = wxID_HIGHEST + 50,
    ID_PAGE_JOBS_LIST,
    ID_VIEWPORT_TIMER
};
}

class PwdProvider : public Pdf::IPasswordProvider {
public:
    PwdProvider(wxWindow* parentWnd, std::string const& password)
        : m_parentWnd(parentWnd)
        , m_password(password)
    {
    }

    std::string const& GetPassword() const override {
        return m_password;
    }

    bool RequestPassword() override {
        wxPasswordEntryDialog dlg(m_parentWnd, L"Password to open the document");
        dlg.SetValue(wxString::FromUTF8(m_password));
        if (dlg.ShowModal() == wxID_OK) {
            m_password = dlg.GetValue().ToUTF8();
            return true;
        }
        return false;
    }

private:
    wxWindow* m_parentWnd;
    std::string m_password;
};

class ApplicationTaskBarIcon : public wxTaskBarIcon {
public:
    ApplicationTaskBarIcon(MainWindow* mainWindow)
        : wxTaskBarIcon()
        , m_mainWindow(mainWindow)
    {
    }

    wxDECLARE_EVENT_TABLE();

protected:
    virtual wxMenu* CreatePopupMenu() override {
        wxMenu *menu = new wxMenu();
        menu->Append(CommandId::TaskBarIconShow, L"Show");
        menu->Append(CommandId::TaskBarIconHide, L"Hide");
        menu->AppendSeparator();
        menu->Append(CommandId::TaskBarIconExit, L"Quit");
        return menu;
    }

private:
    void OnMenuItemShow(wxCommandEvent& /*ev*/) {
        m_mainWindow->ShowAndBringToFront();
    }
    void OnMenuItemHide(wxCommandEvent& /*ev*/) {
        m_mainWindow->Show(false);
    }
    void OnMenuItemExit(wxCommandEvent& /*ev*/) {
        m_mainWindow->CheckPendingJobsAndCloseApp();
    }
    void OnMenuUpdateItemEnable(wxUpdateUIEvent& ev) {
        bool const enable = m_mainWindow->IsShown() ? ev.GetId() == CommandId::TaskBarIconHide
            : ev.GetId() == CommandId::TaskBarIconShow;
        ev.Enable(enable);
    }
    void OnLeftButtonDblClick(wxTaskBarIconEvent& /*ev*/){
        m_mainWindow->ShowAndBringToFront();
    }

private:
    MainWindow* m_mainWindow;
};

wxBEGIN_EVENT_TABLE(ApplicationTaskBarIcon, wxTaskBarIcon)
    EVT_MENU(CommandId::TaskBarIconShow, ApplicationTaskBarIcon::OnMenuItemShow)
    EVT_MENU(CommandId::TaskBarIconHide, ApplicationTaskBarIcon::OnMenuItemHide)
    EVT_MENU(CommandId::TaskBarIconExit, ApplicationTaskBarIcon::OnMenuItemExit)
    EVT_UPDATE_UI_RANGE(CommandId::TaskBarIconShow, CommandId::TaskBarIconHide, ApplicationTaskBarIcon::OnMenuUpdateItemEnable)
    EVT_TASKBAR_LEFT_DCLICK(ApplicationTaskBarIcon::OnLeftButtonDblClick)
wxEND_EVENT_TABLE()

MainWindow::MainWindow()
    : wxFrame(nullptr, wxID_ANY, L"Remote print client")
    , m_activated(false)
    , m_closeOnLastJobDestroyed(false)
    , m_loginMode(LoginMode::LogIn)
    , m_viewportChangedTimer(this, CtrlId::ID_VIEWPORT_TIMER)
    , m_lastUpdateCheckTime(std::chrono::system_clock::now() - UpdateCheckPeriod)
    , m_processingCheckUpdate(false)
{
    std::unique_ptr<wxTaskBarIcon> taskBarIcon(new ApplicationTaskBarIcon(this));
    if (taskBarIcon->SetIcon(wxICON(APP_ICON_BIG), GetTitle())) {
        m_taskBarIcon = std::move(taskBarIcon);
    }

    SetInitialSize(FromDIP(wxSize(800, 600)));
    InitMenu();
    CreateStatusBar();
    m_header = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(wxDefaultCoord, 50));
    // m_header->SetAutoLayout(true);
    wxPanel* panBody = new wxPanel(this);
    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);
    sizerMain->Add(m_header, wxSizerFlags(0).Top().Expand());
    sizerMain->Add(panBody, wxSizerFlags(1).Expand());
    this->SetSizer(sizerMain);

    wxPanel* panLeft = new wxPanel(panBody, wxID_ANY, wxDefaultPosition, wxSize(200, wxDefaultCoord));
    m_tabView = new wxSimplebook(panBody);
    wxBoxSizer* sizerBody = new wxBoxSizer(wxHORIZONTAL);
    sizerBody->Add(panLeft, wxSizerFlags(0).Left().Expand());
    sizerBody->Add(m_tabView, wxSizerFlags(1).Expand());
    panBody->SetSizer(sizerBody);

    InitHeader(m_header);
    InitToolBar(panLeft);
    InitMainView();

    Layout();

    auto unsupportedHandle = [](int type) {
        wxString feature(Pdf::UnsupportedFeautureString(type));
        if (!feature.empty()) {
            wxLogError("PDF unsupported feature: %s", feature);
        }
    };
    GetCoreManager().InitPdfLibrary(std::move(unsupportedHandle),
        std::make_unique<PwdProvider>(this, Gui::StdEmptyString));

    Bind(wxEVT_ACTIVATE, [this](wxActivateEvent& ev) {
            if (ev.GetActive() && !m_activated) {
                m_activated = true;
                OnActivate();
            }
        });
    Bind(wxEVT_ICONIZE, &MainWindow::OnIconize, this);
    Bind(wxEVT_TIMER, &MainWindow::OnViewPortTimer, this, CtrlId::ID_VIEWPORT_TIMER);
    Bind(wxEVT_IDLE, &MainWindow::OnIdle, this);
    Bind(usrEVT_INIT_LOAD_OFFICES, &MainWindow::OnInitLoadOffices, this);
    Bind(usrEVT_LOGIN_UI, &MainWindow::OnLoginUi, this);
    Bind(usrEVT_JOB_PROGRESS, &MainWindow::HandleJobProgress, this);
    Bind(usrEVT_RECEIVE_UPDATES_INFO, &MainWindow::HandleReceiveUpdatesInfo, this);
    Bind(usrEVT_CHECK_UPDATES_INFO, [this](wxCommandEvent&) {
        CheckForUpdates(false);
    });
    Bind(wxEVT_CLOSE_WINDOW, &MainWindow::OnClose, this);
}

void MainWindow::InitMenu() {
    wxMenuBar *menuBar = new wxMenuBar();

    wxMenu* menuFile = new wxMenu();
    // menuFile->Append(CommandId::Settings, L"Settings...\tCtrl-O");
    m_loginMenuItem = menuFile->Append(CommandId::LogIn, L"Login...");
    menuFile->AppendSeparator();
    menuFile->Append(CommandId::CheckForUpdates, L"Check for updates");
    menuFile->AppendSeparator();
    menuFile->Append(CommandId::Quit, L"Quit");
    menuBar->Append(menuFile, L"&File");

    wxMenu* menuHelp = new wxMenu();
    menuHelp->Append(CommandId::About, L"About...");
    menuBar->Append(menuHelp, L"&Help");
    SetMenuBar(menuBar);

    Bind(wxEVT_MENU, &MainWindow::OnLogin, this, CommandId::LogIn);
    Bind(wxEVT_MENU, &MainWindow::OnSettings, this, CommandId::Settings);
    Bind(wxEVT_MENU, &MainWindow::OnMenuQuit, this, CommandId::Quit);
    Bind(wxEVT_MENU, &MainWindow::OnMenuCheckForUpdates, this, CommandId::CheckForUpdates);
    Bind(wxEVT_MENU, &MainWindow::OnMenuAbout, this, CommandId::About);
    Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& ev){
        ev.Enable(!m_processingCheckUpdate);
    }, CommandId::CheckForUpdates);
}

void MainWindow::InitHeader(wxPanel* container) {
    wxBoxSizer* sizerHor = new wxBoxSizer(wxHORIZONTAL);
    wxStaticBitmap* bmpCtrlUser = new wxStaticBitmap(container, wxID_ANY, wxBitmap(L"MAIN_USER", wxBITMAP_TYPE_PNG_RESOURCE),
        wxDefaultPosition, wxSize(40, 40));
    wxBoxSizer* sizerVert = new wxBoxSizer(wxVERTICAL);
    m_txtCtrlUser = new wxStaticText(container, wxID_ANY, wxEmptyString);
    wxFont f = m_txtCtrlUser->GetFont();
    f.SetPointSize(f.GetPointSize() + 2);
    m_txtCtrlUser->SetFont(f);
    m_txtCtrlEmail = new wxStaticText(container, wxID_ANY, wxEmptyString);
    sizerVert->Add(m_txtCtrlUser);
    sizerVert->Add(m_txtCtrlEmail);
    
    wxBoxSizer* sizerOffice = new wxBoxSizer(wxVERTICAL);
    m_bmpCtrlOffice = new wxStaticBitmap(container, wxID_ANY, wxBitmap(L"MAIN_PRINT_OFFICE_GRAY", wxBITMAP_TYPE_PNG_RESOURCE),
        wxDefaultPosition, wxSize(40, 40));
    m_txtCtrlOfficeName = new wxStaticText(container, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxALIGN_RIGHT);
    m_txtCtrlOfficeName->SetFont(f);
    m_txtCtrlOfficeAddress = new wxStaticText(container, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxALIGN_RIGHT);
    sizerOffice->Add(m_txtCtrlOfficeName, wxSizerFlags(0).Right());
    sizerOffice->Add(m_txtCtrlOfficeAddress, wxSizerFlags(0).Right());
    
    sizerHor->Add(bmpCtrlUser, wxSizerFlags(0).Left().Border());
    sizerHor->Add(sizerVert, wxSizerFlags(1).Left().Border(wxLEFT).CenterVertical());
    sizerHor->Add(sizerOffice, wxSizerFlags(1).Border(wxRIGHT).CenterVertical());
    sizerHor->Add(m_bmpCtrlOffice, wxSizerFlags(0).Border());

    container->SetSizer(sizerHor);
}

MainWindow::~MainWindow() {
}

void MainWindow::InitToolBar(wxPanel* container) {
    wxSize const btnSize(wxDefaultCoord, 40);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxButton* btnPrintWhere = new wxButton(container, CommandId::PrintWhere,
        L"Where print", wxDefaultPosition, btnSize, wxBU_LEFT);
    wxButton* btnPrintWhat = new wxButton(container, CommandId::PrintWhat,
        L"What print", wxDefaultPosition, btnSize, wxBU_LEFT);
    wxButton* btnJobList = new wxButton(container, CommandId::JobList,
        L"Job list", wxDefaultPosition, btnSize, wxBU_LEFT);
    wxButton* btnHelp = new wxButton(container, CommandId::Help,
        L"Help", wxDefaultPosition, btnSize, wxBU_LEFT);
    btnHelp->Show(false);
    m_btnLogin = new wxButton(container, CommandId::LogIn,
        L"Login", wxDefaultPosition, btnSize, wxBU_LEFT);

    wxFont btnFont = btnPrintWhere->GetFont();
    btnFont.SetPointSize(btnFont.GetPointSize()+2);
    btnPrintWhere->SetFont(btnFont);
    btnPrintWhere->SetBitmap(wxBitmap(L"MAIN_WHERE", wxBITMAP_TYPE_PNG_RESOURCE));
    btnPrintWhat->SetFont(btnFont);
    btnPrintWhat->SetBitmap(wxBitmap(L"MAIN_WHAT", wxBITMAP_TYPE_PNG_RESOURCE));
    btnJobList->SetFont(btnFont);
    btnJobList->SetBitmap(wxBitmap(L"MAIN_JOBLIST", wxBITMAP_TYPE_PNG_RESOURCE));
    btnHelp->SetFont(btnFont);
    btnHelp->SetBitmap(wxBitmap(L"MAIN_HELP", wxBITMAP_TYPE_PNG_RESOURCE));
    m_btnLogin->SetFont(btnFont);
    m_btnLogin->SetBitmap(wxBitmap(L"MAIN_LOGIN", wxBITMAP_TYPE_PNG_RESOURCE));

    sizer->Add(btnPrintWhere, wxSizerFlags(0).Top().Expand());
    sizer->Add(btnPrintWhat, wxSizerFlags(0).Top().Expand());
    sizer->Add(btnJobList, wxSizerFlags(0).Top().Expand());
    sizer->Add(btnHelp, wxSizerFlags(0).Top().Expand());
    sizer->Add(m_btnLogin, wxSizerFlags(0).Top().Expand());
    container->SetSizer(sizer);
}

void MainWindow::InitMainView() {
    wxWebView* webView = wxWebView::New(m_tabView, wxID_ANY, wxEmptyString);
    webView->Stop();
    m_mapViewWnd = webView;
    m_tabView->AddPage(webView, wxEmptyString);
    m_mapView = CreateMapView(webView, *this);

    m_pageTabWhat = new PagePrintContent(m_tabView, *this, CtrlId::ID_PAGE_PRINT_CONTENT, wxDefaultPosition,
        wxDefaultSize, wxBORDER_STATIC);
    m_tabView->AddPage(m_pageTabWhat, wxEmptyString);
    m_pageTabJobsList = new PageJobsList(m_tabView, *this, CtrlId::ID_PAGE_JOBS_LIST, wxDefaultPosition,
        wxDefaultSize, wxBORDER_STATIC);
    m_tabView->AddPage(m_pageTabJobsList, wxEmptyString);

    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            SelectPage(0);
        }, CommandId::PrintWhere);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            if (!GetCoreManager().GetSelectedOffice()) {
                wxMessageDialog dlg(this, L"Please select a print office first", L"Warning", wxICON_WARNING);
                dlg.ShowModal();
                return;
            }
            SelectPage(1);
        }, CommandId::PrintWhat);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            SelectPage(2);
        }, CommandId::JobList);
    Bind(wxEVT_BUTTON, &MainWindow::OnHelp, this, CommandId::Help);
    Bind(wxEVT_BUTTON, &MainWindow::OnLogin, this, CommandId::LogIn);
}

void MainWindow::SetUserInfo(std::string const& userName, std::string const& email) {
    m_txtCtrlUser->SetLabel(wxString::FromUTF8(userName));
    m_txtCtrlEmail->SetLabel(wxString::FromUTF8(email));
}

void MainWindow::SetLoginMode(LoginMode mode) {
    m_loginMode = mode;
    if (m_loginMode == LoginMode::LogIn) {
        m_loginMenuItem->SetItemLabel(L"Login...");
        m_btnLogin->SetLabel("Login");
        m_btnLogin->SetBitmap(wxBitmap(L"MAIN_LOGIN", wxBITMAP_TYPE_PNG_RESOURCE));
        m_btnLogin->SetBitmapCurrent(wxBitmap(L"MAIN_LOGIN", wxBITMAP_TYPE_PNG_RESOURCE));
        m_btnLogin->SetBitmapFocus(wxBitmap(L"MAIN_LOGIN", wxBITMAP_TYPE_PNG_RESOURCE));
        m_btnLogin->SetBitmapPressed(wxBitmap(L"MAIN_LOGIN", wxBITMAP_TYPE_PNG_RESOURCE));
    }
    else {
        m_loginMenuItem->SetItemLabel(L"Logout");
        m_btnLogin->SetLabel("Logout");
        m_btnLogin->SetBitmap(wxBitmap(L"MAIN_LOGOUT", wxBITMAP_TYPE_PNG_RESOURCE));
        m_btnLogin->SetBitmapCurrent(wxBitmap(L"MAIN_LOGOUT", wxBITMAP_TYPE_PNG_RESOURCE));
        m_btnLogin->SetBitmapFocus(wxBitmap(L"MAIN_LOGOUT", wxBITMAP_TYPE_PNG_RESOURCE));
        m_btnLogin->SetBitmapPressed(wxBitmap(L"MAIN_LOGOUT", wxBITMAP_TYPE_PNG_RESOURCE));
    }
}

void MainWindow::SetPrintOfficeInfo(PrintOfficePtr office) { 
    m_bmpCtrlOffice->SetBitmap(wxBitmap(office ? L"MAIN_PRINT_OFFICE" : L"MAIN_PRINT_OFFICE_GRAY", wxBITMAP_TYPE_PNG_RESOURCE));
    if (office) {
        m_txtCtrlOfficeName->SetLabel(wxString::FromUTF8(office->Name));
        m_txtCtrlOfficeAddress->SetLabel(wxString::FromUTF8(office->Address));
    }
    else {
        m_txtCtrlOfficeName->SetLabel(wxEmptyString);
        m_txtCtrlOfficeAddress->SetLabel(wxEmptyString);
    }
    m_header->Layout();
}

void MainWindow::OnMapLoaded() {
    std::lock_guard<std::mutex> lock(m_loadMutex);
    if (m_onMapLoad) {
        std::function<void()> onMapLoad = std::move(m_onMapLoad);
        onMapLoad();
    }
}

void MainWindow::OnOfficeClicked(std::string const& ouid) {
    PrintOfficePtr officePtr = GetCoreManager().GetPrintOffice(ouid);
    if (officePtr) {
        wxSize size = m_mapViewWnd->GetSize();
        size.SetWidth(std::min(400, static_cast<int>(0.9*size.GetWidth())));
        size.SetHeight(std::min(500, static_cast<int>(0.9*size.GetHeight())));
        DlgOfficeCard dlg(m_mapViewWnd, size, *officePtr);
        if (dlg.ShowModal() == wxID_OK) {
            SelectOfficeAndProfile(ouid, Gui::StdEmptyString, false);
        }
    }
}

void MainWindow::OnViewPortChanged(ViewPort const& viewPort) {
    std::lock_guard<std::mutex> lock(m_viewPortMutex);
    m_viewportChangedTimer.Stop();
    CheckViewPortToApply();
    m_viewPortHistory.emplace_front(viewPort);
    m_viewportChangedTimer.StartOnce(ViewPortCheckMs);
}

void MainWindow::OnActivate() {
    IAsyncResultPtr asyncResult = GetCoreManager().AsyncLoginWithStoredCreds(
        std::bind(&MainWindow::OnAsyncQuietLogin, this, std::placeholders::_1));
    StoreAsyncResult(asyncResult);
}

void MainWindow::OnIconize(wxIconizeEvent& ev) {
    if (!ev.IsIconized()) {
        CheckForUpdates(false);
    }
}

void MainWindow::OnViewPortTimer(wxTimerEvent& ev) {
    std::lock_guard<std::mutex> lock(m_viewPortMutex);
    CheckViewPortToApply();
}

void MainWindow::OnLogin(wxCommandEvent&) {
    if (m_loginMode == LoginMode::LogIn) {
        Login(false);
    }
    else {
        Logout();
    }
}

void MainWindow::OnHelp(wxCommandEvent&) {
}

void MainWindow::OnIdle(wxIdleEvent& /* ev */) {
    RetrieveAsyncResult([this](char const* title, char const* errorMsg, ExceptionCategory category){
        if (category == ExceptionCategory::Update) {
            m_processingCheckUpdate = false;
        }
        wxNotificationMessage notifError(title, errorMsg, this, wxICON_ERROR);
        notifError.Show();
    });
}

void MainWindow::OnClose(wxCloseEvent& ev) {
    if (ev.CanVeto()) {
        ev.Veto();
        Show(false);
        return;
    }
    ev.Skip();
}

void MainWindow::OnInitLoadOffices(wxCommandEvent& ev) {
    std::lock_guard<std::mutex> lock(m_loadMutex);
    if (m_mapView->IsLoaded()) {
        LoadOfficesAtInit();
    }
    else {
        m_onMapLoad = std::bind(&MainWindow::LoadOfficesAtInit, this);
    }
}

void MainWindow::OnLoginUi(wxCommandEvent& ev) {
    Login(true);
}

void MainWindow::OnSettings(wxCommandEvent& ev) {
}

void MainWindow::OnMenuQuit(wxCommandEvent& ev) {
    CheckPendingJobsAndCloseApp();
}

void MainWindow::OnMenuAbout(wxCommandEvent& ev) {
    wxAboutDialogInfo info;
    info.SetName(L"Remote print client");
    wxString versionString = wxString::Format(L"%d.%d.%d", APP_MAJOR_VERSION, APP_MINOR_VERSION, APP_RELEASE_NUMBER);
    info.SetVersion(versionString, wxString::Format(L"Version: %s", versionString));
    info.SetCopyright("(c) BugSlayer Inc. All rights reserved.");
    info.SetDescription("Remote print client application");
    wxAboutBox(info, this);
}

void MainWindow::OnMenuCheckForUpdates(wxCommandEvent& /*ev*/) {
    CheckForUpdates(true);
}

void MainWindow::HandleJobProgress(JobProgressEvent& ev) {
    switch (ev.GetStatus())
    {
    case PendingJobStatus::Rendering:
        m_pageTabJobsList->UpdatePendingJobStatusRendering(ev.GetJobId(), ev.GetIntParam(), ev.GetStringParam());
        break;
    case PendingJobStatus::Uploading:
        m_pageTabJobsList->UpdatePendingJobStatusUploading(ev.GetJobId(), ev.GetIntParam(), ev.GetStringParam());
        break;
    case PendingJobStatus::Complete:
        m_pageTabJobsList->UpdatePendingJobStatusComplete(ev.GetJobId());
        break;
    case PendingJobStatus::Canceled:
        m_pageTabJobsList->UpdatePendingJobStatusCancelled(ev.GetJobId());
        break;
    case PendingJobStatus::Error:
        m_pageTabJobsList->UpdatePendingJobStatusError(ev.GetJobId(), ev.GetStringParam());
        break;
    }
}

void MainWindow::HandleReceiveUpdatesInfo(ReceiveUpdatesInfoEvent& ev) {
    if (LookForUpdateResultPtr updateInfo = ev.GetUpdatesInfo()) {
        BinaryVersion const& newVersion = updateInfo->NewVersion;
        wxMessageDialog dlg(this, wxString::Format(Gui::UpdateAvailableFmt, newVersion.Major, newVersion.Minor, newVersion.Release),
                            L"Confirmation", wxCENTER|wxYES_NO|wxICON_INFORMATION);
        if (dlg.ShowModal() == wxID_YES) {
            IAsyncResultPtr asyncResult = GetCoreManager().AsyncDownloadUpdate(updateInfo->InstallerName,
                [this](std::shared_ptr<std::wstring> installerPath) {
                    m_processingCheckUpdate = false;
                    wxString installer(*installerPath);
                    wxNotificationMessage notifDownloadedUpdate(Gui::FinishDownloadUpdateCaption, installer, this);
                    notifDownloadedUpdate.Show();
                    if (wxExecute(installer, wxEXEC_ASYNC)) {
                        CheckPendingJobsAndCloseApp();
                    }
                    else {
                        wxNotificationMessage notifError(Gui::FailedLaunchUpdateCaption, installer, this, wxICON_ERROR);
                        notifError.Show();
                    }
                });
            StoreAsyncResult(asyncResult);
            wxNotificationMessage notifDownloadingUpdate(Gui::StartDownloadUpdateCaption,
                wxString::Format(Gui::StartDownloadUpdateFmt, newVersion.Major, newVersion.Minor, newVersion.Release,
                    wxString::FromUTF8(updateInfo->InstallerName)), this);
            notifDownloadingUpdate.Show();
            
        }
        else {
            m_processingCheckUpdate = false;
        }
    }
    else {
        wxMessageBox(wxString::Format(Gui::UpdateNotAvailableFmt, APP_MAJOR_VERSION, APP_MINOR_VERSION, APP_RELEASE_NUMBER),
            wxMessageBoxCaptionStr, wxOK|wxCENTER, this);
        m_processingCheckUpdate = false;
    }
}

void MainWindow::OnAsyncQuietLogin(bool success) {
    if (success) {
        AfterLogin(true);
    }
    else {
        wxCommandEvent ev(usrEVT_LOGIN_UI);
        wxPostEvent(this, ev);
    }
}

void MainWindow::Login(bool isFirstLogin) {
    DlgLogin dlg(this, GetCoreManager(), DlgLoginMode::SignIn);
    if (dlg.ShowModal() == wxID_CANCEL) {
        IAsyncResultPtr asyncResult = GetCoreManager().AsyncLoginAnonymously([this, isFirstLogin]() {
            wxNotificationMessage notifLoginAnon(L"Login", L"Continuing anonymously", GetParent());
            notifLoginAnon.Show();
            SetUserInfo("Anonymous", Gui::StdEmptyString);
            SetLoginMode(LoginMode::LogIn);
            if (isFirstLogin) {
                wxCommandEvent ev(usrEVT_INIT_LOAD_OFFICES);
                wxPostEvent(this, ev);
            }
        });
        StoreAsyncResult(asyncResult);    
    }
    else {
        AfterLogin(isFirstLogin);
    }
}

void MainWindow::AfterLogin(bool isFirstLogin) {
    UserInfo const& userInfo = GetCoreManager().GetUserInfo();
    if (userInfo.Status == UserInfoStatus::LoggedWithCreds) {
        SetUserInfo(userInfo.Name, userInfo.Email);
        SetLoginMode(LoginMode::LogOut);
        if (isFirstLogin) {
            wxCommandEvent ev(usrEVT_INIT_LOAD_OFFICES);
            wxPostEvent(this, ev);
        }
    }
    else {
        wxLogFatalError("Wrong user info after login");
    }
}

void MainWindow::Logout() {
    IAsyncResultPtr asyncResult = GetCoreManager().AsyncLogout([this]() {
        wxNotificationMessage notifLoginAnon(L"Logout", L"Continuing anonymously", GetParent());
        notifLoginAnon.Show();
        SetUserInfo("Anonymous", Gui::StdEmptyString);
        SetLoginMode(LoginMode::LogIn);
    });
    StoreAsyncResult(asyncResult);
}

void MainWindow::SelectOfficeAndProfile(std::string const& ouid, std::string const& puid, bool centerViewOnOffice) {
    PrintOfficePtr office = GetCoreManager().SelectOffice(ouid);
    if (office && !puid.empty()) {
        office->SelectPrintProfile(puid);
    }
    SetPrintOfficeInfo(office);
    m_mapView->SelectOffice(office ? ouid : Gui::StdEmptyString, centerViewOnOffice);
}

void MainWindow::LoadOffices(ViewPort const& viewPort, bool newFound, std::string const& officeIdToSelect,
                             std::string const& profileIdToSelect) {

    IAsyncResultPtr asyncResult = GetCoreManager().AsyncSearchPrintOffices(viewPort.Latitude, viewPort.Longitude,
        viewPort.ViewRadiusMeters, newFound,
        [this, officeIdToSelect, profileIdToSelect, newFound](PrintOfficePreviewListPtr offices) {
            m_mapView->PlaceOffices(*offices);
            if (!officeIdToSelect.empty()) {
                SelectOfficeAndProfile(officeIdToSelect, profileIdToSelect, true);
            }
            if (!newFound) {
                PostCheckForUpdates();
            }
        }
    );
    StoreAsyncResult(asyncResult);
}

void MainWindow::LoadOfficesAtInit() {
    try {
        ViewPort viewPort = m_mapView->GetViewPort();
        ConfigSelectedOfficeInfo officeInfo;
        if (GetCoreManager().GetConfigSelectedOfficeAndProfile(officeInfo)) {
            viewPort.Latitude = officeInfo.Latitude;
            viewPort.Longitude = officeInfo.Longitude;
        }
        LoadOffices(viewPort, false, officeInfo.OfficeId, officeInfo.ProfileId);
    }
    catch (std::exception& ex) {
        wxLogError(wxString(Format("Init load offices fail: %s", ex.what())));
    }
}

void MainWindow::CheckViewPortToApply() {
    if (m_viewPortHistory.empty()) {
        return;
    }
    auto tp = std::chrono::system_clock::now() - std::chrono::milliseconds(static_cast<int>(ViewPortCheckMs*0.9f));
    auto it = std::find_if(m_viewPortHistory.begin(), m_viewPortHistory.end(),
        [tp](ViewPortCache const& v){
            return v.Created < tp;
        });
    if (it != m_viewPortHistory.end()) {
        LoadOffices(*it, true, Gui::StdEmptyString, Gui::StdEmptyString);
        m_viewPortHistory.erase(it, m_viewPortHistory.end());
    }
}

void MainWindow::RunPrinting(JobContentType contentType, wxArrayString const& files,
                             PrintSettings const& printSettings, int copies) {
    
    Log("MainWindow::RunPrinting()\n");
    std::vector<std::wstring> vfiles;
    vfiles.reserve(files.GetCount());
    for (wxString const& file : files) {
        vfiles.push_back(file.ToStdWstring());
    }
    IAsyncResultPtr asyncResult =
        GetCoreManager().AsyncRunPrinting(*this, contentType, std::move(vfiles), printSettings, copies,
            [this](PrintJobInfoPtr printJobInfo, std::size_t pendingPrintJobId) {
                if (printJobInfo) {
                    Log("RunPrinting complete (jobId:%d juid:%s accessCode:%s)\n", pendingPrintJobId,
                        printJobInfo->Id.c_str(), printJobInfo->AccessCode.c_str());
                    m_pageTabJobsList->ReplacePendingJobWithComplete(pendingPrintJobId, printJobInfo);
                    wxNotificationMessage notifJobComplete(Format("Print job %s", printJobInfo->AccessCode.c_str()),
                        L"New print job successfully uploaded", this);
                    notifJobComplete.Show();
                }
                else {
                    Log("MainWindow::RunPrinting() canceled (jobId:%d)\n", pendingPrintJobId);
                    // m_pageTabJobsList->DeletePendingJob(pendingPrintJobId);
                }
            });
    StoreAsyncResult(asyncResult);
}

void MainWindow::OnJobProgress(std::size_t id, PendingJobStatus status, int intParam, std::string const& strParam) {
    JobProgressEvent* ev = new JobProgressEvent(usrEVT_JOB_PROGRESS, GetId(), id, status, intParam,
        wxString::FromUTF8(strParam));
    wxQueueEvent(this, ev);
}

void MainWindow::OnJobCreate(std::size_t id, std::string const& name, std::size_t pages, std::size_t copies,
                             std::string const& officeId, std::string const& printProfileId) {
    PendingJobCtrlInitInfo pjii;
    pjii.Name = wxString::FromUTF8(name);
    CoreManager& coreMgr = GetCoreManager();
    if (PrintOfficePtr office = coreMgr.GetPrintOffice(officeId)) {
        pjii.OfficeName = wxString::FromUTF8(office->Name);
        if (PrintProfilePtr profile = office->GetPrintProfile(printProfileId)) {
            pjii.TotalCost = wxString::Format(Gui::CostFmt, profile->Price * pages * copies,
                wxString::FromUTF8(office->CurrencySymbol));
        }
    }
    pjii.PagesCount = static_cast<int>(pages);
    pjii.CreateDate = Strings::UTCDateTimeToLocalTimeString(std::time(nullptr));
    m_pageTabJobsList->CreatePendingJob(id, pjii);
    SelectPage(2);
    UpdateTrayTooltip();
}

void MainWindow::OnJobDestroy(std::size_t /* id */) {
    UpdateTrayTooltip();
    if (m_closeOnLastJobDestroyed && !GetCoreManager().GetPendingJobsCount()) {
        Close(true);
    }
}

void MainWindow::OnJobCancelClick(std::size_t pendingJobId, bool doCancel) {
    if (doCancel) {
        GetCoreManager().CancelPendingJob(pendingJobId);
    }
    else {
        IAsyncResultPtr asyncResult = GetCoreManager().AsyncGetJobsList([this, pendingJobId](PrintJobInfoListPtr jobs) {
            m_pageTabJobsList->DeletePendingJob(pendingJobId);
            m_pageTabJobsList->UpdateContent(jobs);
        });
        StoreAsyncResult(asyncResult);
    }
}

WXLRESULT MainWindow::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) {
    if (nMsg == AlreadyRunningBroadcast) {
        ShowAndBringToFront();
        return 0;
    }
    return wxFrame::MSWWindowProc(nMsg, wParam, lParam);
}

void MainWindow::CheckPendingJobsAndCloseApp() {
    int const pendingJobsCount = static_cast<int>(GetCoreManager().GetPendingJobsCount());
    if (pendingJobsCount) {
        if (IsShown() && m_tabView->GetSelection() != 2) {
            SelectPage(2);
        }
        wxMessageDialog dlg(this, wxString::Format(L"%d jobs are still performing.", pendingJobsCount),
            L"Exit application", wxCENTER|wxYES_NO|wxCANCEL|wxICON_WARNING);
        dlg.SetExtendedMessage(wxString::Format(L"%d jobs are still performing. Wait for their completion?", pendingJobsCount));
        switch(dlg.ShowModal()) {
        case wxID_YES:
            m_closeOnLastJobDestroyed = true;
            break;
        case wxID_NO:
            m_closeOnLastJobDestroyed = true;
            GetCoreManager().CancelAllPendingJobs();
            break;
        case wxID_CANCEL:
            break;
        }
        return;
    }
    Close(true);
}

void MainWindow::ShowAndBringToFront() {
    Iconize(false);
    SetFocus();
    Raise();
    Show(true);
    CheckForUpdates(false);
}

void MainWindow::SelectPage(int pageNum) {
    m_tabView->SetSelection(pageNum);
    switch (pageNum)
    {
    case 1:
        m_pageTabWhat->UpdatePrintOffice();
        break;
    case 2:
        {
            IAsyncResultPtr asyncResult = GetCoreManager().AsyncGetJobsList([this](PrintJobInfoListPtr jobs) {
                m_pageTabJobsList->UpdateContent(jobs);
            });
            StoreAsyncResult(asyncResult);
        }
        break;
    default:
        break;
    }
}

void MainWindow::UpdateTrayTooltip() {
    if (m_taskBarIcon) {
        std::size_t const pendingJobsCount = GetCoreManager().GetPendingJobsCount();
        wxString tooltip(GetTitle());
        if (pendingJobsCount) {
            tooltip.Append(wxString::Format(Gui::PendingJobsFmt, static_cast<int>(pendingJobsCount)));
        }
        m_taskBarIcon->SetIcon(wxICON(APP_ICON_BIG), tooltip);
    }
}

void MainWindow::CheckForUpdates(bool manually) {
    std::chrono::system_clock::time_point const now = std::chrono::system_clock::now();
    if (!manually && m_lastUpdateCheckTime + UpdateCheckPeriod > now) {
        return;
    }
    m_processingCheckUpdate = true;
    m_lastUpdateCheckTime = now;
    BinaryVersion const currentVersion{APP_MAJOR_VERSION, APP_MINOR_VERSION, APP_RELEASE_NUMBER, APP_SUBRELEASE_NUMBER};
    IAsyncResultPtr asyncResult = GetCoreManager().AsyncCheckForUpdates(currentVersion,
        [this, manually](LookForUpdateResultPtr result) {
            if (result || manually) {
                ReceiveUpdatesInfoEvent ev(usrEVT_RECEIVE_UPDATES_INFO, GetId(), result);
                wxPostEvent(this, ev);
            }
            else {
                m_processingCheckUpdate = false;
            }
        }
    );
    StoreAsyncResult(asyncResult);
}

void MainWindow::PostCheckForUpdates() {
    wxCommandEvent ev(usrEVT_CHECK_UPDATES_INFO, GetId());
    wxPostEvent(this, ev);
}