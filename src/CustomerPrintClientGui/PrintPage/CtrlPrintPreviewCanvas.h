#pragma once

#include "../../CustomerPrintClientCore/PrintProfile.h"

#include "wx/scrolwin.h"

#include <memory>

class DrawPageCache;
class PrintSettings;
class IPreviewRenderer;

class CtrlPrintPreviewCanvas : public wxScrolledCanvas {
public:
    CtrlPrintPreviewCanvas(wxWindow *parent, PrintProfilePtr printProfile,
                            PrintSettings const& printSettings,
                            IPreviewRenderer const& previewRenderer,
                            wxWindowID winid = wxID_ANY,
                            const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize,
                            long style = wxScrolledWindowStyle);

    void SetPrintProfile(PrintProfilePtr printProfile);
    void NotifyPrintSettingsChanged();

private:
    void ApplyPrintSettingsChanges();
    void RecalcPageRects(bool forceUpdate);
    void DrawBlankPage(wxDC& dc, wxRect const& pageRect);
    void OnPaint(wxPaintEvent& evt);
    void OnSize(wxSizeEvent& ext);
    void OnScroll(wxScrollWinEvent& event);

    wxDECLARE_EVENT_TABLE();

private:
    PrintProfilePtr m_printProfile;
    PrintSettings const& m_printSettings;
    std::unique_ptr<DrawPageCache> m_drawPageCache;
};