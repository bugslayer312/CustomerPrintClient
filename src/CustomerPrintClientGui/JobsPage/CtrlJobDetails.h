#pragma once

#include "../../CustomerPrintClientCore/Types.h"

#include <wx/window.h>

class IJobListItemCallback;
class wxStaticText;

class CtrlJobDetails : public wxWindow {
public:
    class ICallback {
    public:
        virtual ~ICallback() = default;
        virtual void OnJobClicked(wxWindow* wnd) = 0;
    };

    CtrlJobDetails(wxWindow* parent, IJobListItemCallback& callback, PrintJobInfoPtr job);
    
    void UpdateJob();

    // wxDECLARE_EVENT_TABLE();

private:
    void OnMouseLeftDown(wxMouseEvent& ev);
    void OnMouseLeftDoubleClick(wxMouseEvent& ev);

private:
    IJobListItemCallback& m_callback;
    PrintJobInfoPtr m_job;
    static int const m_margin;
    static wxSize const m_imgSize;
    wxStaticText* m_lblStatus;
    wxStaticText* m_lblPayStatus;
    wxStaticText* m_lblUpdateTime;
};