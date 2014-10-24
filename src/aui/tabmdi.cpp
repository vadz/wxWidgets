/////////////////////////////////////////////////////////////////////////////
// Name:        src/aui/tabmdi.cpp
// Purpose:     Generic MDI (Multiple Document Interface) classes
// Author:      Hans Van Leemputten
// Modified by: Benjamin I. Williams / Kirix Corporation
// Created:     29/07/2002
// RCS-ID:      $Id$
// Copyright:   (c) Hans Van Leemputten
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_AUI
#if wxUSE_MDI

#include "wx/aui/tabmdi.h"

#ifndef WX_PRECOMP
    #include "wx/panel.h"
    #include "wx/menu.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/settings.h"
#endif //WX_PRECOMP

#include "wx/stockitem.h"
#include "wx/aui/dockart.h"

//-----------------------------------------------------------------------------
// wxAuiMDIParentFrame
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxAuiMDIParentFrame, wxAuiMDIParentFrameBase)

BEGIN_EVENT_TABLE(wxAuiMDIParentFrame, wxAuiMDIParentFrameBase)
#if wxUSE_MENUS
    EVT_MENU (wxID_ANY, wxAuiMDIParentFrame::OnHandleMenu)
    EVT_UPDATE_UI (wxID_ANY, wxAuiMDIParentFrame::DoHandleUpdateUI)
#endif
END_EVENT_TABLE()

wxAuiMDIParentFrame::wxAuiMDIParentFrame()
    : m_pLastEvt(NULL)
#if wxUSE_MENUS
    , m_pMyMenuBar(NULL)
#endif // wxUSE_MENUS
{}

wxAuiMDIParentFrame::wxAuiMDIParentFrame(wxWindow *parent,
                                         wxWindowID id,
                                         const wxString& title,
                                         const wxPoint& pos,
                                         const wxSize& size,
                                         long style,
                                         const wxString& name)
    : m_pLastEvt(NULL)
#if wxUSE_MENUS
    , m_pMyMenuBar(NULL)
#endif // wxUSE_MENUS
{
    (void)Create(parent, id, title, pos, size, style, name);
}

wxAuiMDIParentFrame::~wxAuiMDIParentFrame()
{
    // Avoid having GetActiveChild() called after m_pClientWindow is destroyed
    SendDestroyEvent();

#if wxUSE_MENUS
    wxDELETE(m_pMyMenuBar);
    RemoveWindowMenu(GetMenuBar());
#endif // wxUSE_MENUS
}

bool wxAuiMDIParentFrame::Create(wxWindow *parent,
                                 wxWindowID id,
                                 const wxString& title,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                 long style,
                                 const wxString& name)
{
#if wxUSE_MENUS
    // this style can be used to prevent a window from having the standard MDI
    // "Window" menu
    if (!(style & wxFRAME_NO_WINDOW_MENU))
    {
        m_windowMenu = new wxMenu;
        m_windowMenu->Append(wxWINDOWCLOSE,    _("Cl&ose"));
        m_windowMenu->Append(wxWINDOWCLOSEALL, _("Close All"));
        m_windowMenu->AppendSeparator();
        m_windowMenu->Append(wxWINDOWNEXT,     _("&Next"));
        m_windowMenu->Append(wxWINDOWPREV,     _("&Previous"));
    }
#endif // wxUSE_MENUS

    if ( !wxAuiMDIParentFrameBase::Create(parent, id, title, pos, size, style, name) )
        return false;

    m_clientWindow = OnCreateClient();
    return m_clientWindow != NULL;
}


void wxAuiMDIParentFrame::SetArtProvider(wxAuiTabArt* provider)
{
    if (m_clientWindow)
        m_clientWindow->SetArtProvider(provider);
}

wxAuiTabArt* wxAuiMDIParentFrame::GetArtProvider()
{
    if (!m_clientWindow)
        return NULL;

    return m_clientWindow->GetArtProvider();
}

#if wxUSE_MENUS
void wxAuiMDIParentFrame::SetWindowMenu(wxMenu* pMenu)
{
    // Replace the window menu from the currently loaded menu bar.
    wxMenuBar *pMenuBar = GetMenuBar();

    if (m_windowMenu)
    {
        RemoveWindowMenu(pMenuBar);
        wxDELETE(m_windowMenu);
    }

    if (pMenu)
    {
        m_windowMenu = pMenu;
        AddWindowMenu(pMenuBar);
    }
}

