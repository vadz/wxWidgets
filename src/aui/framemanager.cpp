///////////////////////////////////////////////////////////////////////////////
// Name:        src/aui/framemanager.cpp
// Purpose:     wxaui: wx advanced user interface - docking window manager
// Author:      Benjamin I. Williams
// Modified by: Malcolm MacLeod (mmacleod@webmail.co.za)
// Created:     2005-05-17
// RCS-ID:      $Id: framemanager.cpp 61740 2009-08-23 09:21:31Z MJM $
// Copyright:   (C) Copyright 2005-2006, Kirix Corporation, All Rights Reserved
// Licence:     wxWindows Library Licence, Version 3.1
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_AUI

#include "wx/aui/framemanager.h"
#include "wx/aui/dockart.h"
#include "wx/aui/floatpane.h"
#include "wx/aui/tabmdi.h"
#include "wx/aui/tabart.h"
#include "wx/aui/auibar.h"
#include "wx/aui/auibook.h"
#include "wx/mdi.h"

#ifndef WX_PRECOMP
    #include "wx/panel.h"
    #include "wx/settings.h"
    #include "wx/app.h"
    #include "wx/dcclient.h"
    #include "wx/dcscreen.h"
    #include "wx/toolbar.h"
    #include "wx/image.h"
    #include "wx/statusbr.h"
#endif

WX_CHECK_BUILD_OPTIONS("wxAUI")

#include "wx/arrimpl.cpp"
WX_DECLARE_OBJARRAY(wxRect, wxAuiRectArray);
WX_DEFINE_OBJARRAY(wxAuiRectArray)
WX_DEFINE_OBJARRAY(wxAuiDockUIPartArray)
WX_DEFINE_OBJARRAY(wxAuiDockInfoArray)
WX_DEFINE_OBJARRAY(wxAuiPaneButtonArray)
WX_DEFINE_OBJARRAY(wxAuiPaneInfoArray)

WX_DEFINE_OBJARRAY(wxAuiTabContainerPointerArray)
WX_DEFINE_OBJARRAY(wxAuiSizerItemPointerArray)

wxAuiPaneInfo wxAuiNullPaneInfo;
wxAuiDockInfo wxAuiNullDockInfo;
wxDEFINE_EVENT( wxEVT_AUI_PANE_BUTTON, wxAuiManagerEvent );
wxDEFINE_EVENT( wxEVT_AUI_PANE_CLOSE, wxAuiManagerEvent );
wxDEFINE_EVENT( wxEVT_AUI_PANE_MAXIMIZE, wxAuiManagerEvent );
wxDEFINE_EVENT( wxEVT_AUI_PANE_RESTORE, wxAuiManagerEvent );
wxDEFINE_EVENT( wxEVT_AUI_PANE_ACTIVATED, wxAuiManagerEvent );
wxDEFINE_EVENT( wxEVT_AUI_RENDER, wxAuiManagerEvent );
wxDEFINE_EVENT( wxEVT_AUI_FIND_MANAGER, wxAuiManagerEvent );
wxDEFINE_EVENT( wxEVT_AUI_ALLOW_DND, wxAuiManagerEvent );

#ifdef __WXMAC__
    // a few defines to avoid nameclashes
    #define __MAC_OS_X_MEMORY_MANAGER_CLEAN__ 1
    #define __AIFF__
    #include "wx/osx/private.h"
#endif

#ifdef __WXMSW__
    #include "wx/msw/wrapwin.h"
    #include "wx/msw/private.h"
    #include "wx/msw/dc.h"
#endif

IMPLEMENT_DYNAMIC_CLASS(wxAuiManagerEvent, wxEvent)
IMPLEMENT_CLASS(wxAuiManager, wxEvtHandler)

// the utility function ShowWnd() is the same as show,
// except it handles wxAuiMDIChildFrame windows as well,
// as the Show() method on this class is "unplugged"
static void ShowWnd(wxWindow* wnd, bool show)
{
#if wxUSE_MDI
    if (wxDynamicCast(wnd,wxAuiMDIChildFrame))
    {
        wxAuiMDIChildFrame* cf = (wxAuiMDIChildFrame*)wnd;
        cf->DoShow(show);
        cf->ApplyMDIChildFrameRect();
    }
    else
#endif
    {
        wnd->Show(show);
    }
}

// Determine if a close button should appear on the tab for the passed pane
bool TabHasCloseButton(unsigned int flags,wxAuiPaneInfo& page)
{
    if ((flags & wxAUI_MGR_NB_CLOSE_ON_ALL_TABS) != 0 || ((flags & wxAUI_MGR_NB_CLOSE_ON_ACTIVE_TAB) != 0 && page.HasFlag(wxAuiPaneInfo::optionActiveNotebook)))
        return true;
    return false;
}

const int auiToolBarLayer = 10;
//fixme: (MJM) This should be set by the art somewhere not hardcoded, temporary hardcoding while we iron out some issues with the tab art providers.
const int notebookTabHeight = 42;


wxAuiPaneInfo::wxAuiPaneInfo()
:
#if WXWIN_COMPATIBILITY_2_8
  name(m_name)
, caption(m_caption)
, window(m_window)
, frame(m_frame)
, state(m_state)
, dock_direction(m_dock_direction)
, dock_layer(m_dock_layer)
, dock_row(m_dock_row)
, dock_pos(m_dock_pos)
, best_size(m_best_size)
, min_size(m_min_size)
, max_size(m_max_size)
, floating_pos(m_floating_pos)
, floating_size(m_floating_size)
, dock_proportion(m_dock_proportion)
, buttons(m_buttons)
, rect(m_rect)
#else // !WXWIN_COMPATIBILITY_2_8
#endif // WXWIN_COMPATIBILITY_2_8/!WXWIN_COMPATIBILITY_2_8
, m_name(wxT(""))
, m_caption(wxT(""))
, m_tooltip(wxT(""))
, m_window(NULL)
, m_frame(NULL)
, m_state(0)
, m_dock_direction(wxAUI_DOCK_LEFT)
, m_dock_layer(0)
, m_dock_row(0)
, m_dock_pos(0)
, m_dock_page(0)
, m_best_size(wxDefaultSize)
, m_min_size(wxDefaultSize)
, m_max_size(wxDefaultSize)
, m_floating_pos(wxDefaultPosition)
, m_floating_size(wxDefaultSize)
, m_dock_proportion(0)
{
    SetDefaultPane();
}

#ifndef SWIG
wxAuiPaneInfo::wxAuiPaneInfo(const wxAuiPaneInfo& c)
#if WXWIN_COMPATIBILITY_2_8
: name(m_name)
, caption(m_caption)
, window(m_window)
, frame(m_frame)
, state(m_state)
, dock_direction(m_dock_direction)
, dock_layer(m_dock_layer)
, dock_row(m_dock_row)
, dock_pos(m_dock_pos)
, best_size(m_best_size)
, min_size(m_min_size)
, max_size(m_max_size)
, floating_pos(m_floating_pos)
, floating_size(m_floating_size)
, dock_proportion(m_dock_proportion)
, buttons(m_buttons)
, rect(m_rect)
#else // !WXWIN_COMPATIBILITY_2_8
#endif // WXWIN_COMPATIBILITY_2_8/!WXWIN_COMPATIBILITY_2_8
{
    m_name = c.m_name;
    m_caption = c.m_caption;
    m_tooltip = c.m_tooltip;
    m_window = c.m_window;
    m_frame = c.m_frame;
    m_state = c.m_state;
    m_dock_direction = c.m_dock_direction;
    m_dock_layer = c.m_dock_layer;
    m_dock_row = c.m_dock_row;
    m_dock_pos = c.m_dock_pos;
    m_dock_page = c.m_dock_page;
    m_dock_bitmap = c.m_dock_bitmap;
    m_best_size = c.m_best_size;
    m_min_size = c.m_min_size;
    m_max_size = c.m_max_size;
    m_floating_pos = c.m_floating_pos;
    m_floating_size = c.m_floating_size;
    m_dock_proportion = c.m_dock_proportion;
    m_buttons = c.m_buttons;
    m_rect = c.m_rect;
}

wxAuiPaneInfo& wxAuiPaneInfo::operator=(const wxAuiPaneInfo& c)
{
    m_name = c.m_name;
    m_caption = c.m_caption;
    m_tooltip = c.m_tooltip;
    m_window = c.m_window;
    m_frame = c.m_frame;
    m_state = c.m_state;
    m_dock_direction = c.m_dock_direction;
    m_dock_layer = c.m_dock_layer;
    m_dock_row = c.m_dock_row;
    m_dock_pos = c.m_dock_pos;
    m_dock_page = c.m_dock_page;
    m_dock_bitmap = c.m_dock_bitmap;
    m_best_size = c.m_best_size;
    m_min_size = c.m_min_size;
    m_max_size = c.m_max_size;
    m_floating_pos = c.m_floating_pos;
    m_floating_size = c.m_floating_size;
    m_dock_proportion = c.m_dock_proportion;
    m_buttons = c.m_buttons;
    m_rect = c.m_rect;
    return *this;
}
#endif // !SWIG

#ifndef __WXGTK20__


class wxPseudoTransparentFrame : public wxFrame
{
public:
    wxPseudoTransparentFrame(wxWindow* parent = NULL,
                wxWindowID id = wxID_ANY,
                const wxString& title = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE,
                const wxString &name = wxT("frame"))
                    : wxFrame(parent, id, title, pos, size, style | wxFRAME_SHAPED, name)
    {
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        m_amount=0;
        m_maxWidth=0;
        m_maxHeight=0;
        m_lastWidth=0;
        m_lastHeight=0;
#ifdef __WXGTK__
        m_canSetShape = false; // have to wait for window create event on GTK
#else
        m_canSetShape = true;
#endif
        m_region = wxRegion(0, 0, 0, 0);
        SetTransparent(0);
    }

    virtual bool SetTransparent(wxByte alpha)
    {
        if (m_canSetShape)
        {
            int w=100; // some defaults
            int h=100;
            GetClientSize(&w, &h);

            m_maxWidth = w;
            m_maxHeight = h;
            m_amount = alpha;
            m_region.Clear();
//            m_region.Union(0, 0, 1, m_maxWidth);
            if (m_amount)
            {
                for (int y=0; y<m_maxHeight; y++)
                {
                    // Reverse the order of the bottom 4 bits
                    int j=((y&8)?1:0)|((y&4)?2:0)|((y&2)?4:0)|((y&1)?8:0);
                    if ((j*16+8)<m_amount)
                        m_region.Union(0, y, m_maxWidth, 1);
                }
            }
            SetShape(m_region);
            Refresh();
        }
        return true;
    }

    void OnPaint(wxPaintEvent& WXUNUSED(event))
    {
        wxPaintDC dc(this);

        if (m_region.IsEmpty())
            return;

#ifdef __WXMAC__
        dc.SetBrush(wxColour(128, 192, 255));
#else
        dc.SetBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION));
#endif
        dc.SetPen(*wxTRANSPARENT_PEN);

        wxRegionIterator upd(GetUpdateRegion()); // get the update rect list

        while (upd)
        {
            wxRect rect(upd.GetRect());
            dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);

            ++upd;
        }
    }

#ifdef __WXGTK__
    void OnWindowCreate(wxWindowCreateEvent& WXUNUSED(evt))
    {
        m_canSetShape=true;
        SetTransparent(0);
    }
#endif

    void OnSize(wxSizeEvent& evt)
    {
        // We sometimes get surplus size events
        if ((evt.GetSize().GetWidth() == m_lastWidth) &&
            (evt.GetSize().GetHeight() == m_lastHeight))
        {
            evt.Skip();
            return;
        }
        m_lastWidth = evt.GetSize().GetWidth();
        m_lastHeight = evt.GetSize().GetHeight();

        SetTransparent(m_amount);
        m_region.Intersect(0, 0, evt.GetSize().GetWidth(), evt.GetSize().GetHeight());
        SetShape(m_region);
        Refresh();
        evt.Skip();
    }

private:
    wxByte m_amount;
    int m_maxWidth;
    int m_maxHeight;
    bool m_canSetShape;
    int m_lastWidth,m_lastHeight;

    wxRegion m_region;

    DECLARE_DYNAMIC_CLASS(wxPseudoTransparentFrame)
    DECLARE_EVENT_TABLE()
};


IMPLEMENT_DYNAMIC_CLASS(wxPseudoTransparentFrame, wxFrame)

BEGIN_EVENT_TABLE(wxPseudoTransparentFrame, wxFrame)
    EVT_PAINT(wxPseudoTransparentFrame::OnPaint)
    EVT_SIZE(wxPseudoTransparentFrame::OnSize)
#ifdef __WXGTK__
    EVT_WINDOW_CREATE(wxPseudoTransparentFrame::OnWindowCreate)
#endif
END_EVENT_TABLE()


#else
  // __WXGTK20__

#include <gtk/gtk.h>
#include "wx/gtk/private/gtk2-compat.h"

static void gtk_pseudo_window_realized_callback( GtkWidget* m_widget, void* WXUNUSED(win) )
{
        wxSize disp = wxGetDisplaySize();
        int amount = 128;
        wxRegion region;
        for (int y=0; y<disp.y; y++)
                {
                    // Reverse the order of the bottom 4 bits
                    int j=((y&8)?1:0)|((y&4)?2:0)|((y&2)?4:0)|((y&1)?8:0);
                    if ((j*16+8)<amount)
                        region.Union(0, y, disp.x, 1);
                }
        gdk_window_shape_combine_region(gtk_widget_get_window(m_widget), region.GetRegion(), 0, 0);
}


class wxPseudoTransparentFrame: public wxFrame
{
public:
    wxPseudoTransparentFrame(wxWindow* parent = NULL,
                wxWindowID id = wxID_ANY,
                const wxString& title = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE,
                const wxString &name = wxT("frame"))
    {
         if (!CreateBase( parent, id, pos, size, style, wxDefaultValidator, name ))
            return;

        m_title = title;

        m_widget = gtk_window_new( GTK_WINDOW_POPUP );
        g_object_ref(m_widget);

        if (parent) parent->AddChild(this);

        g_signal_connect( m_widget, "realize", G_CALLBACK (gtk_pseudo_window_realized_callback), this );

        GdkColor col;
        col.red = 128 * 256;
        col.green = 192 * 256;
        col.blue = 255 * 256;
        gtk_widget_modify_bg( m_widget, GTK_STATE_NORMAL, &col );
    }

    bool SetTransparent(wxByte WXUNUSED(alpha))
    {
        return true;
    }

protected:
    virtual void DoSetSizeHints( int minW, int minH, int maxW, int maxH, int incW, int incH)
    {
        // the real wxFrame method doesn't work for us because we're not really
        // a top level window so skip it
        wxWindow::DoSetSizeHints(minW, minH, maxW, maxH, incW, incH);
    }

private:
    DECLARE_DYNAMIC_CLASS(wxPseudoTransparentFrame)
};

IMPLEMENT_DYNAMIC_CLASS(wxPseudoTransparentFrame, wxFrame)

#endif
 // __WXGTK20__



// -- static utility functions --

static wxBitmap wxPaneCreateStippleBitmap()
{
    unsigned char data[] = { 0,0,0,192,192,192, 192,192,192,0,0,0 };
    wxImage img(2,2,data,true);
    return wxBitmap(img);
}

static void DrawResizeHint(wxDC& dc, const wxRect& rect)
{
    wxBitmap stipple = wxPaneCreateStippleBitmap();
    wxBrush brush(stipple);
    dc.SetBrush(brush);
#ifdef __WXMSW__
    wxMSWDCImpl* impl = (wxMSWDCImpl*) dc.GetImpl();
    PatBlt(GetHdcOf(*impl), rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight(), PATINVERT);
#else
    dc.SetPen(*wxTRANSPARENT_PEN);

    dc.SetLogicalFunction(wxXOR);
    dc.DrawRectangle(rect);
#endif
}



// CopyDocksAndPanes() - this utility function creates copies of
// the dock and pane info.  wxAuiDockInfo's usually contain pointers
// to wxAuiPaneInfo classes, thus this function is necessary to reliably
// reconstruct that relationship in the new dock info and pane info arrays

static void CopyDocksAndPanes(wxAuiDockInfoArray& destDocks, wxAuiPaneInfoArray& destPanes, const wxAuiDockInfoArray& srcDocks, const wxAuiPaneInfoArray& srcPanes)
{
    destDocks = srcDocks;
    destPanes = srcPanes;
    int i, j, k, dockCount, pc1, pc2;
    for (i = 0, dockCount = destDocks.GetCount(); i < dockCount; ++i)
    {
        wxAuiDockInfo& dock = destDocks.Item(i);
        for (j = 0, pc1 = dock.panes.GetCount(); j < pc1; ++j)
            for (k = 0, pc2 = srcPanes.GetCount(); k < pc2; ++k)
                if (dock.panes.Item(j) == &srcPanes.Item(k))
                    dock.panes.Item(j) = &destPanes.Item(k);
    }
}

// GetMaxLayer() is an internal function which returns
// the highest layer inside the specified dock
static int GetMaxLayer(const wxAuiDockInfoArray& docks,
                       int dock_direction)
{
    int i, dockCount, maxLayer = 0;
    for (i = 0, dockCount = docks.GetCount(); i < dockCount; ++i)
    {
        wxAuiDockInfo& dock = docks.Item(i);
        if (dock.dock_direction == dock_direction &&
            dock.dock_layer > maxLayer && !dock.fixed)
                maxLayer = dock.dock_layer;
    }
    return maxLayer;
}


// GetMaxRow() is an internal function which returns
// the highest layer inside the specified dock
static int GetMaxRow(const wxAuiPaneInfoArray& panes, int direction, int layer)
{
    int i, paneCount, maxRow = 0;
    for (i = 0, paneCount = panes.GetCount(); i < paneCount; ++i)
    {
        wxAuiPaneInfo& pane = panes.Item(i);
        if (pane.GetDirection() == direction &&
            pane.GetLayer() == layer &&
            pane.GetRow() > maxRow)
                maxRow = pane.GetRow();
    }
    return maxRow;
}



// wxAuiDoInsertDockLayer() is an internal function that inserts a new dock
// layer by incrementing all existing dock layer values by one
void wxAuiDoInsertDockLayer(wxAuiPaneInfoArray& panes, int dockDirection, int dockLayer)
{
    int i, paneCount;
    for (i = 0, paneCount = panes.GetCount(); i < paneCount; ++i)
    {
        wxAuiPaneInfo& pane = panes.Item(i);
        if (!pane.IsFloating() &&
            pane.GetDirection() == dockDirection &&
            pane.GetLayer() >= dockLayer)
                pane.SetLayer(pane.GetLayer()+1);
    }
}

// wxAuiDoInsertDockRow() is an internal function that inserts a new dock
// row by incrementing all existing dock row values by one
void wxAuiDoInsertDockRow(wxAuiPaneInfoArray& panes, int dockDirection, int dockLayer, int dockRow)
{
    int i, paneCount;
    for (i = 0, paneCount = panes.GetCount(); i < paneCount; ++i)
    {
        wxAuiPaneInfo& pane = panes.Item(i);
        if (!pane.IsFloating() &&
            pane.GetDirection() == dockDirection &&
            pane.GetLayer() == dockLayer &&
            pane.GetRow() >= dockRow)
                pane.SetRow(pane.GetRow()+1);
    }
}

// wxAuiDoInsertPane() is an internal function that inserts a space for
// another dock pane by incrementing all existing dock position values by one
void wxAuiDoInsertPane(wxAuiPaneInfoArray& panes, int dockDirection, int dockLayer, int dockRow, int dockPos)
{
    int i, paneCount;
    for (i = 0, paneCount = panes.GetCount(); i < paneCount; ++i)
    {
        wxAuiPaneInfo& pane = panes.Item(i);
        if (!pane.IsFloating() &&
            pane.GetDirection() == dockDirection &&
            pane.GetLayer() == dockLayer &&
            pane.GetRow() == dockRow &&
            pane.GetPosition() >= dockPos)
        {
                pane.SetPosition(pane.GetPosition()+1);
        }
    }
}

// wxAuiDoInsertPane() is an internal function that inserts a space for
// another notebook page by incrementing all existing page values by one
void wxAuiDoInsertPage(wxAuiPaneInfoArray& panes, int dockDirection, int dockLayer, int dockRow, int dockPos, int dockPage)
{
    int i, paneCount;
    for (i = 0, paneCount = panes.GetCount(); i < paneCount; ++i)
    {
        wxAuiPaneInfo& pane = panes.Item(i);
        if (!pane.IsFloating() &&
            pane.GetDirection() == dockDirection &&
            pane.GetLayer() == dockLayer &&
            pane.GetRow() == dockRow &&
            pane.GetPosition() == dockPos &&
            pane.GetPage() >= dockPage)
            {
                pane.SetPage(pane.GetPage()+1);
            }
    }
}

// FindDocks() is an internal function that returns a list of docks which meet
// the specified conditions in the parameters and returns a sorted array
// (sorted by layer and then row)
static void FindDocks(wxAuiDockInfoArray& docks, int dockDirection, int dockLayer, int dockRow, wxAuiDockInfoPtrArray& arr)
{
    int beginLayer = dockLayer;
    int endLayer = dockLayer;
    int beginRow = dockRow;
    int endRow = dockRow;
    int dockCount = docks.GetCount();
    int layer, row, i, maxRow = 0, maxLayer = 0;

    // discover the maximum dock layer and the max row
    for (i = 0; i < dockCount; ++i)
    {
        maxRow = wxMax(maxRow, docks.Item(i).dock_row);
        maxLayer = wxMax(maxLayer, docks.Item(i).dock_layer);
    }

    // if no dock layer was specified, search all dock layers
    if (dockLayer == -1)
    {
        beginLayer = 0;
        endLayer = maxLayer;
    }

    // if no dock row was specified, search all dock row
    if (dockRow == -1)
    {
        beginRow = 0;
        endRow = maxRow;
    }

    arr.Clear();

    for (layer = beginLayer; layer <= endLayer; ++layer)
        for (row = beginRow; row <= endRow; ++row)
            for (i = 0; i < dockCount; ++i)
            {
                wxAuiDockInfo& d = docks.Item(i);
                if (dockDirection == -1 || dockDirection == d.dock_direction)
                {
                    if (d.dock_layer == layer && d.dock_row == row)
                        arr.Add(&d);
                }
            }
}

// FindPaneInDock() looks up a specified window pointer inside a dock.
// If found, the corresponding wxAuiPaneInfo pointer is returned, otherwise NULL.
static wxAuiPaneInfo* FindPaneInDock(const wxAuiDockInfo& dock, wxWindow* window)
{
    int i;
    int count = dock.panes.GetCount();
    for (i = 0; i < count; ++i)
    {
        wxAuiPaneInfo* p = dock.panes.Item(i);
        if (p->GetWindow() == window)
            return p;
    }
    return NULL;
}

// RemovePaneFromDocks() removes a pane window from all docks
// with a possible exception specified by parameter "ex_cept"
static void RemovePaneFromDocks(wxAuiDockInfoArray& docks, wxAuiPaneInfo& pane, wxAuiDockInfo* ex_cept  = NULL  )
{
    int i, dockCount;
    for (i = 0, dockCount = docks.GetCount(); i < dockCount; ++i)
    {
        wxAuiDockInfo& d = docks.Item(i);
        if (&d == ex_cept)
            continue;
        wxAuiPaneInfo* pi = FindPaneInDock(d, pane.GetWindow());
        if (pi)
            d.panes.Remove(pi);
    }
}

#if 0
// This function works fine, and may be used in the future

// RenumberDockRows() takes a dock and assigns sequential numbers
// to existing rows.  Basically it takes out the gaps; so if a
// dock has rows with numbers 0,2,5, they will become 0,1,2
static void RenumberDockRows(wxAuiDockInfoPtrArray& docks)
{
    int i, dockCount;
    for (i = 0, dockCount = docks.GetCount(); i < dockCount; ++i)
    {
        wxAuiDockInfo& dock = *docks.Item(i);
        dock.dock_row = i;

        int j, paneCount;
        for (j = 0, paneCount = dock.panes.GetCount(); j < paneCount; ++j)
            dock.panes.Item(j)->dock_row = i;
    }
}
#endif

// IsNotebookPane() determines if the pane in array panes at position paneIndex is currently forming part of a notebook.
static bool IsNotebookPane(wxAuiPaneInfoArray& panes, int paneIndex)
{
    int paneCount = panes.GetCount();
    wxAuiPaneInfo& p = panes[paneIndex];

    //Take hidden and floating panes into account
    int indexPaneBefore=1;
    while(paneIndex-indexPaneBefore>=0)
    {
        if(panes[paneIndex-indexPaneBefore].IsShown()&&!panes[paneIndex-indexPaneBefore].IsFloating())
            break;
        indexPaneBefore++;
    }
    int indexPaneAfter=1;
    while(paneIndex+indexPaneAfter<paneCount)
    {
        if(panes[paneIndex+indexPaneAfter].IsShown()&&!panes[paneIndex+indexPaneAfter].IsFloating())
            break;
        indexPaneAfter++;
    }
    //If first visible pane before or after us in the array has the same position, layer and row as us then we are part of a notebook.
    if(paneIndex+indexPaneAfter<paneCount && p.GetPosition() == panes.Item(paneIndex+indexPaneAfter).GetPosition() && p.GetLayer() == panes.Item(paneIndex+indexPaneAfter).GetLayer() && p.GetRow() == panes.Item(paneIndex+indexPaneAfter).GetRow())
    {
        return true;
    }
    else if(paneIndex-indexPaneBefore >= 0 && p.GetPosition() == panes.Item(paneIndex-indexPaneBefore).GetPosition() && p.GetLayer() == panes.Item(paneIndex-indexPaneBefore).GetLayer()  && p.GetRow() == panes.Item(paneIndex-indexPaneBefore).GetRow())
    {
        return true;
    }
    else
    {
        return false;
    }
}

// SetActivePane() sets the active pane, as well as cycles through
// every other pane and makes sure that all others' active flags
// are turned off
int wxAuiManager::SetActivePane(wxWindow* activePane)
{
    int i, paneCount;
    wxAuiPaneInfo* activePaneInfo = NULL;
    for (i = 0, paneCount = m_panes.GetCount(); i < paneCount; ++i)
    {
        wxAuiPaneInfo& pane = m_panes.Item(i);
        pane.SetFlag(wxAuiPaneInfo::optionActive,false);
        if (pane.GetWindow() == activePane)
        {
            if(IsNotebookPane(m_panes,i))
            {
                activePaneInfo = &pane;
                wxAuiTabContainer* ctrl;
                int ctrlIndex;
                FindTab(pane.GetWindow(), &ctrl, &ctrlIndex);
                ctrl->SetActivePage(pane.GetWindow());
            }
            //else
            {
                pane.SetFlag(wxAuiPaneInfo::optionActive,true);
            }
        }
    }

    // send the 'activated' event after all panes have been updated
    if ( activePaneInfo )
    {
        wxAuiManagerEvent evt(wxEVT_AUI_PANE_ACTIVATED);
        evt.SetManager(this);
        evt.SetPane(activePaneInfo);
        ProcessMgrEvent(evt);
    }

    return 0;
}

size_t wxAuiManager::GetActivePane(wxWindow* focus) const
{
    int i, paneCount;

    // First try to find a pane that has the focus flag and whose window has actual focus.
    // This allows us to pick the right pane in the event that multiple panes have the focus flag set.
    for (i = 0, paneCount = m_panes.GetCount(); i < paneCount; ++i)
    {
        wxAuiPaneInfo& pane = m_panes[i];
        if(pane.HasFlag(wxAuiPaneInfo::optionActive))
        {
            if(!focus || pane.GetWindow()==focus)
            {
                return i;
            }
        }
    }
    // If no panes had the focus flag and actual focus then fall back to returning the first pane with focus flag.
    for (i = 0, paneCount = m_panes.GetCount(); i < paneCount; ++i)
    {
        wxAuiPaneInfo& pane = m_panes[i];
        if(pane.HasFlag(wxAuiPaneInfo::optionActive))
        {
            return i;
        }
    }
    return 0;
}


