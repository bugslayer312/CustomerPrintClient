#include "CtrlPrintPreviewCanvas.h"

#include "../../Core/Log.h"
#include "../../Printing/IPreviewRenderer.h"
#include "../../Printing/Primitives.h"
#include "../../Printing/PrintSettings.h"
#include "../wxUtilities.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include <vector>
#include <map>
#include <cstdlib>

int constexpr PageMarginXPx = 20;
int constexpr PageMarginYPx = 20;
int constexpr InterPageDistPx = 24;
int constexpr ShadowOffset = 4;
std::size_t constexpr InvalidPageNum = static_cast<size_t>(-1);

class DrawPageCache {
    struct PageInfo {
        wxRect Rect;
        bool BitmapIsSet;
        std::unique_ptr<wxBitmap> Bitmap;
        wxPoint BmpOffset;

        PageInfo() : BitmapIsSet(false), BmpOffset(0, 0)
        {
        }
        PageInfo(PageInfo const&) = delete;
        PageInfo& operator=(PageInfo const&) = delete;
        PageInfo(PageInfo&&) = default;
        PageInfo& operator=(PageInfo&&) = default;
        
        void SetBitmap(std::unique_ptr<wxBitmap> bitmap) {
            Bitmap = std::move(bitmap);
            BitmapIsSet = true;
            //if (Bitmap) {
            //    Log("Set bitmap\n");
            //}
        }
        void ResetBitmap() {
            //if (Bitmap) {
            //    Log("Reset bitmap\n");
            //}
            BitmapIsSet = false;
            Bitmap.reset();
        }
    };
public:
    DrawPageCache(PrintProfile* printProfile, IPreviewRenderer const& previewRenderer)
        : m_printProfile(printProfile)
        , m_previewRenderer(previewRenderer)
        , m_pages(m_previewRenderer.GetPageCount())
        , m_prevPageSize(-1, -1)
        , m_visibleFirst(InvalidPageNum)
        , m_visibleLast(InvalidPageNum)
    {
    }

    void SetPrintProfile(PrintProfile* printProfile) {
        m_printProfile = printProfile;
    }

    /*std::vector<std::size_t> GetVisiblePageNums(wxRect const& viewRect) const {
        std::vector<std::size_t> result;
        if (!m_pages.empty()) {
            int pageH = m_pages.front().Rect.height;
            result.reserve(viewRect.height / pageH + 1);
            for (std::size_t i(0); i < m_pages.size(); ++i) {
                wxRect const& pageRect = m_pages[i].Rect;
                wxRect boundRect(pageRect.x - 1, pageRect.y - 1, pageRect.width + 2 + ShadowOffset, pageRect.height + 2 + ShadowOffset);
                if (viewRect.Intersects(boundRect)) {
                    result.push_back(i);
                }
                // TODO: optimize, get rid of wxRect.Intersects()
            }
        }
        return result;
    } */

    void CalcPageRects(wxRect const& viewRect, PrintOrientation orientation, bool force) {
        if (!m_printProfile || m_pages.empty()) {
            return;
        }
        uint32_t settingsW(m_printProfile->Paper.Width), settingsH(m_printProfile->Paper.Height);
        if (orientation == PrintOrientation::Landscape) {
            std::swap(settingsW, settingsH);
        }
        wxRect pageRect(wxPoint(0, 0), viewRect.GetSize());
        pageRect.Deflate(PageMarginXPx, PageMarginYPx);
        int propSide = settingsH * pageRect.width / settingsW; // new Height
        if (propSide > pageRect.height) {
            propSide = settingsW * pageRect.height / settingsH; // new Width
            pageRect.Deflate((pageRect.width - propSide)/2, 0);
        }
        else {
            pageRect.Deflate(0, (pageRect.height - propSide)/2);
        }
        bool const resetBitmaps = true || force || std::abs(pageRect.width - m_prevPageSize.x) > 1 ||
            std::abs(pageRect.height - m_prevPageSize.y) > 1;
        m_pages.front().Rect = pageRect;
        if (resetBitmaps) {
            m_pages.front().ResetBitmap();
        }
        for (int i(1); i < m_pages.size(); ++i) {
            int nextPageTop = m_pages[i-1].Rect.GetBottom() + InterPageDistPx;
            auto& nextPageRect = m_pages[i].Rect = m_pages[i-1].Rect;
            nextPageRect.y = nextPageTop;
            if (resetBitmaps) {
                m_pages[i].ResetBitmap();
            }
        }
        CalcVisiblePageNums(viewRect);
        if (!resetBitmaps) {
            ClearInvisiblePagesCache();
        }
        m_prevPageSize = pageRect.GetSize();
    }

