#pragma once

#include "../../Printing/ImageFormat.h"
#include "../../Printing/GdiplusUtilities.h"

#include <wx/frame.h>
#include <wx/bitmap.h>

#include <memory>

class wxCommandEvent;
class wxTreeEvent;
class wxComboBox;

namespace Pdf {

class Library;

}

class MainWindow : public wxFrame {
public:
    MainWindow();

private:
    void OnPrintDlg(wxCommandEvent& ev);
    void OnPrintDlgEx(wxCommandEvent& ev);
    void OnPageSetupDlg(wxCommandEvent& ev);
    void OnFileActivated(wxTreeEvent& ev);
    void OnPrintBmp(wxCommandEvent& ev);
    void OnPrintToFile(wxCommandEvent& ev);
    void OnPrintToStream(wxCommandEvent& ev);
    void OnTestBmpConvert(wxCommandEvent& ev);
    void OnPrintModeSelected(wxCommandEvent& ev);
    void OnOrientationSelected(wxCommandEvent& ev);
    void OnHorAlignSelected(wxCommandEvent& ev);
    void OnVertAlignSelected(wxCommandEvent& ev);
    void OnDpiSelected(wxCommandEvent& ev);
    void OnGrayscaleClick(wxCommandEvent& ev);
    bool PickSavePath(wxString& destPath, ImageFormat& format);
    bool LoadPdf();

private:
    GdiplusInit m_gpi;
    std::unique_ptr<Pdf::Library> m_pdfLib;
    wxBitmap m_bitmap;
    wxString m_selectedFile;
    std::size_t m_selectedPage;
    wxWindow* m_wndPreview;
    wxComboBox* m_cmbPrinters;
};