// this function is used to sort panes by Direction, Layer, Row, Position and then finally by Page
static int PaneSortFunc(wxAuiPaneInfo** p1, wxAuiPaneInfo** p2)
{
    if((*p1)->GetDirection() < (*p2)->GetDirection())
    {
        return -1;
    }
    else if((*p1)->GetDirection() == (*p2)->GetDirection())
    {
        if((*p1)->GetLayer() < (*p2)->GetLayer())
        {
            return -1;
        }
        else if((*p1)->GetLayer() == (*p2)->GetLayer())
        {
            if((*p1)->GetRow() < (*p2)->GetRow())
            {
                return -1;
            }
            else if((*p1)->GetRow() == (*p2)->GetRow())
            {
                if((*p1)->GetPosition() < (*p2)->GetPosition())
                {
                    return -1;
                }
                else if((*p1)->GetPosition() == (*p2)->GetPosition())
                {
                    if((*p1)->GetPage() < (*p2)->GetPage())
                    {
                        return -1;
                    }
                }
            }
        }
    }
    return 1;
}

// this function is used to sort panes primarily by dock position and secondly by page
static int DockPaneSortFunc(wxAuiPaneInfo** p1, wxAuiPaneInfo** p2)
{
    if((*p1)->GetPosition() < (*p2)->GetPosition())
    {
        return -1;
    }
    else if((*p1)->GetPosition() == (*p2)->GetPosition())
    {
        if((*p1)->GetPage() < (*p2)->GetPage())
        {
            return -1;
        }
    }
    return 1;
}



bool wxAuiPaneInfo::IsValid() const
{
    // Should this RTTI and function call be rewritten as
    // sending a new event type to allow other window types
    // to check the pane settings?
    wxAuiToolBar* toolbar = wxDynamicCast(window, wxAuiToolBar);
    return !toolbar || toolbar->IsPaneValid(*this);
}

// -- wxAuiManager class implementation --


BEGIN_EVENT_TABLE(wxAuiManager, wxEvtHandler)
    EVT_AUI_PANE_BUTTON(wxAuiManager::OnPaneButton)
    EVT_AUI_RENDER(wxAuiManager::OnRender)
    EVT_PAINT(wxAuiManager::OnPaint)
    EVT_ERASE_BACKGROUND(wxAuiManager::OnEraseBackground)
    EVT_SIZE(wxAuiManager::OnSize)
    EVT_SET_CURSOR(wxAuiManager::OnSetCursor)
    EVT_LEFT_DCLICK(wxAuiManager::OnLeftDClick)
    EVT_LEFT_DOWN(wxAuiManager::OnLeftDown)
    EVT_LEFT_UP(wxAuiManager::OnLeftUp)
    EVT_RIGHT_DOWN(wxAuiManager::OnRightDown)
    EVT_RIGHT_UP(wxAuiManager::OnRightUp)
    EVT_MIDDLE_DOWN(wxAuiManager::OnMiddleDown)
    EVT_MIDDLE_UP(wxAuiManager::OnMiddleUp)
    EVT_MOTION(wxAuiManager::OnMotion)
    EVT_LEAVE_WINDOW(wxAuiManager::OnLeaveWindow)
    EVT_MOUSE_CAPTURE_LOST(wxAuiManager::OnCaptureLost)
    EVT_CHILD_FOCUS(wxAuiManager::OnChildFocus)
    EVT_AUI_FIND_MANAGER(wxAuiManager::OnFindManager)
END_EVENT_TABLE()


wxAuiManager::wxAuiManager(wxWindow* managedWindow, unsigned int flags)
: m_frame(NULL)
, m_actionDeadZone(NULL)
, m_actionPart(NULL)
, m_actionWindow(NULL)
, m_hoverButton(NULL)
, m_hoverButton2(NULL)
, m_hintWnd(NULL)
, m_reserved(NULL)
{
    m_action = actionNone;
    m_lastMouseMove = wxPoint();

    m_art = new wxAuiDefaultDockArt;
    m_tab_art = new wxAuiDefaultTabArt;
    m_flags = flags;
    m_skipping = false;
    m_hasMaximized = false;
    m_doingHintCalculation = false;

    m_dockConstraintX = 0.3;
    m_dockConstraintY = 0.3;
    m_currentDragItem = -1;

    if (managedWindow)
    {
        SetManagedWindow(managedWindow);
    }
}

wxAuiManager::~wxAuiManager()
{
    // NOTE: It's possible that the windows have already been destroyed by the
    // time this dtor is called, so this loop can result in memory access via
    // invalid pointers, resulting in a crash.  So it will be disabled while
    // waiting for a better solution.
#if 0
    for ( size_t i = 0; i < m_panes.size(); i++ )
    {
        wxAuiPaneInfo& pinfo = m_panes[i];
        if (pinfo.window && !pinfo.window->GetParent())
            delete pinfo.window;
    }
#endif

    delete m_art;
    delete m_tab_art;
}

// creates a floating frame for the windows
wxAuiFloatingFrame* wxAuiManager::CreateFloatingFrame(wxWindow* parent, const wxAuiPaneInfo& paneInfo)
{
    return new wxAuiFloatingFrame(parent, this, paneInfo);
}

bool wxAuiManager::CanDockPanel(const wxAuiPaneInfo& WXUNUSED(p))
{
    // if a key modifier is pressed while dragging the frame,
    // don't dock the window
    return !(wxGetKeyState(WXK_CONTROL) || wxGetKeyState(WXK_ALT));
}

// GetPane() looks up a wxAuiPaneInfo structure based
// on the supplied window pointer.  Upon failure, GetPane()
// returns an empty wxAuiPaneInfo, a condition which can be checked
// by calling wxAuiPaneInfo::IsOk().
//
// The pane info's structure may then be modified.  Once a pane's
// info is modified, wxAuiManager::Update() must be called to
// realize the changes in the UI.

wxAuiPaneInfo& wxAuiManager::GetPane(wxWindow* window) const
{
    int i, paneCount;
    for (i = 0, paneCount = m_panes.GetCount(); i < paneCount; ++i)
    {
        wxAuiPaneInfo& p = m_panes.Item(i);
        if (p.GetWindow() == window)
            return p;
    }
    return wxAuiNullPaneInfo;
}

// this version of GetPane() looks up a pane based on a
// 'pane name', see above comment for more info
wxAuiPaneInfo& wxAuiManager::GetPane(const wxString& name) const
{
    int i, paneCount;
    for (i = 0, paneCount = m_panes.GetCount(); i < paneCount; ++i)
    {
        wxAuiPaneInfo& p = m_panes.Item(i);
        if (p.GetName() == name)
            return p;
    }
    return wxAuiNullPaneInfo;
}

// This version of GetPane() looks up a pane based on its index position.
wxAuiPaneInfo& wxAuiManager::GetPane(size_t paneIndex) const
{
    size_t paneCount = m_panes.GetCount();
    if(paneCount>paneIndex)
    {
        return m_panes[paneIndex];
    }

    wxASSERT_MSG(0, wxT("Invalid pane index passed to wxAuiManager::GetPane"));
    return wxAuiNullPaneInfo;
}

// GetAllPanes() returns a reference to all the pane info structures
wxAuiPaneInfoArray& wxAuiManager::GetAllPanes()
{
    return m_panes;
}

// GetPaneCount() returns the total number of pages managed by the multi-notebook.
size_t wxAuiManager::GetPaneCount() const
{
    return m_panes.GetCount();
}

// FindTab() finds the tab control that currently contains the window as well
// as the index of the window in the tab control.  It returns true if the
// window was found, otherwise false.
bool wxAuiManager::FindTab(wxWindow* page, wxAuiTabContainer** ctrl, int* idx)
{
    int uiPartsCount = m_uiParts.GetCount();
    int i;
    for (i = 0; i < uiPartsCount; i++)
    {
        wxAuiDockUIPart& part = m_uiParts.Item(i);
        if(part.m_tab_container)
        {
            int pageIndex = part.m_tab_container->GetIdxFromWindow(page);
            if(pageIndex != wxNOT_FOUND)
            {
                *ctrl = part.m_tab_container;
                *idx = pageIndex;
                return true;
            }
        }
    }
    return false;
}

// HitTest() is an internal function which determines
// which UI item the specified coordinates are over
// (x,y) specify a position in client coordinates
wxAuiDockUIPart* wxAuiManager::HitTest(int x, int y)
{
    wxAuiDockUIPart* result = NULL;

    int i, partCount;
    for (i = 0, partCount = m_uiParts.GetCount(); i < partCount; ++i)
    {
        wxAuiDockUIPart* item = &m_uiParts.Item(i);

        // we are not interested in typeDock, because this space
        // isn't used to draw anything, just for measurements;
        // besides, the entire dock area is covered with other
        // rectangles, which we are interested in.
        if (item->type == wxAuiDockUIPart::typeDock)
            continue;

        // if we already have a hit on a more specific item, we are not
        // interested in a pane hit.  If, however, we don't already have
        // a hit, returning a pane hit is necessary for some operations
        if ((item->type == wxAuiDockUIPart::typePane ||
            item->type == wxAuiDockUIPart::typePaneBorder) && result)
            continue;

        // if the point is inside the rectangle, we have a hit
        if (item->rect.Contains(x,y))
            result = item;
    }

    return result;
}


// SetFlags() and GetFlags() allow the owner to set/get various
// options which are global to wxAuiManager
void wxAuiManager::SetFlags(unsigned int flags)
{
    // find out if we have to call UpdateHintWindowConfig()
    bool updateHintWnd = false;
    unsigned int hintMask = wxAUI_MGR_TRANSPARENT_HINT | wxAUI_MGR_VENETIAN_BLINDS_HINT | wxAUI_MGR_RECTANGLE_HINT;
    if ((flags & hintMask) != (m_flags & hintMask))
        updateHintWnd = true;


    // set the new flags
    m_flags = flags;

    if (updateHintWnd)
    {
        UpdateHintWindowConfig();
    }
}

unsigned int wxAuiManager::GetFlags() const
{
    return m_flags;
}

// SetFlag() and HasFlag() allow the owner to set/get various
// options which are global to wxAuiManager
bool wxAuiManager::HasFlag(int flag) const
{
    return (m_flags & flag) != 0;
}

void wxAuiManager::SetFlag(int flag, bool optionState)
{
    if (optionState)
        SetFlags(m_flags | flag);
    else
        SetFlags(m_flags & ~flag);
}


// Convenience function
bool wxAuiManager_HasLiveResize(wxAuiManager& manager)
{
    // With Core Graphics on Mac, it's not possible to show sash feedback,
    // so we'll always use live update instead.
#if defined(__WXMAC__)
    wxUnusedVar(manager);
    return true;
#else
    return manager.HasFlag(wxAUI_MGR_LIVE_RESIZE);
#endif
}

// don't use these anymore as they are deprecated
// use Set/GetManagedFrame() instead
void wxAuiManager::SetFrame(wxFrame* frame)
{
    SetManagedWindow((wxWindow*)frame);
}

wxFrame* wxAuiManager::GetFrame() const
{
    return (wxFrame*)m_frame;
}


// this function will return the aui manager for a given
// window.  The |window| parameter should be any child window
// or grand-child window (and so on) of the frame/window
// managed by wxAuiManager.  The |window| parameter does not
// need to be managed by the manager itself.
wxAuiManager* wxAuiManager::GetManager(wxWindow* window)
{
    wxAuiManagerEvent evt(wxEVT_AUI_FIND_MANAGER);
    evt.SetManager(NULL);
    evt.ResumePropagation(wxEVENT_PROPAGATE_MAX);
    if (!window->GetEventHandler()->ProcessEvent(evt))
        return NULL;

    return evt.GetManager();
}


void wxAuiManager::UpdateHintWindowConfig()
{
    // find out if the system can do transparent frames
    bool canDoTransparent = false;

    wxWindow* w = m_frame;
    while (w)
    {
        if (wxDynamicCast(w,wxFrame))
        {
            wxFrame* f = static_cast<wxFrame*>(w);
            canDoTransparent = f->CanSetTransparent();

            break;
        }

        w = w->GetParent();
    }

    // if there is an existing hint window, delete it
    if (m_hintWnd)
    {
        m_hintWnd->Destroy();
        m_hintWnd = NULL;
    }

    m_hintFadeMax = 50;
    m_hintWnd = NULL;

    if (HasFlag(wxAUI_MGR_TRANSPARENT_HINT) && canDoTransparent)
    {
        // Make a window to use for a transparent hint
        #if defined(__WXMSW__) || defined(__WXGTK__)
            m_hintWnd = new wxFrame(m_frame, wxID_ANY, wxEmptyString,
                                     wxDefaultPosition, wxSize(1,1),
                                         wxFRAME_TOOL_WINDOW |
                                         wxFRAME_FLOAT_ON_PARENT |
                                         wxFRAME_NO_TASKBAR |
                                         wxNO_BORDER);

            m_hintWnd->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION));
        #elif defined(__WXMAC__)
            // Using a miniframe with float and tool styles keeps the parent
            // frame activated and highlighted as such...
            m_hintWnd = new wxMiniFrame(m_frame, wxID_ANY, wxEmptyString,
                                         wxDefaultPosition, wxSize(1,1),
                                         wxFRAME_FLOAT_ON_PARENT
                                         | wxFRAME_TOOL_WINDOW );
            m_hintWnd->Connect(wxEVT_ACTIVATE, wxActivateEventHandler(wxAuiManager::OnHintActivate), NULL, this);

            // Can't set the bg colour of a Frame in wxMac
            wxPanel* p = new wxPanel(m_hintWnd);

            // The default wxSYS_COLOUR_ACTIVECAPTION colour is a light silver
            // color that is really hard to see, especially transparent.
            // Until a better system color is decided upon we'll just use
            // blue.
            p->SetBackgroundColour(*wxBLUE);
        #endif

    }
    else
    {
        if (HasFlag(wxAUI_MGR_TRANSPARENT_HINT) || HasFlag(wxAUI_MGR_VENETIAN_BLINDS_HINT))
        {
            // system can't support transparent fade, or the venetian
            // blinds effect was explicitly requested
            m_hintWnd = new wxPseudoTransparentFrame(m_frame,
                                                      wxID_ANY,
                                                      wxEmptyString,
                                                      wxDefaultPosition,
                                                      wxSize(1,1),
                                                            wxFRAME_TOOL_WINDOW |
                                                            wxFRAME_FLOAT_ON_PARENT |
                                                            wxFRAME_NO_TASKBAR |
                                                            wxNO_BORDER);
            m_hintFadeMax = 128;
        }
    }
}


// SetManagedWindow() is usually called once when the frame
// manager class is being initialized.  "frame" specifies
// the frame which should be managed by the frame mananger
void wxAuiManager::SetManagedWindow(wxWindow* wnd)
{
    wxASSERT_MSG(wnd, wxT("specified window must be non-NULL"));

    m_frame = wnd;
    m_frame->PushEventHandler(this);

#if wxUSE_MDI
    // if the owner is going to manage an MDI parent frame,
    // we need to add the MDI client window as the default
    // center pane

    if (wxDynamicCast(m_frame,wxMDIParentFrame))
    {
        wxMDIParentFrame* mdi_frame = (wxMDIParentFrame*)m_frame;
        wxWindow* client_window = mdi_frame->GetClientWindow();

        wxASSERT_MSG(client_window, wxT("Client window is NULL!"));

        AddPane(client_window, wxAuiPaneInfo().SetName(wxT("mdiclient")).SetCenterPane().SetBorder(false));
    }
    else if (wxDynamicCast(m_frame,wxAuiMDIParentFrame))
    {
        wxAuiMDIParentFrame* mdi_frame = (wxAuiMDIParentFrame*)m_frame;
        wxAuiMDIClientWindow* client_window = mdi_frame->GetClientWindow();
        wxASSERT_MSG(client_window, wxT("Client window is NULL!"));

        AddPane(client_window,
                wxAuiPaneInfo().SetName(wxT("mdiclient")).
                SetCenterPane().SetBorder(false));
    }
#endif

    UpdateHintWindowConfig();
}


// UnInit() must be called, usually in the destructor
// of the frame class.   If it is not called, usually this
// will result in a crash upon program exit
void wxAuiManager::UnInit()
{
    if (m_frame)
    {
        m_frame->RemoveEventHandler(this);
    }
}

// GetManagedWindow() returns the window pointer being managed
wxWindow* wxAuiManager::GetManagedWindow() const
{
    return m_frame;
}

wxAuiDockArt* wxAuiManager::GetArtProvider() const
{
    return m_art;
}

wxAuiTabArt* wxAuiManager::GetTabArtProvider() const
{
    return m_tab_art;
}

void wxAuiManager::ProcessMgrEvent(wxAuiManagerEvent& evt)
{
    // first, give the owner frame a chance to override
    if (m_frame)
    {
        if (m_frame->GetEventHandler()->ProcessEvent(evt))
            return;
    }

    ProcessEvent(evt);
}

// SetArtProvider() instructs wxAuiManager to use the
// specified art provider for all drawing calls.  This allows
// plugable look-and-feel features.  The pointer that is
// passed to this method subsequently belongs to wxAuiManager,
// and is deleted in the frame manager destructor
void wxAuiManager::SetArtProvider(wxAuiDockArt* artProvider)
{
    // delete the last art provider, if any
    delete m_art;

    // assign the new art provider
    m_art = artProvider;
}

void wxAuiManager::SetTabArtProvider(wxAuiTabArt* artProvider)
{
    // delete the last art provider, if any
    delete m_tab_art;

    // assign the new art provider
    m_tab_art = artProvider;
}


bool wxAuiManager::AddPane(wxWindow* window, const wxAuiPaneInfo& paneInfo)
{
    wxASSERT_MSG(window, wxT("NULL window ptrs are not allowed"));

    // check if the pane has a valid window
    if (!window)
        return false;

    // check if the window is already managed by us
    if (GetPane(paneInfo.GetWindow()).IsOk())
        return false;

    // check if the pane name already exists, this could reveal a
    // bug in the library user's application
    bool alreadyExists = false;
    if (!paneInfo.GetName().empty() && GetPane(paneInfo.GetName()).IsOk())
    {
        wxFAIL_MSG(wxT("A pane with that name already exists in the manager!"));
        alreadyExists = true;
    }

    // if the new pane is docked then we should undo maximize
    if (paneInfo.IsDocked())
        RestoreMaximizedPane();

    // special case:  wxAuiToolBar style interacts with docking flags
    wxAuiPaneInfo test(paneInfo);
    wxAuiToolBar* toolbar = wxDynamicCast(window, wxAuiToolBar);
    if (toolbar)
    {
        // if pane has default docking flags
        const unsigned int dockMask = wxAuiPaneInfo::optionLeftDockable |
                                        wxAuiPaneInfo::optionRightDockable |
                                        wxAuiPaneInfo::optionTopDockable |
                                        wxAuiPaneInfo::optionBottomDockable;
        const unsigned int defaultDock = wxAuiPaneInfo().SetDefaultPane().state & dockMask;
        if ((test.state & dockMask) == defaultDock)
        {
            // set docking flags based on toolbar style
            if (toolbar->GetWindowStyleFlag() & wxAUI_TB_VERTICAL)
            {
                test.SetTopDockable(false).SetBottomDockable(false);
            }
            else if (toolbar->GetWindowStyleFlag() & wxAUI_TB_HORIZONTAL)
            {
                test.SetLeftDockable(false).SetRightDockable(false);
            }
        }
        else
        {
            // see whether non-default docking flags are valid
            test.window = window;
            wxCHECK_MSG(test.IsValid(), false,
                        "toolbar style and pane docking flags are incompatible");
        }
    }

    m_panes.Add(test);

    wxAuiPaneInfo& pinfo = m_panes.Last();

    // set the pane window
    pinfo.SetWindow(window);


    // if the pane's name identifier is blank, create a random string
    if (pinfo.GetName().empty() || alreadyExists)
    {
        pinfo.GetName().Printf(wxT("%08lx%08x%08x%08lx"),
             (unsigned long)wxPtrToUInt(pinfo.GetWindow()) & 0xffffffff,
             (unsigned int)time(NULL),
#ifdef __WXWINCE__
             (unsigned int)GetTickCount(),
#else
             (unsigned int)clock(),
#endif
             (unsigned long)m_panes.GetCount());
    }

    // set initial proportion (if not already set)
    if (pinfo.GetProportion() == 0)
        pinfo.SetProportion(100000);

    if (pinfo.HasMaximizeButton())
    {
        wxAuiPaneButton button;
        button.button_id = wxAUI_BUTTON_MAXIMIZE_RESTORE;
        pinfo.GetButtons().Add(button);
    }

    if (pinfo.HasPinButton())
    {
        wxAuiPaneButton button;
        button.button_id = wxAUI_BUTTON_PIN;
        pinfo.GetButtons().Add(button);
    }

    if (pinfo.HasCloseButton())
    {
        wxAuiPaneButton button;
        button.button_id = wxAUI_BUTTON_CLOSE;
        pinfo.GetButtons().Add(button);
    }

    if (pinfo.HasGripper())
    {
        if (wxDynamicCast(pinfo.GetWindow(),wxAuiToolBar))
        {
            // prevent duplicate gripper -- both wxAuiManager and wxAuiToolBar
            // have a gripper control.  The toolbar's built-in gripper
            // meshes better with the look and feel of the control than ours,
            // so turn wxAuiManager's gripper off, and the toolbar's on.

            wxAuiToolBar* tb = static_cast<wxAuiToolBar*>(pinfo.GetWindow());
            pinfo.SetFlag(wxAuiPaneInfo::optionGripper, false);
            tb->SetGripperVisible(true);
        }
    }


    if (pinfo.GetBestSize() == wxDefaultSize &&
        pinfo.GetWindow())
    {
        pinfo.SetBestSize(pinfo.GetWindow()->GetClientSize());

#if wxUSE_TOOLBAR
        if (wxDynamicCast(pinfo.GetWindow(),wxToolBar))
        {
            // GetClientSize() doesn't get the best size for
            // a toolbar under some newer versions of wxWidgets,
            // so use GetBestSize()
            pinfo.SetBestSize(pinfo.GetWindow()->GetBestSize());
        }
#endif // wxUSE_TOOLBAR

        if (pinfo.GetMinSize() != wxDefaultSize)
        {
            if (pinfo.GetBestSize().x < pinfo.GetMinSize().x)
                pinfo.SetBestSize(pinfo.GetMinSize().x,pinfo.GetBestSize().y);
            if (pinfo.GetBestSize().y < pinfo.GetMinSize().y)
                pinfo.SetBestSize(pinfo.GetBestSize().x,pinfo.GetMinSize().y);
        }
    }



    return true;
}

bool wxAuiManager::AddPane(wxWindow* window, int direction, const wxString& caption)
{
    wxAuiPaneInfo pinfo;
    pinfo.SetCaption(caption);
    switch (direction)
    {
        case wxTOP:    pinfo.SetDirectionTop(); break;
        case wxBOTTOM: pinfo.SetDirectionBottom(); break;
        case wxLEFT:   pinfo.SetDirectionLeft(); break;
        case wxRIGHT:  pinfo.SetDirectionRight(); break;
        case wxCENTER: pinfo.SetCenterPane(); break;
    }
    return AddPane(window, pinfo);
}

bool wxAuiManager::AddPane(wxWindow* window, const wxAuiPaneInfo& paneInfo, const wxPoint& dropPos)
{
    if (!AddPane(window, paneInfo))
        return false;

    wxAuiPaneInfo& pane = GetPane(window);

    DoDrop(m_docks, m_panes, pane, dropPos, wxPoint(0,0));

    return true;
}

bool wxAuiManager::InsertPane(wxWindow* window, const wxAuiPaneInfo& paneInfo, int insertLevel)
{
    wxASSERT_MSG(window, wxT("NULL window ptrs are not allowed"));

    // shift the panes around, depending on the insert level
    switch (insertLevel)
    {
        case wxAUI_INSERT_PANE:
            wxAuiDoInsertPane(m_panes,
                 paneInfo.GetDirection(),
                 paneInfo.GetLayer(),
                 paneInfo.GetRow(),
                 paneInfo.GetPosition());
            break;
        case wxAUI_INSERT_ROW:
            wxAuiDoInsertDockRow(m_panes,
                 paneInfo.GetDirection(),
                 paneInfo.GetLayer(),
                 paneInfo.GetRow());
            break;
        case wxAUI_INSERT_DOCK:
            wxAuiDoInsertDockLayer(m_panes,
                 paneInfo.GetDirection(),
                 paneInfo.GetLayer());
            break;
    }

    // if the window already exists, we are basically just moving/inserting the
    // existing window.  If it doesn't exist, we need to add it and insert it
    wxAuiPaneInfo& existingPane = GetPane(window);
    if (!existingPane.IsOk())
    {
        return AddPane(window, paneInfo);
    }
    else
    {
        if (paneInfo.IsFloating())
        {
            existingPane.Float();
            if (paneInfo.GetFloatingPosition() != wxDefaultPosition)
                existingPane.SetFloatingPosition(paneInfo.GetFloatingPosition());
            if (paneInfo.GetFloatingSize() != wxDefaultSize)
                existingPane.SetFloatingSize(paneInfo.GetFloatingSize());
        }
        else
        {
            // if the new pane is docked then we should undo maximize
            RestoreMaximizedPane();

            existingPane.SetDirection(paneInfo.GetDirection());
            existingPane.SetLayer(paneInfo.GetLayer());
            existingPane.SetRow(paneInfo.GetRow());
            existingPane.SetPosition(paneInfo.GetPosition());
        }
    }

    return true;
}


// DetachPane() removes a pane from the frame manager.  This
// method will not destroy the window that is removed.
bool wxAuiManager::DetachPane(wxWindow* window)
{
    wxASSERT_MSG(window, wxT("NULL window ptrs are not allowed"));

    int i, count;
    for (i = 0, count = m_panes.GetCount(); i < count; ++i)
    {
        wxAuiPaneInfo& p = m_panes.Item(i);
        if (p.GetWindow() == window)
        {
            if (p.GetFrame())
            {
                // we have a floating frame which is being detached. We need to
                // reparent it to m_frame and destroy the floating frame

                // reduce flicker
                p.GetWindow()->SetSize(1,1);

                if (p.GetFrame()->IsShown())
                   ShowWnd(p.GetFrame(),false);

                // reparent to m_frame and destroy the pane
                if (m_actionWindow == p.GetFrame())
                {
                    m_actionWindow = NULL;
                }

                p.GetWindow()->Reparent(m_frame);
                p.GetFrame()->SetSizer(NULL);
                p.GetFrame()->Destroy();
                p.SetFrame(NULL);
            }

            // make sure there are no references to this pane in our uiparts,
            // just in case the caller doesn't call Update() immediately after
            // the DetachPane() call.  This prevets obscure crashes which would
            // happen at window repaint if the caller forgets to call Update()
            int pi, partCount;
            for (pi = 0, partCount = (int)m_uiParts.GetCount(); pi < partCount; ++pi)
            {
                wxAuiDockUIPart& part = m_uiParts.Item(pi);
                if (part.pane == &p)
                {
                    m_uiParts.RemoveAt(pi);
                    partCount--;
                    pi--;
                    continue;
                }
            }

            m_panes.RemoveAt(i);
            return true;
        }
    }
    return false;
}






// ClosePane() destroys or hides the pane depending on its flags
bool wxAuiManager::ClosePane(wxAuiPaneInfo& paneInfo)
{
    // If we are a wxAuiNotebook then we must fire off a EVT_AUINOTEBOOK_PAGE_CLOSE event and give the user an opportunity to veto it.
    if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
    {
        wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, GetManagedWindow()->GetId());
        e.SetSelection(GetAllPanes().Index(paneInfo));
        e.SetEventObject(GetManagedWindow());
        GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
        if (!e.IsAllowed())
            return false;
    }

    // fire pane close event, allow opportunity for users to veto close
    wxAuiManagerEvent e(wxEVT_AUI_PANE_CLOSE);
    e.SetPane(&paneInfo);
    ProcessMgrEvent(e);
    if (e.GetVeto())
        return false;

    // if we were maximized, restore
    if (paneInfo.IsMaximized())
    {
        RestorePane(paneInfo);
    }

    // first, hide the window
    if (paneInfo.GetWindow() && paneInfo.GetWindow()->IsShown())
    {
        ShowWnd(paneInfo.GetWindow(),false);
    }

    // make sure that we are the parent of this window
    if (paneInfo.GetWindow() && paneInfo.GetWindow()->GetParent() != m_frame)
    {
        paneInfo.GetWindow()->Reparent(m_frame);
    }


    // if we have a frame, destroy it
    if (paneInfo.GetFrame())
    {
        paneInfo.GetFrame()->Destroy();
        paneInfo.SetFrame(NULL);
    }

    // now we need to either destroy or hide the pane
    if (paneInfo.IsDestroyOnClose())
    {
        wxWindow* window = paneInfo.GetWindow();
        DetachPane(window);
        if (window)
        {
            window->Destroy();
        }
    }
    else
    {
        paneInfo.Hide();
    }

    // If we are a wxAuiNotebook then we must fire off a EVT_AUINOTEBOOK_PAGE_CLOSED event to notify user of change.
    if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
    {
        wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSED, GetManagedWindow()->GetId());
        e.SetSelection(GetAllPanes().Index(paneInfo));
        e.SetEventObject(GetManagedWindow());
        GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
    }
    return true;
}

