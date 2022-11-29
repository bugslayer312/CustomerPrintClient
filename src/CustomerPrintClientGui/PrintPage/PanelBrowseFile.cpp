#include "PanelBrowseFile.h"

#include "../../Core/Log.h"
#include "../../Core/OS.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dirctrl.h>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace BrowseFile {
enum CtrlIds {
    ID_NEXT = wxID_HIGHEST + 200,
    ID_SEL_CHANGED
};
} // namespace PrintContent

PanelBrowseFile::PanelBrowseFile(wxWindow* parent, ICallback& callback, JobContentType contentType,
                                 wxArrayString& selectedFiles, wxWindowID id, wxPoint const& pos,
                                 wxSize const& size, long style)
    : wxWindow(parent, id, pos, size, style)
    , m_callback(callback)
    , m_selectedFiles(selectedFiles)
{
    int dirCtrlStyle = wxDIRCTRL_SHOW_FILTERS|wxDIRCTRL_3D_INTERNAL;
    if (contentType == JobContentType::Images) {
        dirCtrlStyle |= wxDIRCTRL_MULTIPLE;
    }
    m_ctrlFile = new wxGenericDirCtrl(this, BrowseFile::ID_SEL_CHANGED, OS::GetUserDocumentDir(),
        wxDefaultPosition, wxDefaultSize, dirCtrlStyle,
        L"Images (*.bmp;*.png;*.jpg;*.jpeg;*.gif;*.tiff)|*.bmp;*.png;*.jpg;*.jpeg;*.gif;*.tiff|PDF documents (*.pdf)|*.pdf",
        static_cast<int>(contentType));
    m_btnNext = new wxButton(this, BrowseFile::ID_NEXT, L"Next");
    m_btnNext->Enable(false);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_ctrlFile, wxSizerFlags(1).Expand());
    sizer->Add(m_btnNext, wxSizerFlags(0).Right().Border());
    SetSizer(sizer);
    Bind(wxEVT_BUTTON, &PanelBrowseFile::OnNextClick, this, BrowseFile::ID_NEXT);
    Bind(wxEVT_DIRCTRL_SELECTIONCHANGED, &PanelBrowseFile::OnFileSelected, this, BrowseFile::ID_SEL_CHANGED);
    m_ctrlFile->GetFilterListCtrl()->Bind(wxEVT_CHOICE, &PanelBrowseFile::OnFilterSelected, this);
}

void PanelBrowseFile::OnFileSelected(wxTreeEvent& /* event */) {
    m_ctrlFile->GetPaths(m_selectedFiles);
    std::size_t i(0);
    while (i < m_selectedFiles.GetCount()) {
        if (fs::is_regular_file(fs::path(m_selectedFiles[i].ToUTF8().data()))) {
            ++i;
        }
        else {
            m_selectedFiles.RemoveAt(i);
        }
    }
    m_btnNext->Enable(!m_selectedFiles.IsEmpty());
}

void PanelBrowseFile::OnNextClick(wxCommandEvent& /* event */) {
    m_callback.OnBrowseResult();
}

void PanelBrowseFile::OnFilterSelected(wxCommandEvent& event) {
    JobContentType const newContentType = static_cast<JobContentType>(event.GetInt());
    m_callback.OnContentTypeSelected(newContentType);
    m_selectedFiles.Clear();
    m_btnNext->Enable(false);
    int treeStyle = m_ctrlFile->GetTreeCtrl()->GetWindowStyle();
    if (newContentType == JobContentType::Images) {
        treeStyle |= wxTR_MULTIPLE;
    }
    else {
        treeStyle &= ~wxTR_MULTIPLE;
    }
    m_ctrlFile->GetTreeCtrl()->SetWindowStyle(treeStyle);
    event.Skip();
}