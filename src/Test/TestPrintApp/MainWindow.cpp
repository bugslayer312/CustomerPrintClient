#include "MainWindow.h"

#include "../../Core/Log.h"
#include "../../Printing/PrintSettings.h"
// #include "../Printing/ImageRenderer.h"
#include "../../Printing/ImageBundleRenderer.h"
#include "../../Printing/PdfDocumentRenderer.h"
#include "../../Printing/Pdf/PdfError.h"
#include "../../Printing/Pdf/PdfLibrary.h"
#include "../../Printing/Primitives.h"
//#include "../Printing/GdiplusUtilities.h"
#include "wxUtilities.h"

#include <wx/toolbar.h>
#include <wx/combobox.h>
#include <wx/dcclient.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/dirctrl.h>
#include <wx/print.h>
#include <wx/log.h>
#include <wx/dcmemory.h>
#include <wx/filedlg.h>
#include <wx/radiobox.h>
#include <wx/checkbox.h>
#include <wx/textdlg.h>
#include <wx/numdlg.h>

#define NOMINMAX
#include <windows.h>
#include <winspool.h>
using std::min;
using std::max;
#include <gdiplus.h>

#include <memory>
#include <fstream>
#include <vector>

enum class PrintDpi {
    Dpi96,
    Dpi150,
    Dpi300,
    Dpi600,
    Dpi1200
};

PrintDpi PrintDpiFromInt(int dpi) {
    if (dpi < (96 + 150)/2) return PrintDpi::Dpi96;
    if (dpi < (150 + 300)/2) return PrintDpi::Dpi150;
    if (dpi < (300 + 600)/2) return PrintDpi::Dpi300;
    if (dpi < (600 + 1200)/2) return PrintDpi::Dpi600;
    return PrintDpi::Dpi1200;
}