void wxAuiManager::MaximizePane(wxAuiPaneInfo& paneInfo)
{
    int i, paneCount;

    // un-maximize and hide all other panes
    for (i = 0, paneCount = m_panes.GetCount(); i < paneCount; ++i)
    {
        wxAuiPaneInfo& p = m_panes.Item(i);
        if (!p.IsToolbar() && !p.IsFloating())
        {
            p.Restore();

            // save hidden state
            p.SetFlag(wxAuiPaneInfo::savedHiddenState,
                      p.HasFlag(wxAuiPaneInfo::optionHidden));

            // hide the pane, because only the newly
            // maximized pane should show
            p.Hide();
        }
    }

    // mark ourselves maximized
    paneInfo.Maximize();
    paneInfo.Show();
    m_hasMaximized = true;

    // last, show the window
    if (paneInfo.GetWindow() && !paneInfo.GetWindow()->IsShown())
    {
        ShowWnd(paneInfo.GetWindow(),true);
    }
}

void wxAuiManager::RestorePane(wxAuiPaneInfo& paneInfo)
{
    int i, paneCount;

    // restore all the panes
    for (i = 0, paneCount = m_panes.GetCount(); i < paneCount; ++i)
    {
        wxAuiPaneInfo& p = m_panes.Item(i);
        if (!p.IsToolbar() && !p.IsFloating())
        {
            p.SetFlag(wxAuiPaneInfo::optionHidden,
                      p.HasFlag(wxAuiPaneInfo::savedHiddenState));
        }
    }

    // mark ourselves non-maximized
    paneInfo.Restore();
    m_hasMaximized = false;

    // last, show the window
    if (paneInfo.GetWindow() && !paneInfo.GetWindow()->IsShown())
    {
        ShowWnd(paneInfo.GetWindow(),true);
    }
}

void wxAuiManager::RestoreMaximizedPane()
{
    int i, paneCount;

    // restore all the panes
    for (i = 0, paneCount = m_panes.GetCount(); i < paneCount; ++i)
    {
        wxAuiPaneInfo& p = m_panes.Item(i);
        if (p.IsMaximized())
        {
            RestorePane(p);
            break;
        }
    }
}

// EscapeDelimiters() changes ";" into "\;" and "|" into "\|"
// in the input string.  This is an internal functions which is
// used for saving perspectives
static wxString EscapeDelimiters(const wxString& s)
{
    wxString result;
    result.Alloc(s.length());
    const wxChar* ch = s.c_str();
    while (*ch)
    {
        if (*ch == wxT(';') || *ch == wxT('|'))
            result += wxT('\\');
        result += *ch;
        ++ch;
    }
    return result;
}

wxString wxAuiPaneInfo::GetInfo() const
{
    wxString result = wxT("name=");
    result += EscapeDelimiters(m_name);
    result += wxT(";");

    result += wxT("caption=");
    result += EscapeDelimiters(m_caption);
    result += wxT(";");

    result += wxString::Format(wxT("state=%u;"), m_state);
    result += wxString::Format(wxT("dir=%d;"), m_dock_direction);
    result += wxString::Format(wxT("layer=%d;"), m_dock_layer);
    result += wxString::Format(wxT("row=%d;"), m_dock_row);
    result += wxString::Format(wxT("pos=%d;"), m_dock_pos);
    result += wxString::Format(wxT("prop=%d;"), m_dock_proportion);
    result += wxString::Format(wxT("page=%d"), m_dock_page);
    result += wxString::Format(wxT("bestw=%d;"), m_best_size.x);
    result += wxString::Format(wxT("besth=%d;"), m_best_size.y);
    result += wxString::Format(wxT("minw=%d;"), m_min_size.x);
    result += wxString::Format(wxT("minh=%d;"), m_min_size.y);
    result += wxString::Format(wxT("maxw=%d;"), m_max_size.x);
    result += wxString::Format(wxT("maxh=%d;"), m_max_size.y);
    result += wxString::Format(wxT("floatx=%d;"), m_floating_pos.x);
    result += wxString::Format(wxT("floaty=%d;"), m_floating_pos.y);
    result += wxString::Format(wxT("floatw=%d;"), m_floating_size.x);
    result += wxString::Format(wxT("floath=%d"), m_floating_size.y);
    return result;
}
wxString wxAuiManager::SavePaneInfo(wxAuiPaneInfo& pane)
{
    return pane.GetInfo();
}

void wxAuiPaneInfo::LoadInfo(wxString& info)
{
    // replace escaped characters so we can
    // split up the string easily
    info.Replace(wxT("\\|"), wxT("\a"));
    info.Replace(wxT("\\;"), wxT("\b"));

    while(1)
    {
        wxString valPart = info.BeforeFirst(wxT(';'));
        info = info.AfterFirst(wxT(';'));
        wxString valName = valPart.BeforeFirst(wxT('='));
        wxString value = valPart.AfterFirst(wxT('='));
        valName.MakeLower();
        valName.Trim(true);
        valName.Trim(false);
        value.Trim(true);
        value.Trim(false);

        if (valName.empty())
            break;

        if (valName == wxT("name"))
            m_name = value;
        else if (valName == wxT("caption"))
            m_caption = value;
        else if (valName == wxT("tooltip"))
            m_tooltip = value;
        else if (valName == wxT("state"))
            m_state = (unsigned int)wxAtoi(value.c_str());
        else if (valName == wxT("dir"))
            m_dock_direction = wxAtoi(value.c_str());
        else if (valName == wxT("layer"))
            m_dock_layer = wxAtoi(value.c_str());
        else if (valName == wxT("row"))
            m_dock_row = wxAtoi(value.c_str());
        else if (valName == wxT("pos"))
            m_dock_pos = wxAtoi(value.c_str());
        else if (valName == wxT("prop"))
            m_dock_proportion = wxAtoi(value.c_str());
        else if (valName == wxT("page"))
            m_dock_page = wxAtoi(value.c_str());
        else if (valName == wxT("bestw"))
            m_best_size.x = wxAtoi(value.c_str());
        else if (valName == wxT("besth"))
            m_best_size.y = wxAtoi(value.c_str());
        else if (valName == wxT("minw"))
            m_min_size.x = wxAtoi(value.c_str());
        else if (valName == wxT("minh"))
            m_min_size.y = wxAtoi(value.c_str());
        else if (valName == wxT("maxw"))
            m_max_size.x = wxAtoi(value.c_str());
        else if (valName == wxT("maxh"))
            m_max_size.y = wxAtoi(value.c_str());
        else if (valName == wxT("floatx"))
            m_floating_pos.x = wxAtoi(value.c_str());
        else if (valName == wxT("floaty"))
            m_floating_pos.y = wxAtoi(value.c_str());
        else if (valName == wxT("floatw"))
            m_floating_size.x = wxAtoi(value.c_str());
        else if (valName == wxT("floath"))
            m_floating_size.y = wxAtoi(value.c_str());
        else {
            wxFAIL_MSG(wxT("Bad Perspective String"));
        }
    }

    // replace escaped characters so we can
    // split up the string easily
    m_name.Replace(wxT("\a"), wxT("|"));
    m_name.Replace(wxT("\b"), wxT(";"));
    m_caption.Replace(wxT("\a"), wxT("|"));
    m_caption.Replace(wxT("\b"), wxT(";"));
    m_tooltip.Replace(wxT("\a"), wxT("|"));
    m_tooltip.Replace(wxT("\b"), wxT(";"));
    info.Replace(wxT("\a"), wxT("|"));
    info.Replace(wxT("\b"), wxT(";"));
}

// Load a "pane" with the pane infor settings in panePart
void wxAuiManager::LoadPaneInfo(wxString panePart, wxAuiPaneInfo &pane)
{
    pane.LoadInfo(panePart);

    return;
}


// SavePerspective() saves all pane information as a single string.
// This string may later be fed into LoadPerspective() to restore
// all pane settings.  This save and load mechanism allows an
// exact pane configuration to be saved and restored at a later time

wxString wxAuiManager::SavePerspective()
{
    wxString result;
    result.Alloc(500);
    result = wxT("layout2|");

    int paneIndex, paneCount = m_panes.GetCount();
    for (paneIndex = 0; paneIndex < paneCount; ++paneIndex)
    {
        wxAuiPaneInfo& pane = m_panes.Item(paneIndex);
        result += SavePaneInfo(pane)+wxT("|");
    }

    int dockIndex, dockCount = m_docks.GetCount();
    for (dockIndex = 0; dockIndex < dockCount; ++dockIndex)
    {
        wxAuiDockInfo& dock = m_docks.Item(dockIndex);

        result += wxString::Format(wxT("dock_size(%d,%d,%d)=%d|"),
                                   dock.dock_direction, dock.dock_layer,
                                   dock.dock_row, dock.size);
    }

    return result;
}

// LoadPerspective() loads a layout which was saved with SavePerspective()
// If the "update" flag parameter is true, the GUI will immediately be updated

bool wxAuiManager::LoadPerspective(const wxString& layout, bool update)
{
    wxString input = layout;
    wxString part;

    // check layout string version
    //    'layout1' = wxAUI 0.9.0 - wxAUI 0.9.2
    //    'layout2' = wxAUI 0.9.2 (wxWidgets 2.8)
    part = input.BeforeFirst(wxT('|'));
    input = input.AfterFirst(wxT('|'));
    part.Trim(true);
    part.Trim(false);
    if (part != wxT("layout2"))
        return false;

    // Mark all panes currently managed as hidden. Also, dock all panes that are dockable.
    int paneIndex, paneCount = m_panes.GetCount();
    for (paneIndex = 0; paneIndex < paneCount; ++paneIndex)
    {
        wxAuiPaneInfo& p = m_panes.Item(paneIndex);
        if(p.IsDockable())
            p.Dock();
        p.Hide();
    }

    // clear out the dock array; this will be reconstructed
    m_docks.Clear();

    // replace escaped characters so we can
    // split up the string easily
    input.Replace(wxT("\\|"), wxT("\a"));
    input.Replace(wxT("\\;"), wxT("\b"));

    m_hasMaximized = false;
    while (1)
    {
        wxAuiPaneInfo pane;

        wxString panePart = input.BeforeFirst(wxT('|'));
        input = input.AfterFirst(wxT('|'));
        panePart.Trim(true);

        // if the string is empty, we're done parsing
        if (panePart.empty())
            break;

        if (panePart.Left(9) == wxT("dock_size"))
        {
            wxString valName = panePart.BeforeFirst(wxT('='));
            wxString value = panePart.AfterFirst(wxT('='));

            long dir, layer, row, size;
            wxString piece = valName.AfterFirst(wxT('('));
            piece = piece.BeforeLast(wxT(')'));
            piece.BeforeFirst(wxT(',')).ToLong(&dir);
            piece = piece.AfterFirst(wxT(','));
            piece.BeforeFirst(wxT(',')).ToLong(&layer);
            piece.AfterFirst(wxT(',')).ToLong(&row);
            value.ToLong(&size);

            wxAuiDockInfo dock;
            dock.dock_direction = dir;
            dock.dock_layer = layer;
            dock.dock_row = row;
            dock.size = size;
            m_docks.Add(dock);
            continue;
        }

        // Undo our escaping as LoadPaneInfo needs to take an unescaped
        // name so it can be called by external callers
        panePart.Replace(wxT("\a"), wxT("|"));
        panePart.Replace(wxT("\b"), wxT(";"));

        LoadPaneInfo(panePart, pane);

        if ( pane.IsMaximized() )
            m_hasMaximized = true;

        wxAuiPaneInfo& p = GetPane(pane.GetName());
        if (!p.IsOk())
        {
            // the pane window couldn't be found
            // in the existing layout -- skip it
            continue;
        }

        p.SafeSet(pane);
    }

    if (update)
        Update();

    return true;
}

void wxAuiManager::GetPanePositionsAndSizes(wxAuiDockInfo& dock,
                                            wxArrayInt& positions,
                                            wxArrayInt& sizes)
{
    int captionSize = m_art->GetMetric(wxAUI_DOCKART_CAPTION_SIZE);
    int paneBorderSize = m_art->GetMetric(wxAUI_DOCKART_PANE_BORDER_SIZE);
    int gripperSize = m_art->GetMetric(wxAUI_DOCKART_GRIPPER_SIZE);

    positions.Empty();
    sizes.Empty();

    int offset, actionPane = -1;
    int paneIndex, paneCount = dock.panes.GetCount();

    // find the pane marked as our action pane
    for (paneIndex = 0; paneIndex < paneCount; ++paneIndex)
    {
        wxAuiPaneInfo& pane = *(dock.panes.Item(paneIndex));

        if (pane.HasFlag(wxAuiPaneInfo::actionPane))
        {
            wxASSERT_MSG(actionPane==-1, wxT("Too many fixed action panes"));
            actionPane = paneIndex;
        }
    }

    // set up each panes default position, and
    // determine the size (width or height, depending
    // on the dock's orientation) of each pane
    for (paneIndex = 0; paneIndex < paneCount; ++paneIndex)
    {
        wxAuiPaneInfo& pane = *(dock.panes.Item(paneIndex));
        positions.Add(pane.GetPosition());
        int size = 0;

        if (pane.HasBorder())
            size += (paneBorderSize*2);

        if (dock.IsHorizontal())
        {
            if (pane.HasGripper() && !pane.HasGripperTop())
                size += gripperSize;
            size += pane.GetBestSize().x;
        }
        else
        {
            if (pane.HasGripper() && pane.HasGripperTop())
                size += gripperSize;

            if (pane.HasCaption())
                size += captionSize;
            size += pane.GetBestSize().y;
        }

        sizes.Add(size);
    }

    // if there is no action pane, just return the default
    // positions (as specified in pane.pane_pos)
    if (actionPane == -1)
        return;

    offset = 0;
    for (paneIndex = actionPane-1; paneIndex >= 0; --paneIndex)
    {
        int amount = positions[paneIndex+1] - (positions[paneIndex] + sizes[paneIndex]);

        if (amount >= 0)
            offset += amount;
        else
            positions[paneIndex] -= -amount;

        offset += sizes[paneIndex];
    }

    // if the dock mode is fixed, make sure none of the panes
    // overlap; we will bump panes that overlap
    offset = 0;
    for (paneIndex = actionPane; paneIndex < paneCount; ++paneIndex)
    {
        int amount = positions[paneIndex] - offset;
        if (amount >= 0)
            offset += amount;
        else
            positions[paneIndex] += -amount;

        offset += sizes[paneIndex];
    }
}


void wxAuiManager::LayoutAddPane(wxSizer* cont, wxAuiDockInfo& dock, wxAuiPaneInfo& pane, wxAuiDockUIPartArray& uiparts, bool spacerOnly, bool allowtitlebar)
{
    wxAuiDockUIPart part;
    wxSizerItem* sizerItem;

    int captionSize = m_art->GetMetric(wxAUI_DOCKART_CAPTION_SIZE);
    int gripperSize = m_art->GetMetric(wxAUI_DOCKART_GRIPPER_SIZE);
    int paneBorderSize = m_art->GetMetric(wxAUI_DOCKART_PANE_BORDER_SIZE);
    int pane_button_size = m_art->GetMetric(wxAUI_DOCKART_PANE_BUTTON_SIZE);

    // find out the orientation of the item (orientation for panes
    // is the same as the dock's orientation)
    int orientation;
    if (dock.IsHorizontal())
        orientation = wxHORIZONTAL;
    else
        orientation = wxVERTICAL;

    // this variable will store the proportion
    // value that the pane will receive
    int paneProportion = pane.GetProportion();

    wxBoxSizer* horzPaneSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* vertPaneSizer = new wxBoxSizer(wxVERTICAL);

    if (allowtitlebar && pane.HasGripper())
    {
        if (pane.HasGripperTop())
            sizerItem = vertPaneSizer ->Add(1, gripperSize, 0, wxEXPAND);
        else
            sizerItem = horzPaneSizer ->Add(gripperSize, 1, 0, wxEXPAND);

        part.type = wxAuiDockUIPart::typeGripper;
        part.dock = &dock;
        part.pane = &pane;
        part.button = NULL;
        part.orientation = orientation;
        part.cont_sizer = horzPaneSizer;
        part.sizer_item = sizerItem;
        part.m_tab_container = NULL;
        uiparts.Add(part);
    }

    if (allowtitlebar && pane.HasCaption())
    {
        // create the caption sizer
        wxBoxSizer* captionSizer = new wxBoxSizer(wxHORIZONTAL);

        sizerItem = captionSizer->Add(1, captionSize, 1, wxEXPAND);

        part.type = wxAuiDockUIPart::typeCaption;
        part.dock = &dock;
        part.pane = &pane;
        part.button = NULL;
        part.orientation = orientation;
        part.cont_sizer = vertPaneSizer;
        part.sizer_item = sizerItem;
        part.m_tab_container = NULL;
        int captionPartIndex = uiparts.GetCount();
        uiparts.Add(part);

        // add pane buttons to the caption
        int i, buttonCount;
        for (i = 0, buttonCount = pane.GetButtons().GetCount();
             i < buttonCount; ++i)
        {
            wxAuiPaneButton& button = pane.GetButtons().Item(i);

            sizerItem = captionSizer->Add(pane_button_size,
                                            captionSize,
                                            0, wxEXPAND);

            part.type = wxAuiDockUIPart::typePaneButton;
            part.dock = &dock;
            part.pane = &pane;
            part.button = &button;
            part.orientation = orientation;
            part.cont_sizer = captionSizer;
            part.sizer_item = sizerItem;
            part.m_tab_container = NULL;
            uiparts.Add(part);
        }

        // if we have buttons, add a little space to the right
        // of them to ease visual crowding
        if (buttonCount >= 1)
        {
            captionSizer->Add(3,1);
        }

        // add the caption sizer
        sizerItem = vertPaneSizer->Add(captionSizer, 0, wxEXPAND);

        uiparts.Item(captionPartIndex).sizer_item = sizerItem;
    }

    // add the pane window itself
    if (spacerOnly)
    {
        sizerItem = vertPaneSizer->Add(1, 1, 1, wxEXPAND);
    }
    else
    {
        sizerItem = vertPaneSizer->Add(pane.GetWindow(), 1, wxEXPAND);
        // Don't do this because it breaks the pane size in floating windows
        // BIW: Right now commenting this out is causing problems with
        // an mdi client window as the center pane.
        vertPaneSizer->SetItemMinSize(pane.GetWindow(), 1, 1);
    }

    part.type = wxAuiDockUIPart::typePane;
    part.dock = &dock;
    part.pane = &pane;
    part.button = NULL;
    part.orientation = orientation;
    part.cont_sizer = vertPaneSizer;
    part.sizer_item = sizerItem;
    part.m_tab_container = NULL;
    uiparts.Add(part);


    // determine if the pane should have a minimum size; if the pane is
    // non-resizable (fixed) then we must set a minimum size. Alternatively,
    // if the pane.min_size is set, we must use that value as well

    wxSize minSize = pane.GetMinSize();
    if (pane.IsFixed())
    {
        if (minSize == wxDefaultSize)
        {
            minSize = pane.GetBestSize();
            paneProportion = 0;
        }
    }

    if (minSize != wxDefaultSize)
    {
        vertPaneSizer->SetItemMinSize(
                        vertPaneSizer->GetChildren().GetCount()-1,
                        minSize.x, minSize.y);
    }


    // add the verticle sizer (caption, pane window) to the
    // horizontal sizer (gripper, verticle sizer)
    horzPaneSizer->Add(vertPaneSizer, 1, wxEXPAND);

    // finally, add the pane sizer to the dock sizer

    if (pane.HasBorder())
    {
        // allowing space for the pane's border
        sizerItem = cont->Add(horzPaneSizer, paneProportion,
                               wxEXPAND | wxALL, paneBorderSize);

        part.type = wxAuiDockUIPart::typePaneBorder;
        part.dock = &dock;
        part.pane = &pane;
        part.button = NULL;
        part.orientation = orientation;
        part.cont_sizer = cont;
        part.sizer_item = sizerItem;
        part.m_tab_container = NULL;
        uiparts.Add(part);
    }
    else
    {
        sizerItem = cont->Add(horzPaneSizer, paneProportion, wxEXPAND);
    }
}

int wxAuiManager::GetNotebookFlags()
{
    int flags=0;
    if(HasFlag(wxAUI_MGR_NB_BOTTOM))
    {
        flags |= wxAUI_MGR_NB_BOTTOM;
    }
    else if(HasFlag(wxAUI_MGR_NB_LEFT))
    {
        flags |= wxAUI_MGR_NB_LEFT;
    }
    else if(HasFlag(wxAUI_MGR_NB_RIGHT))
    {
        flags |= wxAUI_MGR_NB_RIGHT;
    }
    else
    {
        flags |= wxAUI_MGR_NB_TOP;
    }

    if(HasFlag(wxAUI_MGR_NB_WINDOWLIST_BUTTON))
    {
        flags |= wxAUI_MGR_NB_WINDOWLIST_BUTTON;
    }

    if(HasFlag(wxAUI_MGR_NB_SCROLL_BUTTONS))
    {
        flags |= wxAUI_MGR_NB_SCROLL_BUTTONS;
    }

    if(HasFlag(wxAUI_MGR_NB_TAB_FIXED_WIDTH))
    {
        flags |= wxAUI_MGR_NB_TAB_FIXED_WIDTH;
    }


    if(HasFlag(wxAUI_MGR_NB_CLOSE_BUTTON))
    {
        flags |= wxAUI_MGR_NB_CLOSE_BUTTON;
    }
    else if(HasFlag(wxAUI_MGR_NB_CLOSE_ON_ACTIVE_TAB))
    {
        flags |= wxAUI_MGR_NB_CLOSE_ON_ACTIVE_TAB;
    }
    else if(HasFlag(wxAUI_MGR_NB_CLOSE_ON_ALL_TABS))
    {
        flags |= wxAUI_MGR_NB_CLOSE_ON_ALL_TABS;
    }

    return flags;
}


void wxAuiManager::LayoutAddNotebook(wxAuiTabArt* tabArt,wxAuiTabContainer* notebookContainer,wxSizer* notebookSizer,wxAuiDockUIPart& part,wxAuiDockInfo& dock,wxAuiDockUIPartArray& uiparts,wxAuiTabContainerPointerArray& tabContainerRecalcList,wxAuiSizerItemPointerArray& tabContainerRecalcSizers,wxAuiPaneInfo* pane,int orient)
{
    wxSizerItem* sizerItem;
    if(orient==wxHORIZONTAL)
    {
        //fixme: (MJM) This should be set via the art provider not hardcoded, temporary hardcoding while we iron out some issues with the tab art providers.
        sizerItem = notebookSizer->Add(m_art->GetMetric(wxAUI_DOCKART_SASH_SIZE), /*tabArt->m_tabCtrlHeight*/notebookTabHeight, 0, wxEXPAND);
        if(!HasFlag(wxAUI_MGR_NB_TAB_FIXED_HEIGHT))
        {
            tabContainerRecalcList.Add(notebookContainer);
            tabContainerRecalcSizers.Add(sizerItem);
        }
    }
    else
    {
        sizerItem = notebookSizer->Add(tabArt->m_fixedTabSize, m_art->GetMetric(wxAUI_DOCKART_SASH_SIZE), 0, wxEXPAND);
        if(!HasFlag(wxAUI_MGR_NB_TAB_FIXED_WIDTH))
        {
            tabContainerRecalcList.Add(notebookContainer);
            tabContainerRecalcSizers.Add(sizerItem);
        }
    }

    part.type = wxAuiDockUIPart::typePaneTab;
    part.dock = &dock;
    part.pane = pane;
    part.button = NULL;
    part.orientation = orient;
    part.cont_sizer = notebookSizer;
    part.sizer_item = sizerItem;
    part.m_tab_container = notebookContainer;
    uiparts.Add(part);
}

