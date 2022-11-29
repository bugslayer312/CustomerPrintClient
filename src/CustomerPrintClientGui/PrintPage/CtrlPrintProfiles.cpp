#include "CtrlPrintProfiles.h"

#include "../../Core/Log.h"
#include "../../CustomerPrintClientCore/PrintOffice.h"
#include "../StringResources.h"

#include <wx/dcclient.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/odcombo.h>

#include <vector>

struct PrintProfileFonts {
    wxFont HeaderFont;
    wxFont PaperFont;
    wxFont PriceFont;

    PrintProfileFonts(wxFont const& baseFont)
        : HeaderFont(baseFont)
        , PaperFont(baseFont)
        , PriceFont(baseFont)
    {
        HeaderFont.MakeBold();
        PaperFont.Scale(0.8f);
        PriceFont.Scale(0.8f).MakeBold();
    }
};

wxArrayString GetIdsArray(PrintOffice* printOffice) {
    wxArrayString result;
    if (printOffice) {
        std::size_t const ppCount = printOffice->GetPrintProfileCount();
        if (ppCount) {
            result.Alloc(ppCount);
            for (std::size_t i(0); i < ppCount; ++i) {
                result.Add(wxString::FromUTF8(printOffice->GetPrintProfile(i)->Id));
            }
        }
    }
    return result;
}

wxString GetSelected(PrintOffice* printOffice) {
    wxString result;
    if (printOffice) {
        result = wxString::FromUTF8(printOffice->GetSelectedPrintProfileId());
    }
    return result;
}

wxString GetCurrency(PrintOffice* printOffice) {
    wxString result;
    if (printOffice) {
        result = wxString::FromUTF8(printOffice->CurrencySymbol);
    }
    return result;
}

// CtrlPrintProfileCombo

class CtrlPrintProfileCombo : public wxOwnerDrawnComboBox {
    struct FontsMetrix {
        int HeaderFontHeight;
        int HeaderFontWidth;
        int PaperFontHeight;
        int PaperFontWidth;
        int PriceFontHeight;
        int PriceFontWidth;
        int ItemHeight;
    };
    struct ItemInfo {
        wxString Caption;
        wxString Paper;
        wxString Price;
        bool Prepared = false;

        wxCoord CalcWidth(FontsMetrix const& fm) const {
            wxCoord result = Caption.Length() * fm.HeaderFontWidth;
            wxCoord w = Paper.Length() * fm.PaperFontWidth;
            if (w > result) {
                result = w;
            }
            w = Price.Length() * fm.PriceFontWidth;
            if (w > result) {
                result = w;
            }
            return static_cast<wxCoord>(result*0.9f);
        }
    };
public:
    CtrlPrintProfileCombo(PrintProfileFonts const& printProfileFonts, PrintOffice* printOffice,
                          wxString const& currencySymbol = wxEmptyString)
        : wxOwnerDrawnComboBox()
        , m_printProfileFonts(printProfileFonts)
        , m_printOffice(printOffice)
        , m_currencySymbol(currencySymbol)
        , m_preparedItems(m_printOffice ? m_printOffice->GetPrintProfileCount() : 0)
    {
    }

    virtual void OnDrawItem(wxDC& dc, wxRect const& rect, int item, int flags) const override {
        if (item == wxNOT_FOUND) {
            return;
        }
        wxRect r(rect);
        r.Deflate(m_padding);
        ItemInfo const& itemInfo = GetPreparedItem(static_cast<std::size_t>(item));
        FontsMetrix const& fms = GetFontsMetrix();
        dc.SetFont(m_printProfileFonts.HeaderFont);
        if (flags & wxODCB_PAINTING_CONTROL) {
            dc.DrawText(itemInfo.Caption, r.GetX(), r.GetY() + r.GetHeight()/2 - fms.HeaderFontHeight/2);
        }
        else {
            int y = r.GetY();
            dc.DrawText(itemInfo.Caption, r.GetX(), y);
            dc.SetFont(m_printProfileFonts.PaperFont);
            y += fms.HeaderFontHeight + fms.HeaderFontHeight/2;
            dc.DrawText(itemInfo.Paper, r.GetX(), y);
            dc.SetFont(m_printProfileFonts.PriceFont);
            y += fms.PaperFontHeight + fms.PaperFontHeight/2;
            dc.DrawText(itemInfo.Price, r.GetX(), y);
        }
    }

    virtual wxCoord OnMeasureItem(size_t item) const override {
        return GetFontsMetrix().ItemHeight;
    }

    virtual wxCoord OnMeasureItemWidth(size_t item) const override {
        return GetPreparedItem(item).CalcWidth(GetFontsMetrix());
    }

private:
    FontsMetrix const& GetFontsMetrix() const {
        if (!m_fontsMetrix) {
            CtrlPrintProfileCombo* NonConstThis = const_cast<CtrlPrintProfileCombo*>(this);
            NonConstThis->m_fontsMetrix.reset(new FontsMetrix());
            wxClientDC dc(NonConstThis);
            dc.SetFont(m_printProfileFonts.HeaderFont);
            m_fontsMetrix->HeaderFontHeight = dc.GetCharHeight();
            m_fontsMetrix->HeaderFontWidth = dc.GetCharWidth();
            dc.SetFont(m_printProfileFonts.PaperFont);
            m_fontsMetrix->PaperFontHeight = dc.GetCharHeight();
            m_fontsMetrix->PaperFontWidth = dc.GetCharWidth();
            dc.SetFont(m_printProfileFonts.PriceFont);
            m_fontsMetrix->PriceFontHeight = dc.GetCharHeight();
            m_fontsMetrix->PriceFontWidth = dc.GetCharWidth();
            m_fontsMetrix->ItemHeight = m_padding*2 + m_fontsMetrix->HeaderFontHeight + m_fontsMetrix->HeaderFontHeight/2 +
                m_fontsMetrix->PaperFontHeight + m_fontsMetrix->PaperFontHeight/2 + m_fontsMetrix->PriceFontHeight;
        }
        return *m_fontsMetrix;
    }

