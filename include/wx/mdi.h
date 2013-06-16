/////////////////////////////////////////////////////////////////////////////
// Name:        wx/mdi.h
// Purpose:     wxMDI base header
// Author:      Julian Smart (original)
//              Vadim Zeitlin (base MDI classes refactoring)
// Copyright:   (c) 1998 Julian Smart
//              (c) 2008 Vadim Zeitlin
// RCS-ID:      $Id$
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MDI_H_BASE_
#define _WX_MDI_H_BASE_

#include "wx/defs.h"

#if wxUSE_MDI

#include "wx/frame.h"
#include "wx/menu.h"

// forward declarations
class WXDLLIMPEXP_FWD_CORE wxMDIParentFrame;
class WXDLLIMPEXP_FWD_CORE wxMDIChildFrame;
class WXDLLIMPEXP_FWD_CORE wxMDIClientWindow;

/*
    The MDI classes defined in this header can actually work with either the
    standard wxMDI{Parent,Child}Frame classes or with other classes providing
    the necessary interface, notably wxAUI-based equivalents. So the code here
    is parametrized by a traits-like template parameter defining the classes to
    use. This allows us to avoid duplicating it for the default and AUI cases.
 */

// ----------------------------------------------------------------------------
// wxMDIStandardClasses: define the standard MDI classes to use
// ----------------------------------------------------------------------------

struct wxMDIStandardClasses
{
    // The base classes for MDI parent, child and client window classes
    // respectively.
    typedef wxFrame ParentBaseClass;
    typedef wxFrame ChildBaseClass;
    typedef wxWindow ClientBaseClass;


    // The derived, i.e. really used, classes of these windows.
    typedef wxMDIParentFrame ParentClass;
    typedef wxMDIChildFrame ChildClass;
    typedef wxMDIClientWindow ClientClass;
};

// ----------------------------------------------------------------------------
// wxMDIAnyParentWindow: base class for parent frame for MDI children
// ----------------------------------------------------------------------------

template <class MDIClasses>
class WXDLLIMPEXP_CORE wxMDIAnyParentWindow : public MDIClasses::ParentBaseClass
{
    typedef typename MDIClasses::ChildClass  MDIChild;
    typedef typename MDIClasses::ClientClass MDIClient;

public:
    wxMDIAnyParentWindow()
    {
        m_clientWindow = NULL;
#if wxUSE_MENUS
        m_windowMenu = NULL;
#endif // wxUSE_MENUS
    }

    /*
        Derived classes should provide ctor and Create() with the following
        declaration:

    bool Create(wxWindow *parent,
                wxWindowID winid,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE | wxVSCROLL | wxHSCROLL,
                const wxString& name = wxFrameNameStr);
     */

#if wxUSE_MENUS
    virtual ~wxMDIAnyParentWindow()
    {
        delete m_windowMenu;
    }
#endif // wxUSE_MENUS

    // accessors
    // ---------

    // Get the client window
    MDIClient *GetClientWindow() const { return m_clientWindow; }

    // Get or change the active MDI child window
    virtual MDIChild *GetActiveChild() const
    {
        MDIClient* const client = GetClientWindow();
        return client ? client->GetActiveChild() : NULL;
    }

    virtual void SetActiveChild(MDIChild *child)
    {
        MDIClient* const client = GetClientWindow();
        wxCHECK_RET( client, wxS("Can't activate without client window") );

        if ( client->GetActiveChild() != child )
            client->SetActiveChild(child);
    }


    // MDI windows menu functions
    // --------------------------

#if wxUSE_MENUS
    // return the pointer to the current window menu or NULL if we don't have
    // because of wxFRAME_NO_WINDOW_MENU style
    wxMenu* GetWindowMenu() const { return m_windowMenu; }

    // use the given menu instead of the default window menu
    //
    // menu can be NULL to disable the window menu completely
    virtual void SetWindowMenu(wxMenu *menu)
    {
        if ( menu != m_windowMenu )
        {
            delete m_windowMenu;
            m_windowMenu = menu;
        }
    }
#endif // wxUSE_MENUS


    // standard MDI window management functions
    // ----------------------------------------

    virtual void Cascade() { }
    virtual void Tile(wxOrientation WXUNUSED(orient) = wxHORIZONTAL) { }
    virtual void ArrangeIcons() { }
    virtual void ActivateNext() = 0;
    virtual void ActivatePrevious() = 0;

    /*
        Derived classes must provide the following function:

    static bool IsTDI();
    */