void wxAuiMDIParentFrame::SetMenuBar(wxMenuBar* pMenuBar)
{
    // Remove the Window menu from the old menu bar
    RemoveWindowMenu(GetMenuBar());

    // Add the Window menu to the new menu bar.
    AddWindowMenu(pMenuBar);

    wxAuiMDIParentFrameBase::SetMenuBar(pMenuBar);
    //m_pMyMenuBar = GetMenuBar();
}
#endif // wxUSE_MENUS

void wxAuiMDIParentFrame::SetChildMenuBar(wxAuiMDIChildFrame* pChild)
{
#if wxUSE_MENUS
    if (!pChild)
    {
        // No Child, set Our menu bar back.
        if (m_pMyMenuBar)
            SetMenuBar(m_pMyMenuBar);
        else
            SetMenuBar(GetMenuBar());

        // Make sure we know our menu bar is in use
        m_pMyMenuBar = NULL;
    }
    else
    {
        if (pChild->GetMenuBar() == NULL)
            return;

        // Do we need to save the current bar?
        if (m_pMyMenuBar == NULL)
            m_pMyMenuBar = GetMenuBar();

        SetMenuBar(pChild->GetMenuBar());
    }
#endif // wxUSE_MENUS
}

wxAuiMDIClientWindow *wxAuiMDIParentFrame::OnCreateClient()
{
    if (!m_clientWindow)
        m_clientWindow = new wxAuiMDIClientWindow( this );
    return m_clientWindow;
}

wxAuiNotebook* wxAuiMDIParentFrame::GetNotebook() const
{
    return m_clientWindow;
}


bool wxAuiMDIParentFrame::ProcessEvent(wxEvent& event)
{
    // stops the same event being processed repeatedly
    if (m_pLastEvt == &event)
        return false;
    m_pLastEvt = &event;

    bool res = false;
    //Do not treat the events which cause the arbitrary selection of the first
    //notebook tab when the parent frame is restored from minimized state
    if (!(event.GetEventType() == wxEVT_ACTIVATE ||
          event.GetEventType() == wxEVT_SET_FOCUS ||
          event.GetEventType() == wxEVT_KILL_FOCUS ||
          event.GetEventType() == wxEVT_CHILD_FOCUS ||
          event.GetEventType() == wxEVT_COMMAND_SET_FOCUS ||
          event.GetEventType() == wxEVT_COMMAND_KILL_FOCUS) )
    {
        // let the active child (if any) process the event first.
        wxAuiMDIChildFrame* pActiveChild = GetActiveChild();
        if (pActiveChild &&
            event.IsCommandEvent() &&
            (event.GetEventObject() != m_clientWindow) )
        {
            res = pActiveChild->GetEventHandler()->ProcessEvent(event);
        }

        if (!res)
        {
            // if the event was not handled this frame will handle it,
            // which is why we need the protection code at the beginning
            // of this method
            res = wxEvtHandler::ProcessEvent(event);
        }
    }

    m_pLastEvt = NULL;

    return res;
}

void wxAuiMDIParentFrame::ActivateNext()
{
    if (GetClientWindow() && GetClientWindow()->GetSelection() != wxNOT_FOUND)
    {
        size_t active = GetClientWindow()->GetSelection() + 1;
        if (active >= GetClientWindow()->GetPageCount())
            active = 0;

        GetClientWindow()->SetSelection(active);
    }
}

void wxAuiMDIParentFrame::ActivatePrevious()
{
    if (GetClientWindow() && GetClientWindow()->GetSelection() != wxNOT_FOUND)
    {
        int active = GetClientWindow()->GetSelection() - 1;
        if (active < 0)
            active = GetClientWindow()->GetPageCount() - 1;

        GetClientWindow()->SetSelection(active);
    }
}

#if wxUSE_MENUS
void wxAuiMDIParentFrame::RemoveWindowMenu(wxMenuBar* pMenuBar)
{
    if (pMenuBar && m_windowMenu)
    {
        // Remove old window menu
        int pos = pMenuBar->FindMenu(_("&Window"));
        if (pos != wxNOT_FOUND)
        {
            // DBG:: We're going to delete the wrong menu!!!
            wxASSERT(m_windowMenu == pMenuBar->GetMenu(pos));
            pMenuBar->Remove(pos);
        }
    }
}