    ItemInfo const& GetPreparedItem(size_t idx) const {
        ItemInfo& result = const_cast<CtrlPrintProfileCombo*>(this)->m_preparedItems[idx];
        if (!result.Prepared) {
            PrintProfilePtr pp = m_printOffice->GetPrintProfile(static_cast<std::size_t>(idx));
            result.Caption = wxString::FromUTF8(pp->Name);
            result.Paper = wxString::Format(Gui::PaperFmt, wxString::FromUTF8(pp->Paper.Name).c_str(),
                pp->Paper.Width/100, pp->Paper.Height/100);
            result.Price = wxString::Format(Gui::PriceFmt, pp->Price, m_currencySymbol);
            result.Prepared = true;
        }
        return result;
    }

private:
    static const int m_padding;
    PrintProfileFonts const& m_printProfileFonts;
    PrintOffice* m_printOffice;
    wxString const& m_currencySymbol;
    std::vector<ItemInfo> m_preparedItems;
    std::unique_ptr<FontsMetrix> m_fontsMetrix;
};

namespace PrintProfiles {
enum CtrlIds {
    ID_SELECT_PRINT_PROFILE_CMB = wxID_HIGHEST + 100
};
} // namespace PrintProfiles

const int CtrlPrintProfileCombo::m_padding = 3;


// CtrlPrintProfiles

wxDEFINE_EVENT(wxEVT_PRINT_PROFILES, wxCommandEvent);

CtrlPrintProfiles::CtrlPrintProfiles(wxWindow* parent, wxWindowID id,
                                    PrintOffice* printOffice,
                                    wxPoint const& pos, wxSize const& size, long style)
    : wxControl(parent, id, pos, size, style|wxBORDER_NONE)
    , m_printOffice(printOffice)
    , m_currencySymbol(GetCurrency(printOffice))
    , m_selectedPrintProfileId(GetSelected(printOffice))
{
    m_printProfileFonts.reset(new PrintProfileFonts(GetFont()));
    m_cmbProfiles = new CtrlPrintProfileCombo(*m_printProfileFonts, m_printOffice, m_currencySymbol);
    m_cmbProfiles->Create(this, PrintProfiles::ID_SELECT_PRINT_PROFILE_CMB, GetSelected(printOffice),
        wxDefaultPosition, wxDefaultSize, GetIdsArray(printOffice), wxCB_READONLY);

    m_txtPaper = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_txtPaper->SetFont(m_printProfileFonts->PaperFont);
    m_txtPrice = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_txtPrice->SetFont(m_printProfileFonts->PriceFont);
    if (m_printOffice) {
        UpdateStatics(m_printOffice->GetSelectedPrintProfile());
    }
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_cmbProfiles);
    sizer->AddSpacer(m_cmbProfiles->GetCharHeight()/2);
    sizer->Add(m_txtPaper, wxSizerFlags(0).Border(wxLEFT));
    sizer->AddSpacer(m_txtPaper->GetCharHeight()/2);
    sizer->Add(m_txtPrice, wxSizerFlags(0).Border(wxLEFT));
    SetSizerAndFit(sizer);
    Bind(wxEVT_COMBOBOX, &CtrlPrintProfiles::OnCmbSelectionChanged, this, PrintProfiles::ID_SELECT_PRINT_PROFILE_CMB);
}

CtrlPrintProfiles::~CtrlPrintProfiles() {
}

void CtrlPrintProfiles::SelectItem(wxString const& profileId) {
    if (m_printOffice) {
        PrintProfilePtr pp = m_printOffice->GetPrintProfile(profileId);
        if (pp) {
            m_cmbProfiles->SetValueByUser(profileId);
            UpdateStatics(pp);
        }
    }
}

void CtrlPrintProfiles::OnCmbSelectionChanged(wxCommandEvent& ev) {
    if (ev.GetString() != m_selectedPrintProfileId) {
        m_selectedPrintProfileId = ev.GetString();
        UpdateStatics(m_printOffice->GetPrintProfile(ev.GetInt()));
        wxCommandEvent eventOut(wxEVT_PRINT_PROFILES, GetId());
        eventOut.SetEventObject(this);
        eventOut.SetString(ev.GetString());
        eventOut.SetInt(ev.GetInt());
        wxPostEvent(this, eventOut);
    }
}

void CtrlPrintProfiles::UpdateStatics(PrintProfilePtr pp) {
    if (pp) {
        m_txtPaper->SetLabel(wxString::Format(Gui::PaperFmt, wxString::FromUTF8(pp->Paper.Name),
            pp->Paper.Width/100, pp->Paper.Height/100));
        m_txtPrice->SetLabel(wxString::Format(Gui::PriceFmt, pp->Price, m_currencySymbol));
    }
}