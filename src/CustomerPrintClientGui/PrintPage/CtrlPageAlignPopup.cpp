#include "CtrlPageAlignPopup.h"

#include <wx/sizer.h>
#include <wx/radiobox.h>

namespace PageAlignPopup {
enum Ids {
    ID_HOR_RADIO = wxID_HIGHEST + 250,
    ID_VERT_RADIO
};
}

CtrlPageAlignPopup::CtrlPageAlignPopup(wxWindow* parent, ICallBack& callback)
    : wxPopupTransientWindow(parent)
    , m_callback(callback)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wchar_t const* horAlignValues[] = {L"Left", L"Center", L"Right"};
    m_radioHorAlign = new wxRadioBox(this, PageAlignPopup::ID_HOR_RADIO, wxT("Horizontal align"),
        wxDefaultPosition, wxDefaultSize, wxArrayString(3, horAlignValues));
    sizer->Add(m_radioHorAlign, wxSizerFlags(0).Border().Expand());
    wchar_t const* vertAlignValues[] = {L"Top", L"Center", L"Bottom"};
    m_radioVertAlign = new wxRadioBox(this, PageAlignPopup::ID_VERT_RADIO, wxT("Vertical align"),
        wxDefaultPosition, wxDefaultSize, wxArrayString(3, vertAlignValues));
    sizer->Add(m_radioVertAlign, wxSizerFlags(0).Border().Expand());
    SetSizerAndFit(sizer);
}

void CtrlPageAlignPopup::SetDataAndPopup(HorizontalPageAlign horAlign, VerticalPageAlign vertAlign) {
    m_radioHorAlign->SetSelection(static_cast<int>(horAlign));
    m_radioVertAlign->SetSelection(static_cast<int>(vertAlign));
    Popup();
}

bool CtrlPageAlignPopup::Show(bool show /*true*/) {
    if (!show) {
        m_callback.OnPageAlignPopupClose(static_cast<HorizontalPageAlign>(m_radioHorAlign->GetSelection()),
            static_cast<VerticalPageAlign>(m_radioVertAlign->GetSelection()));
    }
    return wxPopupTransientWindow::Show(show);
}