void wxAuiManager::LayoutAddDock(wxSizer* cont, wxAuiDockInfo& dock, wxAuiDockUIPartArray& uiparts, bool spacerOnly)
{
    wxSizerItem* sizerItem;
    wxAuiDockUIPart part;

    int sashSize = m_art->GetMetric(wxAUI_DOCKART_SASH_SIZE);
    int orientation = dock.IsHorizontal() ? wxHORIZONTAL : wxVERTICAL;

    // resizable bottom and right docks have a sash before them
    if (!m_hasMaximized && !dock.fixed && (dock.dock_direction == wxAUI_DOCK_BOTTOM ||
                        dock.dock_direction == wxAUI_DOCK_RIGHT))
    {
        sizerItem = cont->Add(sashSize, sashSize, 0, wxEXPAND);

        part.type = wxAuiDockUIPart::typeDockSizer;
        part.orientation = orientation;
        part.dock = &dock;
        part.pane = NULL;
        part.button = NULL;
        part.cont_sizer = cont;
        part.sizer_item = sizerItem;
        part.m_tab_container = NULL;
        uiparts.Add(part);
    }

    // create the sizer for the dock
    wxSizer* dockSizer = new wxBoxSizer(orientation);

    // add each pane to the dock
    bool hasMaximizedPane = false;
    int paneIndex, paneCount = dock.panes.GetCount();

    //For left/right notebooks (with horizontal tabs) we need to first know all tabs in notebook before we can calculate the width of the tab sizer, so we store them and then do the calculation at the end
    wxAuiTabContainerPointerArray tabContainerRecalcList;
    wxAuiSizerItemPointerArray tabContainerRecalcSizers;

    if (dock.fixed)
    {
        wxArrayInt panePositions, paneSizes;

        // figure out the real pane positions we will
        // use, without modifying the each pane's pane_pos member
        GetPanePositionsAndSizes(dock, panePositions, paneSizes);

        int offset = 0;

        // Variables to keep track of the first page of a notebook we encounter so that we can add further panes
        // to the same notebook.
        wxAuiPaneInfo* firstPaneInNotebook = NULL;
        wxAuiTabContainer* notebookContainer = NULL;
        wxSizer* notebookSizer = NULL;
        // Variable to keep track of whether an active page has been set for the current notebook, so that if we pass
        // an entire notebook without encountering an active page we can set the first page as the active one.
        bool activenotebookpagefound = false;

        for (paneIndex = 0; paneIndex < paneCount; ++paneIndex)
        {
            wxAuiPaneInfo& pane = *(dock.panes.Item(paneIndex));
            int panePos = panePositions.Item(paneIndex);

            if (pane.IsMaximized())
                hasMaximizedPane = true;

            if(firstPaneInNotebook && pane.GetPosition()==firstPaneInNotebook->GetPosition())
            {
                // This page is part of an existing notebook so add it to the container.
                // If it is the active page then it is visible, otherwise hide it.
                notebookContainer->AddPage(pane);
                if(pane.HasFlag(wxAuiPaneInfo::optionActiveNotebook))
                {
                    if(!activenotebookpagefound)
                    {
                        // Don't ever hide or show a window during hint calculation as this can affect display of windows other than the hint one.
                        if(!m_doingHintCalculation)
                            ShowWnd(pane.GetWindow(),true);
                        activenotebookpagefound = true;
                    }
                    else
                    {
                        // Only hide the window if it belongs to us.
                        // It might not belong to us if we are in the middle of a drop calculation for
                        // a floating frame, hiding it in this case would make the floating frame blank.
                        if(pane.GetWindow()->GetParent()==m_frame)
                        {
                            // Don't ever hide or show a window during hint calculation as this can affect display of windows other than the hint one.
                            if(!m_doingHintCalculation)
                                ShowWnd(pane.GetWindow(),false);
                        }
                        // Add a debug warning?
                    }
                }
                else
                {
                    // Only hide the window if it belongs to us.
                    // It might not belong to us if we are in the middle of a drop calculation for
                    // a floating frame, hiding it in this case would make the floating frame blank.
                    if(pane.GetWindow()->GetParent()==m_frame)
                    {
                        // Don't ever hide or show a window during hint calculation as this can affect display of windows other than the hint one.
                        if(!m_doingHintCalculation)
                            ShowWnd(pane.GetWindow(),false);
                    }
                }
                // If we are only doing a drop calculation then we only want the first
                // pane in the notebook and the hint window to be added as spacers are
                // not affected by the window being hidden and therefore make the hint
                // calculation incorrect.
                if(spacerOnly && pane.GetName()!= wxT("__HINT__"))
                    continue;
            }
            else
            {
                if(firstPaneInNotebook)
                {
                    // Right/Bottom notebook
                    if(HasFlag(wxAUI_MGR_NB_RIGHT))
                    {
                        LayoutAddNotebook(m_tab_art,notebookContainer,notebookSizer,part,dock,uiparts,tabContainerRecalcList,tabContainerRecalcSizers,firstPaneInNotebook,wxVERTICAL);
                    }
                    else if(HasFlag(wxAUI_MGR_NB_BOTTOM))
                    {
                        LayoutAddNotebook(m_tab_art,notebookContainer,notebookSizer,part,dock,uiparts,tabContainerRecalcList,tabContainerRecalcSizers,firstPaneInNotebook,wxHORIZONTAL);
                    }

                    if(!activenotebookpagefound)
                    {
                        // The previous page was a notebook that did not have an active page and we are not part of,
                        // set the first page in that notebook to be the active page.
                        // Don't ever hide or show a window during hint calculation as this can affect display of windows other than the hint one.
                        if(!m_doingHintCalculation)
                        {
                            firstPaneInNotebook->SetFlag(wxAuiPaneInfo::optionActiveNotebook,true);
                            ShowWnd(firstPaneInNotebook->GetWindow(),true);
                            activenotebookpagefound=true;
                        }
                    }
                }

                // If the next pane has the same position as us then we are the first page in a notebook.
                // Create a new notebook container and add it as a part.
                if(paneIndex<paneCount-1 && pane.GetPosition()==dock.panes.Item(paneIndex+1)->GetPosition())
                {
                    firstPaneInNotebook = &pane;
                    notebookContainer =  new wxAuiTabContainer(m_tab_art,this);
                    // Left/Right notebook
                    if(HasFlag(wxAUI_MGR_NB_LEFT)||HasFlag(wxAUI_MGR_NB_RIGHT))
                    {
                        notebookSizer = new wxBoxSizer(wxHORIZONTAL);
                    }
                    else
                    {
                        notebookSizer = new wxBoxSizer(wxVERTICAL);
                    }
                    notebookSizer = new wxBoxSizer(wxVERTICAL);
                    dockSizer->Add(notebookSizer, pane.GetProportion(), wxEXPAND);
                    int flags = GetNotebookFlags();
                    notebookContainer->SetFlags(flags);
                    notebookContainer->AddPage(pane);

                    // Top/Left Notebook
                    if(HasFlag(wxAUI_MGR_NB_LEFT))
                    {
                        LayoutAddNotebook(m_tab_art,notebookContainer,notebookSizer,part,dock,uiparts,tabContainerRecalcList,tabContainerRecalcSizers,firstPaneInNotebook,wxVERTICAL);
                    }
                    else if(!HasFlag(wxAUI_MGR_NB_RIGHT)&&!HasFlag(wxAUI_MGR_NB_BOTTOM))
                    {
                        LayoutAddNotebook(m_tab_art,notebookContainer,notebookSizer,part,dock,uiparts,tabContainerRecalcList,tabContainerRecalcSizers,firstPaneInNotebook,wxHORIZONTAL);
                    }

                    if(pane.HasFlag(wxAuiPaneInfo::optionActiveNotebook))
                    {
                        // Don't ever hide or show a window during hint calculation as this can affect display of windows other than the hint one.
                        if(!m_doingHintCalculation)
                            ShowWnd(pane.GetWindow(),true);
                        activenotebookpagefound = true;
                    }
                    else
                    {
                        activenotebookpagefound = false;
                        // Only hide the window if it belongs to us.
                        // It might not belong to us if we are in the middle of a drop calculation for
                        // a floating frame, hiding it in this case would make the floating frame blank.
                        if(pane.GetWindow()->GetParent()==m_frame)
                        {
                            // Don't ever hide or show a window during hint calculation as this can affect display of windows other than the hint one.
                            if(!m_doingHintCalculation)
                                ShowWnd(pane.GetWindow(),false);
                        }
                    }
                }
                else
                {
                    // We are a normal pane not part of a notebook so set the notebook tracking variables to NULL.
                    firstPaneInNotebook = NULL;
                    notebookContainer = NULL;
                    notebookSizer = NULL;
                }
            }

            int amount = panePos - offset;
            if (amount > 0)
            {
                if (dock.IsVertical())
                    sizerItem = dockSizer->Add(1, amount, 0, wxEXPAND);
                else
                    sizerItem = dockSizer->Add(amount, 1, 0, wxEXPAND);

                part.type = wxAuiDockUIPart::typeBackground;
                part.dock = &dock;
                part.pane = NULL;
                part.button = NULL;
                part.orientation = (orientation==wxHORIZONTAL) ? wxVERTICAL:wxHORIZONTAL;
                part.cont_sizer = dockSizer;
                part.sizer_item = sizerItem;
                part.m_tab_container = NULL;
                uiparts.Add(part);

                offset += amount;
            }

            LayoutAddPane(notebookSizer?notebookSizer:dockSizer, dock, pane, uiparts, spacerOnly, firstPaneInNotebook==NULL);

            offset += paneSizes.Item(paneIndex);
        }
        // Done adding panes, if the last pane we added was part of a notebook which didn't have an active page set
        // then set the first pange in that notebook to be the active page.
        if(firstPaneInNotebook)
        {
            // Right/Bottom notebook
            if(HasFlag(wxAUI_MGR_NB_RIGHT))
            {
                LayoutAddNotebook(m_tab_art,notebookContainer,notebookSizer,part,dock,uiparts,tabContainerRecalcList,tabContainerRecalcSizers,firstPaneInNotebook,wxVERTICAL);
            }
            else if(HasFlag(wxAUI_MGR_NB_BOTTOM))
            {
                LayoutAddNotebook(m_tab_art,notebookContainer,notebookSizer,part,dock,uiparts,tabContainerRecalcList,tabContainerRecalcSizers,firstPaneInNotebook,wxHORIZONTAL);
            }


            if(!activenotebookpagefound && firstPaneInNotebook)
            {
                // Don't ever hide or show a window during hint calculation as this can affect display of windows other than the hint one.
                if(!m_doingHintCalculation)
                {
                    firstPaneInNotebook->SetFlag(wxAuiPaneInfo::optionActiveNotebook,true);
                    ShowWnd(firstPaneInNotebook->GetWindow(),true);
                    activenotebookpagefound=true;
                }
            }
        }

        // at the end add a very small stretchable background area
        sizerItem = dockSizer->Add(0,0, 1, wxEXPAND);

        part.type = wxAuiDockUIPart::typeBackground;
        part.dock = &dock;
        part.pane = NULL;
        part.button = NULL;
        part.orientation = orientation;
        part.cont_sizer = dockSizer;
        part.sizer_item = sizerItem;
        part.m_tab_container = NULL;
        uiparts.Add(part);
    }
    else
    {
        // Variables to keep track of the first page of a notebook we encounter so that we can add further panes
        // to the same notebook.
        wxAuiPaneInfo* firstPaneInNotebook = NULL;
        wxAuiTabContainer* notebookContainer = NULL;
        wxSizer* notebookSizer = NULL;
        // Variable to keep track of whether an active page has been set for the current notebook, so that if we pass
        // an entire notebook without encountering an active page we can set the first page as the active one.
        bool activenotebookpagefound = false;

        for (paneIndex = 0; paneIndex < paneCount; ++paneIndex)
        {
            wxAuiPaneInfo& pane = *(dock.panes.Item(paneIndex));

            if (pane.IsMaximized())
                hasMaximizedPane = true;

            if(firstPaneInNotebook && pane.GetPosition()==firstPaneInNotebook->GetPosition())
            {
                // This page is part of an existing notebook so add it to the container.
                // If it is the active page then it is visible, otherwise hide it.
                notebookContainer->AddPage(pane);
                if(pane.HasFlag(wxAuiPaneInfo::optionActiveNotebook))
                {
                    if(!activenotebookpagefound)
                    {
                        // Don't ever hide or show a window during hint calculation as this can affect display of windows other than the hint one.
                        if(!m_doingHintCalculation)
                            ShowWnd(pane.GetWindow(),true);
                        activenotebookpagefound = true;
                    }
                    //else
                        // Add a debug warning
                }
                else
                {
                    // Only hide the window if it belongs to us.
                    // It might not belong to us if we are in the middle of a drop calculation for
                    // a floating frame, hiding it in this case would make the floating frame blank.
                    if(pane.GetWindow()->GetParent()==m_frame)
                    {
                        // Don't ever hide or show a window during hint calculation as this can affect display of windows other than the hint one.
                        if(!m_doingHintCalculation)
                            ShowWnd(pane.GetWindow(),false);
                    }
                }
                // If we are only doing a drop calculation then we only want the first
                // pane in the notebook and the hint window to be added as spacers are
                // not affected by the window being hidden and therefore make the hint
                // calculation incorrect.
                if(spacerOnly&&pane.GetName()!= wxT("__HINT__"))
                    continue;
            }
            else
            {
                if(firstPaneInNotebook)
                {
                    // Right/Bottom notebook
                    if(HasFlag(wxAUI_MGR_NB_RIGHT))
                    {
                        LayoutAddNotebook(m_tab_art,notebookContainer,notebookSizer,part,dock,uiparts,tabContainerRecalcList,tabContainerRecalcSizers,firstPaneInNotebook,wxVERTICAL);
                    }
                    else if(HasFlag(wxAUI_MGR_NB_BOTTOM))
                    {
                        LayoutAddNotebook(m_tab_art,notebookContainer,notebookSizer,part,dock,uiparts,tabContainerRecalcList,tabContainerRecalcSizers,firstPaneInNotebook,wxHORIZONTAL);
                    }


                    if(!activenotebookpagefound)
                    {
                        // The previous page was a notebook that did not have an active page and we are not part of,
                        // set the first page in that notebook to be the active page.
                        // Don't ever hide or show a window during hint calculation as this can affect display of windows other than the hint one.
                        if(!m_doingHintCalculation)
                        {
                            firstPaneInNotebook->SetFlag(wxAuiPaneInfo::optionActiveNotebook,true);
                            ShowWnd(firstPaneInNotebook->GetWindow(),true);
                            activenotebookpagefound=true;
                        }
                    }
                }

                // if this is not the first pane being added,
                // we need to add a pane sizer
                if (!m_hasMaximized && paneIndex > 0)
                {
                    sizerItem = dockSizer->Add(sashSize, sashSize, 0, wxEXPAND);

                    part.type = wxAuiDockUIPart::typePaneSizer;
                    part.dock = &dock;
                    part.pane = dock.panes.Item(paneIndex-1);
                    part.button = NULL;
                    part.orientation = (orientation==wxHORIZONTAL) ? wxVERTICAL:wxHORIZONTAL;
                    part.cont_sizer = dockSizer;
                    part.sizer_item = sizerItem;
                    part.m_tab_container = NULL;
                    uiparts.Add(part);
                }

                // If the next pane has the same position as us then we are the first page in a notebook.
                // Create a new notebook container and add it as a part.
                if(paneIndex<paneCount-1 && pane.GetPosition()==dock.panes.Item(paneIndex+1)->GetPosition())
                {
                    firstPaneInNotebook = &pane;
                    notebookContainer =  new wxAuiTabContainer(m_tab_art,this);
                    // Left/Right notebook
                    if(HasFlag(wxAUI_MGR_NB_LEFT)||HasFlag(wxAUI_MGR_NB_RIGHT))
                    {
                        notebookSizer = new wxBoxSizer(wxHORIZONTAL);
                    }
                    else
                    {
                        notebookSizer = new wxBoxSizer(wxVERTICAL);
                    }
                    dockSizer->Add(notebookSizer, pane.GetProportion(), wxEXPAND);
                    int flags= GetNotebookFlags();
                    notebookContainer->SetFlags(flags);
                    notebookContainer->AddPage(pane);

                    // Left/Top Notebook
                    if(HasFlag(wxAUI_MGR_NB_LEFT))
                    {
                        LayoutAddNotebook(m_tab_art,notebookContainer,notebookSizer,part,dock,uiparts,tabContainerRecalcList,tabContainerRecalcSizers,firstPaneInNotebook,wxVERTICAL);
                    }
                    else if(!HasFlag(wxAUI_MGR_NB_RIGHT)&&!HasFlag(wxAUI_MGR_NB_BOTTOM))
                    {
                        LayoutAddNotebook(m_tab_art,notebookContainer,notebookSizer,part,dock,uiparts,tabContainerRecalcList,tabContainerRecalcSizers,firstPaneInNotebook,wxHORIZONTAL);
                    }

                    if(pane.HasFlag(wxAuiPaneInfo::optionActiveNotebook))
                    {
                        // Don't ever hide or show a window during hint calculation as this can affect display of windows other than the hint one.
                        if(!m_doingHintCalculation)
                            ShowWnd(pane.GetWindow(),true);
                        activenotebookpagefound = true;
                    }
                    else
                    {
                        activenotebookpagefound = false;
                        // Only hide the window if it belongs to us.
                        // It might not belong to us if we are in the middle of a drop calculation for
                        // a floating frame, hiding it in this case would make the floating frame blank.
                        if(pane.GetWindow()->GetParent()==m_frame)
                        {
                            // Don't ever hide or show a window during hint calculation as this can affect display of windows other than the hint one.
                            if(!m_doingHintCalculation)
                                ShowWnd(pane.GetWindow(),false);
                        }
                    }
                }
                else
                {
                    // We are a normal pane not part of a notebook so set the notebook tracking variables to NULL.
                    firstPaneInNotebook = NULL;
                    notebookContainer = NULL;
                    notebookSizer = NULL;
                }
            }

            // Add the pane itself to the layout, if it is part of a notebook then don't allow it to have a title bar.
            LayoutAddPane(notebookSizer?notebookSizer:dockSizer, dock, pane, uiparts, spacerOnly, firstPaneInNotebook==NULL);
        }
        // Done adding panes, if the last pane we added was part of a notebook which didn't have an active page set
        // then set the first pange in that notebook to be the active page.
        if(firstPaneInNotebook)
        {
            // Right/Bottom notebook
            if(HasFlag(wxAUI_MGR_NB_RIGHT))
            {
                LayoutAddNotebook(m_tab_art,notebookContainer,notebookSizer,part,dock,uiparts,tabContainerRecalcList,tabContainerRecalcSizers,firstPaneInNotebook,wxVERTICAL);
            }
            else if(HasFlag(wxAUI_MGR_NB_BOTTOM))
            {
                LayoutAddNotebook(m_tab_art,notebookContainer,notebookSizer,part,dock,uiparts,tabContainerRecalcList,tabContainerRecalcSizers,firstPaneInNotebook,wxHORIZONTAL);
            }


            if(!activenotebookpagefound)
            {
                // The previous page was a notebook that did not have an active page and we are not part of,
                // set the first page in that notebook to be the active page.
                // Don't ever hide or show a window during hint calculation as this can affect display of windows other than the hint one.
                if(!m_doingHintCalculation)
                {
                    firstPaneInNotebook->SetFlag(wxAuiPaneInfo::optionActiveNotebook,true);
                    ShowWnd(firstPaneInNotebook->GetWindow(),true);
                    activenotebookpagefound=true;
                }
            }
        }
    }

    unsigned int recalcTabSizeIndex;
    for(recalcTabSizeIndex=0; recalcTabSizeIndex < tabContainerRecalcList.size(); recalcTabSizeIndex++)
    {
        wxSize tabSize = m_tab_art->GetBestTabSize(m_frame, tabContainerRecalcList[recalcTabSizeIndex]->GetPages(), wxSize(16,16));
        tabContainerRecalcSizers[recalcTabSizeIndex]->SetMinSize(tabSize);
    }

    if (dock.dock_direction == wxAUI_DOCK_CENTER || hasMaximizedPane)
        sizerItem = cont->Add(dockSizer, 1, wxEXPAND);
    else
        sizerItem = cont->Add(dockSizer, 0, wxEXPAND);

    part.type = wxAuiDockUIPart::typeDock;
    part.dock = &dock;
    part.pane = NULL;
    part.button = NULL;
    part.orientation = orientation;
    part.cont_sizer = cont;
    part.sizer_item = sizerItem;
    part.m_tab_container = NULL;
    uiparts.Add(part);

    if (dock.IsHorizontal())
        cont->SetItemMinSize(dockSizer, 0, dock.size);
    else
        cont->SetItemMinSize(dockSizer, dock.size, 0);

    //  top and left docks have a sash after them
    if (!m_hasMaximized &&
        !dock.fixed &&
          (dock.dock_direction == wxAUI_DOCK_TOP ||
           dock.dock_direction == wxAUI_DOCK_LEFT))
    {
        sizerItem = cont->Add(sashSize, sashSize, 0, wxEXPAND);

        part.type = wxAuiDockUIPart::typeDockSizer;
        part.dock = &dock;
        part.pane = NULL;
        part.button = NULL;
        part.orientation = orientation;
        part.cont_sizer = cont;
        part.sizer_item = sizerItem;
        part.m_tab_container = NULL;
        uiparts.Add(part);
    }
}

wxSizer* wxAuiManager::LayoutAll(wxAuiPaneInfoArray& panes,
                                 wxAuiDockInfoArray& docks,
                                 wxAuiDockUIPartArray& uiparts,
                                 bool spacerOnly)
{
    wxBoxSizer* container = new wxBoxSizer(wxVERTICAL);

    int paneBorderSize = m_art->GetMetric(wxAUI_DOCKART_PANE_BORDER_SIZE);
    int captionSize = m_art->GetMetric(wxAUI_DOCKART_CAPTION_SIZE);
    wxSize cliSize = m_frame->GetClientSize();
    int i, dockCount, paneCount;

    // empty all docks out
    for (i = 0, dockCount = docks.GetCount(); i < dockCount; ++i)
    {
        wxAuiDockInfo& dock = docks.Item(i);

        // empty out all panes, as they will be readded below
        dock.panes.Empty();

        if (dock.fixed)
        {
            // always reset fixed docks' sizes, because
            // the contained windows may have been resized
            dock.size = 0;
        }
    }


    // iterate through all known panes, filing each
    // of them into the appropriate dock. If the
    // pane does not exist in the dock, add it
    for (i = 0, paneCount = panes.GetCount(); i < paneCount; ++i)
    {
        wxAuiPaneInfo& p = panes.Item(i);

        // find any docks with the same dock direction, dock layer, and
        // dock row as the pane we are working on
        wxAuiDockInfo* dock;
        wxAuiDockInfoPtrArray arr;
        FindDocks(docks, p.GetDirection(), p.GetLayer(), p.GetRow(), arr);

        if (arr.GetCount() > 0)
        {
            // found the right dock
            dock = arr.Item(0);
        }
        else
        {
            // dock was not found, so we need to create a new one
            wxAuiDockInfo d;
            d.dock_direction = p.GetDirection();
            d.dock_layer = p.GetLayer();
            d.dock_row = p.GetRow();
            docks.Add(d);
            dock = &docks.Last();
        }


        if (p.IsDocked() && p.IsShown())
        {
            // remove the pane from any existing docks except this one
            RemovePaneFromDocks(docks, p, dock);

            // pane needs to be added to the dock,
            // if it doesn't already exist
            if (!FindPaneInDock(*dock, p.GetWindow()))
                dock->panes.Add(&p);
        }
        else
        {
            // remove the pane from any existing docks
            RemovePaneFromDocks(docks, p);
        }

    }

    // remove any empty docks
    for (i = docks.GetCount()-1; i >= 0; --i)
    {
        if (docks.Item(i).panes.GetCount() == 0)
            docks.RemoveAt(i);
    }

    // configure the docks further
    for (i = 0, dockCount = docks.GetCount(); i < dockCount; ++i)
    {
        wxAuiDockInfo& dock = docks.Item(i);
        int j, dockPaneCount = dock.panes.GetCount();

        // sort the dock pane array by the pane's
        // dock position (dock_pos), in ascending order
        dock.panes.Sort(DockPaneSortFunc);

        // for newly created docks, set up their initial size
        if (dock.size == 0)
        {
            int size = 0;

            for (j = 0; j < dockPaneCount; ++j)
            {
                wxAuiPaneInfo& pane = *dock.panes.Item(j);
                wxSize paneSize = pane.GetBestSize();
                if (paneSize == wxDefaultSize)
                    paneSize = pane.GetMinSize();
                if (paneSize == wxDefaultSize)
                    paneSize = pane.GetWindow()->GetSize();

                if (dock.IsHorizontal())
                    size = wxMax(paneSize.y, size);
                else
                    size = wxMax(paneSize.x, size);
            }

            // add space for the border (two times), but only
            // if at least one pane inside the dock has a pane border
            for (j = 0; j < dockPaneCount; ++j)
            {
                if (dock.panes.Item(j)->HasBorder())
                {
                    size += (paneBorderSize*2);
                    break;
                }
            }

            // if pane is on the top or bottom, add the caption height,
            // but only if at least one pane inside the dock has a caption
            if (dock.IsHorizontal())
            {
                for (j = 0; j < dockPaneCount; ++j)
                {
                    if (dock.panes.Item(j)->HasCaption())
                    {
                        size += captionSize;
                        break;
                    }
                }
            }


            // new dock's size may not be more than the dock constraint
            // parameter specifies.  See SetDockSizeConstraint()

            int maxDockXSize = (int)(m_dockConstraintX * ((double)cliSize.x));
            int maxDockYSize = (int)(m_dockConstraintY * ((double)cliSize.y));

            if (dock.IsHorizontal())
                size = wxMin(size, maxDockYSize);
            else
                size = wxMin(size, maxDockXSize);

            // absolute minimum size for a dock is 10 pixels
            if (size < 10)
                size = 10;

            dock.size = size;
        }


        // determine the dock's minimum size
        bool plusBorder = false;
        bool plusCaption = false;
        int dockMinSize = 0;
        for (j = 0; j < dockPaneCount; ++j)
        {
            wxAuiPaneInfo& pane = *dock.panes.Item(j);
            if (pane.GetMinSize() != wxDefaultSize)
            {
                if (pane.HasBorder())
                    plusBorder = true;
                if (pane.HasCaption())
                    plusCaption = true;
                if (dock.IsHorizontal())
                {
                    if (pane.GetMinSize().y > dockMinSize)
                        dockMinSize = pane.GetMinSize().y;
                }
                else
                {
                    if (pane.GetMinSize().x > dockMinSize)
                        dockMinSize = pane.GetMinSize().x;
                }
            }
        }

        if (plusBorder)
            dockMinSize += (paneBorderSize*2);
        if (plusCaption && dock.IsHorizontal())
            dockMinSize += (captionSize);

        dock.min_size = dockMinSize;


        // if the pane's current size is less than its
        // minimum, increase the dock's size to its minimum
        if (dock.size < dock.min_size)
            dock.size = dock.min_size;


        // determine the dock's mode (fixed or proportional);
        // determine whether the dock has only toolbars
        bool actionPaneMarked = false;
        dock.fixed = true;
        dock.toolbar = true;
        for (j = 0; j < dockPaneCount; ++j)
        {
            wxAuiPaneInfo& pane = *dock.panes.Item(j);
            if (!pane.IsFixed())
                dock.fixed = false;
            if (!pane.IsToolbar())
                dock.toolbar = false;
            if (pane.HasFlag(wxAuiPaneInfo::optionDockFixed))
                dock.fixed = true;
            if (pane.HasFlag(wxAuiPaneInfo::actionPane))
                actionPaneMarked = true;
        }


        // If the dock mode is proportional and not fixed-pixel,
        // reassign the dock_pos to the sequential 0, 1, 2, 3;
        // e.g. remove gaps like 1, 2, 30, 500
        // If panes currently share a position they must continue to do so
        // e.g. 1, 5, 5, 9 -> 0, 1, 1, 2
        if (!dock.fixed)
        {
            int i=0;
            int lastposition=-1;
            for (j = 0; j < dockPaneCount; ++j)
            {
                wxAuiPaneInfo& pane = *dock.panes.Item(j);
                if(pane.GetPosition()!=lastposition)
                    ++i;
                lastposition=pane.GetPosition();
                pane.SetPosition(i);
            }
        }

        // If the dock mode is fixed, and none of the panes
        // are being moved right now, make sure the panes
        // do not overlap each other.  If they do, we will
        // adjust the positions of the panes
        // If panes share an identical position they must continue to do so as they are then part of a notebook.
        if (dock.fixed && !actionPaneMarked)
        {
            wxArrayInt panePositions, paneSizes;
            GetPanePositionsAndSizes(dock, panePositions, paneSizes);

            int offset = 0;
            int lastposition=-1;
            for (j = 0; j < dockPaneCount; ++j)
            {
                wxAuiPaneInfo& pane = *(dock.panes.Item(j));
                if(pane.GetPosition()==lastposition)
                {
                    pane.SetPosition(dock.panes.Item(j-1)->GetPosition());
                    continue;
                }
                lastposition = pane.GetPosition();
                pane.SetPosition(panePositions[j]);

                int amount = pane.GetPosition() - offset;
                if (amount >= 0)
                    offset += amount;
                else
                    pane.SetPosition(pane.GetPosition()+(-amount));

                offset += paneSizes[j];
            }
        }
    }

    // discover the maximum dock layer
    int maxLayer = 0;
    for (i = 0; i < dockCount; ++i)
        maxLayer = wxMax(maxLayer, docks.Item(i).dock_layer);


    // clear out uiparts
    uiparts.Empty();

    // create a bunch of box sizers,
    // from the innermost level outwards.
    wxSizer* cont = NULL;
    wxSizer* middle = NULL;
    int layer = 0;
    int row, rowCount;

    for (layer = 0; layer <= maxLayer; ++layer)
    {
        wxAuiDockInfoPtrArray arr;

        // find any docks in this layer
        FindDocks(docks, -1, layer, -1, arr);

        // if there aren't any, skip to the next layer
        if (arr.IsEmpty())
            continue;

        wxSizer* oldCont = cont;

        // create a container which will hold this layer's
        // docks (top, bottom, left, right)
        cont = new wxBoxSizer(wxVERTICAL);


        // find any top docks in this layer
        FindDocks(docks, wxAUI_DOCK_TOP, layer, -1, arr);
        if (!arr.IsEmpty())
        {
            for (row = 0, rowCount = arr.GetCount(); row < rowCount; ++row)
                LayoutAddDock(cont, *arr.Item(row), uiparts, spacerOnly);
        }


        // fill out the middle layer (which consists
        // of left docks, content area and right docks)

        middle = new wxBoxSizer(wxHORIZONTAL);

        // find any left docks in this layer
        FindDocks(docks, wxAUI_DOCK_LEFT, layer, -1, arr);
        if (!arr.IsEmpty())
        {
            for (row = 0, rowCount = arr.GetCount(); row < rowCount; ++row)
                LayoutAddDock(middle, *arr.Item(row), uiparts, spacerOnly);
        }

        // add content dock (or previous layer's sizer
        // to the middle
        if (!oldCont)
        {
            // find any center docks
            FindDocks(docks, wxAUI_DOCK_CENTER, -1, -1, arr);
            if (!arr.IsEmpty())
            {
                for (row = 0,rowCount = arr.GetCount(); row<rowCount; ++row)
                   LayoutAddDock(middle, *arr.Item(row), uiparts, spacerOnly);
            }
            else if (!m_hasMaximized)
            {
                // there are no center docks, add a background area
                wxSizerItem* sizerItem = middle->Add(1,1, 1, wxEXPAND);
                wxAuiDockUIPart part;
                part.type = wxAuiDockUIPart::typeBackground;
                part.pane = NULL;
                part.dock = NULL;
                part.button = NULL;
                part.cont_sizer = middle;
                part.sizer_item = sizerItem;
                part.m_tab_container = NULL;
                uiparts.Add(part);
            }
        }
        else
        {
            middle->Add(oldCont, 1, wxEXPAND);
        }

        // find any right docks in this layer
        FindDocks(docks, wxAUI_DOCK_RIGHT, layer, -1, arr);
        if (!arr.IsEmpty())
        {
            for (row = arr.GetCount()-1; row >= 0; --row)
                LayoutAddDock(middle, *arr.Item(row), uiparts, spacerOnly);
        }

        if (middle->GetChildren().GetCount() > 0)
            cont->Add(middle, 1, wxEXPAND);
             else
            delete middle;



        // find any bottom docks in this layer
        FindDocks(docks, wxAUI_DOCK_BOTTOM, layer, -1, arr);
        if (!arr.IsEmpty())
        {
            for (row = arr.GetCount()-1; row >= 0; --row)
                LayoutAddDock(cont, *arr.Item(row), uiparts, spacerOnly);
        }

    }

    if (!cont)
    {
        // no sizer available, because there are no docks,
        // therefore we will create a simple background area
        cont = new wxBoxSizer(wxVERTICAL);
        wxSizerItem* sizerItem = cont->Add(1,1, 1, wxEXPAND);
        wxAuiDockUIPart part;
        part.type = wxAuiDockUIPart::typeBackground;
        part.pane = NULL;
        part.dock = NULL;
        part.button = NULL;
        part.cont_sizer = middle;
        part.sizer_item = sizerItem;
        part.m_tab_container = NULL;
        uiparts.Add(part);
    }

    container->Add(cont, 1, wxEXPAND);
    return container;
}


