#pragma once

#include "wxUtilities.h"

#include <wx/frame.h>

#include <memory>

namespace Pdf {

class Library;
class Document;

} // namespace Pdf

class wxFileDirPickerEvent;
class wxTextCtrl;

class MainWindow : public wxFrame
{
public:
    MainWindow();
    ~MainWindow();
    void OnPdfFilePicked(wxFileDirPickerEvent& event);
    void OnRenderPage(wxCommandEvent& event);
    wxBitmap* GetCachedBitmap();

private:
    std::unique_ptr<Pdf::Library> m_pdfLibrary;
    std::unique_ptr<Pdf::Document> m_pdfDocument;
    wxBitmapUPtr m_cachedBitmap;
    wxWindow* m_viewWnd;
    wxTextCtrl* m_txtPageCount;
    wxSpinCtrl* m_editPageNum;
};