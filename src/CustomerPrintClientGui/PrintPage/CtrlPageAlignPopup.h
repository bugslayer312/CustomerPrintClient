#pragma once

#include "../../Printing/PrintSettings.h"

#include <wx/popupwin.h>

class wxRadioBox;

class CtrlPageAlignPopup : public wxPopupTransientWindow {
public:
    class ICallBack {
    public:
        virtual ~ICallBack() = default;
        virtual void OnPageAlignPopupClose(HorizontalPageAlign horAlign, VerticalPageAlign vertAlign) = 0;
    };
    
    CtrlPageAlignPopup(wxWindow* parent, ICallBack& callback);
    void SetDataAndPopup(HorizontalPageAlign horAlign, VerticalPageAlign vertAlign);
    virtual bool Show(bool show = true) override;

private:
    ICallBack& m_callback;
    wxRadioBox* m_radioHorAlign;
    wxRadioBox* m_radioVertAlign;
};