int PrintDpiToInt(PrintDpi dpi) {
    switch (dpi)
    {
    case PrintDpi::Dpi150:
        return 150;
    case PrintDpi::Dpi300:
        return 300;
    case PrintDpi::Dpi600:
        return 600;
    case PrintDpi::Dpi1200:
        return 1200;
    default:
        return 96;
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

PrintSettings g_printSettings;
IPreviewRenderer::PrintProfile g_printProfile{600, 600, 210, 297, true};

class TestWnd : public wxWindow {
public:
    TestWnd(wxWindow *parent, wxBitmap& bitmap,
            wxWindowID winId = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxFULL_REPAINT_ON_RESIZE)
        : wxWindow(parent, winId, pos, size, style)
        , m_bitmap(bitmap)
    {
    }
private:
    wxDECLARE_EVENT_TABLE();

    void OnPaint(wxPaintEvent& evt) {
        wxPaintDC dc(this);
        wxRect r = GetClientRect();
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(r);
        //wxRect logRect(wxPoint(dc.DeviceToLogicalX(r.GetLeft()), dc.DeviceToLogicalY(r.GetTop())),
        //    wxPoint(dc.DeviceToLogicalX(r.GetRight()), dc.DeviceToLogicalY(r.GetBottom())));
        wxRect logRect = r;
        wxPen borderPen(*wxBLACK, 1, wxPENSTYLE_SOLID);
        logRect.Deflate(5, 5);
        dc.SetPen(borderPen);
        dc.DrawRectangle(logRect);
        if (m_bitmap.IsOk()) {
            logRect.Deflate(1, 1);
            dc.SetClippingRegion(logRect);
            dc.DrawBitmap(m_bitmap, logRect.GetTopLeft());
            dc.DestroyClippingRegion();
        }
    }

private:
    wxBitmap& m_bitmap;
};

BEGIN_EVENT_TABLE(TestWnd, wxWindow)
    EVT_PAINT(TestWnd::OnPaint)
END_EVENT_TABLE()

class BitmapPrintout : public wxPrintout {
public:
    BitmapPrintout(wxBitmap const& bitmap, wxString const& tittle = L"Remote Print Printout")
        : wxPrintout(tittle)
        , m_bitmap(bitmap)
    {
    }

    virtual void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo) override {
        *minPage = *maxPage = *selPageFrom = *selPageTo = 1;
    }

    virtual bool HasPage(int pageNum) override {
        return pageNum == 1 && m_bitmap.IsOk();
    }

    virtual bool OnPrintPage(int page) override {
        if (wxDC* dc = GetDC()) {
            if (m_bitmap.IsOk()) {
                /*int ppiScreenX(96), ppiScreenY(96);
                GetPPIScreen(&ppiScreenX, &ppiScreenY);
                int ppiPrinterX(300), ppiPrinterY(300);
                GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
                float const scaleX = static_cast<float>(ppiPrinterX)/ppiScreenX;
                float const scaleY = static_cast<float>(ppiPrinterY)/ppiScreenY;
                dc->SetUserScale(scaleX, scaleY);
                dc->DrawBitmap(m_bitmap, 0, 0);
                */
                wxSize bmpSize = m_bitmap.GetSize();
                // MapScreenSizeToPage();
                // MapScreenSizeToDevice();
                FitThisSizeToPage(bmpSize);
                wxRect fitLogRect = GetLogicalPaperRect();
                Log("LogicalPaperRect x:%d y:%d w:%d h:%d\n", fitLogRect.x, fitLogRect.y, fitLogRect.width, fitLogRect.height);
                dc->DrawBitmap(m_bitmap, 0, 0);
                return true;
            }
        }
        return false;
    }

    virtual bool OnBeginDocument(int startPage, int endPage) override {
        if (!wxPrintout::OnBeginDocument(startPage, endPage)) {
            return false;
        }
        return true;
    }

private:
    wxBitmap const& m_bitmap;
};

namespace Ids {
    enum Id {
        ID_PRINTDLG = wxID_HIGHEST + 1,
        ID_PRINTDLG_EX,
        ID_PAGE_SETUP_DLG,
        ID_CMB_PRINTERS,
        ID_DIR_CTRL,
        ID_PRINT_BMP,
        ID_PRINT_TO_FILE,
        ID_PRINT_TO_STREAM,
        ID_TEST_BMP_CONV,
        ID_CMB_PRINT_MODE,
        ID_CMB_ORIENTATION,
        ID_RADIO_HOR_ALIGN,
        ID_RADIO_VERT_ALIGN,
        ID_CMB_DPI,
        ID_CB_GRAYSCALE
    };
}

wxArrayString GetPrinters() {
    wxArrayString result;
    DWORD dwNeeded(0), dwReturned(0);
    if (EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &dwNeeded, &dwReturned)) {
        Log("Failed EnumPrinters(1)\n");
        return result;
    }
    std::unique_ptr<BYTE[]> pi2_buff(new BYTE[dwNeeded]);
    if (!EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 2, pi2_buff.get(), dwNeeded, &dwNeeded, &dwReturned)) {
        Log("Failed EnumPrinters(2)\n");
        return result;
    }
    result.Alloc(dwReturned);
    PRINTER_INFO_2* pi = (PRINTER_INFO_2*)pi2_buff.get();
    for (DWORD i(0); i < dwReturned; ++i) {
        result.Add(pi[i].pPrinterName);
    }
    return result;
}