// SetDockSizeConstraint() allows the dock constraints to be set.  For example,
// specifying values of 0.5, 0.5 will mean that upon dock creation, a dock may
// not be larger than half of the window's size

void wxAuiManager::SetDockSizeConstraint(double width_pct, double height_pct)
{
    m_dockConstraintX = wxMax(0.0, wxMin(1.0, width_pct));
    m_dockConstraintY = wxMax(0.0, wxMin(1.0, height_pct));
}

void wxAuiManager::GetDockSizeConstraint(double* width_pct, double* height_pct) const
{
    if (width_pct)
        *width_pct = m_dockConstraintX;

    if (height_pct)
        *height_pct = m_dockConstraintY;
}


// Used inside Update to store the tab offset of previous notebooks so that they can be restored if a notebook
// with the same hash still exists. This prevents tab offsets from resetting every time we drag a pane for example.
WX_DECLARE_STRING_HASH_MAP( int, NotebookOffsetHash );

// Update() updates the layout.  Whenever changes are made to
// one or more panes, this function should be called.  It is the
// external entry point for running the layout engine.
void wxAuiManager::Update()
{
    m_hoverButton = NULL;
    m_actionPart = NULL;
    delete m_actionDeadZone;
    m_actionDeadZone = NULL;

    // Ensure pane array is always sorted before doing layout - otherwise we can have odd results
    m_panes.Sort(PaneSortFunc);

    wxSizer* sizer;
    int i, paneCount = m_panes.GetCount();


    // destroy floating panes which have been
    // redocked or are becoming non-floating
    for (i = 0; i < paneCount; ++i)
    {
        wxAuiPaneInfo& p = m_panes.Item(i);

        if (!p.IsFloating() && p.GetFrame())
        {
            // because the pane is no longer in a floating, we need to
            // reparent it to m_frame and destroy the floating frame

            // reduce flicker
            p.GetWindow()->SetSize(1,1);


            // the following block is a workaround for bug #1531361
            // (see wxWidgets sourceforge page).  On wxGTK (only), when
            // a frame is shown/hidden, a move event unfortunately
            // also gets fired.  Because we may be dragging around
            // a pane, we need to cancel that action here to prevent
            // a spurious crash.
            if (m_actionWindow == p.GetFrame())
            {
                if (wxWindow::GetCapture() == m_frame)
                    m_frame->ReleaseMouse();
                m_action = actionNone;
                m_actionWindow = NULL;
            }

            // hide the frame
            if (p.GetFrame()->IsShown())
                ShowWnd(p.GetFrame(),false);

            // reparent to m_frame and destroy the pane
            if (m_actionWindow == p.GetFrame())
            {
                m_actionWindow = NULL;
            }

            p.GetWindow()->Reparent(m_frame);
            p.GetFrame()->SetSizer(NULL);
            p.GetFrame()->Destroy();
            p.SetFrame(NULL);
        }
    }


    // delete old sizer first
    m_frame->SetSizer(NULL);


    // hide or show panes as necessary,
    // and float panes as necessary
    for (i = 0; i < paneCount; ++i)
    {
        wxAuiPaneInfo& p = m_panes.Item(i);

        if (p.IsFloating())
        {
            if (p.GetFrame() == NULL)
            {
                // we need to create a frame for this
                // pane, which has recently been floated
                wxAuiFloatingFrame* frame = CreateFloatingFrame(m_frame, p);

                // on MSW and Mac, if the owner desires transparent dragging, and
                // the dragging is happening right now, then the floating
                // window should have this style by default
                if (m_action == actionDragFloatingPane && HasFlag(wxAUI_MGR_TRANSPARENT_DRAG))
                        frame->SetTransparent(150);

                frame->SetPaneWindow(p);
                if(p.GetDirection()==wxAUI_DOCK_CENTER)
                    p.SetCaptionVisible(true);
                p.SetFrame(frame);

                if (p.IsShown() && !frame->IsShown())
                    ShowWnd(frame,true);
            }
            else
            {
                // frame already exists, make sure its position
                // and size reflect the information in wxAuiPaneInfo
                if ((p.GetFrame()->GetPosition() != p.GetFloatingPosition())
                    || (p.GetFrame()->GetSize() != p.GetFloatingSize()))
                {
                    p.GetFrame()->SetSize(p.GetFloatingPosition().x, p.GetFloatingPosition().y,
                                     p.GetFloatingSize().x, p.GetFloatingSize().y,
                                     wxSIZE_USE_EXISTING);
                /*
                    p.frame->SetSize(p.floating_pos.x, p.floating_pos.y,
                                     wxDefaultCoord, wxDefaultCoord,
                                     wxSIZE_USE_EXISTING);
                    //p.frame->Move(p.floating_pos.x, p.floating_pos.y);
                */
                }

                if (p.GetFrame()->IsShown() != p.IsShown())
                    ShowWnd(p.GetFrame(),p.IsShown());

                // update whether the pane is resizable or not
                long style = p.frame->GetWindowStyleFlag();
                if (p.IsFixed())
                    style &= ~wxRESIZE_BORDER;
                else
                    style |= wxRESIZE_BORDER;
                p.frame->SetWindowStyleFlag(style);

                if (p.frame->GetLabel() != p.caption)
                    p.frame->SetLabel(p.caption);

            }
        }
        else
        {
            if (p.GetWindow()->IsShown() != p.IsShown())
            {
                // Only reveal windows that are not part of a notebook, notebook windows will be taken care of inside LayoutAll()
                // Doing so here as well can cause flicker(especially under MSW) due to windows being shown here and then hidden inside LayoutAll()
                // All windows must be hidden here though, even notebook ones.
                if(!p.IsShown())
                    ShowWnd(p.GetWindow(),false);
                else if(!IsNotebookPane(m_panes,i))
                {
                    ShowWnd(p.GetWindow(),true);
                }
            }
        }

        // if "active panes" are no longer allowed, clear
        // any optionActive values from the pane states
        if (!HasFlag(wxAUI_MGR_ALLOW_ACTIVE_PANE) == 0)
        {
            p.SetFlag(wxAuiPaneInfo::optionActive,false);
        }
    }
    // We have to do the hiding and showing of panes before we call LayoutAll
    // As LayoutAll may wany to hide frames even though they are technically "visible"
    // If they are in a notebook.

    // cache the offset positions for any notebooks we have, so that if we are just resizing a dock for example our notebook tabs don't jump around.
    NotebookOffsetHash cachednotebookoffsets;
    // cache the notebook that has the current focus so that it can keep the focus when doing a resize for example.
    wxString focusnotebook = wxT("");
    int uiPartsCount = m_uiParts.GetCount();
    for (i = 0; i < uiPartsCount; i++)
    {
        wxAuiDockUIPart& part = m_uiParts.Item(i);
        if(part.m_tab_container)
        {
            wxAuiPaneInfo &pane = part.m_tab_container->GetPage(0);
            wxString notebookpositionhash;
            notebookpositionhash << wxString::Format(wxT("%d;"), pane.GetDirection());
            notebookpositionhash << wxString::Format(wxT("%d;"), pane.GetPosition());
            notebookpositionhash << wxString::Format(wxT("%d;"), pane.GetRow());
            notebookpositionhash << wxString::Format(wxT("%d;"), pane.GetLayer());
            cachednotebookoffsets[notebookpositionhash] = part.m_tab_container->GetTabOffset();
            if(part.m_tab_container->HasFocus())
            {
                focusnotebook = notebookpositionhash;
            }
        }
    }


    // create a layout for all of the panes
    sizer = LayoutAll(m_panes, m_docks, m_uiParts, false);

    // restore the offset positions for any notebooks we have, so that if we are just resizing a dock for example our notebook tabs don't jump around.
    uiPartsCount = m_uiParts.GetCount();
    for (i = 0; i < uiPartsCount; i++)
    {
        wxAuiDockUIPart& part = m_uiParts.Item(i);
        if(part.m_tab_container)
        {
            wxAuiPaneInfo &pane = part.m_tab_container->GetPage(0);
            wxString notebookpositionhash;
            notebookpositionhash << wxString::Format(wxT("%d;"), pane.GetDirection());
            notebookpositionhash << wxString::Format(wxT("%d;"), pane.GetPosition());
            notebookpositionhash << wxString::Format(wxT("%d;"), pane.GetRow());
            notebookpositionhash << wxString::Format(wxT("%d;"), pane.GetLayer());
            if(cachednotebookoffsets.find(notebookpositionhash)!=cachednotebookoffsets.end())
            {
                int oldtaboffset = cachednotebookoffsets[notebookpositionhash];
                int numtabs = part.m_tab_container->GetPageCount();
                int newtaboffset;

                // If we have removed tabs then the offset might be greater then the number of tabs in which case we set it to the number of tabs
                // Otherwise we set it to the old offset
                if(oldtaboffset>numtabs)
                    newtaboffset = numtabs;
                else
                    newtaboffset = oldtaboffset;

                part.m_tab_container->SetTabOffset(newtaboffset);

                // restore the focus to the focused notebook, if there was one
                if(focusnotebook==notebookpositionhash)
                {
                    part.m_tab_container->SetFocus(true);
                }
            }
        }
    }

    // keep track of the old window rectangles so we can
    // refresh those windows whose rect has changed
    wxAuiRectArray oldPaneRects;
    for (i = 0; i < paneCount; ++i)
    {
        wxRect r;
        wxAuiPaneInfo& p = m_panes.Item(i);

        if (p.GetWindow() && p.IsShown() && p.IsDocked())
            r = p.GetRect();

        oldPaneRects.Add(r);
    }




    // apply the new sizer
    m_frame->SetSizer(sizer);
    m_frame->SetAutoLayout(false);
    DoFrameLayout();



    // now that the frame layout is done, we need to check
    // the new pane rectangles against the old rectangles that
    // we saved a few lines above here.  If the rectangles have
    // changed, the corresponding panes must also be updated
    for (i = 0; i < paneCount; ++i)
    {
        wxAuiPaneInfo& p = m_panes.Item(i);
        if (p.GetWindow() && p.GetWindow()->IsShown() && p.IsDocked())
        {
            if (p.GetRect() != oldPaneRects[i])
            {
                p.GetWindow()->Refresh();
                p.GetWindow()->Update();
            }
        }
    }

    Repaint();

    // set frame's minimum size

#if 0
    // N.B. More work needs to be done on frame minimum sizes;
    // this is some intresting code that imposes the minimum size,
    // but we may want to include a more flexible mechanism or
    // options for multiple minimum-size modes, e.g. strict or lax
    wxSize minSize = sizer->GetMinSize();
    wxSize frameSize = m_frame->GetSize();
    wxSize clientSize = m_frame->GetClientSize();

    wxSize minframe_size(minSize.x+frameSize.x-clientSize.x,
                         minSize.y+frameSize.y-clientSize.y );

    m_frame->SetMinSize(minframeSize);

    if (frameSize.x < minframeSize.x ||
        frameSize.y < minframeSize.y)
            sizer->Fit(m_frame);
#endif
}


// DoFrameLayout() is an internal function which invokes wxSizer::Layout
// on the frame's main sizer, then measures all the various UI items
// and updates their internal rectangles.  This should always be called
// instead of calling m_frame->Layout() directly

void wxAuiManager::DoFrameLayout()
{
    m_frame->Layout();

    int i, partCount;
    for (i = 0, partCount = m_uiParts.GetCount(); i < partCount; ++i)
    {
        wxAuiDockUIPart& part = m_uiParts.Item(i);

        // get the rectangle of the UI part
        // originally, this code looked like this:
        //    part.rect = wxRect(part.sizer_item->GetPosition(),
        //                       part.sizer_item->GetSize());
        // this worked quite well, with one exception: the mdi
        // client window had a "deferred" size variable
        // that returned the wrong size.  It looks like
        // a bug in wx, because the former size of the window
        // was being returned.  So, we will retrieve the part's
        // rectangle via other means


        part.rect = part.sizer_item->GetRect();
        int flag = part.sizer_item->GetFlag();
        int border = part.sizer_item->GetBorder();
        if (flag & wxTOP)
        {
            part.rect.y -= border;
            part.rect.height += border;
        }
        if (flag & wxLEFT)
        {
            part.rect.x -= border;
            part.rect.width += border;
        }
        if (flag & wxBOTTOM)
            part.rect.height += border;
        if (flag & wxRIGHT)
            part.rect.width += border;


        if (part.type == wxAuiDockUIPart::typeDock)
            part.dock->rect = part.rect;
        if (part.type == wxAuiDockUIPart::typePane)
            part.pane->SetRect(part.rect);
    }
}

// GetPanePart() looks up the pane the pane border UI part (or the regular
// pane part if there is no border). This allows the caller to get the exact
// rectangle of the pane in question, including decorations like
// caption and border (if any).

wxAuiDockUIPart* wxAuiManager::GetPanePart(wxWindow* wnd)
{
    int i, partCount;
    for (i = 0, partCount = m_uiParts.GetCount(); i < partCount; ++i)
    {
        wxAuiDockUIPart& part = m_uiParts.Item(i);
        if (part.type == wxAuiDockUIPart::typePaneBorder &&
            part.pane && part.pane->GetWindow() == wnd)
                return &part;
    }
    for (i = 0, partCount = m_uiParts.GetCount(); i < partCount; ++i)
    {
        wxAuiDockUIPart& part = m_uiParts.Item(i);
        if (part.type == wxAuiDockUIPart::typePane &&
            part.pane && part.pane->GetWindow() == wnd)
                return &part;
    }
    return NULL;
}



// GetDockPixelOffset() is an internal function which returns
// a dock's offset in pixels from the left side of the window
// (for horizontal docks) or from the top of the window (for
// vertical docks).  This value is necessary for calculating
// fixel-pane/toolbar offsets when they are dragged.
int wxAuiManager::GetDockPixelOffset(wxAuiPaneInfo& test)
{
    // the only way to accurately calculate the dock's
    // offset is to actually run a theoretical layout

    int i, partCount, dockCount;
    wxAuiDockInfoArray docks;
    wxAuiPaneInfoArray panes;
    wxAuiDockUIPartArray uiparts;
    CopyDocksAndPanes(docks, panes, m_docks, m_panes);
    panes.Add(test);

    wxSizer* sizer = LayoutAll(panes, docks, uiparts, true);
    wxSize clientSize = m_frame->GetClientSize();
    sizer->SetDimension(0, 0, clientSize.x, clientSize.y);
    sizer->Layout();

    for (i = 0, partCount = uiparts.GetCount(); i < partCount; ++i)
    {
        wxAuiDockUIPart& part = uiparts.Item(i);
        part.rect = wxRect(part.sizer_item->GetPosition(),
                           part.sizer_item->GetSize());
        if (part.type == wxAuiDockUIPart::typeDock)
            part.dock->rect = part.rect;
    }

    delete sizer;

    for (i = 0, dockCount = docks.GetCount(); i < dockCount; ++i)
    {
        wxAuiDockInfo& dock = docks.Item(i);
        if (test.GetDirection() == dock.dock_direction &&
            test.GetLayer()==dock.dock_layer && test.GetRow()==dock.dock_row)
        {
            if (dock.IsVertical())
                return dock.rect.y;
            else
                return dock.rect.x;
        }
    }

    return 0;
}



// ProcessDockResult() is a utility function used by DoDrop() - it checks
// if a dock operation is allowed, the new dock position is copied into
// the target info.  If the operation was allowed, the function returns true.
bool wxAuiManager::ProcessDockResult(wxAuiPaneInfo& target, const wxAuiPaneInfo& newPos)
{
    bool allowed = false;
    switch (newPos.GetDirection())
    {
        case wxAUI_DOCK_TOP:    allowed = target.IsTopDockable();    break;
        case wxAUI_DOCK_BOTTOM: allowed = target.IsBottomDockable(); break;
        case wxAUI_DOCK_LEFT:   allowed = target.IsLeftDockable();   break;
        case wxAUI_DOCK_RIGHT:  allowed = target.IsRightDockable();  break;
        //fixme: (MJM) - The below should perhaps be determined by a new option/method?
        case wxAUI_DOCK_CENTER: allowed = target.IsTopDockable()||target.IsBottomDockable()||target.IsLeftDockable()||target.IsRightDockable(); break;
    }

    if (allowed)
    {
        target = newPos;
        // Should this RTTI and function call be rewritten as
        // sending a new event type to allow other window types
        // to vary size based on dock location?
        wxAuiToolBar* toolbar = wxDynamicCast(target.window, wxAuiToolBar);
        if (toolbar)
        {
            wxSize hintSize = toolbar->GetHintSize(target.dock_direction);
            if (target.best_size != hintSize)
            {
                target.best_size = hintSize;
                target.floating_size = wxDefaultSize;
            }
        }
    }

    return allowed;
}


// DoDrop() is an important function.  It basically takes a mouse position,
// and determines where the pane's new position would be.  If the pane is to be
// dropped, it performs the drop operation using the specified dock and pane
// arrays.  By specifying copied dock and pane arrays when calling, a "what-if"
// scenario can be performed, giving precise coordinates for drop hints.
// If, however, wxAuiManager:m_docks and wxAuiManager::m_panes are specified
// as parameters, the changes will be made to the main state arrays

const int auiInsertRowPixels = 10;
const int auiNewRowPixels = 40;
const int auiLayerInsertPixels = 40;
const int auiLayerInsertOffset = 5;

bool wxAuiManager::DoDrop(wxAuiDockInfoArray& docks, wxAuiPaneInfoArray& panes, wxAuiPaneInfo& target, const wxPoint& pt, const wxPoint& offset)
{
    wxSize cliSize = m_frame->GetClientSize();

    wxAuiPaneInfo drop = target;


    // The result should always be shown
    drop.Show();

    wxAuiDockUIPart* part = HitTest(pt.x, pt.y);

    // Check to see if the pane has been dragged outside of the window
    // (or near to the outside of the window), if so, dock it along the edge
    // (Don't do this if busy dragging in a notebook)
    if(!part||part->m_tab_container==NULL)
    {
        int layerInsertOffset = auiLayerInsertOffset;
        if (drop.IsToolbar())
             layerInsertOffset = 0;


        if (pt.x < layerInsertOffset && pt.x > layerInsertOffset-auiLayerInsertPixels && pt.y > 0 && pt.y < cliSize.y)
        {
            int newLayer = wxMax(wxMax(GetMaxLayer(docks, wxAUI_DOCK_LEFT), GetMaxLayer(docks, wxAUI_DOCK_BOTTOM)), GetMaxLayer(docks, wxAUI_DOCK_TOP)) + 1;

            if (drop.IsToolbar())
                newLayer = auiToolBarLayer;

            drop.Dock().SetDirectionLeft().SetLayer(newLayer).SetRow(0).SetPosition(pt.y - GetDockPixelOffset(drop) - offset.y);
            return ProcessDockResult(target, drop);
        }
        else if (pt.y < layerInsertOffset && pt.y > layerInsertOffset-auiLayerInsertPixels && pt.x > 0 && pt.x < cliSize.x)
        {
            int newLayer = wxMax(wxMax(GetMaxLayer(docks, wxAUI_DOCK_TOP), GetMaxLayer(docks, wxAUI_DOCK_LEFT)), GetMaxLayer(docks, wxAUI_DOCK_RIGHT)) + 1;

            if (drop.IsToolbar())
                newLayer = auiToolBarLayer;

            drop.Dock().SetDirectionTop().SetLayer(newLayer).SetRow(0).SetPosition(pt.x - GetDockPixelOffset(drop) - offset.x);
            return ProcessDockResult(target, drop);
        }
        else if (pt.x >= cliSize.x - layerInsertOffset && pt.x < cliSize.x - layerInsertOffset + auiLayerInsertPixels && pt.y > 0 && pt.y < cliSize.y)
        {
            int newLayer = wxMax(wxMax(GetMaxLayer(docks, wxAUI_DOCK_RIGHT), GetMaxLayer(docks, wxAUI_DOCK_TOP)), GetMaxLayer(docks, wxAUI_DOCK_BOTTOM)) + 1;

            if (drop.IsToolbar())
                newLayer = auiToolBarLayer;

            drop.Dock().SetDirectionRight().SetLayer(newLayer).SetRow(0).SetPosition(pt.y - GetDockPixelOffset(drop) - offset.y);
            return ProcessDockResult(target, drop);
        }
        else if (pt.y >= cliSize.y - layerInsertOffset && pt.y < cliSize.y - layerInsertOffset + auiLayerInsertPixels && pt.x > 0 && pt.x < cliSize.x)
        {
            int newLayer = wxMax( wxMax( GetMaxLayer(docks, wxAUI_DOCK_BOTTOM), GetMaxLayer(docks, wxAUI_DOCK_LEFT)), GetMaxLayer(docks, wxAUI_DOCK_RIGHT)) + 1;

            if (drop.IsToolbar())
                newLayer = auiToolBarLayer;

            drop.Dock().SetDirectionBottom().SetLayer(newLayer).SetRow(0).SetPosition(pt.x - GetDockPixelOffset(drop) - offset.x);
            return ProcessDockResult(target, drop);
        }
    }


    if (drop.IsToolbar())
    {
        if (!part || !part->dock)
            return false;

        // calculate the offset from where the dock begins
        // to the point where the user dropped the pane
        int dockDropOffset = 0;
        if (part->dock->IsHorizontal())
            dockDropOffset = pt.x - part->dock->rect.x - offset.x;
        else
            dockDropOffset = pt.y - part->dock->rect.y - offset.y;


        // toolbars may only be moved in and to fixed-pane docks,
        // otherwise we will try to float the pane.  Also, the pane
        // should float if being dragged over center pane windows
        if (!part->dock->fixed || part->dock->dock_direction == wxAUI_DOCK_CENTER || pt.x >= cliSize.x || pt.x <= 0 || pt.y >= cliSize.y || pt.y <= 0)
        {
            if (m_lastRect.IsEmpty() || m_lastRect.Contains(pt.x, pt.y ))
            {
                m_skipping = true;
            }
            else
            {
                if (HasFlag(wxAUI_MGR_ALLOW_FLOATING) && drop.IsFloatable())
                {
                    drop.Float();
                }

                m_skipping = false;

                return ProcessDockResult(target, drop);
            }

            drop.SetPosition(pt.x - GetDockPixelOffset(drop) - offset.x);

            return ProcessDockResult(target, drop);
        }

        m_skipping = false;

        m_lastRect = part->dock->rect;
        m_lastRect.Inflate( 15, 15 );

        drop.Dock().SetDirection(part->dock->dock_direction).SetLayer(part->dock->dock_layer).SetRow(part->dock->dock_row).SetPosition(dockDropOffset);

        if ((
            ((pt.y < part->dock->rect.y + 1) && part->dock->IsHorizontal()) ||
            ((pt.x < part->dock->rect.x + 1) && part->dock->IsVertical())
            ) && part->dock->panes.GetCount() > 1)
        {
            if ((part->dock->dock_direction == wxAUI_DOCK_TOP) || (part->dock->dock_direction == wxAUI_DOCK_LEFT))
            {
                int row = drop.GetRow();
                wxAuiDoInsertDockRow(panes, part->dock->dock_direction, part->dock->dock_layer, part->dock->dock_row);
                drop.SetRow(row);
            }
            else
            {
                wxAuiDoInsertDockRow(panes, part->dock->dock_direction, part->dock->dock_layer, part->dock->dock_row+1);
                drop.SetRow(part->dock->dock_row+1);
            }
        }

        if ((
            ((pt.y > part->dock->rect.y + part->dock->rect.height - 2 ) && part->dock->IsHorizontal()) ||
            ((pt.x > part->dock->rect.x + part->dock->rect.width - 2 ) && part->dock->IsVertical())
            ) && part->dock->panes.GetCount() > 1)
        {
            if ((part->dock->dock_direction == wxAUI_DOCK_TOP) || (part->dock->dock_direction == wxAUI_DOCK_LEFT))
            {
                wxAuiDoInsertDockRow(panes, part->dock->dock_direction, part->dock->dock_layer, part->dock->dock_row+1);
                drop.SetRow(part->dock->dock_row+1);
            }
            else
            {
                int row = drop.GetRow();
                wxAuiDoInsertDockRow(panes, part->dock->dock_direction, part->dock->dock_layer, part->dock->dock_row);
                drop.SetRow(row);
            }
        }

        return ProcessDockResult(target, drop);
    }




    if (!part)
        return false;



    if (part->type == wxAuiDockUIPart::typePaneTab)
    {
        // Figure out if the pane is already part of the notebook, which will happen when dragging a tab in a notebook.
        int isAlreadyInNotebook = part->m_tab_container->GetPages().Index(&drop);

        // Check if tab moving is allowed
        if(isAlreadyInNotebook && !HasFlag(wxAUI_MGR_NB_TAB_MOVE))
        {
            return false;
        }

        wxAuiPaneInfo* hitPane=NULL;
        int page;
        // If we are above a tab then insert before it, otherwise insert at the end
        if(!part->m_tab_container->TabHitTest(pt.x,pt.y,&hitPane))
        {
            if( part->m_tab_container->TabHitTest(pt.x+10,pt.y,&hitPane) || part->m_tab_container->TabHitTest(pt.x+10,pt.y+10,&hitPane) || part->m_tab_container->TabHitTest(pt.x,pt.y+10,&hitPane) )
            {
                //Before first tab - don't do anything
                return false;
            }
            else
            {
                // Insert at end.
                hitPane = &part->m_tab_container->GetPage(part->m_tab_container->GetPageCount()-1);
                page = hitPane->GetPage()+1;
            }
        }
        else
        {
            // Insert after pane we are hovering over.
            if( isAlreadyInNotebook && drop.GetPage()<hitPane->GetPage() )
            {
                page = hitPane->GetPage()+1;
            }
            else
            {
                // Insert before pane we are hovering over.
                page = hitPane->GetPage();
            }
        }


        wxAuiDoInsertPage(panes, hitPane->GetDirection(), hitPane->GetLayer(), hitPane->GetRow(), hitPane->GetPosition(), page);
        drop.Dock().SetDirection(hitPane->GetDirection()).SetLayer(hitPane->GetLayer()).SetRow(hitPane->GetRow()).SetPosition(hitPane->GetPosition()).SetPage(page);
        return ProcessDockResult(target, drop);
    }
    else if(part->type == wxAuiDockUIPart::typeCaption)
    {
        drop.Dock().SetDirection(part->pane->GetDirection()).SetLayer(part->pane->GetLayer()).SetRow(part->pane->GetRow()).SetPosition(part->pane->GetPosition()).SetPage(part->pane->GetPage()+1);
        return ProcessDockResult(target, drop);
    }
    else if (part->type == wxAuiDockUIPart::typePaneBorder || part->type == wxAuiDockUIPart::typeGripper || part->type == wxAuiDockUIPart::typePaneButton || part->type == wxAuiDockUIPart::typePane || part->type == wxAuiDockUIPart::typePaneSizer || part->type == wxAuiDockUIPart::typeDockSizer || part->type == wxAuiDockUIPart::typeBackground)
    {
        if (part->type == wxAuiDockUIPart::typeDockSizer)
        {
            if (part->dock->panes.GetCount() != 1)
                return false;
            part = GetPanePart(part->dock->panes.Item(0)->GetWindow());
            if (!part)
                return false;
        }



        // If a normal frame is being dragged over a toolbar, insert it
        // along the edge under the toolbar, but over all other panes.
        // (this could be done much better, but somehow factoring this
        // calculation with the one at the beginning of this function)
        if (part->dock && part->dock->toolbar)
        {
            int layer = 0;

            switch (part->dock->dock_direction)
            {
                case wxAUI_DOCK_LEFT:
                    layer = wxMax(wxMax(GetMaxLayer(docks, wxAUI_DOCK_LEFT), GetMaxLayer(docks, wxAUI_DOCK_BOTTOM)), GetMaxLayer(docks, wxAUI_DOCK_TOP));
                    break;
                case wxAUI_DOCK_TOP:
                    layer = wxMax(wxMax(GetMaxLayer(docks, wxAUI_DOCK_TOP), GetMaxLayer(docks, wxAUI_DOCK_LEFT)), GetMaxLayer(docks, wxAUI_DOCK_RIGHT));
                    break;
                case wxAUI_DOCK_RIGHT:
                    layer = wxMax(wxMax(GetMaxLayer(docks, wxAUI_DOCK_RIGHT), GetMaxLayer(docks, wxAUI_DOCK_TOP)), GetMaxLayer(docks, wxAUI_DOCK_BOTTOM));
                    break;
                case wxAUI_DOCK_BOTTOM:
                    layer = wxMax(wxMax(GetMaxLayer(docks, wxAUI_DOCK_BOTTOM), GetMaxLayer(docks, wxAUI_DOCK_LEFT)), GetMaxLayer(docks, wxAUI_DOCK_RIGHT));
                    break;
            }

            wxAuiDoInsertDockRow(panes, part->dock->dock_direction, layer, 0);
            drop.Dock(). SetDirection(part->dock->dock_direction). SetLayer(layer).SetRow(0).SetPosition(0);
            return ProcessDockResult(target, drop);
        }


        if (!part->pane)
            return false;

        part = GetPanePart(part->pane->GetWindow());
        if (!part)
            return false;

        bool insertDockRow = false;
        int insertRow = part->pane->GetRow();
        int insertDir = part->pane->GetDirection();
        int insertLayer = part->pane->GetLayer();

        switch (part->pane->GetDirection())
        {
            case wxAUI_DOCK_TOP:
                if (pt.y >= part->rect.y &&
                    pt.y < part->rect.y+auiInsertRowPixels)
                        insertDockRow = true;
                break;
            case wxAUI_DOCK_BOTTOM:
                if (pt.y > part->rect.y+part->rect.height-auiInsertRowPixels &&
                    pt.y <= part->rect.y + part->rect.height)
                        insertDockRow = true;
                break;
            case wxAUI_DOCK_LEFT:
                if (pt.x >= part->rect.x &&
                    pt.x < part->rect.x+auiInsertRowPixels)
                        insertDockRow = true;
                break;
            case wxAUI_DOCK_RIGHT:
                if (pt.x > part->rect.x+part->rect.width-auiInsertRowPixels &&
                    pt.x <= part->rect.x+part->rect.width)
                        insertDockRow = true;
                break;
            case wxAUI_DOCK_CENTER:
            {
                // "new row pixels" will be set to the default, but
                // must never exceed 20% of the window size
                int newRowPixelsX = auiNewRowPixels;
                int newRowPixelsY = auiNewRowPixels;

                if (newRowPixelsX > (part->rect.width*20)/100)
                    newRowPixelsX = (part->rect.width*20)/100;

                if (newRowPixelsY > (part->rect.height*20)/100)
                    newRowPixelsY = (part->rect.height*20)/100;


                // determine if the mouse pointer is in a location that
                // will cause a new row to be inserted.  The hot spot positions
                // are along the borders of the center pane

                insertLayer = 0;
                insertDockRow = true;
                const wxRect& pr = part->rect;
                if (pt.x >= pr.x && pt.x < pr.x + newRowPixelsX)
                    insertDir = wxAUI_DOCK_LEFT;
                else if (pt.y >= pr.y && pt.y < pr.y + newRowPixelsY)
                    insertDir = wxAUI_DOCK_TOP;
                else if (pt.x >= pr.x + pr.width - newRowPixelsX && pt.x < pr.x + pr.width)
                    insertDir = wxAUI_DOCK_RIGHT;
                else if (pt.y >= pr.y+ pr.height - newRowPixelsY && pt.y < pr.y + pr.height)
                    insertDir = wxAUI_DOCK_BOTTOM;
                else
                    return false;

                insertRow = GetMaxRow(panes, insertDir, insertLayer) + 1;
            }
        }

        if (insertDockRow)
        {
            wxAuiDoInsertDockRow(panes, insertDir, insertLayer, insertRow);
            drop.Dock().SetDirection(insertDir).SetLayer(insertLayer).SetRow(insertRow).SetPosition(0);
            return ProcessDockResult(target, drop);
        }

        // determine the mouse offset and the pane size, both in the
        // direction of the dock itself, and perpendicular to the dock

        int offset, size;

        if (part->orientation == wxVERTICAL)
        {
            offset = pt.y - part->rect.y;
            size = part->rect.GetHeight();
        }
        else
        {
            offset = pt.x - part->rect.x;
            size = part->rect.GetWidth();
        }

        int dropPosition = part->pane->GetPosition();

        // if we are in the top/left part of the pane,
        // insert the pane before the pane being hovered over
        if (offset <= size/2)
        {
            dropPosition = part->pane->GetPosition();
            wxAuiDoInsertPane(panes, part->pane->GetDirection(), part->pane->GetLayer(), part->pane->GetRow(), part->pane->GetPosition());
        }

        // if we are in the bottom/right part of the pane,
        // insert the pane before the pane being hovered over
        if (offset > size/2)
        {
            dropPosition = part->pane->GetPosition()+1;
            wxAuiDoInsertPane(panes, part->pane->GetDirection(), part->pane->GetLayer(), part->pane->GetRow(), part->pane->GetPosition()+1);
        }

        drop.Dock().SetDirection(part->dock->dock_direction).SetLayer(part->dock->dock_layer).SetRow(part->dock->dock_row).SetPosition(dropPosition);
        return ProcessDockResult(target, drop);
    }

    return false;
}