void wxAuiMDIParentFrame::AddWindowMenu(wxMenuBar *pMenuBar)
{
    if (pMenuBar && m_windowMenu)
    {
        int pos = pMenuBar->FindMenu(wxGetStockLabel(wxID_HELP,wxSTOCK_NOFLAGS));
        if (pos == wxNOT_FOUND)
            pMenuBar->Append(m_windowMenu, _("&Window"));
        else
            pMenuBar->Insert(pos, m_windowMenu, _("&Window"));
    }
}

void wxAuiMDIParentFrame::DoHandleMenu(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        case wxWINDOWCLOSE:
        {
            if (GetActiveChild())
                GetActiveChild()->Close();
            break;
        }
        case wxWINDOWCLOSEALL:
        {
            while (GetActiveChild() != NULL)
            {
                if (!GetActiveChild()->Close())
                {
                    return; // failure
                }
            }
            break;
        }
        case wxWINDOWNEXT:
            ActivateNext();
            break;
        case wxWINDOWPREV:
            ActivatePrevious();
            break;
        default:
            event.Skip();
    }
}

void wxAuiMDIParentFrame::DoHandleUpdateUI(wxUpdateUIEvent& event)
{
    switch (event.GetId())
    {
        case wxWINDOWCLOSE:
        case wxWINDOWCLOSEALL:
        {
            wxAuiMDIClientWindow* client_window = GetClientWindow();
            wxCHECK_RET(client_window, wxS("Missing MDI Client Window"));
            size_t pages = client_window->GetPageCount();
            event.Enable(pages >= 1);
            break;
        }

        case wxWINDOWNEXT:
        case wxWINDOWPREV:
        {
            wxAuiMDIClientWindow* client_window = GetClientWindow();
            wxCHECK_RET(client_window, wxS("Missing MDI Client Window"));
            size_t pages = client_window->GetPageCount();
            event.Enable(pages >= 2);
            break;
        }

        default:
            event.Skip();
    }
}
#endif // wxUSE_MENUS

void wxAuiMDIParentFrame::DoGetClientSize(int* width, int* height) const
{
    wxFrame::DoGetClientSize(width, height);
}

void wxAuiMDIParentFrame::Tile(wxOrientation orient)
{
    wxASSERT_MSG(GetClientWindow(), wxT("Missing MDI Client Window"));

    int cur_idx = GetClientWindow()->GetSelection();
    if (cur_idx == -1)
        return;

    if (orient == wxVERTICAL)
    {
        GetClientWindow()->Split(cur_idx, wxLEFT);
    }
    else if (orient == wxHORIZONTAL)
    {
        GetClientWindow()->Split(cur_idx, wxTOP);
    }
}


//-----------------------------------------------------------------------------
// wxAuiMDIChildFrame
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxAuiMDIChildFrame, wxAuiMDIChildFrameBase)

BEGIN_EVENT_TABLE(wxAuiMDIChildFrame, wxAuiMDIChildFrameBase)
    EVT_MENU_HIGHLIGHT_ALL(wxAuiMDIChildFrame::OnMenuHighlight)
END_EVENT_TABLE()

wxAuiMDIChildFrame::wxAuiMDIChildFrame()
    : m_doActivate(true)
    , m_activateOnCreate(true)
#if wxUSE_MENUS
    , m_pMenuBar(NULL)
#endif // wxUSE_MENUS
{}

wxAuiMDIChildFrame::wxAuiMDIChildFrame(wxAuiMDIParentFrame *parent,
                                       wxWindowID id,
                                       const wxString& title,
                                       const wxPoint& WXUNUSED(pos),
                                       const wxSize& size,
                                       long style,
                                       const wxString& name)
    : m_doActivate(true)
    , m_activateOnCreate(true)
#if wxUSE_MENUS
    , m_pMenuBar(NULL)
#endif // wxUSE_MENUS
{

    // There are two ways to create an tabbed MDI child frame without
    // making it the active document.  Either Show(false) can be called
    // before Create() (as is customary on some ports with wxFrame-type
    // windows), or wxMINIMIZE can be passed in the style flags.  Note that
    // wxAuiMDIChildFrame is not really derived from wxFrame, as wxMDIChildFrame
    // is, but those are the expected semantics.  No style flag is passed
    // onto the panel underneath.
    if (style & wxMINIMIZE)
        m_activateOnCreate = false;

    Create(parent, id, title, wxDefaultPosition, size, 0, name);
}