    wxRect const& GetPageRect(std::size_t pageNum) const {
        return m_pages[pageNum].Rect;
    }

    wxCoord GetTotalHeight(wxCoord viewHeight) const {
        return m_pages.empty() ? viewHeight : m_pages.back().Rect.GetBottom() + PageMarginYPx;
    }

    void DrawPageContent(wxDC& dc, PrintSettings const& printSettings, std::size_t pageNum) {
        PageInfo& pageInfo = m_pages[pageNum];
        if (pageInfo.BitmapIsSet || RenderPageContent(pageNum, printSettings, pageInfo)) {
            if (pageInfo.Bitmap) {
                dc.DrawBitmap(*pageInfo.Bitmap, pageInfo.Rect.GetTopLeft() + pageInfo.BmpOffset);
            }
        }
    }

    bool RenderPageContent(std::size_t pageNum, PrintSettings const& printSettings, PageInfo& pageInfo) {
        Drawing::Rect const pageRect(pageInfo.Rect.x, pageInfo.Rect.y, pageInfo.Rect.width, pageInfo.Rect.height);
        Drawing::Point bmpOffset;
        HBITMAPUPtr hBitmap;
        if (HBITMAPUPtr hBitmap = m_previewRenderer.RenderForPreview(pageNum, printSettings, pageRect, bmpOffset)) {
            wxBitmapUPtr wxBmp = CreateWxBitmapFromHBitmap(std::move(hBitmap));
            if (wxBmp && wxBmp->IsOk()) {
                pageInfo.SetBitmap(std::move(wxBmp));
                pageInfo.BmpOffset.x = bmpOffset.X;
                pageInfo.BmpOffset.y = bmpOffset.Y;
                return true;
            }
        }
        pageInfo.SetBitmap(nullptr);
        return false;
    }

    void CalcVisiblePageNums(wxRect const& viewRect) {
        m_visibleFirst = m_visibleLast = InvalidPageNum;
        for (std::size_t i(0), cnt(m_pages.size()); i < cnt; ++i) {
            if (m_pages[i].Rect.GetBottom() + ShadowOffset + 1 >= viewRect.y) {
                m_visibleFirst = i;
                break;
            }
        }
        if (m_visibleFirst == InvalidPageNum) {
            return;
        }
        m_visibleLast = m_visibleFirst;
        for (std::size_t i(m_visibleLast + 1), cnt(m_pages.size()); i < cnt; ++i) {
            if (m_pages[i].Rect.y + 2 <= viewRect.GetBottom()) {
                m_visibleLast = i;
            } else {
                break;
            }
        }
    }

    std::size_t GetFirstVisiblePageNum() const {
        return m_visibleFirst;
    }

    std::size_t GetLastVisiblePageNum() const {
        return m_visibleLast;
    }

    void ClearInvisiblePagesCache() {
        if (m_visibleFirst == InvalidPageNum) {
            return;
        }
        for (std::size_t i(0); i < m_visibleFirst; ++i) {
            m_pages[i].ResetBitmap();
        }
        for (std::size_t i(m_visibleLast + 1), cnt(m_pages.size()); i < cnt; ++i) {
            m_pages[i].ResetBitmap();
        }
    }

private:
    PrintProfile* m_printProfile;
    IPreviewRenderer const& m_previewRenderer;
    std::vector<PageInfo> m_pages;
    wxSize m_prevPageSize;
    std::size_t m_visibleFirst;
    std::size_t m_visibleLast;
};

