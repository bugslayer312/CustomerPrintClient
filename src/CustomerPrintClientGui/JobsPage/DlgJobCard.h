#pragma once

#include "../../CustomerPrintClientCore/Types.h"
#include "../AsyncResultQueue.h"

#include <wx/dialog.h>

class wxStaticText;
class CtrlRating;

class DlgJobCard : public wxDialog
                 , public AsyncResultQueue
{
public:
    DlgJobCard(wxWindow* parent, wxSize const& size, PrintJobInfoPtr job);

private:
    bool UpdateOfficeAndProfile();
    void OnIdle(wxIdleEvent& ev);

private:
    PrintJobInfoPtr m_job;
    wxStaticText* m_lblProfileName;
    wxStaticText* m_lblPaper;
    wxStaticText* m_lblPagePrice;
    CtrlRating* m_ctrlRating;
    wxStaticText* m_lblOfficeAddress;
    wxStaticText* m_lblOfficePhone;
};