MainWindow::MainWindow()
    : wxFrame(nullptr, wxID_ANY, L"TestPrint")
    , m_selectedPage(0)
    , m_bitmap(wxNullBitmap)
    , m_wndPreview(nullptr)
    , m_cmbPrinters(nullptr)
{
    auto unsupportedHandle = [](int type) {
        wxString feature(Pdf::UnsupportedFeautureString(type));
        if (!feature.empty()) {
            wxLogError("PDF unsupported feature: %s", feature);
        }
    };
    m_pdfLib.reset(new Pdf::Library(std::move(unsupportedHandle), std::make_unique<PwdProvider>(this, "")));

    wxImage::AddHandler(new wxPNGHandler());
    wxImage::AddHandler(new wxJPEGHandler());
    SetInitialSize(FromDIP(wxSize(800, 600)));
    wxToolBar* toolBar = CreateToolBar(wxTB_NOICONS|wxTB_TEXT);
    m_cmbPrinters = new wxComboBox(toolBar, Ids::ID_CMB_PRINTERS, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(200,-1)),
        GetPrinters(), wxCB_READONLY);
    m_cmbPrinters->SetSelection(0);

    toolBar->AddControl(m_cmbPrinters, L"Printers");
    toolBar->AddTool(Ids::ID_PRINTDLG, L"PrintDlg", wxNullBitmap);
    toolBar->AddTool(Ids::ID_PRINTDLG_EX, L"PrintDlgEx", wxNullBitmap);
    toolBar->AddTool(Ids::ID_PAGE_SETUP_DLG, L"PageSetupDlg", wxNullBitmap);
    toolBar->AddTool(Ids::ID_PRINT_BMP, L"Print BMP", wxNullBitmap);
    toolBar->AddTool(Ids::ID_PRINT_TO_FILE, L"Print to file", wxNullBitmap);
    toolBar->AddTool(Ids::ID_PRINT_TO_STREAM, L"Print to stream", wxNullBitmap);
    toolBar->AddTool(Ids::ID_TEST_BMP_CONV, L"Bmp conv", wxNullBitmap);
    toolBar->Realize();
    wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    wxPanel* panLeft = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(300, wxDefaultCoord), wxBORDER_SIMPLE);
    sizer->Add(panLeft, wxSizerFlags(0).Left().Expand());
    m_wndPreview = new TestWnd(this, m_bitmap);
    sizer->Add(m_wndPreview, wxSizerFlags(1).Expand());
    wxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    wxGenericDirCtrl* dirCtrl = new wxGenericDirCtrl(panLeft, Ids::ID_DIR_CTRL, "D:\\TestPreview", wxDefaultPosition,
        wxSize(wxDefaultCoord, 300), wxDIRCTRL_SHOW_FILTERS,
        "Images (*.bmp;*.png;*.jpg;*.jpeg)|*.bmp;*.png;*.jpg;*.jpeg|PDF document (*.pdf)|*.pdf");
    leftSizer->Add(dirCtrl, wxSizerFlags(0).Top().Expand());
    
    wxSizer* mod_or_sizer = new wxBoxSizer(wxHORIZONTAL);
    wchar_t const* modes[] = {L"Original size", L"Screen size", L"Stretch to page"};
    wxComboBox* cmbMode = new wxComboBox(panLeft, Ids::ID_CMB_PRINT_MODE,
        modes[static_cast<int>(g_printSettings.Mode())], wxDefaultPosition,
        wxDefaultSize, wxArrayString(3, modes), wxCB_READONLY);
    mod_or_sizer->Add(cmbMode, wxSizerFlags(0).DoubleBorder(wxRIGHT|wxLEFT));
    
    wchar_t const* orientations[] = {L"Portrait", L"Landscape"};
    wxComboBox* cmbOrientation = new wxComboBox(panLeft, Ids::ID_CMB_ORIENTATION,
        orientations[static_cast<int>(g_printSettings.Orientation())], wxDefaultPosition,
        wxDefaultSize, wxArrayString(2, orientations), wxCB_READONLY);
    mod_or_sizer->Add(cmbOrientation, wxSizerFlags(0).DoubleBorder(wxLEFT));
    leftSizer->Add(mod_or_sizer, wxSizerFlags(0).Border(wxTOP));
    
    wchar_t const* horAlignValues[] = {L"Left", L"Center", L"Right"};
    wxRadioBox* rHorAlign = new wxRadioBox(panLeft, Ids::ID_RADIO_HOR_ALIGN, wxT("Horizontal align"),
        wxDefaultPosition, wxDefaultSize, wxArrayString(3, horAlignValues));
    rHorAlign->Select(static_cast<int>(g_printSettings.HorAlign()));
    leftSizer->Add(rHorAlign, wxSizerFlags(0).Border(wxTOP));
    
    wchar_t const* vertAlignValues[] = {L"Top", L"Center", L"Bottom"};
    wxRadioBox* rVertAlign = new wxRadioBox(panLeft, Ids::ID_RADIO_VERT_ALIGN, wxT("Vertical align"),
        wxDefaultPosition, wxDefaultSize, wxArrayString(3, vertAlignValues));
    rVertAlign->Select(static_cast<int>(g_printSettings.VertAlign()));
    leftSizer->Add(rVertAlign, wxSizerFlags(0).Border(wxTOP));

    wxBoxSizer* dpi_gs_sizer = new wxBoxSizer(wxHORIZONTAL);
    wchar_t const* dpis[] = {L"96", L"150", L"300", L"600", L"1200"};
    wxComboBox* cmbDpi = new wxComboBox(panLeft, Ids::ID_CMB_DPI,
        dpis[static_cast<int>(PrintDpiFromInt(g_printProfile.DpiX))], wxDefaultPosition,
        wxDefaultSize, wxArrayString(5, dpis), wxCB_READONLY);
    dpi_gs_sizer->Add(cmbDpi, wxSizerFlags(0).DoubleBorder(wxRIGHT|wxLEFT));

    wxCheckBox* cbGrayScale = new wxCheckBox(panLeft, Ids::ID_CB_GRAYSCALE, L"GrayScale");
    cbGrayScale->SetValue(!g_printProfile.IsColor);
    dpi_gs_sizer->Add(cbGrayScale, wxSizerFlags(0).DoubleBorder(wxLEFT).CenterVertical());
    leftSizer->Add(dpi_gs_sizer, wxSizerFlags(0).Border(wxTOP));
    
    panLeft->SetSizer(leftSizer);
    SetSizer(sizer);
    Layout();
    Bind(wxEVT_TOOL, &MainWindow::OnPrintDlg, this, Ids::ID_PRINTDLG);
    Bind(wxEVT_TOOL, &MainWindow::OnPrintDlgEx, this, Ids::ID_PRINTDLG_EX);
    Bind(wxEVT_TOOL, &MainWindow::OnPageSetupDlg, this, Ids::ID_PAGE_SETUP_DLG);
    Bind(wxEVT_DIRCTRL_FILEACTIVATED, &MainWindow::OnFileActivated, this, Ids::ID_DIR_CTRL);
    Bind(wxEVT_TOOL, &MainWindow::OnPrintBmp, this, Ids::ID_PRINT_BMP);
    Bind(wxEVT_TOOL, &MainWindow::OnPrintToFile, this, Ids::ID_PRINT_TO_FILE);
    Bind(wxEVT_TOOL, &MainWindow::OnPrintToStream, this, Ids::ID_PRINT_TO_STREAM);
    Bind(wxEVT_TOOL, &MainWindow::OnTestBmpConvert, this, Ids::ID_TEST_BMP_CONV);
    Bind(wxEVT_COMBOBOX, &MainWindow::OnPrintModeSelected, this, Ids::ID_CMB_PRINT_MODE);
    Bind(wxEVT_COMBOBOX, &MainWindow::OnOrientationSelected, this, Ids::ID_CMB_ORIENTATION);
    Bind(wxEVT_RADIOBOX, &MainWindow::OnHorAlignSelected, this, Ids::ID_RADIO_HOR_ALIGN);
    Bind(wxEVT_RADIOBOX, &MainWindow::OnVertAlignSelected, this, Ids::ID_RADIO_VERT_ALIGN);
    Bind(wxEVT_COMBOBOX, &MainWindow::OnDpiSelected, this, Ids::ID_CMB_DPI);
    Bind(wxEVT_CHECKBOX, &MainWindow::OnGrayscaleClick, this, Ids::ID_CB_GRAYSCALE);
}