CtrlPrintPreviewCanvas::CtrlPrintPreviewCanvas(wxWindow *parent, PrintProfilePtr printProfile, 
                                               PrintSettings const& printSettings, IPreviewRenderer const& previewRenderer,
                                               wxWindowID winid, wxPoint const& pos, wxSize const& size, long style)
    : wxScrolledCanvas(parent, winid, pos, size, style|wxFULL_REPAINT_ON_RESIZE)
    , m_printProfile(printProfile)
    , m_printSettings(printSettings)
    , m_drawPageCache(new DrawPageCache(m_printProfile.get(), previewRenderer))
{
    SetScrollRate(10, 10);
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void CtrlPrintPreviewCanvas::SetPrintProfile(PrintProfilePtr printProfile) {
    if (m_printProfile->Id != printProfile->Id) {
        m_printProfile = printProfile;
        m_drawPageCache->SetPrintProfile(m_printProfile.get());
        ApplyPrintSettingsChanges();
    }
}

void CtrlPrintPreviewCanvas::NotifyPrintSettingsChanged() {
    ApplyPrintSettingsChanges();
}

void CtrlPrintPreviewCanvas::OnPaint(wxPaintEvent& evt) {
    //wxPaintDC dc(this);
    wxAutoBufferedPaintDC dc(this);
    PrepareDC(dc);

    //std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));

	wxRect rectUpdate = GetUpdateClientRect();
	rectUpdate.SetPosition(CalcUnscrolledPosition(rectUpdate.GetPosition()));
	dc.SetBackground(GetBackgroundColour());
	dc.Clear();

    m_drawPageCache->CalcVisiblePageNums(wxRect(CalcUnscrolledPosition(wxPoint(0, 0)), GetClientSize()));
    std::size_t pageNum = m_drawPageCache->GetFirstVisiblePageNum();
    if (pageNum != InvalidPageNum) {
        for (std::size_t lastPageNum = m_drawPageCache->GetLastVisiblePageNum(); pageNum <= lastPageNum; ++pageNum) {
            wxRect const& contentRect = m_drawPageCache->GetPageRect(pageNum);
            wxRect pageRect(contentRect.x - 1, contentRect.y - 1, contentRect.width + ShadowOffset + 2,
                contentRect.height + ShadowOffset + 2);
            if (pageRect.Intersects(rectUpdate)) {
                DrawBlankPage(dc, contentRect);
                m_drawPageCache->DrawPageContent(dc, m_printSettings, pageNum);
            }
        }
    }
}

void CtrlPrintPreviewCanvas::RecalcPageRects(bool forceUpdate) {
    wxRect const viewRect(CalcUnscrolledPosition(wxPoint(0, 0)), GetClientSize());
    m_drawPageCache->CalcPageRects(viewRect, m_printSettings.Orientation(), forceUpdate);
    SetVirtualSize(-1, m_drawPageCache->GetTotalHeight(viewRect.y));
}

void CtrlPrintPreviewCanvas::OnSize(wxSizeEvent& evt) {
    RecalcPageRects(false);
    evt.Skip();
}

void CtrlPrintPreviewCanvas::OnScroll(wxScrollWinEvent& event) {
    m_drawPageCache->CalcVisiblePageNums(wxRect(CalcUnscrolledPosition(wxPoint(0, 0)), GetClientSize()));
    m_drawPageCache->ClearInvisiblePagesCache();
    event.Skip();
}

void CtrlPrintPreviewCanvas::ApplyPrintSettingsChanges() {
    RecalcPageRects(true);
    Refresh();
}

void CtrlPrintPreviewCanvas::DrawBlankPage(wxDC& dc, wxRect const& pageRect) {
    dc.SetPen(*wxMEDIUM_GREY_PEN);
    dc.SetBrush(*wxMEDIUM_GREY_BRUSH);
    dc.DrawRectangle(pageRect.x + ShadowOffset, pageRect.GetBottom() + 2, pageRect.width + 1, ShadowOffset);
    dc.DrawRectangle(pageRect.GetRight() + 2, pageRect.y + ShadowOffset, ShadowOffset, pageRect.height);
    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawRectangle(pageRect.x - 1, pageRect.y - 1, pageRect.width + 2, pageRect.height + 2);
}

BEGIN_EVENT_TABLE(CtrlPrintPreviewCanvas, wxScrolledWindow)
    EVT_PAINT(CtrlPrintPreviewCanvas::OnPaint)
    EVT_SIZE(CtrlPrintPreviewCanvas::OnSize)
    EVT_SCROLLWIN(CtrlPrintPreviewCanvas::OnScroll)
END_EVENT_TABLE()