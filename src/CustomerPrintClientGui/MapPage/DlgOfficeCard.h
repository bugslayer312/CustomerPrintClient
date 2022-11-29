#pragma once

#include <wx/dialog.h>

struct PrintOffice;

class DlgOfficeCard : public wxDialog {
public:
    DlgOfficeCard(wxWindow* parent, wxSize const& size, PrintOffice const& office);

private:
    PrintOffice const& m_office;
};