wxAuiMDIChildFrame::~wxAuiMDIChildFrame()
{
    if (m_mdiParent)
    {
        if (m_mdiParent->GetActiveChild() == this)
        {
            m_doActivate = false;
            m_mdiParent->SetChildMenuBar(NULL);
        }
        wxAuiMDIClientWindow* pClientWindow = m_mdiParent->GetClientWindow();
        wxASSERT(pClientWindow);
        int idx = pClientWindow->GetPageIndex(this);
        if (idx != wxNOT_FOUND)
        {
            pClientWindow->RemovePage(idx);
        }
    }

#if wxUSE_MENUS
    wxDELETE(m_pMenuBar);
#endif // wxUSE_MENUS
}

bool wxAuiMDIChildFrame::Create(wxAuiMDIParentFrame* parent,
                                wxWindowID id,
                                const wxString& title,
                                const wxPoint& WXUNUSED(pos),
                                const wxSize& size,
                                long style,
                                const wxString& name)
{
    wxAuiMDIClientWindow* pClientWindow = parent->GetClientWindow();
    wxASSERT_MSG((pClientWindow != NULL), wxT("Missing MDI client window."));

    // see comment in constructor
    if (style & wxMINIMIZE)
        m_activateOnCreate = false;

    wxSize cli_size;
    if (pClientWindow)
        cli_size = pClientWindow->GetClientSize();

    // create the window off-screen to prevent flicker
    wxAuiMDIChildFrameBase::Create(parent,
                                   id,
                                   title,
                                   wxPoint(cli_size.x+1, cli_size.y+1),
                                   size,
                                   wxNO_BORDER,
                                   name);

    DoShow(false);

    m_mdiParent = parent;

    m_title = title;

    pClientWindow->AddPage(this, title, m_activateOnCreate);

    // Check that the parent notion of the active child coincides with our one.
    // This is less obvious that it seems because we must honour
    // m_activateOnCreate flag but only if it's not the first child because
    // this one becomes active unconditionally.
    wxASSERT_MSG
    (
        (m_activateOnCreate || pClientWindow->GetPageCount() == 1)
            == (parent->GetActiveChild() == this),
        wxS("Logic error: child [not] activated when it should [not] have been.")
    );

    pClientWindow->Refresh();

    return true;
}

bool wxAuiMDIChildFrame::Destroy()
{
    wxASSERT_MSG(m_mdiParent, wxT("Missing MDI Parent Frame"));

    wxAuiMDIClientWindow* pClientWindow = m_mdiParent->GetClientWindow();
    wxASSERT_MSG(pClientWindow, wxT("Missing MDI Client Window"));

    if (m_mdiParent->GetActiveChild() == this)
    {
        // deactivate ourself
        wxActivateEvent event(wxEVT_ACTIVATE, false, GetId());
        event.SetEventObject(this);
        GetEventHandler()->ProcessEvent(event);
        m_doActivate = false;

        m_mdiParent->SetChildMenuBar(NULL);
    }

    size_t page_count = pClientWindow->GetPageCount();
    for (size_t pos = 0; pos < page_count; pos++)
    {
        if (pClientWindow->GetPage(pos) == this)
            return pClientWindow->DeletePage(pos);
    }

    return false;
}

#if wxUSE_MENUS
void wxAuiMDIChildFrame::SetMenuBar(wxMenuBar *menu_bar)
{
    wxMenuBar *pOldMenuBar = m_pMenuBar;
    m_pMenuBar = menu_bar;

    if (m_pMenuBar)
    {
        wxASSERT_MSG(m_mdiParent, wxT("Missing MDI Parent Frame"));

        m_pMenuBar->SetParent(m_mdiParent);
        if (m_mdiParent->GetActiveChild() == this)
        {
            // replace current menu bars
            if (pOldMenuBar)
                m_mdiParent->SetChildMenuBar(NULL);
            m_mdiParent->SetChildMenuBar(this);
        }
    }
}

wxMenuBar *wxAuiMDIChildFrame::GetMenuBar() const
{
    return m_pMenuBar;
}
#endif // wxUSE_MENUS

void wxAuiMDIChildFrame::SetTitle(const wxString& title)
{
    m_title = title;

    wxASSERT_MSG(m_mdiParent, wxT("Missing MDI Parent Frame"));

    wxAuiMDIClientWindow* pClientWindow = m_mdiParent->GetClientWindow();
    if (pClientWindow != NULL)
    {
        size_t pos;
        for (pos = 0; pos < pClientWindow->GetPageCount(); pos++)
        {
            if (pClientWindow->GetPage(pos) == this)
            {
                pClientWindow->SetPageText(pos, m_title);
                break;
            }
        }
    }
}

