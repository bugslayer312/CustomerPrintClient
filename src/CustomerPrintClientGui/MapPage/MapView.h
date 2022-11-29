#pragma once

#include <memory>
#include <string>
#include <list>

class wxWebView;
struct PrintOfficePreview;
typedef std::shared_ptr<PrintOfficePreview> PrintOfficePreviewPtr;
typedef std::list<PrintOfficePreviewPtr> PrintOfficePreviewList;

struct ViewPort {
    float Latitude;
    float Longitude;
    std::uint32_t ViewRadiusMeters;
};

class IMapView {
public:
    virtual ~IMapView() = default;
    virtual bool IsLoaded() const = 0;
    virtual ViewPort GetViewPort() = 0;
    virtual void PlaceOffices(PrintOfficePreviewList const& offices) = 0;
    virtual void SelectOffice(std::string const& ouid, bool centerOnOffice) = 0;
};
typedef std::unique_ptr<IMapView> IMapViewPtr;

class IMapViewCallback {
public:
    virtual ~IMapViewCallback() = default;
    virtual void OnMapLoaded() = 0;
    virtual void OnOfficeClicked(std::string const& ouid) = 0;
    virtual void OnViewPortChanged(ViewPort const& viewPort) = 0;
};

IMapViewPtr CreateMapView(wxWebView* webView, IMapViewCallback& callback);