bool wxAuiManager::DoDropExternal(wxAuiPaneInfo& drop, const wxPoint& pt, const wxPoint& actionOffset)
{
    if(HasFlag(wxAUI_MGR_ALLOW_EXTERNAL_MOVE))
    {
        wxAuiDockUIPart* part = HitTest(pt.x, pt.y);
        if(!part)
        {
            wxPoint screenPt = ::wxGetMousePosition();
            wxWindow* targetCtrl = ::wxFindWindowAtPoint(screenPt);
            // If we are on top of a hint window then quickly hide the hint window and get the window that is underneath it.
            if(targetCtrl==m_hintWnd)
            {
                m_hintWnd->Hide();
                targetCtrl = ::wxFindWindowAtPoint(screenPt);
                m_hintWnd->Show();
            }

            // Find the manager this window belongs to (if it does belong to one)
            wxAuiManager* otherMgr = NULL;
            while(targetCtrl)
            {
                if(!wxDynamicCast(targetCtrl,wxAuiFloatingFrame))
                {
                    if(targetCtrl->GetEventHandler() && wxDynamicCast(targetCtrl->GetEventHandler(),wxAuiManager))
                    {
                        otherMgr = ((wxAuiManager*)targetCtrl->GetEventHandler());
                        break;
                    }
                }
                targetCtrl = targetCtrl->GetParent();
            }
            // Alert other manager of the drop and have it show hint.
            if(otherMgr && otherMgr != this)
            {
                // find out from the destination control if it's ok to drop this tab here.
                if(wxDynamicCast(targetCtrl,wxAuiNotebook))
                {
                    wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_ALLOW_DND, GetManagedWindow()->GetId());
                    int selIndex=GetAllPanes().Index(drop);
                    e.SetSelection(selIndex);
                    e.SetOldSelection(selIndex);
                    e.SetEventObject((wxAuiNotebook*)GetManagedWindow());
                    e.SetDragSource((wxAuiNotebook*)GetManagedWindow());
                    e.Veto(); // dropping must be explicitly approved by control owner

                    targetCtrl->GetEventHandler()->ProcessEvent(e);

                    if (!e.IsAllowed())
                    {
                        // no answer or negative answer
                        return false;
                    }
                }
                else
                {
                    wxAuiManagerEvent e(wxEVT_AUI_ALLOW_DND);
                    e.SetManager(this);
                    e.SetPane(&drop);
                    e.Veto(); // dropping must be explicitly approved by control owner
                    targetCtrl->GetEventHandler()->ProcessEvent(e);
                    if(e.GetVeto())
                    {
                        // no answer or negative answer
                        return false;
                    }
                }

                wxAuiPaneInfo dropExternal=drop;
                dropExternal.SetFlag(wxAuiPaneInfo::optionActive,false);
                dropExternal.SetFlag(wxAuiPaneInfo::optionActiveNotebook,false);

                otherMgr->AddPane(dropExternal.GetWindow(),dropExternal);
                wxPoint clientPt = otherMgr->GetManagedWindow()->ScreenToClient(screenPt);
                if(otherMgr->DoDrop(otherMgr->m_docks, otherMgr->m_panes, dropExternal, clientPt, actionOffset))
                {
                    DetachPane(drop.GetWindow());
                    drop.GetWindow()->Reparent(targetCtrl);

                    //fixme: (MJM) The below (Double update) is inefficient and can probably be replaced with a better mechanism.
                    // Ensure active before doing actual display.
                    otherMgr->SetActivePane(drop.GetWindow());

                    // Update the layout to realize new position and e.g. form notebooks if needed.
                    otherMgr->Update();

                    // If a notebook formed we may have lost our active status so set it again.
                    otherMgr->SetActivePane(drop.GetWindow());

                    // Update once again so that notebook can reflect our new active status.
                    otherMgr->Update();

                    // Make changes visible to user.
                    otherMgr->Repaint();

                    return true;
                }
                else
                {
                    otherMgr->DetachPane(drop.GetWindow());
                    return false;
                }
            }
        }
    }
    return false;
}

void wxAuiManager::OnHintFadeTimer(wxTimerEvent& WXUNUSED(evt))
{
    if (!m_hintWnd || m_hintFadeAmt >= m_hintFadeMax)
    {
        m_hintFadeTimer.Stop();
        Disconnect(m_hintFadeTimer.GetId(), wxEVT_TIMER, wxTimerEventHandler(wxAuiManager::OnHintFadeTimer));
        return;
    }

    m_hintFadeAmt += 4;
    m_hintWnd->SetTransparent(m_hintFadeAmt);
}

void wxAuiManager::ShowHint(const wxRect& rect)
{
    if (m_hintWnd)
    {
        // if the hint rect is the same as last time, don't do anything
        if (m_lastHint == rect)
            return;
        m_lastHint = rect;

        m_hintFadeAmt = m_hintFadeMax;

        if ( HasFlag(wxAUI_MGR_HINT_FADE) && !( wxDynamicCast(m_hintWnd,wxPseudoTransparentFrame) && HasFlag(wxAUI_MGR_NO_VENETIAN_BLINDS_FADE)) )
            m_hintFadeAmt = 0;

        m_hintWnd->SetSize(rect);
        m_hintWnd->SetTransparent(m_hintFadeAmt);

        if (!m_hintWnd->IsShown())
            ShowWnd(m_hintWnd,true);

        // if we are dragging a floating pane, set the focus
        // back to that floating pane (otherwise it becomes unfocused)
        if (m_action == actionDragFloatingPane && m_actionWindow)
            m_actionWindow->SetFocus();

        m_hintWnd->Raise();


        if (m_hintFadeAmt != m_hintFadeMax) //  Only fade if we need to
        {
            // start fade in timer
            m_hintFadeTimer.SetOwner(this);
            m_hintFadeTimer.Start(5);
            Connect(m_hintFadeTimer.GetId(), wxEVT_TIMER, wxTimerEventHandler(wxAuiManager::OnHintFadeTimer));
        }
    }
    else  // Not using a transparent hint window...
    {
        if (!HasFlag(wxAUI_MGR_RECTANGLE_HINT))
            return;

        if (m_lastHint != rect)
        {
            // remove the last hint rectangle
            m_lastHint = rect;
            m_frame->Refresh();
            m_frame->Update();
        }

        wxScreenDC screendc;
        wxRegion clip(1, 1, 10000, 10000);

        // clip all floating windows, so we don't draw over them
        int i, paneCount;
        for (i = 0, paneCount = m_panes.GetCount(); i < paneCount; ++i)
        {
            wxAuiPaneInfo& pane = m_panes.Item(i);

            if(pane.IsFloating() && pane.GetFrame() && pane.GetFrame()->IsShown())
            {
                wxRect rect = pane.GetFrame()->GetRect();
#ifdef __WXGTK__
                // wxGTK returns the client size, not the whole frame size
                rect.width += 15;
                rect.height += 35;
                rect.Inflate(5);
#endif

                clip.Subtract(rect);
            }
        }

        // As we can only hide the hint by redrawing the managed window, we
        // need to clip the region to the managed window too or we get
        // nasty redrawn problems.
        clip.Intersect(m_frame->GetRect());

        screendc.SetDeviceClippingRegion(clip);

        wxBitmap stipple = wxPaneCreateStippleBitmap();
        wxBrush brush(stipple);
        screendc.SetBrush(brush);
        screendc.SetPen(*wxTRANSPARENT_PEN);

        screendc.DrawRectangle(rect.x, rect.y, 5, rect.height);
        screendc.DrawRectangle(rect.x+5, rect.y, rect.width-10, 5);
        screendc.DrawRectangle(rect.x+rect.width-5, rect.y, 5, rect.height);
        screendc.DrawRectangle(rect.x+5, rect.y+rect.height-5, rect.width-10, 5);
    }
}

void wxAuiManager::HideHint()
{
    // hides a transparent window hint, if there is one
    if (m_hintWnd)
    {
        if (m_hintWnd->IsShown())
            ShowWnd(m_hintWnd,false);
        m_hintWnd->SetTransparent(0);
        m_hintFadeTimer.Stop();
        // In case this is called while a hint fade is going, we need to
        // disconnect the event handler.
        Disconnect(m_hintFadeTimer.GetId(), wxEVT_TIMER, wxTimerEventHandler(wxAuiManager::OnHintFadeTimer));
        m_lastHint = wxRect();
        return;
    }

    // hides a painted hint by redrawing the frame window
    if (!m_lastHint.IsEmpty())
    {
        m_frame->Refresh();
        m_frame->Update();
        m_lastHint = wxRect();
    }
}

void wxAuiManager::OnHintActivate(wxActivateEvent& WXUNUSED(evt))
{
    // Do nothing so this event isn't handled in the base handlers.

    // Letting the hint window activate without this handler can lead to
    // weird behaviour on Mac where the menu is switched out to the top
    // window's menu in MDI applications when it shouldn't be. So since
    // we don't want user interaction with the hint window anyway, we just
    // prevent it from activating here.
}



void wxAuiManager::StartPaneDrag(wxWindow* paneWindow, const wxPoint& offset)
{
    wxAuiPaneInfo& pane = GetPane(paneWindow);
    if (!pane.IsOk())
        return;

    if (pane.IsToolbar())
    {
        m_action = actionDragToolbarPane;
    }
    else
    {
        m_action = actionDragFloatingPane;
    }

    m_actionWindow = paneWindow;
    m_actionOffset = offset;
    m_frame->CaptureMouse();

    if (pane.frame)
    {
        wxRect windowRect = pane.frame->GetRect();
        wxRect clientRect = pane.frame->GetClientRect();
        wxPoint clientPt = pane.frame->ClientToScreen(clientRect.GetTopLeft());
        wxPoint originPt = clientPt - windowRect.GetTopLeft();
        m_actionOffset += originPt;
    }
}


// CalculateHintRect() calculates the drop hint rectangle.  The method
// first calls DoDrop() to determine the exact position the pane would
// be at were if dropped.  If the pane would indeed become docked at the
// specified drop point, the rectangle hint will be returned in
// screen coordinates.  Otherwise, an empty rectangle is returned.
// |paneWindow| is the window pointer of the pane being dragged, |pt| is
// the mouse position, in client coordinates.  |offset| describes the offset
// that the mouse is from the upper-left corner of the item being dragged

wxRect wxAuiManager::CalculateHintRect(wxWindow* paneWindow, const wxPoint& pt, const wxPoint& offset)
{
    wxRect rect;
    m_doingHintCalculation = true;

    // we need to paint a hint rectangle; to find out the exact hint rectangle,
    // we will create a new temporary layout and then measure the resulting
    // rectangle; we will create a copy of the docking structures (m_dock)
    // so that we don't modify the real thing on screen

    int i, paneCount, partCount;
    wxAuiDockInfoArray docks;
    wxAuiPaneInfoArray panes;
    wxAuiDockUIPartArray uiparts;
    wxAuiPaneInfo hint = GetPane(paneWindow);
    hint.SetName(wxT("__HINT__"));
    hint.SetBorder(true);
    hint.Show();

    if (!hint.IsOk())
    {
        m_doingHintCalculation = false;
        return rect;
    }

    CopyDocksAndPanes(docks, panes, m_docks, m_panes);

    // remove any pane already there which bears the same window;
    // this happens when you are moving a pane around in a dock
    for (i = 0, paneCount = panes.GetCount(); i < paneCount; ++i)
    {
        if (panes.Item(i).GetWindow() == paneWindow)
        {
            RemovePaneFromDocks(docks, panes.Item(i));
            panes.RemoveAt(i);
            break;
        }
    }


    // find out where the new pane would be
    if (!DoDrop(docks, panes, hint, pt, offset))
    {
        m_doingHintCalculation = false;
        return rect;
    }

    panes.Add(hint);

    wxSizer* sizer = LayoutAll(panes, docks, uiparts, true);
    wxSize clientSize = m_frame->GetClientSize();
    sizer->SetDimension(0, 0, clientSize.x, clientSize.y);
    sizer->Layout();

    // First check if the hint is somewhere in a notebook, if it is then take the hint rect from the active notebook page.
    for (i = 0, partCount = uiparts.GetCount(); i < partCount; ++i)
    {
        wxAuiDockUIPart& part = uiparts.Item(i);

        if (part.type == wxAuiDockUIPart::typePaneTab)
        {
            wxAuiPaneInfoPtrArray& pages = part.m_tab_container->GetPages();
            int pageCount = pages.GetCount();
            int j = 0;
            for (j = 0; j < pageCount;j++)
            {
                if(pages[j]->GetName() == wxT("__HINT__"))
                {
                    int activePage=part.m_tab_container->GetActivePage();
                    //It is possible in some instances (when forming a new notebook via drag) - that no page is yet active, if this is the case act as if the first one is active.
                    if(activePage==-1)
                        activePage=0;

                    rect = wxRect(part.m_tab_container->GetPage(activePage).GetWindow()->GetPosition(),
                    part.m_tab_container->GetPage(activePage).GetWindow()->GetSize());
                    break;
                }
            }
        }
    }
    // If it is not in the notebook then take the hint from the pane border.
    if (rect.IsEmpty())
    {
        for (i = 0, partCount = uiparts.GetCount();
            i < partCount; ++i)
        {
            wxAuiDockUIPart& part = uiparts.Item(i);

            if (part.type == wxAuiDockUIPart::typePaneBorder &&
            part.pane && part.pane->GetName() == wxT("__HINT__"))
            {
                rect = wxRect(part.sizer_item->GetPosition(),
                          part.sizer_item->GetSize());
                break;
            }
        }
    }

    delete sizer;

    if (rect.IsEmpty())
    {
        m_doingHintCalculation = false;
        return rect;
    }

    // actually show the hint rectangle on the screen
    m_frame->ClientToScreen(&rect.x, &rect.y);

    if ( m_frame->GetLayoutDirection() == wxLayout_RightToLeft )
    {
        // Mirror rectangle in RTL mode
        rect.x -= rect.GetWidth();
    }

    m_doingHintCalculation = false;
    return rect;
}

// DrawHintRect() calculates the hint rectangle by calling
// CalculateHintRect().  If there is a rectangle, it shows it
// by calling ShowHint(), otherwise it hides any hint
// rectangle currently shown
void wxAuiManager::DrawHintRect(wxWindow* paneWindow, const wxPoint& pt, const wxPoint& offset)
{
    //Special case: if we allow external drops to windows managed by other managers then handle this first before all other cases.
    if(HasFlag(wxAUI_MGR_ALLOW_EXTERNAL_MOVE))
    {
        wxAuiDockUIPart* part = HitTest(pt.x, pt.y);
        if(!part)
        {
            wxPoint screenPt = ::wxGetMousePosition();
            wxWindow* targetCtrl = ::wxFindWindowAtPoint(screenPt);
            // If we are on top of a hint window then quickly hide the hint window and get the window that is underneath it.
            if(targetCtrl==m_hintWnd)
            {
                m_hintWnd->Hide();
                targetCtrl = ::wxFindWindowAtPoint(screenPt);
                m_hintWnd->Show();
            }

            // Find the manager this window belongs to (if it does belong to one)
            wxAuiManager* otherMgr = NULL;
            while(targetCtrl)
            {
                if(!wxDynamicCast(targetCtrl,wxAuiFloatingFrame))
                {
                    if(targetCtrl->GetEventHandler() && wxDynamicCast(targetCtrl->GetEventHandler(),wxAuiManager))
                    {
                        otherMgr = ((wxAuiManager*)targetCtrl->GetEventHandler());
                        break;
                    }
                }
                targetCtrl = targetCtrl->GetParent();
            }
            // Alert other manager of the drop and have it show hint.
            if(otherMgr && otherMgr != this)
            {
                // find out from the destination control if it's ok to drop this tab here.
                if(wxDynamicCast(targetCtrl,wxAuiNotebook))
                {
                    wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_ALLOW_DND, GetManagedWindow()->GetId());
                    int selIndex=GetAllPanes().Index(GetPane(paneWindow));
                    e.SetSelection(selIndex);
                    e.SetOldSelection(selIndex);
                    e.SetEventObject((wxAuiNotebook*)GetManagedWindow());
                    e.SetDragSource((wxAuiNotebook*)GetManagedWindow());
                    e.Veto(); // dropping must be explicitly approved by control owner

                    targetCtrl->GetEventHandler()->ProcessEvent(e);

                    if (!e.IsAllowed())
                    {
                        // no answer or negative answer
                        HideHint();
                        return;
                    }
                }
                else
                {
                    wxAuiManagerEvent e(wxEVT_AUI_ALLOW_DND);
                    e.SetManager(this);
                    e.SetPane(&GetPane(paneWindow));
                    e.Veto(); // dropping must be explicitly approved by control owner
                    targetCtrl->GetEventHandler()->ProcessEvent(e);
                    if(e.GetVeto())
                    {
                        // no answer or negative answer
                        HideHint();
                        return;
                    }
                }

                otherMgr->AddPane(paneWindow);
                wxPoint clientPt = otherMgr->GetManagedWindow()->ScreenToClient(screenPt);
                wxRect hint_rect = otherMgr->CalculateHintRect(paneWindow, clientPt, offset);
                otherMgr->DetachPane(paneWindow);
                if(hint_rect.IsEmpty())
                {
                    HideHint();
                }
                else
                {
                    ShowHint(hint_rect);
                }
                return;
            }
        }
    }

    //Normal case: calculate and display hint for a drop in our own window.
    wxRect rect = CalculateHintRect(paneWindow, pt, offset);
    if (rect.IsEmpty())
    {
        HideHint();
    }
    else
    {
        ShowHint(rect);
    }
}

void wxAuiManager::OnFloatingPaneMoveStart(wxWindow* wnd)
{
    // try to find the pane
    wxAuiPaneInfo& pane = GetPane(wnd);
    wxASSERT_MSG(pane.IsOk(), wxT("Pane window not found"));

    if(!pane.frame)
        return;

    if (HasFlag(wxAUI_MGR_TRANSPARENT_DRAG))
        pane.GetFrame()->SetTransparent(150);
}

void wxAuiManager::OnFloatingPaneMoving(wxWindow* wnd, wxDirection dir)
{
    // try to find the pane
    wxAuiPaneInfo& pane = GetPane(wnd);
    wxASSERT_MSG(pane.IsOk(), wxT("Pane window not found"));

    wxPoint pt = ::wxGetMousePosition();

#if 0
    // Adapt pt to direction
    if (dir == wxNORTH)
    {
        // move to pane's upper border
        wxPoint pos( 0,0 );
        pos = wnd->ClientToScreen( pos );
        pt.y = pos.y;
        // and some more pixels for the title bar
        pt.y -= 5;
    }
    else if (dir == wxWEST)
    {
        // move to pane's left border
        wxPoint pos( 0,0 );
        pos = wnd->ClientToScreen( pos );
        pt.x = pos.x;
    }
    else if (dir == wxEAST)
    {
        // move to pane's right border
        wxPoint pos( wnd->GetSize().x, 0 );
        pos = wnd->ClientToScreen( pos );
        pt.x = pos.x;
    }
    else if (dir == wxSOUTH)
    {
        // move to pane's bottom border
        wxPoint pos( 0, wnd->GetSize().y );
        pos = wnd->ClientToScreen( pos );
        pt.y = pos.y;
    }
#else
    wxUnusedVar(dir);
#endif

    wxPoint clientPt = m_frame->ScreenToClient(pt);

    // calculate the offset from the upper left-hand corner
    // of the frame to the mouse pointer
    wxPoint framePos = pane.GetFrame()->GetPosition();
    wxPoint actionOffset(pt.x-framePos.x, pt.y-framePos.y);

    // no hint for toolbar floating windows
    if (pane.IsToolbar() && m_action == actionDragFloatingPane)
    {
        wxAuiDockInfoArray docks;
        wxAuiPaneInfoArray panes;
        wxAuiDockUIPartArray uiparts;
        wxAuiPaneInfo hint = pane;

        CopyDocksAndPanes(docks, panes, m_docks, m_panes);

        // find out where the new pane would be
        if (!DoDrop(docks, panes, hint, clientPt))
            return;
        if (hint.IsFloating())
            return;

        pane = hint;
        m_action = actionDragToolbarPane;
        m_actionWindow = pane.GetWindow();

        Update();

        return;
    }


    // if a key modifier is pressed while dragging the frame,
    // don't dock the window
    if (!CanDockPanel(pane))
    {
        HideHint();
        return;
    }


    DrawHintRect(wnd, clientPt, actionOffset);

#ifdef __WXGTK__
    // this cleans up some screen artifacts that are caused on GTK because
    // we aren't getting the exact size of the window (see comment
    // in DrawHintRect)
    //Refresh();
#endif


    // reduces flicker
    m_frame->Update();
}

void wxAuiManager::OnFloatingPaneMoved(wxWindow* wnd, wxDirection dir)
{
    // try to find the pane
    wxAuiPaneInfo& pane = GetPane(wnd);
    wxASSERT_MSG(pane.IsOk(), wxT("Pane window not found"));

    wxPoint pt = ::wxGetMousePosition();

#if 0
    // Adapt pt to direction
    if (dir == wxNORTH)
    {
        // move to pane's upper border
        wxPoint pos( 0,0 );
        pos = wnd->ClientToScreen( pos );
        pt.y = pos.y;
        // and some more pixels for the title bar
        pt.y -= 10;
    }
    else if (dir == wxWEST)
    {
        // move to pane's left border
        wxPoint pos( 0,0 );
        pos = wnd->ClientToScreen( pos );
        pt.x = pos.x;
    }
    else if (dir == wxEAST)
    {
        // move to pane's right border
        wxPoint pos( wnd->GetSize().x, 0 );
        pos = wnd->ClientToScreen( pos );
        pt.x = pos.x;
    }
    else if (dir == wxSOUTH)
    {
        // move to pane's bottom border
        wxPoint pos( 0, wnd->GetSize().y );
        pos = wnd->ClientToScreen( pos );
        pt.y = pos.y;
    }
#else
    wxUnusedVar(dir);
#endif

    wxPoint clientPt = m_frame->ScreenToClient(pt);

    // calculate the offset from the upper left-hand corner
    // of the frame to the mouse pointer
    wxPoint framePos = pane.GetFrame()->GetPosition();
    wxPoint actionOffset(pt.x-framePos.x, pt.y-framePos.y);

    // if a key modifier is pressed while dragging the frame,
    // don't dock the window
    if (CanDockPanel(pane))
    {
        // do the drop calculation
        DoDrop(m_docks, m_panes, pane, clientPt, actionOffset);
    }

    // if the pane is still floating, update its floating
    // position (that we store)
    if (pane.IsFloating())
    {
        pane.SetFloatingPosition(pane.GetFrame()->GetPosition());

        if (HasFlag(wxAUI_MGR_TRANSPARENT_DRAG))
            pane.GetFrame()->SetTransparent(255);
    }
    else if (m_hasMaximized)
    {
        RestoreMaximizedPane();
    }

    HideHint();

    //fixme: (MJM) The below (Triple update) is inefficient and can probably be replaced with a better mechanism.
    // Update the layout to realize new position and e.g. form notebooks if needed.
    Update();

    // Ensure active before doing actual display.
    SetActivePane(pane.GetWindow());

    // Update the layout to realize new position and e.g. form notebooks if needed.
    Update();

    // If a notebook formed we may have lost our active status so set it again.
    SetActivePane(pane.GetWindow());

    // Update once again so that notebook can reflect our new active status.
    Update();

    // Make changes visible to user.
    Repaint();
}

