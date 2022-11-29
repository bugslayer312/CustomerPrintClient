#include "MapView.h"

#include "../../Core/Format.h"
#include "../../Core/Log.h"
#include "../../Core/jsonxx.h"
#include "../../Resources/Resources.h"
#include "../../CustomerPrintClientCore/PrintOffice.h"
// #include "../../CustomerPrintClientCore/CoreManager.h"

#include <wx/webview.h>
#include <wx/filesys.h>
#include <wx/fs_mem.h>
#include <wx/webviewfshandler.h>
#include <wx/log.h>

#include <stdexcept> 

const char* const MapViewHtml = "MapView.html";
const char* const OfficeMarkerMail = "office-marker-mail.png";
const char* const OfficeMarkerNormal = "office-marker-normal.png";
const char* const OfficeMarkerSelected = "office-marker-selected.png";

const char* const MarkerClickEvent = "MarkerClick";
const char* const ViewPortChanged = "ViewPortChanged";
const char* const MapCreated = "MapCreated";

class WebViewMapImpl : public IMapView {
private:
    wxWebView* m_webView;
    IMapViewCallback& m_callback;
    bool m_isMapLoaded;
    std::uint64_t m_prevEventId;
public:
    WebViewMapImpl(wxWebView* webView, IMapViewCallback& callback)
        : m_webView(webView)
        , m_callback(callback)
        , m_isMapLoaded(false)
        , m_prevEventId(0)
    {
        wxFileSystem::AddHandler(new wxMemoryFSHandler());
        auto fs = cmrc::Res::get_filesystem();
        auto fd = fs.open(Format("web/%s", MapViewHtml));
        wxMemoryFSHandler::AddFile(MapViewHtml, fd.cbegin(), fd.size());
        fd = fs.open(Format("web/%s", OfficeMarkerMail));
        wxMemoryFSHandler::AddFile(OfficeMarkerMail, fd.cbegin(), fd.size());
        fd = fs.open(Format("web/%s", OfficeMarkerNormal));
        wxMemoryFSHandler::AddFile(OfficeMarkerNormal, fd.cbegin(), fd.size());
        fd = fs.open(Format("web/%s", OfficeMarkerSelected));
        wxMemoryFSHandler::AddFile(OfficeMarkerSelected, fd.cbegin(), fd.size());
        m_webView->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewFSHandler("memory")));
        m_webView->Bind(wxEVT_WEBVIEW_TITLE_CHANGED, &WebViewMapImpl::OnWebDocumentTittleChanged, this);
        m_webView->Bind(wxEVT_WEBVIEW_ERROR, &WebViewMapImpl::OnWebDocumentError, this);
        m_webView->LoadURL(Format("memory:%s", MapViewHtml));
    }

    virtual ~WebViewMapImpl() override = default;

    virtual bool IsLoaded() const override {
        return m_isMapLoaded;
    }

    virtual ViewPort GetViewPort() override {
        ViewPort result;
        wxString output;
        jsonxx::Object json;
        if (m_webView->RunScript("getViewPort();", &output) && json.parse(output.ToStdString()) &&
            ReadNumber(json, "lat", result.Latitude) && ReadNumber(json, "lng", result.Longitude) &&
            ReadNumber(json, "radius", result.ViewRadiusMeters)) {
        
            return result;
        }
        throw std::runtime_error("MapView: Failed get viewport");
    }

    virtual void PlaceOffices(PrintOfficePreviewList const& offices) override {
        if (!offices.empty()) {
            wxString script(L"placeOffices([");
            for (PrintOfficePreviewPtr office : offices) {
                script << L"{Lat:" << std::to_string(office->Latitude)
                    << L",Lng:" << std::to_string(office->Longitude)
                    << L",Mailgate:" << std::to_string(office->Mailgate())
                    << L",Name:\"" << wxString::FromUTF8(office->Name)
                    << L"\",Id:\"" << office->Id << L"\"},";
            }
            script[script.length()-1] = ' ';
            script << L"]);";
            m_webView->RunScript(script);
        }
    }

    virtual void SelectOffice(std::string const& ouid, bool centerOnOffice) override {
        wxString script(L"selectOffice({");
        script << L"Id:\"" << wxString::FromUTF8(ouid);
        script << L"\",CenterView:" << std::to_string(centerOnOffice) << L"});";
        m_webView->RunScript(script);
    }

private:
    void OnWebDocumentTittleChanged(wxWebViewEvent&) {
        std::string title = m_webView->GetCurrentTitle().ToUTF8();
        if (!title.empty()) {
            jsonxx::Object json;
            if (json.parse(title)) {
                std::uint64_t eventId(0);
                std::string event;
                if (ReadNumber(json, "id", eventId) && eventId > m_prevEventId &&
                    ReadString(json, "event", event) && json.has<jsonxx::Object>("params")) {

                    Log("MapEvent: %s\n", title.c_str());
                    m_prevEventId = eventId;
                    jsonxx::Object const& jsonParams = json.get<jsonxx::Object>("params");
                    DispatchMapViewEvent(event, jsonParams);
                }
            }
        }
    }

    void OnWebDocumentError(wxWebViewEvent& ev) {
        Log("MapView error:\n%s\n", ev.GetString().ToStdString().c_str());
    }

    void DispatchMapViewEvent(std::string const& event, jsonxx::Object const& jsonParams) {
        if (event == MarkerClickEvent) {
            std::string ouid;
            if (ReadString(jsonParams, "ouid", ouid)) {
                m_callback.OnOfficeClicked(ouid);
            }
        }
        else if (event == ViewPortChanged) {
            ViewPort viewPort;
            if (ReadNumber(jsonParams, "lat", viewPort.Latitude) && ReadNumber(jsonParams, "lng", viewPort.Longitude) &&
                ReadNumber(jsonParams, "radius", viewPort.ViewRadiusMeters)) {
                
                m_callback.OnViewPortChanged(viewPort);
            }
        }
        else if (event == MapCreated) {
            if (jsonParams.get<jsonxx::Boolean>("result", false)) {
                m_isMapLoaded = true;
                m_callback.OnMapLoaded();
            }
            else {
                wxString errorMsg("MapView: ");
                std::string error;
                errorMsg += ReadString(jsonParams, "error", error) ? error : std::string("Failed to get location");
                wxLogError(errorMsg);
            }
        }
        else {
            Log("Unsupported MapView event: %s", event.c_str());
        }
    }
};

IMapViewPtr CreateMapView(wxWebView* webView, IMapViewCallback& callback) {
    return IMapViewPtr(new WebViewMapImpl(webView, callback));
}