    // Create the client window class (don't Create() the window here, just
    // return a new object of a MDIClasses::ClientClass-derived class)
    //
    // Notice that if you override this method you should use the default
    // constructor and Create() and not the constructor creating the window
    // when creating the frame or your overridden version is not going to be
    // called (as the call to a virtual function from ctor will be dispatched
    // to this class version)
    virtual MDIClient *OnCreateClient()
        { return new MDIClient; }

protected:
    // This is MDIClasses::ClientClass for all the native implementations but
    // not for the generic MDI which has its own wxGenericMDIClientWindow and
    // so we store it as just a base class pointer because we don't need its
    // exact type anyhow
    MDIClient *m_clientWindow;

#if wxUSE_MENUS
    // the current window menu or NULL if we are not using it
    wxMenu *m_windowMenu;
#endif // wxUSE_MENUS
};

typedef wxMDIAnyParentWindow<wxMDIStandardClasses> wxMDIParentFrameBase;

// ----------------------------------------------------------------------------
// wxMDIAnyChildWindow: child frame managed by MDI parent frame
// ----------------------------------------------------------------------------

template <class MDIClasses>
class WXDLLIMPEXP_CORE wxMDIAnyChildWindow : public MDIClasses::ChildBaseClass
{
    typedef typename MDIClasses::ParentClass MDIParent;

public:
    wxMDIAnyChildWindow() { m_mdiParent = NULL; }

    /*
        Derived classes should provide Create() with the following signature:

    bool Create(MDIClasses::ParentClass *parent,
                wxWindowID id,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE,
                const wxString& name = wxFrameNameStr);

        And setting m_mdiParent to parent parameter.
     */

    // MDI children specific methods
    virtual void Activate() = 0;

    // Return the MDI parent frame: notice that it may not be the same as
    // GetParent() (our parent may be the client window or even its subwindow
    // in some implementations)
    MDIParent *GetMDIParent() const { return m_mdiParent; }

    // Synonym for GetMDIParent(), was used in some other ports
    MDIParent *GetMDIParentFrame() const { return GetMDIParent(); }


    // in most ports MDI children frames are not really top-level, the only
    // exception are the Mac ports in which MDI children are just normal top
    // level windows too
    virtual bool IsTopLevel() const { return false; }

    // In all ports keyboard navigation must stop at MDI child frame level and
    // can't cross its boundary. Indicate this by overriding this function to
    // return true.
    virtual bool IsTopNavigationDomain() const { return true; }

    // Raising any frame is supposed to show it but wxFrame Raise()
    // implementation doesn't work for MDI child frames in most forms so
    // forward this to Activate() which serves the same purpose by default.
    virtual void Raise() { Activate(); }

protected:
    MDIParent *m_mdiParent;
};

typedef wxMDIAnyChildWindow<wxMDIStandardClasses> wxMDIChildFrameBase;