void wxAuiManager::OnFloatingPaneResized(wxWindow* wnd, const wxRect& rect)
{
    // try to find the pane
    wxAuiPaneInfo& pane = GetPane(wnd);
    wxASSERT_MSG(pane.IsOk(), wxT("Pane window not found"));

    pane.SetFloatingSize(rect.GetWidth(), rect.GetHeight());

    // the top-left position may change as well as the size
    pane.SetFloatingPosition(rect.x, rect.y);
}


void wxAuiManager::OnFloatingPaneClosed(wxWindow* wnd, wxCloseEvent& WXUNUSED(evt))
{
    // try to find the pane
    wxAuiPaneInfo& pane = GetPane(wnd);
    wxASSERT_MSG(pane.IsOk(), wxT("Pane window not found"));

    ClosePane(pane);
}



void wxAuiManager::OnFloatingPaneActivated(wxWindow* wnd)
{
    if (HasFlag(wxAUI_MGR_ALLOW_ACTIVE_PANE) && GetPane(wnd).IsOk())
    {
        SetActivePane(wnd);
        Repaint();
    }
}

// OnRender() draws all of the pane captions, sashes,
// backgrounds, captions, grippers, pane borders and buttons.
// It renders the entire user interface.

void wxAuiManager::OnRender(wxAuiManagerEvent& evt)
{
    // if the frame is about to be deleted, don't bother
    if (!m_frame || wxPendingDelete.Member(m_frame))
        return;

    wxDC* dc = evt.GetDC();

#ifdef __WXMAC__
    dc->Clear() ;
#endif
    int i, partCount;
    for (i = 0, partCount = m_uiParts.GetCount();
         i < partCount; ++i)
    {
        wxAuiDockUIPart& part = m_uiParts.Item(i);

        // don't draw hidden pane items or items that aren't windows
        if (part.sizer_item && ((!part.sizer_item->IsWindow() && !part.sizer_item->IsSpacer() && !part.sizer_item->IsSizer()) || !part.sizer_item->IsShown()))
            continue;

        switch (part.type)
        {
            case wxAuiDockUIPart::typePaneTab:
                part.m_tab_container->DrawTabs(dc, m_frame, part.rect);
                break;
            case wxAuiDockUIPart::typeDockSizer:
            case wxAuiDockUIPart::typePaneSizer:
                m_art->DrawSash(*dc, m_frame, part.orientation, part.rect);
                break;
            case wxAuiDockUIPart::typeBackground:
                m_art->DrawBackground(*dc, m_frame, part.orientation, part.rect);
                break;
            case wxAuiDockUIPart::typeCaption:
                m_art->DrawCaption(*dc, m_frame, part.pane->GetCaption(), part.rect, *part.pane);
                break;
            case wxAuiDockUIPart::typeGripper:
                m_art->DrawGripper(*dc, m_frame, part.rect, *part.pane);
                break;
            case wxAuiDockUIPart::typePaneBorder:
                m_art->DrawBorder(*dc, m_frame, part.rect, *part.pane);
                break;
            case wxAuiDockUIPart::typePaneButton:
                m_art->DrawPaneButton(*dc, m_frame, part.button->button_id,
                        wxAUI_BUTTON_STATE_NORMAL, part.rect, *part.pane);
                break;
        }
    }
}

 wxAuiDockUIPart::~wxAuiDockUIPart()
    {
        if(m_tab_container)
        {
            delete m_tab_container;
            m_tab_container = NULL;
        }
    }

// Render() fire a render event, which is normally handled by
// wxAuiManager::OnRender().  This allows the render function to
// be overridden via the render event.  This can be useful for paintin
// custom graphics in the main window. Default behaviour can be
// invoked in the overridden function by calling OnRender()

void wxAuiManager::Render(wxDC* dc)
{
    wxAuiManagerEvent e(wxEVT_AUI_RENDER);
    e.SetManager(this);
    e.SetDC(dc);
    ProcessMgrEvent(e);
}

void wxAuiManager::Repaint(wxDC* dc)
{
#ifdef __WXMAC__ 
    if ( dc == NULL )
    {
        m_frame->Refresh() ;
        m_frame->Update() ;
        return ;
    }
#endif
    int w, h;
    m_frame->GetClientSize(&w, &h);

    // figure out which dc to use; if one
    // has been specified, use it, otherwise
    // make a client dc
    wxClientDC* clientDC = NULL;
    if (!dc)
    {
        clientDC = new wxClientDC(m_frame);
        dc = clientDC;
    }

    // if the frame has a toolbar, the client area
    // origin will not be (0,0).
    wxPoint pt = m_frame->GetClientAreaOrigin();
    if (pt.x != 0 || pt.y != 0)
        dc->SetDeviceOrigin(pt.x, pt.y);

    // render all the items
    Render(dc);

    // if we created a clientDC, delete it
    if (clientDC)
        delete clientDC;
}

void wxAuiManager::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
    wxPaintDC dc(m_frame);
    Repaint(&dc);
}

void wxAuiManager::OnEraseBackground(wxEraseEvent& evt)
{
#ifdef __WXMAC__
    evt.Skip() ;
#else
    wxUnusedVar(evt);
#endif
}

void wxAuiManager::OnSize(wxSizeEvent& evt)
{
    if (m_frame)
    {
        DoFrameLayout();
        Repaint();

#if wxUSE_MDI
        if (wxDynamicCast(m_frame,wxMDIParentFrame))
        {
            // for MDI parent frames, this event must not
            // be "skipped".  In other words, the parent frame
            // must not be allowed to resize the client window
            // after we are finished processing sizing changes
            return;
        }
#endif
    }
    evt.Skip();
}

void wxAuiManager::OnFindManager(wxAuiManagerEvent& evt)
{
    // get the window we are managing, if none, return NULL
    wxWindow* window = GetManagedWindow();
    if (!window)
    {
        evt.SetManager(NULL);
        return;
    }

    // if we are managing a child frame, get the 'real' manager
    if (wxDynamicCast(window,wxAuiFloatingFrame))
    {
        wxAuiFloatingFrame* floatFrame = static_cast<wxAuiFloatingFrame*>(window);
        evt.SetManager(floatFrame->GetOwnerManager());
        return;
    }

    // return pointer to ourself
    evt.SetManager(this);
}


void wxAuiManager::OnSetCursor(wxSetCursorEvent& evt)
{
    // determine cursor
    wxAuiDockUIPart* part = HitTest(evt.GetX(), evt.GetY());
    wxCursor cursor = wxNullCursor;

    if (part)
    {
        if (part->type == wxAuiDockUIPart::typeDockSizer ||
            part->type == wxAuiDockUIPart::typePaneSizer)
        {
            // a dock may not be resized if it has a single
            // pane which is not resizable
            if (part->type == wxAuiDockUIPart::typeDockSizer && part->dock &&
                part->dock->panes.GetCount() == 1 &&
                part->dock->panes.Item(0)->IsFixed())
                    return;

            // panes that may not be resized do not get a sizing cursor
            if (part->pane && part->pane->IsFixed())
                return;

            if (part->orientation == wxVERTICAL)
                cursor = wxCursor(wxCURSOR_SIZEWE);
            else
                cursor = wxCursor(wxCURSOR_SIZENS);
        }
        else if (part->type == wxAuiDockUIPart::typeGripper)
        {
            cursor = wxCursor(wxCURSOR_SIZING);
        }
    }

    evt.SetCursor(cursor);
}



void wxAuiManager::UpdateButtonOnScreen(wxAuiDockUIPart* buttonUIPart, const wxMouseEvent& evt)
{
    wxAuiDockUIPart* hitTest = HitTest(evt.GetX(), evt.GetY());
    if (!hitTest || !buttonUIPart)
        return;

    int state = wxAUI_BUTTON_STATE_NORMAL;

    if (hitTest == buttonUIPart)
    {
        if (evt.LeftIsDown())
            state = wxAUI_BUTTON_STATE_PRESSED;
        else
            state = wxAUI_BUTTON_STATE_HOVER;
    }
    else
    {
        if (evt.LeftIsDown())
            state = wxAUI_BUTTON_STATE_HOVER;
    }

    // now repaint the button with hover state
    wxClientDC cdc(m_frame);

    // if the frame has a toolbar, the client area
    // origin will not be (0,0).
    wxPoint pt = m_frame->GetClientAreaOrigin();
    if (pt.x != 0 || pt.y != 0)
        cdc.SetDeviceOrigin(pt.x, pt.y);

    if (hitTest->pane)
    {
        m_art->DrawPaneButton(cdc, m_frame,
                  buttonUIPart->button->button_id,
                  state,
                  buttonUIPart->rect,
                  *hitTest->pane);
    }
}

void wxAuiManager::OnLeftDClick(wxMouseEvent& evt)
{
    wxAuiDockUIPart* part = HitTest(evt.GetX(), evt.GetY());
    if(part)
    {
        if(part->type == wxAuiDockUIPart::typePaneTab)
        {
            wxAuiPaneInfo* hitPane;
            wxAuiTabContainerButton* hitbutton;
            if(!part->m_tab_container->ButtonHitTest(evt.m_x,evt.m_y,&hitbutton) && !part->m_tab_container->TabHitTest(evt.m_x,evt.m_y,&hitPane))
            {
                if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
                {
                    wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_BG_DCLICK, GetManagedWindow()->GetId());
                    e.SetEventObject(GetManagedWindow());
                    GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
                }
            }
        }
    }
}

void wxAuiManager::OnLeftDown(wxMouseEvent& evt)
{
    m_currentDragItem = -1;

    wxAuiDockUIPart* part = HitTest(evt.GetX(), evt.GetY());
    if (part)
    {
        if (part->type == wxAuiDockUIPart::typeDockSizer || part->type == wxAuiDockUIPart::typePaneSizer)
        {
            // Removing this restriction so that a centre pane can be resized
            //if (part->dock && part->dock->dock_direction == wxAUI_DOCK_CENTER)
            //    return;

            // a dock may not be resized if it has a single
            // pane which is not resizable
            if (part->type == wxAuiDockUIPart::typeDockSizer && part->dock &&
                part->dock->panes.GetCount() == 1 &&
                part->dock->panes.Item(0)->IsFixed())
                    return;

            // panes that may not be resized should be ignored here
            if (part->pane && part->pane->IsFixed())
                return;

            m_action = actionResize;
            m_actionPart = part;
            m_actionHintRect = wxRect();
            m_actionStart = wxPoint(evt.m_x, evt.m_y);
            m_actionOffset = wxPoint(evt.m_x - part->rect.x,
                                      evt.m_y - part->rect.y);
            m_frame->CaptureMouse();
        }
        else if (part->type == wxAuiDockUIPart::typePaneButton)
        {
            m_action = actionClickButton;
            m_actionPart = part;
            m_actionStart = wxPoint(evt.m_x, evt.m_y);
            m_frame->CaptureMouse();

            UpdateButtonOnScreen(part, evt);
        }
        else if (part->type == wxAuiDockUIPart::typeCaption || part->type == wxAuiDockUIPart::typeGripper)
        {
            // if we are managing a wxAuiFloatingFrame window, then
            // we are an embedded wxAuiManager inside the wxAuiFloatingFrame.
            // We want to initiate a toolbar drag in our owner manager
            wxWindow* managedWindow = GetManagedWindow();

            if (part->pane &&
                part->pane->GetWindow() &&
                managedWindow &&
                wxDynamicCast(managedWindow,wxAuiFloatingFrame))
            {
                wxAuiFloatingFrame* floatingFrame = (wxAuiFloatingFrame*)managedWindow;
                wxAuiManager* ownerManager = floatingFrame->GetOwnerManager();
                ownerManager->StartPaneDrag(part->pane->GetWindow(), wxPoint(evt.m_x - part->rect.x, evt.m_y - part->rect.y));
                return;
            }



            if (HasFlag(wxAUI_MGR_ALLOW_ACTIVE_PANE))
            {
                // set the caption as active
                SetActivePane(part->pane->GetWindow());
                Repaint();
            }

            if (part->dock && part->dock->dock_direction == wxAUI_DOCK_CENTER)
                return;

            m_action = actionClickCaption;
            m_actionPart = part;
            m_actionStart = wxPoint(evt.m_x, evt.m_y);
            m_actionOffset = wxPoint(evt.m_x - part->rect.x, evt.m_y - part->rect.y);
            m_frame->CaptureMouse();
        }
        else if(part->type == wxAuiDockUIPart::typePaneTab)
        {
            wxAuiPaneInfo* hitPane;
            if(part->m_tab_container->ButtonHitTest(evt.m_x,evt.m_y,&m_hoverButton2))
            {
                m_action = actionClickButton;
                m_actionPart = part;
                m_actionStart = wxPoint(evt.m_x, evt.m_y);
                m_frame->CaptureMouse();
                part->pane = &part->m_tab_container->GetPage(part->m_tab_container->GetActivePage());

                m_hoverButton2->curState = wxAUI_BUTTON_STATE_PRESSED;
                Repaint();
            }
            else if(part->m_tab_container->TabHitTest(evt.m_x,evt.m_y,&hitPane))
            {
                int oldActivePaneIndex=GetActivePane(NULL);
                int newActivePaneIndex=GetAllPanes().Index(*hitPane);

                // If we are a wxAuiNotebook then we must fire off a wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGING event and give the user an opportunity to veto it.
                bool vetoed=false;
                if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
                {
                    wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGING, GetManagedWindow()->GetId());
                    e.SetSelection(newActivePaneIndex);
                    e.SetOldSelection(oldActivePaneIndex);
                    e.SetEventObject(GetManagedWindow());
                    GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
                    vetoed = !e.IsAllowed();
                }

                // Either no event sent or event not vetoed - either way go ahead and do the change
                if(!vetoed)
                {
                    SetActivePane(hitPane->GetWindow());

                    m_actionOffset = wxPoint(evt.m_x-hitPane->rect.x,evt.m_y-part->rect.y);

                    m_action = actionClickCaption;
                    m_actionPart = part;
                    m_actionPart->pane = hitPane;
                    m_actionStart = wxPoint(evt.m_x, evt.m_y);
                    m_frame->CaptureMouse();

                    DoFrameLayout();
                    Repaint();

                    // If we are a wxAuiNotebook then we must fire off a wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED event.
                    if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
                    {
                        wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, GetManagedWindow()->GetId());
                        e.SetSelection(newActivePaneIndex);
                        e.SetOldSelection(oldActivePaneIndex);
                        e.SetEventObject(GetManagedWindow());
                        GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
                    }
                }
            }
        }
#ifdef __WXMAC__
        else
        {
            evt.Skip();
        }
#endif
    }
#ifdef __WXMAC__
    else
    {
        evt.Skip();
    }
#else
    evt.Skip();
#endif
}

/// Ends a resize action, or for live update, resizes the sash
bool wxAuiManager::DoEndResizeAction(wxMouseEvent& evt)
{
    // resize the dock or the pane
    if (m_actionPart && m_actionPart->type==wxAuiDockUIPart::typeDockSizer)
    {
        // first, we must calculate the maximum size the dock may be
        int sashSize = m_art->GetMetric(wxAUI_DOCKART_SASH_SIZE);

        int usedWidth = 0, usedHeight = 0;

        wxSize clientSize = m_frame->GetClientSize();

        size_t dockIndex, dockCount = m_docks.GetCount();
        for (dockIndex = 0; dockIndex < dockCount; ++dockIndex)
        {
            wxAuiDockInfo& dock = m_docks.Item(dockIndex);
            if (dock.dock_direction == wxAUI_DOCK_TOP ||
                dock.dock_direction == wxAUI_DOCK_BOTTOM)
            {
                usedHeight += dock.size;
            }
            if (dock.dock_direction == wxAUI_DOCK_LEFT ||
                dock.dock_direction == wxAUI_DOCK_RIGHT)
            {
                usedWidth += dock.size;
            }
            if (dock.resizable)
                usedWidth += sashSize;
        }


        int availableWidth = clientSize.GetWidth() - usedWidth;
        int availableHeight = clientSize.GetHeight() - usedHeight;


#if wxUSE_STATUSBAR
        // if there's a status control, the available
        // height decreases accordingly
        if (m_frame && wxDynamicCast(m_frame,wxFrame))
        {
            wxFrame* frame = static_cast<wxFrame*>(m_frame);
            wxStatusBar* status = frame->GetStatusBar();
            if (status)
            {
                wxSize statusClientSize = status->GetClientSize();
                availableHeight -= statusClientSize.GetHeight();
            }
        }
#endif

        wxRect& rect = m_actionPart->dock->rect;

        wxPoint newPos(evt.m_x - m_actionOffset.x,
            evt.m_y - m_actionOffset.y);
        int newSize, oldSize = m_actionPart->dock->size;

        switch (m_actionPart->dock->dock_direction)
        {
        case wxAUI_DOCK_LEFT:
            newSize = newPos.x - rect.x;
            if (newSize-oldSize > availableWidth)
                newSize = oldSize+availableWidth;
            m_actionPart->dock->size = newSize;
            break;
        case wxAUI_DOCK_TOP:
            newSize = newPos.y - rect.y;
            if (newSize-oldSize > availableHeight)
                newSize = oldSize+availableHeight;
            m_actionPart->dock->size = newSize;
            break;
        case wxAUI_DOCK_RIGHT:
            newSize = rect.x + rect.width - newPos.x -
                       m_actionPart->rect.GetWidth();
            if (newSize-oldSize > availableWidth)
                newSize = oldSize+availableWidth;
            m_actionPart->dock->size = newSize;
            break;
        case wxAUI_DOCK_BOTTOM:
            newSize = rect.y + rect.height -
                newPos.y - m_actionPart->rect.GetHeight();
            if (newSize-oldSize > availableHeight)
                newSize = oldSize+availableHeight;
            m_actionPart->dock->size = newSize;
            break;
        }

        Update();
        Repaint(NULL);
    }
    else if (m_actionPart && m_actionPart->type == wxAuiDockUIPart::typePaneSizer)
    {
        wxAuiDockInfo& dock = *m_actionPart->dock;
        wxAuiPaneInfo* pane = m_actionPart->pane;

        // If the pane is part of a notebook then its window might not currently be the visible one within the notebook
        // So we must then find which window in the notebook is the visible one and use the pane for that instead.
        if(!pane->GetWindow()->IsShown())
        {
            int i;
            int dockPaneCount = dock.panes.GetCount();
            // First find where in the panes array our current pane is
            for (i = 0; i < dockPaneCount; ++i)
            {
                wxAuiPaneInfo& p = *dock.panes.Item(i);
                if (p.GetWindow() == pane->GetWindow())
                {
                    // Our current pane will be the last pane in the notebook,
                    // so now search backwards through the notebook till we find the visible pane.
                    int j;
                    for (j = i - 1; j >= 0; --j)
                    {
                        if(dock.panes.Item(j)->GetWindow()->IsShown() && dock.panes.Item(j)->GetPosition() == pane->GetPosition())
                        {
                            pane = dock.panes.Item(j);
                            break;
                        }
                    }
                    break;
                }
            }
        }

        int totalProportion = 0;
        int dockPixels = 0;
        int newPixSize = 0;

        int captionSize = m_art->GetMetric(wxAUI_DOCKART_CAPTION_SIZE);
        int paneBorderSize = m_art->GetMetric(wxAUI_DOCKART_PANE_BORDER_SIZE);
        int sashSize = m_art->GetMetric(wxAUI_DOCKART_SASH_SIZE);

        wxPoint newPos(evt.m_x - m_actionOffset.x,
            evt.m_y - m_actionOffset.y);

        // determine the pane rectangle by getting the pane part
        wxAuiDockUIPart* panePart = GetPanePart(pane->GetWindow());
        wxASSERT_MSG(panePart, wxT("Pane border part not found -- shouldn't happen"));

        // determine the new pixel size that the user wants;
        // this will help us recalculate the pane's proportion
        if (dock.IsHorizontal())
            newPixSize = newPos.x - panePart->rect.x;
        else
            newPixSize = newPos.y - panePart->rect.y;

        // determine the size of the dock, based on orientation
        if (dock.IsHorizontal())
            dockPixels = dock.rect.GetWidth();
        else
            dockPixels = dock.rect.GetHeight();

        // determine the total proportion of all resizable panes,
        // and the total size of the dock minus the size of all
        // the fixed panes
        int i;
        int dockPaneCount = dock.panes.GetCount();
        int panePosition = -1;
        for (i = 0; i < dockPaneCount; ++i)
        {
            wxAuiPaneInfo& p = *dock.panes.Item(i);
            if (p.GetWindow() == pane->GetWindow())
                panePosition = i;

            // Don't include all the panes from a notenook in the calculation, only the first one.
            if(i==0 || p.GetPosition()!=dock.panes.Item(i-1)->GetPosition())
            {
                // If this pane is part of a notebook then subtract the height of the notebook tabs as well.
                if(i<dockPaneCount-1 && p.GetPosition()==dock.panes.Item(i+1)->GetPosition())
                {
                    dockPixels-=notebookTabHeight;
                }
                // while we're at it, subtract the pane sash
                // width from the dock width, because this would
                // skew our proportion calculations
                if (i > 0)
                    dockPixels -= sashSize;

                // also, the whole size (including decorations) of
                // all fixed panes must also be subtracted, because they
                // are not part of the proportion calculation
                if (p.IsFixed())
                {
                    if (dock.IsHorizontal())
                        dockPixels -= p.GetBestSize().x;
                    else
                        dockPixels -= p.GetBestSize().y;
                }
                else
                {
                    totalProportion += p.GetProportion();
                }
            }
        }

        // new size can never be more than the number of dock pixels
        if (newPixSize > dockPixels)
            newPixSize = dockPixels;


        // find a pane in our dock to 'steal' space from or to 'give'
        // space to -- this is essentially what is done when a pane is
        // resized; the pane should usually be the first non-fixed pane
        // to the right of the action pane
        int borrowPane = -1;
        for (i = panePosition+1; i < dockPaneCount; ++i)
        {
            wxAuiPaneInfo& p = *dock.panes.Item(i);
            // We must not select a pane from the same notebook as the 'lender' to 'steal' space from. (If the 'lender' is in a notebook)
            if (!p.IsFixed() && p.GetPosition()!=dock.panes.Item(i-1)->GetPosition())
            {
                borrowPane = i;
                break;
            }
        }


        // demand that the pane being resized is found in this dock
        // (this assert really never should be raised)
        wxASSERT_MSG(panePosition != -1, wxT("Pane not found in dock"));

        // prevent division by zero
        if (dockPixels == 0 || totalProportion == 0 || borrowPane == -1)
        {
            m_action = actionNone;
            return false;
        }

        // calculate the new proportion of the pane
        int newProportion = (newPixSize*totalProportion)/dockPixels;

        // default minimum size
        int minSize = 0;

        // check against the pane's minimum size, if specified. please note
        // that this is not enough to ensure that the minimum size will
        // not be violated, because the whole frame might later be shrunk,
        // causing the size of the pane to violate its minimum size
        if (pane->GetMinSize().IsFullySpecified())
        {
            minSize = 0;

            if (pane->HasBorder())
                minSize += (paneBorderSize*2);

            // calculate minimum size with decorations (border,caption)
            if (panePart->orientation == wxVERTICAL)
            {
                minSize += pane->GetMinSize().y;
                if (pane->HasCaption())
                    minSize += captionSize;
            }
            else
            {
                minSize += pane->GetMinSize().x;
            }
        }


        // for some reason, an arithmatic error somewhere is causing
        // the proportion calculations to always be off by 1 pixel;
        // for now we will add the 1 pixel on, but we really should
        // determine what's causing this.
        minSize++;

        int minProportion = (minSize*totalProportion)/dockPixels;

        if (newProportion < minProportion)
            newProportion = minProportion;



        int propDiff = newProportion - pane->GetProportion();

        // borrow the space from our neighbor pane to the
        // right or bottom (depending on orientation);
        // also make sure we don't make the neighbor too small
        int propBorrow = dock.panes.Item(borrowPane)->GetProportion();

        if (propBorrow - propDiff < 0)
        {
            // borrowing from other pane would make it too small,
            // so cancel the resize operation
            propBorrow = minProportion;
        }
         else
        {
            propBorrow -= propDiff;
        }


        dock.panes.Item(borrowPane)->SetProportion(propBorrow);
        // If the above proportion change was to a pane in a notebook then all other panes in the notebook
        // must have the same change, so we do this below.
        for(i=borrowPane-1;i>0;i--)
        {
            if(dock.panes.Item(borrowPane)->GetPosition()==dock.panes.Item(i)->GetPosition())
            {
                dock.panes.Item(i)->SetProportion(propBorrow);
            }
            else
            {
                break;
            }
        }
        for(i=borrowPane+1;i<dockPaneCount;i++)
        {
            if(dock.panes.Item(borrowPane)->GetPosition()==dock.panes.Item(i)->GetPosition())
            {
                dock.panes.Item(i)->SetProportion(propBorrow);
            }
            else
            {
                break;
            }
        }
        pane->SetProportion(newProportion);
        // If the above proportion change was to a pane in a notebook then all other panes in the notebook
        // must have the same change, so we do this below.
        for(i=panePosition-1;i>0;i--)
        {
            if(pane->GetPosition()==dock.panes.Item(i)->GetPosition())
            {
                dock.panes.Item(i)->SetProportion(newProportion);
            }
            else
            {
                break;
            }
        }
        for(i=panePosition+1;i<dockPaneCount;i++)
        {
            if(pane->GetPosition()==dock.panes.Item(i)->GetPosition())
            {
                dock.panes.Item(i)->SetProportion(newProportion);
            }
            else
            {
                break;
            }
        }

        // repaint
        Update();
        Repaint(NULL);
    }

    return true;
}

