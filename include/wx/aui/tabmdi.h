/////////////////////////////////////////////////////////////////////////////
// Name:        wx/aui/tabmdi.h
// Purpose:     Generic MDI (Multiple Document Interface) classes
// Author:      Hans Van Leemputten
// Modified by: Benjamin I. Williams / Kirix Corporation
// Created:     29/07/2002
// RCS-ID:      $Id$
// Copyright:   (c) Hans Van Leemputten
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_AUITABMDI_H_
#define _WX_AUITABMDI_H_

#if wxUSE_AUI

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/mdi.h"
#include "wx/panel.h"
#include "wx/notebook.h"
#include "wx/icon.h"
#include "wx/aui/auibook.h"

class WXDLLIMPEXP_FWD_AUI wxAuiMDIParentFrame;
class WXDLLIMPEXP_FWD_AUI wxAuiMDIClientWindow;
class WXDLLIMPEXP_FWD_AUI wxAuiMDIChildFrame;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

enum MDI_MENU_ID
{
    wxWINDOWCLOSE = 4001,
    wxWINDOWCLOSEALL,
    wxWINDOWNEXT,
    wxWINDOWPREV
};

// ----------------------------------------------------------------------------
// wxMDIAUIClasses: define the kind of windows the MDI base classes use
// ----------------------------------------------------------------------------

struct wxMDIAUIClasses
{
    // The classes to inherit our MDI classes from.
    typedef wxFrame ParentBaseClass;
    typedef wxFrame ChildBaseClass;
    typedef wxAuiNotebook ClientBaseClass;

    // And the AUI MDI classes themselves.
    typedef wxAuiMDIParentFrame ParentClass;
    typedef wxAuiMDIChildFrame ChildClass;
    typedef wxAuiMDIClientWindow ClientClass;
};

//-----------------------------------------------------------------------------
// wxAuiMDIParentFrame
//-----------------------------------------------------------------------------

typedef wxMDIAnyParentWindow<wxMDIAUIClasses> wxAuiMDIParentFrameBase;

class WXDLLIMPEXP_AUI wxAuiMDIParentFrame : public wxAuiMDIParentFrameBase
{
public:
    wxAuiMDIParentFrame();
    wxAuiMDIParentFrame(wxWindow *parent,
                        wxWindowID winid,
                        const wxString& title,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = wxDEFAULT_FRAME_STYLE | wxVSCROLL | wxHSCROLL,
                        const wxString& name = wxFrameNameStr);

    ~wxAuiMDIParentFrame();

    bool Create(wxWindow *parent,
                wxWindowID winid,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE | wxVSCROLL | wxHSCROLL,
                const wxString& name = wxFrameNameStr );

    void SetArtProvider(wxAuiTabArt* provider);
    wxAuiTabArt* GetArtProvider();
    wxAuiNotebook* GetNotebook() const;

#if wxUSE_MENUS
    void SetWindowMenu(wxMenu* pMenu);

    virtual void SetMenuBar(wxMenuBar *pMenuBar);
#endif // wxUSE_MENUS

    void SetChildMenuBar(wxAuiMDIChildFrame *pChild);

    virtual wxAuiMDIClientWindow *OnCreateClient();

    virtual void Cascade() { /* Has no effect */ }
    virtual void Tile(wxOrientation orient = wxHORIZONTAL);
    virtual void ArrangeIcons() { /* Has no effect */ }
    virtual void ActivateNext();
    virtual void ActivatePrevious();

protected:
    wxEvent    *m_pLastEvt;

#if wxUSE_MENUS
    wxMenuBar  *m_pMyMenuBar;
#endif // wxUSE_MENUS

protected:

#if wxUSE_MENUS
    void RemoveWindowMenu(wxMenuBar *pMenuBar);
    void AddWindowMenu(wxMenuBar *pMenuBar);

    void OnHandleMenu(wxCommandEvent &event) { DoHandleMenu(event); }
    virtual void DoHandleMenu(wxCommandEvent &event);
    void DoHandleUpdateUI(wxUpdateUIEvent &event);
#endif // wxUSE_MENUS

    virtual bool ProcessEvent(wxEvent& event);

    virtual void DoGetClientSize(int *width, int *height) const;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxAuiMDIParentFrame)
};

//-----------------------------------------------------------------------------
// wxAuiMDIChildFrame
//-----------------------------------------------------------------------------

typedef wxMDIAnyChildWindow<wxMDIAUIClasses>  wxAuiMDIChildFrameBase;

class WXDLLIMPEXP_AUI wxAuiMDIChildFrame : public wxAuiMDIChildFrameBase
{
public:
    wxAuiMDIChildFrame();
    wxAuiMDIChildFrame(wxAuiMDIParentFrame *parent,
                       wxWindowID winid,
                       const wxString& title,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = wxDefaultSize,
                       long style = wxDEFAULT_FRAME_STYLE,
                       const wxString& name = wxFrameNameStr);

    virtual ~wxAuiMDIChildFrame();
    bool Create(wxAuiMDIParentFrame *parent,
                wxWindowID winid,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE,
                const wxString& name = wxFrameNameStr);

#if wxUSE_MENUS
    virtual void SetMenuBar(wxMenuBar *menuBar);
    virtual wxMenuBar *GetMenuBar() const;
#endif // wxUSE_MENUS