// ----------------------------------------------------------------------------
// wxTDIChildFrame: child frame used by TDI implementations
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxTDIChildFrame : public wxMDIChildFrameBase
{
public:
    // override wxFrame methods for this non top-level window

#if wxUSE_STATUSBAR
    // no status bars
    //
    // TODO: MDI children should have their own status bars, why not?
    virtual wxStatusBar* CreateStatusBar(int WXUNUSED(number) = 1,
                                         long WXUNUSED(style) = 1,
                                         wxWindowID WXUNUSED(id) = 1,
                                         const wxString& WXUNUSED(name)
                                            = wxEmptyString)
      { return NULL; }

    virtual wxStatusBar *GetStatusBar() const
        { return NULL; }
    virtual void SetStatusText(const wxString &WXUNUSED(text),
                               int WXUNUSED(number)=0)
        { }
    virtual void SetStatusWidths(int WXUNUSED(n),
                                 const int WXUNUSED(widths)[])
        { }
#endif // wxUSE_STATUSBAR

#if wxUSE_TOOLBAR
    // no toolbar
    //
    // TODO: again, it should be possible to have tool bars
    virtual wxToolBar *CreateToolBar(long WXUNUSED(style),
                                     wxWindowID WXUNUSED(id),
                                     const wxString& WXUNUSED(name))
        { return NULL; }
    virtual wxToolBar *GetToolBar() const { return NULL; }
#endif // wxUSE_TOOLBAR

    // no icon
    virtual void SetIcons(const wxIconBundle& WXUNUSED(icons)) { }

    // title is used as the tab label
    virtual wxString GetTitle() const { return m_title; }
    virtual void SetTitle(const wxString& title) = 0;

    // no maximize etc
    virtual void Maximize(bool WXUNUSED(maximize) = true) { }
    virtual bool IsMaximized() const { return true; }
    virtual bool IsAlwaysMaximized() const { return true; }
    virtual void Iconize(bool WXUNUSED(iconize) = true) { }
    virtual bool IsIconized() const { return false; }
    virtual void Restore() { }

    virtual bool ShowFullScreen(bool WXUNUSED(show),
                                long WXUNUSED(style)) { return false; }
    virtual bool IsFullScreen() const { return false; }


    // we need to override these functions to ensure that a child window is
    // created even though we derive from wxFrame -- basically we make it
    // behave as just a wxWindow by short-circuiting wxTLW changes to the base
    // class behaviour

    virtual void AddChild(wxWindowBase *child) { wxWindow::AddChild(child); }

    virtual bool Destroy() { return wxWindow::Destroy(); }

    // extra platform-specific hacks
#ifdef __WXMSW__
    virtual WXDWORD MSWGetStyle(long flags, WXDWORD *exstyle = NULL) const
    {
        return wxWindow::MSWGetStyle(flags, exstyle);
    }

    virtual WXHWND MSWGetParent() const
    {
        return wxWindow::MSWGetParent();
    }

    WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
    {
        return wxWindow::MSWWindowProc(message, wParam, lParam);
    }
#endif // __WXMSW__

protected:
    virtual void DoGetSize(int *width, int *height) const
    {
        wxWindow::DoGetSize(width, height);
    }

    virtual void DoSetSize(int x, int y, int width, int height, int sizeFlags)
    {
        wxWindow::DoSetSize(x, y, width, height, sizeFlags);
    }

    virtual void DoGetClientSize(int *width, int *height) const
    {
        wxWindow::DoGetClientSize(width, height);
    }

    virtual void DoSetClientSize(int width, int height)
    {
        wxWindow::DoSetClientSize(width, height);
    }

    // no size hints
    virtual void DoSetSizeHints(int WXUNUSED(minW), int WXUNUSED(minH),
                                int WXUNUSED(maxW), int WXUNUSED(maxH),
                                int WXUNUSED(incW), int WXUNUSED(incH)) { }

    wxString m_title;
};

// ----------------------------------------------------------------------------
// wxMDIAnyClientWindow: child of parent frame, parent of children frames
// ----------------------------------------------------------------------------

template <class MDIClasses>
class WXDLLIMPEXP_CORE wxMDIAnyClientWindow : public MDIClasses::ClientBaseClass
{
    typedef typename MDIClasses::ParentClass MDIParent;
    typedef typename MDIClasses::ChildClass  MDIChild;

public:
    /*
        The derived class must provide the default ctor only (CreateClient()
        will be called later).
    */

    // Can be overridden in the derived classes but the base class version must
    // be usually called first to really create the client window.
    virtual bool CreateClient(MDIParent *parent,
                              long style = wxVSCROLL | wxHSCROLL) = 0;

    virtual MDIChild* GetActiveChild() = 0;
    virtual void SetActiveChild(MDIChild* pChildFrame) = 0;
};

typedef wxMDIAnyClientWindow<wxMDIStandardClasses> wxMDIClientWindowBase;

// ----------------------------------------------------------------------------
// Include the port-specific implementation of the base classes defined above
// ----------------------------------------------------------------------------

// wxUSE_GENERIC_MDI_AS_NATIVE may be predefined to force the generic MDI
// implementation use even on the platforms which usually don't use it
//
// notice that generic MDI can still be used without this, but you would need
// to explicitly use wxGenericMDIXXX classes in your code (and currently also
// add src/generic/mdig.cpp to your build as it's not compiled in if generic
// MDI is not used by default -- but this may change later...)
#ifndef wxUSE_GENERIC_MDI_AS_NATIVE
    // wxUniv always uses the generic MDI implementation and so do the ports
    // without native version (although wxCocoa seems to have one -- but it's
    // probably not functional?)
    #if defined(__WXCOCOA__) || \
        defined(__WXMOTIF__) || \
        defined(__WXPM__) || \
        defined(__WXUNIVERSAL__)
        #define wxUSE_GENERIC_MDI_AS_NATIVE   1
    #else
        #define wxUSE_GENERIC_MDI_AS_NATIVE   0
    #endif
#endif // wxUSE_GENERIC_MDI_AS_NATIVE

#if wxUSE_GENERIC_MDI_AS_NATIVE
    #include "wx/generic/mdig.h"
#elif defined(__WXMSW__)
    #include "wx/msw/mdi.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/mdi.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/mdi.h"
#elif defined(__WXMAC__)
    #include "wx/osx/mdi.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/mdi.h"
#endif

#endif // wxUSE_MDI

#endif // _WX_MDI_H_BASE_