void wxAuiManager::OnLeftUp(wxMouseEvent& evt)
{
    if (m_action == actionResize)
    {
        m_frame->ReleaseMouse();

        if (!wxAuiManager_HasLiveResize(*this))
        {
            // get rid of the hint rectangle
            wxScreenDC dc;
            DrawResizeHint(dc, m_actionHintRect);
        }
        if (m_currentDragItem != -1 && wxAuiManager_HasLiveResize(*this))
            m_actionPart = & (m_uiParts.Item(m_currentDragItem));

        DoEndResizeAction(evt);

        m_currentDragItem = -1;

    }
    else if (m_action == actionClickButton)
    {
        m_frame->ReleaseMouse();
        m_hoverButton = NULL;
        wxAuiPaneInfo& pane = *m_actionPart->pane;

        if (m_actionPart)
        {
            // make sure we're still over the item that was originally clicked
            bool passHitTest=false;
            int buttonid=0;
            if(m_actionPart->type == wxAuiDockUIPart::typePaneTab)
            {
                passHitTest = (m_actionPart->m_tab_container->ButtonHitTest(evt.m_x,evt.m_y,&m_hoverButton2));
                buttonid=m_hoverButton2->id;
                m_hoverButton2->curState = wxAUI_BUTTON_STATE_NORMAL;
                m_hoverButton2=NULL;

                // If we are a wxAuiNotebook then we must fire off a wxEVT_COMMAND_AUINOTEBOOK_BUTTON event to notify user of change.
                if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
                {
                    wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_BUTTON, GetManagedWindow()->GetId());
                    e.SetSelection(GetAllPanes().Index(pane));
                    e.SetInt(buttonid);
                    e.SetEventObject(GetManagedWindow());
                    GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
                }
            }
            else
            {
                UpdateButtonOnScreen(m_actionPart, evt);
                passHitTest = (m_actionPart == HitTest(evt.GetX(), evt.GetY()));
                buttonid=m_actionPart->button->button_id;
            }
            if (passHitTest)
            {
                // fire button-click event
                wxAuiManagerEvent e(wxEVT_AUI_PANE_BUTTON);
                e.SetManager(this);
                e.SetPane(&pane);
                e.SetButton(buttonid);
                ProcessMgrEvent(e);
            }
            else
            {
                Update();
            }
        }
    }
    else if (m_action == actionClickCaption)
    {
        m_frame->ReleaseMouse();
    }
    else if (m_action == actionDragFloatingPane)
    {
        // If we are a wxAuiNotebook then we must fire off a wxEVT_COMMAND_AUINOTEBOOK_DRAG_DONE event to notify user of change.
        if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
        {
            wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_DRAG_DONE, GetManagedWindow()->GetId());
            e.SetSelection(GetAllPanes().Index(GetPane(m_actionWindow)));
            e.SetEventObject(GetManagedWindow());
            GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
        }

        m_frame->ReleaseMouse();

        // If we are a wxAuiNotebook then we must fire off a wxEVT_COMMAND_AUINOTEBOOK_END_DRAG event to notify user of change.
        if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
        {
            wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_END_DRAG, GetManagedWindow()->GetId());
            e.SetSelection(GetAllPanes().Index(GetPane(m_actionWindow)));
            e.SetEventObject(GetManagedWindow());
            GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
        }
    }
    else if (m_action == actionDragMovablePane)
    {
        // If we are a wxAuiNotebook then we must fire off a wxEVT_COMMAND_AUINOTEBOOK_DRAG_DONE event to notify user of change.
        if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
        {
            wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_DRAG_DONE, GetManagedWindow()->GetId());
            e.SetSelection(GetAllPanes().Index(GetPane(m_actionWindow)));
            e.SetEventObject(GetManagedWindow());
            GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
        }

        // Try to find the pane.
        wxAuiPaneInfo& pane = GetPane(m_actionWindow);
        wxASSERT_MSG(pane.IsOk(), wxT("Pane window not found"));

        // Hide the hint as it is no longer needed.
        HideHint();

        // Move the pane to new position.
        wxPoint pt = evt.GetPosition();
        if(!DoDropExternal(pane, pt, wxPoint(0,0)))
        {
            DoDrop(m_docks, m_panes, pane, pt, wxPoint(0,0));
        }


        //fixme: (MJM) The below (Triple update) is inefficient and can probably be replaced with a better mechanism.
        GetManagedWindow()->Freeze();

        // Update the layout to realize new position and e.g. form notebooks if needed.
        Update();

        // Ensure active before doing actual display.
        SetActivePane(pane.GetWindow());

        // Update the layout to realize new position and e.g. form notebooks if needed.
        Update();

        // If a notebook formed we may have lost our active status so set it again.
        SetActivePane(pane.GetWindow());

        // Update once again so that notebook can reflect our new active status.
        Update();

        GetManagedWindow()->Thaw();
        // Make changes visible to user.
        Repaint();

        // Cancel the action and release the mouse.
        m_action = actionNone;
        m_frame->ReleaseMouse();
        m_actionWindow = NULL;
        delete m_actionDeadZone;
        m_actionDeadZone = NULL;

        // If we are a wxAuiNotebook then we must fire off a wxEVT_COMMAND_AUINOTEBOOK_END_DRAG event to notify user of change.
        if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
        {
            wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_END_DRAG, GetManagedWindow()->GetId());
            e.SetSelection(GetAllPanes().Index(GetPane(m_actionWindow)));
            e.SetEventObject(GetManagedWindow());
            GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
        }
    }
    else if (m_action == actionDragToolbarPane)
    {
        m_frame->ReleaseMouse();

        wxAuiPaneInfo& pane = GetPane(m_actionWindow);
        wxASSERT_MSG(pane.IsOk(), wxT("Pane window not found"));

        // save the new positions
        wxAuiDockInfoPtrArray docks;
        FindDocks(m_docks, pane.GetDirection(),
                  pane.GetLayer(), pane.GetRow(), docks);
        if (docks.GetCount() == 1)
        {
            wxAuiDockInfo& dock = *docks.Item(0);

            wxArrayInt panePositions, paneSizes;
            GetPanePositionsAndSizes(dock, panePositions, paneSizes);

            int i, dockPaneCount = dock.panes.GetCount();
            for (i = 0; i < dockPaneCount; ++i)
                dock.panes.Item(i)->SetPosition(panePositions[i]);
        }

        pane.SetFlag(wxAuiPaneInfo::actionPane,false);
        Update();
    }
    else
    {
        evt.Skip();
    }

    m_action = actionNone;
    m_lastMouseMove = wxPoint(); // see comment in OnMotion()
    delete m_actionDeadZone;
    m_actionDeadZone = NULL;
}

void wxAuiManager::OnRightDown(wxMouseEvent& evt)
{
    wxAuiDockUIPart* part = HitTest(evt.GetX(), evt.GetY());
    if(part)
    {
        if(part->type == wxAuiDockUIPart::typePaneTab)
        {
            wxAuiPaneInfo* hitPane;
            if(!part->m_tab_container->TabHitTest(evt.m_x,evt.m_y,&hitPane))
            {
                if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
                {
                    wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_TAB_RIGHT_DOWN, GetManagedWindow()->GetId());
                    e.SetEventObject(GetManagedWindow());
                    e.SetSelection(GetAllPanes().Index(*hitPane));
                    GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
                }
            }
        }
    }
}

void wxAuiManager::OnRightUp(wxMouseEvent& evt)
{
    wxAuiDockUIPart* part = HitTest(evt.GetX(), evt.GetY());
    if(part)
    {
        if(part->type == wxAuiDockUIPart::typePaneTab)
        {
            wxAuiPaneInfo* hitPane;
            if(!part->m_tab_container->TabHitTest(evt.m_x,evt.m_y,&hitPane))
            {
                if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
                {
                    wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_TAB_RIGHT_UP, GetManagedWindow()->GetId());
                    e.SetEventObject(GetManagedWindow());
                    e.SetSelection(GetAllPanes().Index(*hitPane));
                    GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
                }
            }
        }
    }
}

void wxAuiManager::OnMiddleDown(wxMouseEvent& evt)
{
    wxAuiDockUIPart* part = HitTest(evt.GetX(), evt.GetY());
    if(part)
    {
        if(part->type == wxAuiDockUIPart::typePaneTab)
        {
            wxAuiPaneInfo* hitPane;
            if(!part->m_tab_container->TabHitTest(evt.m_x,evt.m_y,&hitPane))
            {
                if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
                {
                    wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_DOWN, GetManagedWindow()->GetId());
                    e.SetEventObject(GetManagedWindow());
                    e.SetSelection(GetAllPanes().Index(*hitPane));
                    GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
                }
            }
        }
    }
}

void wxAuiManager::OnMiddleUp(wxMouseEvent& evt)
{

    // if the wxAUI_NB_MIDDLE_CLICK_CLOSE is specified, middle
    // click should act like a tab close action.  However, first
    // give the owner an opportunity to handle the middle up event
    // for custom action

    wxAuiDockUIPart* part = HitTest(evt.GetX(), evt.GetY());
    if(!part||!part->pane)
    {
        evt.Skip();
        return;
    }
    if (part && part->type == wxAuiDockUIPart::typePaneTab)
    {
        wxAuiPaneInfo* hitPane=NULL;
        if(part->m_tab_container->TabHitTest(evt.GetX(),evt.GetY(),&hitPane))
        {
            part->pane = hitPane;
        }
    }

    // check if we are supposed to close on middle-up
    if (!HasFlag(wxAUI_MGR_MIDDLE_CLICK_CLOSE) == 0)
        return;


    // If we are a wxAuiNotebook then we must fire off a wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_UP event and give the user an opportunity to veto it.
    if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
    {
        wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_UP, GetManagedWindow()->GetId());
        e.SetSelection(GetAllPanes().Index(*part->pane));
        e.SetEventObject(GetManagedWindow());
        if (GetManagedWindow()->GetEventHandler()->ProcessEvent(e))
            return;
        if (!e.IsAllowed())
            return;
    }

    // simulate the user pressing the close button on the tab
    wxAuiManagerEvent e(wxEVT_AUI_PANE_BUTTON);
    e.SetEventObject(this);
    e.SetPane(part->pane);
    e.SetButton(wxAUI_BUTTON_CLOSE);
    OnPaneButton(e);
}


void wxAuiManager::ShowToolTip(wxAuiPaneInfo* pane)
{
    #if wxUSE_TOOLTIPS
    wxString toolTip(pane->GetToolTip());
    if(toolTip==wxT(""))
    {
        HideToolTip();
    }
    else
    {
        // If the text changes, set it else, keep old, to avoid
        // 'moving tooltip' effect
        if(m_frame->GetToolTipText() != toolTip)
            m_frame->SetToolTip(toolTip);
    }
    #endif
}

void wxAuiManager::HideToolTip()
{
    #if wxUSE_TOOLTIPS
    m_frame->UnsetToolTip();
    #endif
}

void wxAuiManager::OnMotion(wxMouseEvent& evt)
{
    // sometimes when Update() is called from inside this method,
    // a spurious mouse move event is generated; this check will make
    // sure that only real mouse moves will get anywhere in this method;
    // this appears to be a bug somewhere, and I don't know where the
    // mouse move event is being generated.  only verified on MSW

    wxPoint mousePos = evt.GetPosition();
    if (m_lastMouseMove == mousePos)
        return;
    m_lastMouseMove = mousePos;

    if(m_actionDeadZone)
    {
        if(m_actionDeadZone->Contains(mousePos))
        {
            return;
        }
        else
        {
            delete m_actionDeadZone;
            m_actionDeadZone = NULL;
        }
    }


    if (m_action == actionResize)
    {
        HideToolTip();

        // It's necessary to reset m_actionPart since it destroyed
        // by the Update within DoEndResizeAction.
        if (m_currentDragItem != -1)
            m_actionPart = &(m_uiParts.Item(m_currentDragItem));
        else
            m_currentDragItem = m_uiParts.Index(* m_actionPart);

        if (m_actionPart)
        {
            wxPoint pos = m_actionPart->rect.GetPosition();
            if (m_actionPart->orientation == wxHORIZONTAL)
                pos.y = wxMax(0, evt.m_y - m_actionOffset.y);
            else
                pos.x = wxMax(0, evt.m_x - m_actionOffset.x);

            if (wxAuiManager_HasLiveResize(*this))
            {
                m_frame->ReleaseMouse();
                DoEndResizeAction(evt);
                m_frame->CaptureMouse();
            }
            else
            {
                wxRect rect(m_frame->ClientToScreen(pos),
                    m_actionPart->rect.GetSize());
                wxScreenDC dc;

                if (!m_actionHintRect.IsEmpty())
                {
                    // remove old resize hint
                    DrawResizeHint(dc, m_actionHintRect);
                    m_actionHintRect = wxRect();
                }

                // draw new resize hint, if it's inside the managed frame
                wxRect frameScreenRect = m_frame->GetScreenRect();
                if (frameScreenRect.Contains(rect))
                {
                    DrawResizeHint(dc, rect);
                    m_actionHintRect = rect;
                }
            }
        }
    }
    else if (m_action == actionClickCaption)
    {
        HideToolTip();

        int dragXThreshold = wxSystemSettings::GetMetric(wxSYS_DRAG_X);
        int dragYThreshold = wxSystemSettings::GetMetric(wxSYS_DRAG_Y);

        // caption has been clicked.  we need to check if the mouse
        // is now being dragged. if it is, we need to change the
        // mouse action to 'drag'
        if (m_actionPart && (abs(evt.m_x - m_actionStart.x) > dragXThreshold || abs(evt.m_y - m_actionStart.y) > dragYThreshold))
        {
            wxAuiPaneInfo* paneInfo = m_actionPart->pane;

            if (!paneInfo->IsToolbar())
            {
                // If we are a wxAuiNotebook then we must fire off a wxEVT_COMMAND_AUINOTEBOOK_BEGIN_DRAG event to notify user of change.
                if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
                {
                    wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_BEGIN_DRAG, GetManagedWindow()->GetId());
                    e.SetSelection(GetAllPanes().Index(*paneInfo));
                    e.SetEventObject(GetManagedWindow());
                    GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
                }

                // If it is a notebook pane then we don't want to float it unless we have left the notebook tab bar.
                // Instead we want to move it around within the notebook.
                if(m_actionPart->type==wxAuiDockUIPart::typePaneTab)
                {
                    wxAuiDockUIPart* hitpart = HitTest(evt.GetX(), evt.GetY());
                    if(m_actionPart==hitpart)
                    {
                        wxAuiPaneInfo comp = *paneInfo;
                        DoDrop(m_docks, m_panes, *paneInfo, mousePos, wxPoint(0,0));
                        // Only call update if the pane has actually moved within the notebook to prevent flickering.
                        if(comp.GetPage() != paneInfo->GetPage())
                        {
                            Update();
                            Repaint();
                            m_actionPart = HitTest(evt.GetX(), evt.GetY());
                            m_actionPart->pane = paneInfo;
                            // Reset position of 'action start' otherwise we run the possibility of tabs 'jumping'
                            mousePos.x=evt.m_x;
                            mousePos.y=evt.m_y;
                            // If tabs are differently sized then we need to set a 'dead' zone in order to prevent the tab immediately switching around again on next mouse move event
                            if(!HasFlag(wxAUI_MGR_NB_TAB_FIXED_WIDTH))
                            {
                                //Better way to do this?
                                wxAuiPaneInfo* hitPane=NULL;
                                m_actionPart->m_tab_container->TabHitTest(evt.GetX(), evt.GetY(),&hitPane);
                                if(hitPane&&hitPane->GetPage()!=paneInfo->GetPage())
                                {
                                    wxMemoryDC Temp;
                                    bool closeButton = false;
                                    if(TabHasCloseButton(GetFlags(),*paneInfo))
                                    {
                                        closeButton = true;
                                    }
                                    int oldhitextent;
                                    m_actionPart->m_tab_container->GetArtProvider()->GetTabSize(Temp,paneInfo->GetWindow(), paneInfo->GetCaption(), paneInfo->GetBitmap(),true,closeButton ? wxAUI_BUTTON_STATE_NORMAL : wxAUI_BUTTON_STATE_HIDDEN,&oldhitextent);


                                    closeButton=false;
                                    if(TabHasCloseButton(GetFlags(),*paneInfo))
                                    {
                                        closeButton = true;
                                    }
                                    int newhitextent;
                                    m_actionPart->m_tab_container->GetArtProvider()->GetTabSize(Temp,hitPane->GetWindow(),hitPane->GetCaption(),hitPane->GetBitmap(),false,closeButton ? wxAUI_BUTTON_STATE_NORMAL : wxAUI_BUTTON_STATE_HIDDEN,&newhitextent);

                                    if(hitPane->GetPage()<paneInfo->GetPage() && newhitextent>oldhitextent)
                                    {
                                        m_actionDeadZone = new wxRect(mousePos.x,m_actionPart->rect.y,mousePos.x+(newhitextent-oldhitextent),m_actionPart->rect.height);
                                    }
                                    else if(hitPane->GetPage()>paneInfo->GetPage() && newhitextent>oldhitextent)
                                    {
                                        int diff = (newhitextent-(newhitextent-oldhitextent));
                                        m_actionDeadZone = new wxRect(mousePos.x-diff,m_actionPart->rect.y,diff,m_actionPart->rect.height);
                                    }
                                }
                            }
                        }
                        return;
                    }
                }

                if (HasFlag(wxAUI_MGR_ALLOW_FLOATING) && paneInfo->IsFloatable())
                {
                    m_action = actionDragFloatingPane;

                    // set initial float position
                    wxPoint pt = m_frame->ClientToScreen(evt.GetPosition());
                    paneInfo->SetFloatingPosition(wxPoint(pt.x - m_actionOffset.x,
                                                      pt.y - m_actionOffset.y));

                    if(m_actionPart->type == wxAuiDockUIPart::typePaneTab)
                    {
                        m_actionPart->m_tab_container->RemovePage(paneInfo->GetWindow());
                    }
                    // float the window
                    if (paneInfo->IsMaximized())
                        RestorePane(*paneInfo);
                    paneInfo->Float();
                    Update();

                    m_actionWindow = paneInfo->GetFrame();

                    // action offset is used here to make it feel "natural" to the user
                    // to drag a docked pane and suddenly have it become a floating frame.
                    // Sometimes, however, the offset where the user clicked on the docked
                    // caption is bigger than the width of the floating frame itself, so
                    // in that case we need to set the action offset to a sensible value
                    wxSize frameSize = m_actionWindow->GetSize();
                    if (frameSize.x <= m_actionOffset.x)
                        m_actionOffset.x = 30;
                }
                else if(paneInfo->IsMovable())
                {
                    m_action = actionDragMovablePane;
                    m_actionWindow = paneInfo->GetWindow();
                }
            }
            else
            {
                m_action = actionDragToolbarPane;
                m_actionWindow = paneInfo->GetWindow();
            }
        }
    }
    else if (m_action == actionDragFloatingPane)
    {
        HideToolTip();

        if (m_actionWindow)
        {
            // We can't move the child window so we need to get the frame that
            // we want to be really moving. This is probably not the best place
            // to do this but at least it fixes the bug (#13177) for now.
            if (!wxDynamicCast(m_actionWindow,wxAuiFloatingFrame))
            {
                wxAuiPaneInfo& pane = GetPane(m_actionWindow);
                m_actionWindow = pane.frame;
            }

            wxPoint pt = m_frame->ClientToScreen(evt.GetPosition());
            m_actionWindow->Move(pt.x - m_actionOffset.x,
                                pt.y - m_actionOffset.y);

            // If we are a wxAuiNotebook then we must fire off a wxEVT_COMMAND_AUINOTEBOOK_DRAG_MOTION event to notify user of change.
            if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
            {
                wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_DRAG_MOTION, GetManagedWindow()->GetId());
                e.SetSelection(GetAllPanes().Index(GetPane(m_actionWindow)));
                e.SetEventObject(GetManagedWindow());
                GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
            }
        }
    }
    else if (m_action == actionDragMovablePane)
    {
        HideToolTip();

        // If the movable pane is part of a notebook and the mouse has moved back over the notebook tabs then we set the
        // event type back to a caption click so that we can move the actual tab around instead of drawing hints.
        if(m_actionPart->type==wxAuiDockUIPart::typePaneTab)
        {
            wxAuiDockUIPart* hitpart = HitTest(evt.GetX(), evt.GetY());
            if(m_actionPart==hitpart)
            {
                HideHint();
                m_action = actionClickCaption;
                m_actionWindow = NULL;
                return;
            }
        }

        // Try to find the pane.
        wxAuiPaneInfo& pane = GetPane(m_actionWindow);
        wxASSERT_MSG(pane.IsOk(), wxT("Pane window not found"));

        // Draw a hint for where the window will be moved.
        wxPoint pt = evt.GetPosition();
        DrawHintRect(m_actionWindow, pt, wxPoint(0,0));

        // Reduces flicker.
        m_frame->Update();

        // If we are a wxAuiNotebook then we must fire off a wxEVT_COMMAND_AUINOTEBOOK_DRAG_MOTION event to notify user of change.
        if(wxDynamicCast(GetManagedWindow(),wxAuiNotebook))
        {
            wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_DRAG_MOTION, GetManagedWindow()->GetId());
            e.SetSelection(GetAllPanes().Index(GetPane(m_actionWindow)));
            e.SetEventObject(GetManagedWindow());
            GetManagedWindow()->GetEventHandler()->ProcessEvent(e);
        }
    }
    else if (m_action == actionDragToolbarPane)
    {
        HideToolTip();

        wxAuiPaneInfo& pane = GetPane(m_actionWindow);
        wxASSERT_MSG(pane.IsOk(), wxT("Pane window not found"));

        pane.SetFlag(wxAuiPaneInfo::actionPane, true);

        wxPoint pt = evt.GetPosition();
        DoDrop(m_docks, m_panes, pane, pt, m_actionOffset);

        // if DoDrop() decided to float the pane, set up
        // the floating pane's initial position
        if (pane.IsFloating())
        {
            wxPoint pt = m_frame->ClientToScreen(evt.GetPosition());
            pane.SetFloatingPosition(wxPoint(pt.x - m_actionOffset.x,
                                        pt.y - m_actionOffset.y));
        }

        // this will do the actiual move operation;
        // in the case that the pane has been floated,
        // this call will create the floating pane
        // and do the reparenting
        Update();

        // if the pane has been floated, change the mouse
        // action actionDragFloatingPane so that subsequent
        // EVT_MOTION() events will move the floating pane
        if (pane.IsFloating())
        {
            pane.SetFlag(wxAuiPaneInfo::actionPane,false);
            m_action = actionDragFloatingPane;
            m_actionWindow = pane.GetFrame();
        }
    }
    else
    {
        wxAuiDockUIPart* part = HitTest(evt.GetX(), evt.GetY());
        if (part)
        {
            //For caption(s) and tab(s) set tooltip if one is present, for everything else cancel any existing tooltip.
            if(part->type == wxAuiDockUIPart::typePaneTab)
            {
                wxAuiPaneInfo* hitPane=NULL;
                if(part->m_tab_container->TabHitTest(evt.GetX(), evt.GetY(), &hitPane))
                {
                    ShowToolTip(hitPane);
                }
                else
                {
                    HideToolTip();
                }

                wxAuiTabContainerButton* hitbutton;
                if(part->m_tab_container->ButtonHitTest(evt.m_x,evt.m_y,&hitbutton))
                {
                    // make the old button normal
                    if (m_hoverButton)
                    {
                        UpdateButtonOnScreen(m_hoverButton, evt);
                        Repaint();
                    }
                    if(m_hoverButton2 != hitbutton)
                    {
                        if (m_hoverButton2)
                        {
                            m_hoverButton2->curState = wxAUI_BUTTON_STATE_NORMAL;
                            Repaint();
                        }

                        m_hoverButton2 = hitbutton;
                        m_hoverButton2->curState = wxAUI_BUTTON_STATE_HOVER;
                        Repaint();
                    }

                    return;
                }
            }
            else if(part->type == wxAuiDockUIPart::typeCaption)
            {
                ShowToolTip(part->pane);
            }
            else
            {
                HideToolTip();

                //Handle other part types as appropriate.
                if(part->type == wxAuiDockUIPart::typePaneButton)
                {
                    if (m_hoverButton2)
                    {
                        m_hoverButton2->curState = wxAUI_BUTTON_STATE_NORMAL;
                        Repaint();
                    }
                    if (part != m_hoverButton)
                    {
                        // make the old button normal
                        if (m_hoverButton)
                        {
                            UpdateButtonOnScreen(m_hoverButton, evt);
                            Repaint();
                        }

                        // mouse is over a button, so repaint the
                        // button in hover mode
                        UpdateButtonOnScreen(part, evt);
                        m_hoverButton = part;
                    }
                    return;
                }
            }
        }
        else
        {
            HideToolTip();
        }

        if (m_hoverButton||m_hoverButton2)
        {
            m_hoverButton = NULL;
            if(m_hoverButton2)
            {
                m_hoverButton2->curState = wxAUI_BUTTON_STATE_NORMAL;
            }
            Repaint();
            m_hoverButton2 = NULL;
        }
        else
        {
            evt.Skip();
        }
    }
}

void wxAuiManager::OnLeaveWindow(wxMouseEvent& WXUNUSED(evt))
{
    HideToolTip();

    if (m_hoverButton)
    {
        m_hoverButton = NULL;
        Repaint();
    }
}

void wxAuiManager::OnCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(evt))
{
    HideToolTip();

    // cancel the operation in progress, if any
    if ( m_action != actionNone )
    {
        m_action = actionNone;
        HideHint();
        delete m_actionDeadZone;
        m_actionDeadZone = NULL;
    }
}

void wxAuiManager::OnChildFocus(wxChildFocusEvent& evt)
{
    bool refresh = false;
    // Ensure none of the notebook controls have focus set
    unsigned int i;
    for (i = 0; i < m_uiParts.GetCount(); i++)
    {
        wxAuiDockUIPart& part = m_uiParts.Item(i);
        if(part.m_tab_container)
        {
            if(part.m_tab_container->HasFocus())
            {
                part.m_tab_container->SetFocus(false);
                refresh = true;
            }
        }
    }

    // when a child pane has its focus set, we should change the
    // pane's active state to reflect this. (this is only true if
    // active panes are allowed by the owner)
    if (HasFlag(wxAUI_MGR_ALLOW_ACTIVE_PANE))
    {
        wxAuiPaneInfo& pane = GetPane(evt.GetWindow());
        if (pane.IsOk() && !pane.HasFlag(wxAuiPaneInfo::optionActive))
        {
            SetActivePane(evt.GetWindow());
            refresh = true;
        }
    }

    if(refresh)
    {
        m_frame->Refresh();
    }

    evt.Skip();
}


// OnPaneButton() is an event handler that is called
// when a pane button has been pressed.
void wxAuiManager::OnPaneButton(wxAuiManagerEvent& evt)
{
    wxASSERT_MSG(evt.pane, wxT("Pane Info passed to wxAuiManager::OnPaneButton must be non-null"));

    wxAuiPaneInfo& pane = *(evt.pane);

    if (evt.button == wxAUI_BUTTON_CLOSE)
    {
        wxAuiPaneInfo* closepane = evt.pane;

        // If the close button is on a notebook tab then we want to close that tab and not the one from the event.
        wxPoint pt = m_actionStart;
        wxAuiDockUIPart* part = HitTest(pt.x, pt.y);
        if (part && part->type == wxAuiDockUIPart::typePaneTab)
        {
            wxAuiPaneInfo* hitPane=NULL;
            if(part->m_tab_container->TabHitTest(pt.x,pt.y,&hitPane))
            {
                closepane = hitPane;
            }
        }

        if(ClosePane(*closepane))
            Update();
    }
    else if (evt.button == wxAUI_BUTTON_MAXIMIZE_RESTORE && !pane.IsMaximized())
    {
        // fire pane maximize event
        wxAuiManagerEvent e(wxEVT_AUI_PANE_MAXIMIZE);
        e.SetManager(this);
        e.SetPane(evt.pane);
        ProcessMgrEvent(e);

        if (!e.GetVeto())
        {
            MaximizePane(pane);
            Update();
        }
    }
    else if (evt.button == wxAUI_BUTTON_MAXIMIZE_RESTORE && pane.IsMaximized())
    {
        // fire pane restore event
        wxAuiManagerEvent e(wxEVT_AUI_PANE_RESTORE);
        e.SetManager(this);
        e.SetPane(evt.pane);
        ProcessMgrEvent(e);

        if (!e.GetVeto())
        {
            RestorePane(pane);
            Update();
        }
    }
    else if (evt.button == wxAUI_BUTTON_PIN && (m_flags & wxAUI_MGR_ALLOW_FLOATING) && pane.IsFloatable())
    {
        if (pane.IsMaximized())
        {
             // If the pane is maximized, the original state must be restored
             // before trying to float the pane, otherwise the other panels
             // wouldn't appear correctly when it becomes floating.
             wxAuiManagerEvent e(wxEVT_AUI_PANE_RESTORE);
             e.SetManager(this);
             e.SetPane(evt.pane);
             ProcessMgrEvent(e);

             if (e.GetVeto())
             {
                 // If it can't be restored, it can't be floated neither.
                 return;
             }
             RestorePane(pane);
        }
        pane.Float();
        Update();
    }
    else if (evt.button == wxAUI_BUTTON_LEFT || evt.button == wxAUI_BUTTON_UP)
    {
        m_actionPart->m_tab_container->SetTabOffset(m_actionPart->m_tab_container->GetTabOffset()-1);
        Repaint();
    }
    else if (evt.button == wxAUI_BUTTON_RIGHT || evt.button == wxAUI_BUTTON_DOWN)
    {
        m_actionPart->m_tab_container->SetTabOffset(m_actionPart->m_tab_container->GetTabOffset()+1);
        Repaint();
    }
    else if (evt.button == wxAUI_BUTTON_WINDOWLIST)
    {
        int selection = m_actionPart->m_tab_container->GetArtProvider()->ShowDropDown(m_frame, m_actionPart->m_tab_container->GetPages(), m_actionPart->m_tab_container->GetActivePage());
        if (selection != -1)
        {
            m_actionPart->m_tab_container->SetActivePage(selection);
        }
        DoFrameLayout();
        Repaint();
    }
}

#endif // wxUSE_AUI
