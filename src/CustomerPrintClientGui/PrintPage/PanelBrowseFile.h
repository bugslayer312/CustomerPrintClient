#pragma once

#include "../../CustomerPrintClientCore/JobContentType.h"

#include <wx/window.h>
#include <wx/arrstr.h>

class wxCommandEvent;
class wxTreeEvent;
class wxGenericDirCtrl;

class PanelBrowseFile : public wxWindow {
public:
    class ICallback {
    public:
        virtual ~ICallback() = default;
        virtual void OnBrowseResult() = 0;
        virtual void OnContentTypeSelected(JobContentType contentType) = 0;
    };
    PanelBrowseFile(wxWindow* parent, ICallback& callback, JobContentType contentType, wxArrayString& selectedFiles,
                    wxWindowID id = wxID_ANY, wxPoint const& pos = wxDefaultPosition,
                    wxSize const& size = wxDefaultSize, long style = 0);

private:
    void OnFileSelected(wxTreeEvent& event);
    void OnNextClick(wxCommandEvent& event);
    void OnFilterSelected(wxCommandEvent& event);
    // wxGenericDirCtrl* CreateDirCtrl(JobContentType contentType);

private:
    ICallback& m_callback;
    wxArrayString& m_selectedFiles;
    wxGenericDirCtrl* m_ctrlFile;
    wxButton* m_btnNext;
};