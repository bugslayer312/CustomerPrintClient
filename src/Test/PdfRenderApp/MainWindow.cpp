#include "MainWindow.h"

#include "../../Printing/Pdf/PdfError.h"
#include "../../Printing/Pdf/PdfLibrary.h"
#include "../../Printing/Pdf/PdfDocument.h"
#include "../../Printing/Pdf/IPdfPasswordProvider.h"
#include "../../Printing/GdiplusUtilities.h"

#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/filepicker.h>
#include <wx/gbsizer.h>
#include <wx/log.h>
#include <wx/textdlg.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/spinctrl.h>
#include <wx/dcclient.h>

#define NOMINMAX
#include <windows.h>
//#include <winspool.h>
using std::min;
using std::max;
#include <gdiplus.h>

#include <fstream>

//#include <windows.h>
//#include <fpdf_dataavail.h>
//#include <fpdfview.h>
//#include <fpdf_doc.h>
//#include <fpdf_formfill.h>
//#include <fpdf_ext.h>
//#include <fpdf_text.h>
//#include <fpdf_fwlevent.h>
// #include <wx/webview.h>

enum CtrlIds {
    ID_OPEN_PDF_FILE = wxID_HIGHEST + 1,
    ID_RENDER_PAGE
};

class PdfView : public wxWindow {
public:
    PdfView(wxWindow* parent, wxWindowID id = -1, wxPoint const& pos = wxDefaultPosition,
        wxSize const& size = wxDefaultSize, long style = 0)
        : wxWindow(parent, id, pos, size, style)
    {
        Bind(wxEVT_PAINT, &PdfView::OnPaint, this);
    }

    void OnPaint(wxPaintEvent& event) {
        wxPaintDC dc(this);
        PrepareDC(dc);
        dc.SetBackground(GetBackgroundColour());
	    dc.Clear();
        if (wxBitmap* bmp = static_cast<MainWindow*>(GetParent())->GetCachedBitmap()) {
            dc.DrawBitmap(*bmp, 0, 0);
        }
    }
};


class PwdProvider : public Pdf::IPasswordProvider {
public:
    PwdProvider(wxWindow* parentWnd, std::string const& password)
        : m_parentWnd(parentWnd)
        , m_password(password)
    {
    }

    std::string const& GetPassword() const override {
        return m_password;
    }

    bool RequestPassword() override {
        wxPasswordEntryDialog dlg(m_parentWnd, L"Password to open the document");
        dlg.SetValue(m_password);
        if (dlg.ShowModal() == wxID_OK) {
            m_password = dlg.GetValue().ToUTF8();
            return true;
        }
        return false;
    }

private:
    wxWindow* m_parentWnd;
    std::string m_password;
};

MainWindow::MainWindow()
    : wxFrame(nullptr, wxID_ANY, L"Render pdf test")
{
    SetInitialSize(FromDIP(wxSize(800, 600)));

    wxPanel* topPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 100));
    wxPanel* leftPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(200, -1));
    m_viewWnd = new PdfView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_STATIC);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* bodySizer = new wxBoxSizer(wxHORIZONTAL);
    bodySizer->Add(leftPanel, wxSizerFlags(0).Expand());
    bodySizer->Add(m_viewWnd, wxSizerFlags(1).Expand());
    mainSizer->Add(topPanel, wxSizerFlags(0).Expand());
    mainSizer->Add(bodySizer, wxSizerFlags(1).Expand());
    
    wxGridBagSizer* topSizer = new wxGridBagSizer();
    wxFilePickerCtrl* filePicker = new wxFilePickerCtrl(topPanel, ID_OPEN_PDF_FILE, L"F:\\Samples\\Pdf\\",
        wxFileSelectorPromptStr, wxFileSelectorDefaultWildcardStr, wxDefaultPosition, wxSize(300, -1));
    wxStaticText* labPageCount = new wxStaticText(topPanel, wxID_ANY, L"Page count:");
    m_txtPageCount = new wxTextCtrl(topPanel, wxID_ANY, L"0", wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxTE_RIGHT);
    topSizer->Add(filePicker, wxGBPosition(0, 0), wxGBSpan(1, 2), wxALL, 6);
    topSizer->Add(labPageCount, wxGBPosition(1, 0), wxDefaultSpan, wxALL, 6);
    topSizer->Add(m_txtPageCount, wxGBPosition(1, 1), wxDefaultSpan, wxALL, 6);
    topPanel->SetSizer(topSizer);

    wxGridBagSizer* leftSizer = new wxGridBagSizer();
    wxButton* btnRenderPage = new wxButton(leftPanel, ID_RENDER_PAGE, L"Render page");
    m_editPageNum = new wxSpinCtrl(leftPanel, wxID_ANY, L"0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 0, 0);
    leftSizer->Add(btnRenderPage, wxGBPosition(0, 0), wxDefaultSpan, wxALL, 6);
    leftSizer->Add(m_editPageNum, wxGBPosition(0, 1), wxDefaultSpan, wxALL, 6);
    leftPanel->SetSizer(leftSizer);

    SetSizer(mainSizer);

    auto unsupportedHandle = [](int type) {
        wxString feature(Pdf::UnsupportedFeautureString(type));
        if (!feature.empty()) {
            wxLogError("PDF unsupported feature: %s", feature);
        }
    };

    m_pdfLibrary.reset(new Pdf::Library(std::move(unsupportedHandle), std::make_unique<PwdProvider>(this, "")));

    Bind(wxEVT_FILEPICKER_CHANGED, &MainWindow::OnPdfFilePicked, this, ID_OPEN_PDF_FILE);
    Bind(wxEVT_BUTTON, &MainWindow::OnRenderPage, this, ID_RENDER_PAGE);
}

MainWindow::~MainWindow() {
}

void MainWindow::OnPdfFilePicked(wxFileDirPickerEvent& event) {
    m_pdfDocument = m_pdfLibrary->OpenDocument(event.GetPath().ToStdString());
    int pagesCount = m_pdfDocument->GetPageCount();
    m_txtPageCount->SetValue(wxString::Format(L"%d", pagesCount));
    m_editPageNum->SetMax(pagesCount);
    m_editPageNum->SetMin(pagesCount ? 1 : 0);
}

void MainWindow::OnRenderPage(wxCommandEvent& event) {
    if (m_pdfDocument) {
        int pageNum = m_editPageNum->GetValue();
        if (pageNum--) {
            Drawing::Size bmpSize = m_pdfDocument->GetPageSize(pageNum);
            if (HBITMAPUPtr bmp = m_pdfDocument->RenderPage(pageNum, Drawing::Size(bmpSize.Width*1.2, bmpSize.Height), false)) {
                {
                    std::ofstream ofs("F:\\Samples\\Pdf\\out\\out1.bmp", std::ios::out|std::ios::trunc|std::ios::binary);
                    SaveHBITMAPToStdStream(bmp, ImageFormat::Bmp, ofs);
                }
                m_cachedBitmap = CreateWxBitmapFromHBitmap(std::move(bmp));
                //m_cachedBitmap->SaveFile(L"F:\\Samples\\Pdf\\out\\out.bmp", wxBITMAP_TYPE_BMP);
                m_viewWnd->Refresh();
            }
        }
    }
}

wxBitmap* MainWindow::GetCachedBitmap() {
    if (m_cachedBitmap) {
        return m_cachedBitmap.get();
    }
    return nullptr;
}