#pragma once

#include "../../CustomerPrintClientCore/PrintProfile.h"

#include <wx/control.h>

#include <memory>

class wxStaticText;
class wxCommandEvent;
class CtrlPrintProfileCombo;
struct PrintProfileFonts;
struct PrintOffice;

wxDECLARE_EVENT(wxEVT_PRINT_PROFILES, wxCommandEvent);

class CtrlPrintProfiles : public wxControl {
public:
    CtrlPrintProfiles(wxWindow* parent, wxWindowID id, PrintOffice* printOffice,
                      wxPoint const& pos = wxDefaultPosition, wxSize const& size = wxDefaultSize, long style = 0);
    virtual ~CtrlPrintProfiles() override;

    void SelectItem(wxString const& profileId);

private:
    void OnCmbSelectionChanged(wxCommandEvent& ev);
    void UpdateStatics(PrintProfilePtr pp);

private:
    PrintOffice* m_printOffice;
    wxString const m_currencySymbol;
    wxString m_selectedPrintProfileId;
    std::unique_ptr<PrintProfileFonts> m_printProfileFonts;
    CtrlPrintProfileCombo* m_cmbProfiles;
    wxStaticText* m_txtPaper;
    wxStaticText* m_txtPrice;
};