wxString wxAuiMDIChildFrame::GetTitle() const
{
    return m_title;
}

void wxAuiMDIChildFrame::SetIcons(const wxIconBundle& icons)
{
    // get icon with the system icon size
    SetIcon(icons.GetIcon(-1));
    m_iconBundle = icons;
}

const wxIconBundle& wxAuiMDIChildFrame::GetIcons() const
{
    return m_iconBundle;
}

void wxAuiMDIChildFrame::SetIcon(const wxIcon& icon)
{
    wxAuiMDIParentFrame* pParentFrame = GetMDIParentFrame();
    wxASSERT_MSG(pParentFrame, wxT("Missing MDI Parent Frame"));

    m_icon = icon;

    wxBitmap bmp;
    bmp.CopyFromIcon(m_icon);

    wxAuiMDIClientWindow* pClientWindow = pParentFrame->GetClientWindow();
    if (pClientWindow != NULL)
    {
        int idx = pClientWindow->GetPageIndex(this);

        if (idx != -1)
        {
            pClientWindow->SetPageBitmap((size_t)idx, bmp);
        }
    }
}

const wxIcon& wxAuiMDIChildFrame::GetIcon() const
{
    return m_icon;
}


void wxAuiMDIChildFrame::Activate()
{
    wxASSERT_MSG(m_mdiParent, wxT("Missing MDI Parent Frame"));

    wxAuiMDIClientWindow* pClientWindow = m_mdiParent->GetClientWindow();

    if (pClientWindow != NULL)
    {
        size_t pos;
        for (pos = 0; pos < pClientWindow->GetPageCount(); pos++)
        {
            if (pClientWindow->GetPage(pos) == this)
            {
                pClientWindow->SetSelection(pos);
                break;
            }
        }
    }
}

void wxAuiMDIChildFrame::OnMenuHighlight(wxMenuEvent& event)
{
#if wxUSE_STATUSBAR
    if (m_mdiParent)
    {
        // we don't have any help text for this item,
        // but may be the MDI frame does?
        m_mdiParent->OnMenuHighlight(event);
    }
#else
    wxUnusedVar(event);
#endif // wxUSE_STATUSBAR
}

void wxAuiMDIChildFrame::OnActivate(wxActivateEvent& evt)
{
    if (m_doActivate)
        DoActivate(evt);
}

bool wxAuiMDIChildFrame::Show(bool show)
{
    // wxAuiMDIChildFrame uses m_activateOnCreate only to decide whether to
    // activate the frame when it is created.  After Create() is called,
    // m_activateOnCreate will never be read again.  Therefore, calling this
    // function after Create() is pointless and you probably want to call
    // Activate() instead.
    wxCHECK_MSG( !GetHandle(), false,
                 wxS("Show() has no effect after Create(). Do you mean Activate()?") );

    m_activateOnCreate = show;

    // do nothing
    return true;
}

void wxAuiMDIChildFrame::DoShow(bool show)
{
    wxWindow::Show(show);
}

void wxAuiMDIChildFrame::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
    m_mdiNewRect = wxRect(x, y, width, height);
#ifdef __WXGTK__
    wxAuiMDIChildFrameBase::DoSetSize(x,y,width, height, sizeFlags);
#else
    wxUnusedVar(sizeFlags);
#endif
}

void wxAuiMDIChildFrame::DoMoveWindow(int x, int y, int width, int height)
{
    m_mdiNewRect = wxRect(x, y, width, height);
}

void wxAuiMDIChildFrame::ApplyMDIChildFrameRect()
{
    if (m_mdiCurRect != m_mdiNewRect)
    {
        wxAuiMDIChildFrameBase::DoMoveWindow(m_mdiNewRect.x, m_mdiNewRect.y,
                                             m_mdiNewRect.width, m_mdiNewRect.height);
        m_mdiCurRect = m_mdiNewRect;
    }
}


//-----------------------------------------------------------------------------
// wxAuiMDIClientWindow
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxAuiMDIClientWindow, wxAuiNotebook)

BEGIN_EVENT_TABLE(wxAuiMDIClientWindow, wxAuiNotebook)
    EVT_AUINOTEBOOK_PAGE_CHANGED(wxID_ANY, wxAuiMDIClientWindow::OnPageChanged)
    EVT_AUINOTEBOOK_PAGE_CLOSE(wxID_ANY, wxAuiMDIClientWindow::OnPageClose)
    EVT_SIZE(wxAuiMDIClientWindow::OnSize)