void MainWindow::OnPrintDlg(wxCommandEvent& ev) { 
    PRINTDLG pd;
    ZeroMemory(&pd, sizeof(pd));
    pd.lStructSize = sizeof(pd);
    pd.Flags = PD_RETURNDC;
    pd.hwndOwner = this->GetHandle();
    if (PrintDlg(&pd)) {
        Log("PrintDlg ret=1 \n");
        
    }
    if (pd.hDevMode) GlobalFree(pd.hDevMode);
    if (pd.hDevNames) GlobalFree(pd.hDevNames);
    if (pd.hDC) DeleteDC(pd.hDC);
}

void MainWindow::OnPrintDlgEx(wxCommandEvent& ev) {
    HRESULT hResult;
    PRINTDLGEX pdx = {0};
    LPPRINTPAGERANGE pPageRanges = NULL;

    // Allocate an array of PRINTPAGERANGE structures.
    pPageRanges = (LPPRINTPAGERANGE) GlobalAlloc(GPTR, 10 * sizeof(PRINTPAGERANGE));
    if (!pPageRanges)
        return;

    //  Initialize the PRINTDLGEX structure.
    pdx.lStructSize = sizeof(PRINTDLGEX);
    pdx.hwndOwner = GetHandle();
    pdx.hDevMode = NULL;
    pdx.hDevNames = NULL;
    pdx.hDC = NULL;
    pdx.Flags = PD_RETURNDC | PD_COLLATE;
    pdx.Flags2 = 0;
    pdx.ExclusionFlags = 0;
    pdx.nPageRanges = 0;
    pdx.nMaxPageRanges = 10;
    pdx.lpPageRanges = pPageRanges;
    pdx.nMinPage = 1;
    pdx.nMaxPage = 1000;
    pdx.nCopies = 1;
    pdx.hInstance = 0;
    pdx.lpPrintTemplateName = NULL;
    pdx.lpCallback = NULL;
    pdx.nPropertyPages = 0;
    pdx.lphPropertyPages = NULL;
    pdx.nStartPage = START_PAGE_GENERAL;
    pdx.dwResultAction = 0;
    hResult = PrintDlgEx(&pdx);
    if ((hResult == S_OK) && pdx.dwResultAction == PD_RESULT_PRINT) 
    {
        Log("User clicked the Print button, so use the DC and other information returned in the PRINTDLGEX structure to print the document.\n");
    }
    if (pdx.hDevMode) GlobalFree(pdx.hDevMode);
    if (pdx.hDevNames) GlobalFree(pdx.hDevNames);
    if (pdx.lpPageRanges) GlobalFree(pdx.lpPageRanges);
    if (pdx.hDC) DeleteDC(pdx.hDC);
}

