#pragma once

#include "CtrlPageAlignPopup.h"
#include "../../Printing/IPreviewRenderer.h"
#include "../../CustomerPrintClientCore/Types.h"
#include "../../CustomerPrintClientCore/JobContentType.h"

class wxCommandEvent;
class wxStaticText;
class CtrlPrintProfiles;
class CtrlPrintPreviewCanvas;

class PanelPrintPreview : public wxWindow
                        , public CtrlPageAlignPopup::ICallBack {
public:
    class ICallback {
    public:
        virtual ~ICallback() = default;
        virtual void OnPreviewCloseClick() = 0;
        virtual void OnPrintClick() = 0;
    };
    PanelPrintPreview(wxWindow* parent, ICallback& callback, JobContentType contentType, wxArrayString const& files,
                      PrintOfficePtr printOffice, PrintSettings& printSettings, int& copies, wxWindowID id = wxID_ANY,
                      wxPoint const& pos = wxDefaultPosition, wxSize const& size = wxDefaultSize, long style = 0);
    
    virtual ~PanelPrintPreview() override;
    virtual void OnPageAlignPopupClose(HorizontalPageAlign horAlign, VerticalPageAlign vertAlign) override;

    void SetPrintOffice(PrintOfficePtr printOffice);

private:
    void OnHeaderCloseClick(wxCommandEvent& ev);
    void OnPrintProfileSelect(wxCommandEvent& ev);
    void OnOrientationSelected(wxCommandEvent& ev);
    void OnModeChanged(wxCommandEvent& ev);
    void OnPageAlignClick(wxCommandEvent& ev);
    void OnCopiesChanged(wxCommandEvent& ev);
    void OnPrintClick(wxCommandEvent& ev);
    int GetPageCount() const;
    wxString GetTotalCostString() const;
    void UpdateTotalCostText();

private:
    ICallback& m_callback;
    IPreviewRendererUPtr m_previewRenderer;
    PrintOfficePtr m_printOffice;
    PrintSettings& m_printSettings;
    int& m_copies;
    CtrlPageAlignPopup* m_pageAlignPopup;
    CtrlPrintProfiles* m_ctrlPrintProfiles;
    CtrlPrintPreviewCanvas* m_ctrlPreviewCanvas;
    wxStaticText* m_totalCost;
};