END_EVENT_TABLE()

wxAuiMDIClientWindow::wxAuiMDIClientWindow(wxAuiMDIParentFrame* parent, long style)
{
    CreateClient(parent, style);
}

bool wxAuiMDIClientWindow::CreateClient(wxAuiMDIParentFrame* parent, long style)
{
    SetWindowStyleFlag(style);

    wxSize caption_icon_size =
            wxSize(wxSystemSettings::GetMetric(wxSYS_SMALLICON_X),
                   wxSystemSettings::GetMetric(wxSYS_SMALLICON_Y));
    SetUniformBitmapSize(caption_icon_size);

    if (!wxAuiNotebook::Create(parent,
                               wxID_ANY,
                               wxPoint(0,0),
                               wxSize(100, 100),
                               wxAUI_NB_DEFAULT_STYLE | wxNO_BORDER))
    {
        return false;
    }

    wxColour bkcolour = wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE);
    SetOwnBackgroundColour(bkcolour);

    m_mgr.GetArtProvider()->SetColour(wxAUI_DOCKART_BACKGROUND_COLOUR, bkcolour);

    return true;
}

int wxAuiMDIClientWindow::SetSelection(size_t nPage)
{
    return wxAuiNotebook::SetSelection(nPage);
}

wxAuiMDIChildFrame* wxAuiMDIClientWindow::GetActiveChild()
{
    const int sel = GetSelection();
    if ( sel == wxNOT_FOUND )
        return NULL;

    return wxStaticCast(GetPage(sel), wxAuiMDIChildFrame);
}

void wxAuiMDIClientWindow::PageChanged(int old_selection, int new_selection)
{
    // don't do anything if the page doesn't actually change
    if (old_selection == new_selection)
        return;

    /*
    // don't do anything if the new page is already active
    if (new_selection != -1)
    {
        wxAuiMDIChildFrame* child = (wxAuiMDIChildFrame*)GetPage(new_selection);
        if (child->GetMDIParentFrame()->GetActiveChild() == child)
            return;
    }*/


    // notify old active child that it has been deactivated
    if ((old_selection != -1) && (old_selection < (int)GetPageCount()))
    {
        wxAuiMDIChildFrame* old_child = (wxAuiMDIChildFrame*)GetPage(old_selection);
        wxASSERT_MSG(old_child, wxT("wxAuiMDIClientWindow::PageChanged - null page pointer"));

        wxActivateEvent event(wxEVT_ACTIVATE, false, old_child->GetId());
        event.SetEventObject(old_child);
        old_child->GetEventHandler()->ProcessEvent(event);
    }

    // notify new active child that it has been activated
    if (new_selection != -1)
    {
        wxAuiMDIChildFrame* active_child = (wxAuiMDIChildFrame*)GetPage(new_selection);
        wxASSERT_MSG(active_child, wxT("wxAuiMDIClientWindow::PageChanged - null page pointer"));

        wxActivateEvent event(wxEVT_ACTIVATE, true, active_child->GetId());
        event.SetEventObject(active_child);
        active_child->GetEventHandler()->ProcessEvent(event);

        if (active_child->m_mdiParent)
        {
            active_child->m_mdiParent->SetActiveChild(active_child);
            active_child->m_mdiParent->SetChildMenuBar(active_child);
        }
    }


}

void wxAuiMDIClientWindow::OnPageClose(wxAuiNotebookEvent& evt)
{
    wxAuiMDIChildFrame* wnd;
    wnd = static_cast<wxAuiMDIChildFrame*>(GetPage(evt.GetSelection()));

    wnd->Close();

    // regardless of the result of wnd->Close(), we've
    // already taken care of the close operations, so
    // suppress further processing
    evt.Veto();
}

void wxAuiMDIClientWindow::OnPageChanged(wxAuiNotebookEvent& evt)
{
    PageChanged(evt.GetOldSelection(), evt.GetSelection());
}

void wxAuiMDIClientWindow::OnSize(wxSizeEvent& evt)
{
    wxAuiNotebook::OnSize(evt);

    for (size_t pos = 0; pos < GetPageCount(); pos++)
        ((wxAuiMDIChildFrame *)GetPage(pos))->ApplyMDIChildFrameRect();
}

#endif //wxUSE_AUI
#endif // wxUSE_MDI
