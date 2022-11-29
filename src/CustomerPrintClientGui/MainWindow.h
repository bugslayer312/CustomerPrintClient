#pragma once

#include "AsyncResultQueue.h"
#include "../Printing/GdiplusUtilities.h"
#include "../CustomerPrintClientCore/Types.h"
#include "../CustomerPrintClientCore/PendingJobProgressCallback.h"
#include "MapPage/MapView.h"
#include "PrintPage/PagePrintContent.h"
#include "JobsPage/IJobListItemCallback.h"

#include <wx/frame.h>
#include <wx/timer.h>

#include <mutex>
#include <list>
#include <chrono>

class wxSimplebook;
class wxPanel;
class wxStaticText;
class wxMenuItem;
class wxButton;
class wxStaticBitmap;
class wxTaskBarIcon;
class PageJobsList;
class JobProgressEvent;
class ReceiveUpdatesInfoEvent;

enum class LoginMode {
    LogIn,
    LogOut
};

class MainWindow : public wxFrame
                 , public AsyncResultQueue
                 , public IMapViewCallback
                 , public PagePrintContent::ICallback
                 , public IPendingJobProgressCallback
                 , public IJobListPendingItemCallback
{
    struct ViewPortCache : ViewPort {
        std::chrono::system_clock::time_point Created;

        ViewPortCache(ViewPort const& viewPort)
            : ViewPort(viewPort)
            , Created(std::chrono::system_clock::now())
        {
        }
    };
public:
    MainWindow();
    virtual ~MainWindow() override;// = default;

    // IMapViewCallback implementation
    virtual void OnMapLoaded() override;
    virtual void OnOfficeClicked(std::string const& ouid) override;
    virtual void OnViewPortChanged(ViewPort const& viewPort) override;

    // PagePrintContent::ICallback implementation
    virtual void RunPrinting(JobContentType contentType, wxArrayString const& files,
                             PrintSettings const& printSettings, int copies) override;
    
    // IPendingJobProgressCallback implementation
    virtual void OnJobProgress(std::size_t id, PendingJobStatus status, int intParam,
                               std::string const& strParam) override;
    virtual void OnJobCreate(std::size_t id, std::string const& name, std::size_t pages, std::size_t copies,
                             std::string const& officeId, std::string const& printProfileId) override;
    virtual void OnJobDestroy(std::size_t id) override;

    // IJobListPendingItemCallback implementation
    virtual void OnJobCancelClick(std::size_t pendingJobId, bool doCancel) override;

    virtual WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) override;
    
    void CheckPendingJobsAndCloseApp();
    void ShowAndBringToFront();

private:
    void InitMenu();
    void InitHeader(wxPanel* container);
    void InitToolBar(wxPanel* container);
    void InitMainView();
    void SetUserInfo(std::string const& userName, std::string const& email);
    void SetLoginMode(LoginMode mode);
    void SetPrintOfficeInfo(PrintOfficePtr office);

    void OnActivate();
    void OnIconize(wxIconizeEvent& ev);
    void OnViewPortTimer(wxTimerEvent& ev);
    void OnLogin(wxCommandEvent& ev);
    void OnHelp(wxCommandEvent& ev);
    void OnIdle(wxIdleEvent& ev);
    void OnClose(wxCloseEvent& ev);
    void OnInitLoadOffices(wxCommandEvent& ev);
    void OnLoginUi(wxCommandEvent& ev);
    void OnSettings(wxCommandEvent& ev);
    void OnMenuQuit(wxCommandEvent& ev);
    void OnMenuAbout(wxCommandEvent& ev);
    void OnMenuCheckForUpdates(wxCommandEvent& ev);
    void HandleJobProgress(JobProgressEvent& ev);
    void HandleReceiveUpdatesInfo(ReceiveUpdatesInfoEvent& ev);

    void OnAsyncQuietLogin(bool success);

    void Login(bool isFirstLogin);
    void AfterLogin(bool isFirstLogin);
    void Logout();
    void SelectOfficeAndProfile(std::string const& ouid, std::string const& puid, bool centerViewOnOffice);
    void LoadOffices(ViewPort const& viewPort, bool newFound, std::string const& officeIdToSelect,
                     std::string const& profileIdToSelect);
    void LoadOfficesAtInit();
    void CheckViewPortToApply();
    void SelectPage(int pageNum);
    void UpdateTrayTooltip();
    void CheckForUpdates(bool manually);
    void PostCheckForUpdates();

private:
    GdiplusInit m_gdiplusInit;
    bool m_activated;
    bool m_closeOnLastJobDestroyed;
    LoginMode m_loginMode;
    wxPanel* m_header;
    wxStaticText* m_txtCtrlUser;
    wxStaticText* m_txtCtrlEmail;
    wxStaticBitmap* m_bmpCtrlOffice;
    wxStaticText* m_txtCtrlOfficeName;
    wxStaticText* m_txtCtrlOfficeAddress;
    wxMenuItem* m_loginMenuItem;
    wxButton* m_btnLogin;
    wxSimplebook* m_tabView;
    wxWindow* m_mapViewWnd;
    IMapViewPtr m_mapView;
    PagePrintContent* m_pageTabWhat;
    PageJobsList* m_pageTabJobsList;
    std::function<void()> m_onMapLoad;
    std::function<void()> m_onShow;
    std::mutex m_loadMutex;
    std::mutex m_viewPortMutex;
    std::list<ViewPortCache> m_viewPortHistory;
    wxTimer m_viewportChangedTimer;
    std::unique_ptr<wxTaskBarIcon> m_taskBarIcon;
    std::chrono::system_clock::time_point m_lastUpdateCheckTime;
    bool m_processingCheckUpdate;
};