void MainWindow::OnPageSetupDlg(wxCommandEvent& ev) {
    PAGESETUPDLG psd;    // common dialog box structure

    // Initialize PAGESETUPDLG
    ZeroMemory(&psd, sizeof(psd));
    psd.lStructSize = sizeof(psd);
    psd.hwndOwner   = GetHandle();
    psd.hDevMode    = NULL; // Don't forget to free or store hDevMode.
    psd.hDevNames   = NULL; // Don't forget to free or store hDevNames.
    psd.Flags       = PSD_INTHOUSANDTHSOFINCHES | PSD_MARGINS | 
                    PSD_ENABLEPAGEPAINTHOOK; 
    psd.rtMargin.top = 1000;
    psd.rtMargin.left = 1250;
    psd.rtMargin.right = 1250;
    psd.rtMargin.bottom = 1000;
    psd.lpfnPagePaintHook = NULL;

    if (PageSetupDlg(&psd)==TRUE)
    {
        Log("check paper size and margin values here.\n");
    }
}

bool MainWindow::LoadPdf() {
    PdfDocumentRenderer renderer(*m_pdfLib, m_selectedFile.wc_str(), g_printProfile);
    wxNumberEntryDialog dlg(this, L"Enter page num", wxEmptyString, renderer.GetJobName(),
        0, 0, renderer.GetPageCount());
    if (dlg.ShowModal() == wxID_OK) {
        wxRect viewRect = m_wndPreview->GetClientRect();
        Drawing::Point pt;
        if (HBITMAPUPtr hBmp = renderer.RenderForPreview(dlg.GetValue(), g_printSettings,
            Drawing::Rect(viewRect.x, viewRect.y, viewRect.width, viewRect.height), pt)) {
            wxBitmapUPtr bmp = CreateWxBitmapFromHBitmap(std::move(hBmp));
            m_bitmap = *bmp;
            return true;
        }
    }
    return false;
}