    virtual void SetTitle(const wxString& title);
    virtual wxString GetTitle() const;

    virtual void SetIcons(const wxIconBundle& icons);
    virtual const wxIconBundle& GetIcons() const;

    virtual void SetIcon(const wxIcon& icon);
    virtual const wxIcon& GetIcon() const;

    virtual void Activate();
    virtual bool Destroy();

    virtual bool Show(bool show = true);

#if wxUSE_STATUSBAR
    // no status bars
    virtual wxStatusBar* CreateStatusBar(int WXUNUSED(number) = 1,
                                         long WXUNUSED(style) = 1,
                                         wxWindowID WXUNUSED(winid) = 1,
                                         const wxString& WXUNUSED(name) = wxEmptyString)
      { return NULL; }

    virtual wxStatusBar *GetStatusBar() const { return NULL; }
    virtual void SetStatusText( const wxString &WXUNUSED(text), int WXUNUSED(number)=0 ) {}
    virtual void SetStatusWidths( int WXUNUSED(n), const int WXUNUSED(widths_field)[] ) {}
#endif

#if wxUSE_TOOLBAR
    // no toolbar bars
    virtual wxToolBar* CreateToolBar(long WXUNUSED(style),
                                     wxWindowID WXUNUSED(winid),
                                     const wxString& WXUNUSED(name))
        { return NULL; }
    virtual wxToolBar *GetToolBar() const { return NULL; }
#endif


    // no maximize etc
    virtual void Maximize(bool WXUNUSED(maximize) = true) { /* Has no effect */ }
    virtual void Restore() { /* Has no effect */ }
    virtual void Iconize(bool WXUNUSED(iconize)  = true) { /* Has no effect */ }
    virtual bool IsMaximized() const { return true; }
    virtual bool IsIconized() const { return false; }
    virtual bool ShowFullScreen(bool WXUNUSED(show), long WXUNUSED(style)) { return false; }
    virtual bool IsFullScreen() const { return false; }

    virtual bool IsTopLevel() const { return false; }

protected:
    void OnMenuHighlight(wxMenuEvent& evt);
    void OnActivate(wxActivateEvent& evt);

    virtual void DoActivate(wxActivateEvent& WXUNUSED(evt)) {}
    virtual void DoSetSize(int x, int y, int width, int height, int sizeFlags);
    virtual void DoMoveWindow(int x, int y, int width, int height);

    // no size hints
    virtual void DoSetSizeHints(int WXUNUSED(minW), int WXUNUSED(minH),
                                int WXUNUSED(maxW), int WXUNUSED(maxH),
                                int WXUNUSED(incW), int WXUNUSED(incH)) {}
private:
    // set to false by the Destroy method to avoid DoActivate to be called when
    // children class is destroyed (and the pointer to the virtual method is NULL)
    bool m_doActivate;

public:
    // This function needs to be called when a size change is confirmed,
    // we needed this function to prevent anybody from the outside
    // changing the panel... it messes the UI layout when we would allow it.
    void ApplyMDIChildFrameRect();
    void DoShow(bool show);

protected:
    wxRect m_mdiNewRect;
    wxRect m_mdiCurRect;
    wxString m_title;
    wxIcon m_icon;
    wxIconBundle m_iconBundle;
    bool m_activateOnCreate;

#if wxUSE_MENUS
    wxMenuBar* m_pMenuBar;
#endif // wxUSE_MENUS



private:
    DECLARE_DYNAMIC_CLASS(wxAuiMDIChildFrame)
    DECLARE_EVENT_TABLE()

    friend class wxAuiMDIClientWindow;
};

//-----------------------------------------------------------------------------
// wxAuiMDIClientWindow
//-----------------------------------------------------------------------------

typedef wxMDIAnyClientWindow<wxMDIAUIClasses> wxAuiMDIClientWindowBase;

class WXDLLIMPEXP_AUI wxAuiMDIClientWindow : public wxAuiMDIClientWindowBase
{
public:
    wxAuiMDIClientWindow() {}
    wxAuiMDIClientWindow(wxAuiMDIParentFrame *parent, long style = 0);

    virtual bool CreateClient(wxAuiMDIParentFrame *parent,
                              long style = wxVSCROLL | wxHSCROLL);

    virtual int SetSelection(size_t page);
    virtual wxAuiMDIChildFrame* GetActiveChild();
    virtual void SetActiveChild(wxAuiMDIChildFrame* pChildFrame)
    {
        SetSelection(GetPageIndex(pChildFrame));
    }

protected:

    void PageChanged(int oldSelection, int newSelection);
    void OnPageClose(wxAuiNotebookEvent& evt);
    void OnPageChanged(wxAuiNotebookEvent& evt);
    void OnSize(wxSizeEvent& evt);

private:
    DECLARE_DYNAMIC_CLASS(wxAuiMDIClientWindow)
    DECLARE_EVENT_TABLE()
};

#endif // wxUSE_AUI

#endif // _WX_AUITABMDI_H_