void MainWindow::OnFileActivated(wxTreeEvent& ev) {
    wxGenericDirCtrl* dirCtrl = dynamic_cast<wxGenericDirCtrl*>(ev.GetEventObject());
    wxString newSelection = dirCtrl->GetPath(ev.GetItem());
    if (m_selectedFile != newSelection) {
        m_selectedFile = newSelection;
        bool loaded = false;
        if (m_selectedFile.EndsWith(L".pdf")) {
            loaded = LoadPdf();
        }
        else {
            ImageFormat imgFmt = ImageFormatFromWPath(m_selectedFile.wc_str());
            wxBitmapType bmpType = wxBITMAP_TYPE_BMP;
            switch (imgFmt)
            {
            case ImageFormat::Png:
                bmpType = wxBITMAP_TYPE_PNG;
                break;
            case ImageFormat::Jpeg:
                bmpType = wxBITMAP_TYPE_JPEG;
                break;
            }
            loaded = m_bitmap.LoadFile(m_selectedFile, bmpType);
            Log("Bmp is loaded:%d isOk:%d\n", loaded, m_bitmap.IsOk());
        }
        if (loaded) {
            m_selectedFile = newSelection;
            if (m_wndPreview) {
                m_wndPreview->Refresh();
            }
        }
    }
    ev.Skip();
}

void MainWindow::OnPrintBmp(wxCommandEvent& ev) {
    if (!m_bitmap.IsOk()) {
        Log("Invalid BMP\n");
        return;
    }
    Log("Printing file:%s w:%d h:%d\n", m_selectedFile.ToUTF8().data(), m_bitmap.GetWidth(), m_bitmap.GetHeight());
    static wxPrintData printData;
    printData.SetPrinterName(m_cmbPrinters->GetStringSelection());
    //printData.SetPaperId(wxPAPER_A4);
    wxPrintDialogData printDialogData(printData);
    wxPrinter printer(&printDialogData);
    BitmapPrintout printout(m_bitmap);
    if (printer.Print(this, &printout, true)) {
        wxLogMessage("Printed successfully");
    } else if (wxPrinter::GetLastError() == wxPRINTER_ERROR) {
        wxLogError("There was a proble printing");
    } else {
        wxLogMessage("Printing was canceled");
    }
}

void MainWindow::OnPrintToStream(wxCommandEvent& ev) {
    // wxString srcImg(L"F:\\Samples\\Tractor.bmp");
    if (!m_selectedFile.IsEmpty()) {
        Log("Printing file:%s\n", m_selectedFile.ToUTF8().data());
        wxString destPath;
        ImageFormat outFormat;
        if (PickSavePath(destPath, outFormat)) {
            std::ofstream ofs(destPath.ToUTF8(), std::ios::trunc|std::ios::binary|std::ios::out);
            if (ofs) {
                PrintSettings printSettings;
                //printSettings.HorAlign(HorizontalPageAlign::Left)
                //    .VertAlign(VerticalPageAlign::Top)
                //    .Mode(PrintMode::StretchToPage)
                //    .Orientation(PrintOrientation::Landscape);
                std::size_t outStreamSize(0);
                // ImageRenderer renderer({600, 600, 210, 297, false});
                //renderer.RenderForPrinting(m_selectedFile.ToStdWstring(), g_printSettings,
                //    ImageFormatFromWPath(m_selectedFile.ToStdWstring()), ofs, outStreamSize);
                if (m_selectedFile.EndsWith(L".pdf")) {
                    PdfDocumentRenderer renderer(*m_pdfLib, m_selectedFile.wc_str(), g_printProfile);
                    renderer.RenderForPrinting(m_selectedPage, g_printSettings, outFormat, ofs, outStreamSize);
                }
                else {
                    std::vector<std::wstring> vfiles;
                    vfiles.push_back(m_selectedFile.ToStdWstring());
                    ImageBundleRenderer renderer(std::move(vfiles), g_printProfile);
                    renderer.RenderForPrinting(0, g_printSettings, outFormat, ofs, outStreamSize);
                }
                int a = 1;
            }
        }
    }
}

void MainWindow::OnPrintToFile(wxCommandEvent& ev) {
    if (!m_selectedFile.IsEmpty()) {
        Log("Printing file:%s\n", m_selectedFile.ToUTF8().data());
        wxString destPath;
        ImageFormat outFormat;
        if (PickSavePath(destPath, outFormat)) {
            PrintSettings printSettings;
            // printSettings.HorAlign(HorizontalPageAlign::Left).VertAlign(VerticalPageAlign::Top).Mode(PrintMode::StretchToPage);
            // ImageRenderer renderer({600, 600, 210, 297}, printSettings, outFormat);
            //renderer.RenderForPrinting(m_selectedFile.ToStdWstring(), std::string(destPath.ToUTF8()));
        }
    }
}

void MainWindow::OnTestBmpConvert(wxCommandEvent& ev) {
    //GdiplusInit gpi;
    //CLSID clsid;
    //GdiplusUtils::GetEncoderClsid(ImageFormat::Png, &clsid);
    /* for (int i(0);i < 2000; ++i) {
        std::unique_ptr<wxBitmap> wxBmp(new wxBitmap(m_selectedFile, wxBITMAP_TYPE_BMP));
        HBITMAPUPtr hBitmap = CreateHBitmapFromWxBitmap(std::move(wxBmp));
        std::unique_ptr<Gdiplus::Bitmap> gpBmp(Gdiplus::Bitmap::FromHBITMAP(static_cast<HBITMAP>(hBitmap.get()), NULL));
        gpBmp->Save(L"D:\\TestPreview\\out\\conv.png", &clsid);
    } */
    for (int i(0); i < 100; ++i) {
        HBITMAPUPtr hBmp;
        {
            std::unique_ptr<Gdiplus::Bitmap> gpBmp(Gdiplus::Bitmap::FromFile(m_selectedFile.c_str()));
            HBITMAP hbitmap;
            Gdiplus::Status res = gpBmp->GetHBITMAP(Gdiplus::Color::White, &hbitmap);
            if (res == Gdiplus::Ok) {
                hBmp.reset(hbitmap);
            }
        }
        wxBitmapUPtr wxBmp = CreateWxBitmapFromHBitmap(std::move(hBmp));
        bool ok = wxBmp->IsOk();
        bool saved = wxBmp->SaveFile(L"D:\\TestPreview\\out\\conv1.png", wxBITMAP_TYPE_PNG);
        int a = 1;
    }
}

void MainWindow::OnPrintModeSelected(wxCommandEvent& ev) {
    g_printSettings.Mode(static_cast<PrintMode>(ev.GetInt()));
}

void MainWindow::OnOrientationSelected(wxCommandEvent& ev) {
    g_printSettings.Orientation(static_cast<PrintOrientation>(ev.GetInt()));
}

void MainWindow::OnHorAlignSelected(wxCommandEvent& ev) {
    g_printSettings.HorAlign(static_cast<HorizontalPageAlign>(ev.GetInt()));
}

void MainWindow::OnVertAlignSelected(wxCommandEvent& ev) {
    g_printSettings.VertAlign(static_cast<VerticalPageAlign>(ev.GetInt()));
}

void MainWindow::OnDpiSelected(wxCommandEvent& ev) {
    g_printProfile.DpiX = g_printProfile.DpiY = PrintDpiToInt(static_cast<PrintDpi>(ev.GetInt()));
}

void MainWindow::OnGrayscaleClick(wxCommandEvent& ev) {
    g_printProfile.IsColor = ev.GetInt() == 0;
}

bool MainWindow::PickSavePath(wxString& destPath, ImageFormat& format) {
    wxFileDialog dialog(this, wxFileSelectorPromptStr, L"D:\\TestPreview\\out", L"printout",
        L"BMP image file (*.bmp)|*.bmp|PNG image file (*.png)|*.png|JPEG image file (*.jpeg)|*.jpeg",
        wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
    dialog.SetFilterIndex(1);
    if (dialog.ShowModal() == wxID_OK) {
        switch (dialog.GetFilterIndex()) {
        case 1:
            format = ImageFormat::Png;
            break;
        case 2:
            format = ImageFormat::Jpeg;
            break;
        default:
            format = ImageFormat::Bmp;
            break;
        }
        destPath = dialog.GetPath();
        return true;
    }
    return false;
}