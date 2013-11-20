/////////////////////////////////////////////////////////////////////////////
// Name:        wx/aui/tabmdi.h
// Purpose:     interface of wxAuiMDIParentFrame, wxAuiMDIChildFrame, wxAuiMDIClientWindow
// Author:      wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/**
    @struct wxMDIAuiTraits

    Define the kind of windows the wxAui MDI base classes use.
    
    @library{wxaui}
    @category{managedwnd}

    @see wxMDIAnyChildWindow
*/
struct wxMDIAuiTraits
{
    typedef wxFrame                 ParentWindow;
    typedef wxFrame                 ChildWindow;
    typedef wxAuiNotebook           ClientWindow;

    typedef wxAuiMDIParentFrame     MDIParent;
    typedef wxAuiMDIChildFrame      MDIChild;
    typedef wxAuiMDIClientWindow    MDIClient;
};

/**
    @class wxAuiMDIParentFrame

    wxAuiMDIParentFrame inherits from wxAuiMDIParentFrameBase which is a typedef of
    wxMDIAnyParentWindow<wxMDIAuiTraits>.

    A wxAUI MDI (Multiple Document Interface) parent frame is a window which can
    contain wxAUI MDI child frames in its client area which is a wxAuiNotebook.
    
    It is the wxAUI equivalent to wxMDIParentFrame and uses the wxMDIAnyParentWindow
    Traits class.
    
    @library{wxaui}
    @category{managedwnd}

    @see wxAuiMDIChildFrame, wxAuiMDIClientWindow, wxMDIAnyParentWindow
*/

class WXDLLIMPEXP_AUI wxAuiMDIParentFrame : public wxAuiMDIParentFrameBase
{
public:
    /**
        Default constructor.

        Use Create() for the objects created using this constructor.
    */
    wxAuiMDIParentFrame();
    /**
        Constructor, creating the window.

        @param parent
            The window parent. Usually is @NULL.
        @param winid
            The window identifier. It may take a value of @c wxID_ANY to
            indicate a default value.
        @param title
            The caption to be displayed on the frame's title bar.
        @param pos
            The window position. The value ::wxDefaultPosition indicates a
            default position, chosen by either the windowing system or
            wxWidgets, depending on platform.
        @param size
            The window size. The value ::wxDefaultSize indicates a default
            size, chosen by either the windowing system or wxWidgets, depending
            on platform.
        @param style
            The window style. Default value includes wxDEFAULT_FRAME_STYLE,
            wxHSCROLL and wxVSCROLL styles.
        @param name
            The name of the window. This parameter is used to associate a name
            with the item, allowing the application user to set Motif resource
            values for individual windows.

        @see Create()
    */
    wxAuiMDIParentFrame(wxWindow *parent,
                        wxWindowID winid,
                        const wxString& title,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = wxDEFAULT_FRAME_STYLE | wxVSCROLL | wxHSCROLL,
                        const wxString& name = wxFrameNameStr);

    /**
        Destructor.

        Destroys all child windows and menu bar if present.
    */
    ~wxAuiMDIParentFrame();

    /**
        Used in two-step frame construction.

        See wxAuiMDIParentFrame() for further details.
    */
    bool Create(wxWindow *parent,
                wxWindowID winid,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE | wxVSCROLL | wxHSCROLL,
                const wxString& name = wxFrameNameStr );

    /**
        Sets the tab art provider which defines all the drawing functions
        used by the wxAuiMDIClientWindow instance (overload of wxAuiNotebook).
    */
    void SetArtProvider(wxAuiTabArt* provider);
    wxAuiTabArt* GetArtProvider();
    /**
        Gets the wxAuiMDIClientWindow casted in wxAuiNotebook.
    */
    wxAuiNotebook* GetNotebook() const;

    /**
        Replace the current MDI Window menu (the previously defined Window
        menu is destroyed).
        
        See wxMDIParentFrame::SetWindowMenu() for more details

        @param menu
            The menu to be used instead of the standard MDI Window menu or @NULL.
    */
    void SetWindowMenu(wxMenu* pMenu);

    /**
        Relatively equivalent to SetWindowMenu() but do not destroy the previously
        defined MDI Window menu.
    */
    virtual void SetMenuBar(wxMenuBar *pMenuBar);

    /**
        Replace the current MDI Window menu by the menu of the given wxAuiMDIChildFrame
        but do not destroy the previouly defined MDI Window menu (uses SetMenuBar()).
        If a NULL parameter is given, the previous MDI Window menu is restored.
    */
    void SetChildMenuBar(wxAuiMDIChildFrame *pChild);

    /**
        Called by the method Create() to construct the wxAuiMDIClientWindow
        (overloads wxAuiNotebook by default).
        It can be overloaded to modify the default behavior.
        @see GetNotebook()
    */
    virtual wxAuiMDIClientWindow *OnCreateClient();

    /**
        Has no effect if not overloaded.
    */
    virtual void Cascade();
    /**
        Tiles the MDI child windows either horizontally or vertically depending
        on whether @a orient is @c wxHORIZONTAL or @c wxVERTICAL.
    */
    virtual void Tile(wxOrientation orient = wxHORIZONTAL);
    /**
        Has no effect if not overloaded.
    */
    virtual void ArrangeIcons();
    /**
        Activates the wxAuiNotebook tab wich contains the MDI child following
        the currently active tab.

        @see ActivatePrevious()
    */
    virtual void ActivateNext();
    /**
        Activates the wxAuiNotebook tab wich contains the MDI child preceding
        the currently active tab.

        @see ActivateNext()
    */
    virtual void ActivatePrevious();

protected:
    wxEvent    *m_pLastEvt;
    /// Defined if wxUSE_MENUS is set to 1.
    wxMenuBar  *m_pMyMenuBar;

    /// Defined if wxUSE_MENUS is set to 1.
    void RemoveWindowMenu(wxMenuBar *pMenuBar);
    /// Defined if wxUSE_MENUS is set to 1.
    void AddWindowMenu(wxMenuBar *pMenuBar);
    /// Defined if wxUSE_MENUS is set to 1.
    void OnHandleMenu(wxCommandEvent &event);
    /// Defined if wxUSE_MENUS is set to 1.
    virtual void DoHandleMenu(wxCommandEvent &event);
    /// Defined if wxUSE_MENUS is set to 1.
    void DoHandleUpdateUI(wxUpdateUIEvent &event);

    virtual bool ProcessEvent(wxEvent& event);
    virtual void DoGetClientSize(int *width, int *height) const;
};

/**
    @class wxAuiMDIChildFrame

    wxAuiMDIChildFrame inherits from wxAuiMDIChildFrameBase which is a typedef of
    wxMDIAnyChildWindow<wxMDIAuiTraits>.

    A wxAUI MDI child frame is a frame that can only exist inside a
    wxAuiMDIClientWindow, which is itself a child of wxAuiMDIParentFrame.
    
    It is the wxAUI equivalent to wxMDIChildFrame and uses the wxMDIAnyChildWindow
    Traits class.

    @library{wxaui}
    @category{managedwnd}

    @see wxAuiMDIParentFrame, wxAuiMDIClientWindow, wxMDIAnyChildWindow
*/

class WXDLLIMPEXP_AUI wxAuiMDIChildFrame : public wxAuiMDIChildFrameBase
{
public:
    /**
        Default constructor.
    */
    wxAuiMDIChildFrame();
    /**
        Constructor, creating the window.

        @param parent
            The wxAuiMDIParentFrame parent. This should not be @NULL.
        @param winid
            The window identifier. It may take a value of -1 to indicate a default
            value.
        @param title
            The caption to be displayed on the tab's title bar.
        @param pos
            The window position. The value ::wxDefaultPosition indicates a default position,
            chosen by either the windowing system or wxWidgets, depending on platform.
        @param size
            The window size. The value ::wxDefaultSize indicates a default size, chosen by
            either the windowing system or wxWidgets, depending on platform.
        @param style
            The window style. See wxMDIChildFrame.
        @param name
            The name of the window. This parameter is used to associate a name with the
            item, allowing the application user to set Motif resource values for individual
            windows.

        @see Create()
    */
    wxAuiMDIChildFrame(wxAuiMDIParentFrame *parent,
                       wxWindowID winid,
                       const wxString& title,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = wxDefaultSize,
                       long style = wxDEFAULT_FRAME_STYLE,
                       const wxString& name = wxFrameNameStr);

    /**
        Destructor. Destroys all child windows and menu bar if present.
    */
    virtual ~wxAuiMDIChildFrame();
    /**
        Used in two-step frame construction.
        See wxAuiMDIChildFrame() for further details.
    */
    bool Create(wxAuiMDIParentFrame *parent,
                wxWindowID winid,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE,
                const wxString& name = wxFrameNameStr);

    virtual void SetMenuBar(wxMenuBar *menuBar);
    virtual wxMenuBar *GetMenuBar() const;

    virtual void SetTitle(const wxString& title);
    virtual wxString GetTitle() const;

    virtual void SetIcons(const wxIconBundle& icons);
    virtual const wxIconBundle& GetIcons() const;

    virtual void SetIcon(const wxIcon& icon);
    virtual const wxIcon& GetIcon() const;

    virtual void Activate();
    virtual bool Destroy();

    virtual bool Show(bool show = true);

    /**
        Returns NULL if not overloaded.
    */
    virtual wxStatusBar* CreateStatusBar(int WXUNUSED(number) = 1,
                                         long WXUNUSED(style) = 1,
                                         wxWindowID WXUNUSED(winid) = 1,
                                         const wxString& WXUNUSED(name) = wxEmptyString);

    /**
        Returns NULL if not overloaded.
    */
    virtual wxStatusBar *GetStatusBar() const;
    /**
        Has no effect if not overloaded.
    */
    virtual void SetStatusText( const wxString &WXUNUSED(text), int WXUNUSED(number)=0 );
    /**
        Has no effect if not overloaded.
    */
    virtual void SetStatusWidths( int WXUNUSED(n), const int WXUNUSED(widths_field)[] );

    /**
        Returns NULL if not overloaded.
    */
    virtual wxToolBar* CreateToolBar(long WXUNUSED(style),
                                     wxWindowID WXUNUSED(winid),
                                     const wxString& WXUNUSED(name))
    /**
        Returns NULL if not overloaded.
    */
    virtual wxToolBar *GetToolBar() const;


    /**
        Has no effect if not overloaded.
    */
    virtual void Maximize(bool WXUNUSED(maximize) = true);
    /**
        Has no effect if not overloaded.
    */
    virtual void Restore();
    /**
        Has no effect if not overloaded.
    */
    virtual void Iconize(bool WXUNUSED(iconize)  = true);
    /**
        Has no effect if not overloaded.
        @return always @c true.
    */
    virtual bool IsMaximized() const;
    /**
        Has no effect if not overloaded.
        @return always @c false.
    */
    virtual bool IsIconized() const;
    /**
        Has no effect if not overloaded.
        @return always @c false.
    */
    virtual bool ShowFullScreen(bool WXUNUSED(show), long WXUNUSED(style));
    /**
        Has no effect if not overloaded.
        @return always @c false.
    */
    virtual bool IsFullScreen() const;
    /**
        Has no effect if not overloaded.
        @return always @c false.
    */
    virtual bool IsTopLevel() const;

protected:
    void OnMenuHighlight(wxMenuEvent& evt);
    void OnActivate(wxActivateEvent& evt);

    /**
        Has no effect if not overloaded.
    */
    virtual void DoActivate(wxActivateEvent& WXUNUSED(evt));
    virtual void DoSetSize(int x, int y, int width, int height, int sizeFlags);
    virtual void DoMoveWindow(int x, int y, int width, int height);

    /**
        Has no effect if not overloaded.
    */
    virtual void DoSetSizeHints(int WXUNUSED(minW), int WXUNUSED(minH),
                                int WXUNUSED(maxW), int WXUNUSED(maxH),
                                int WXUNUSED(incW), int WXUNUSED(incH));
private:
    /**
        Set to false by the Destroy method to avoid DoActivate to be called when
        children class is destroyed (and the pointer to the virtual method is NULL)
    */
    bool m_doActivate;

public:
    /**
        This function needs to be called when a size change is confirmed.
        It prevents anybody from outside to change the panel.
    */
    void ApplyMDIChildFrameRect();
    void DoShow(bool show);

protected:
    wxRect m_mdiNewRect;
    wxRect m_mdiCurRect;
    wxString m_title;
    wxIcon m_icon;
    wxIconBundle m_iconBundle;
    bool m_activateOnCreate;

    /// Defined if wxUSE_MENUS is set to 1.
    wxMenuBar* m_pMenuBar;

private:
    friend class wxAuiMDIClientWindow;
};

/**
    @class wxAuiMDIClientWindow

    wxAuiMDIClientWindow inherits from wxAuiMDIClientWindowBase which is a typedef of
    wxMDIAnyClientWindow<wxMDIAuiTraits>.

    A wxAUI MDI client window is a child of wxAuiMDIParentFrame, and manages zero or
    more wxAuiMDIChildFrame objects. It is the MDI overload of a wxAuiNotebook.
    
    It is the wxAUI equivalent to wxMDIClientWindow and uses the wxMDIAnyChildWindow
    Traits class.
    
    @library{wxaui}
    @category{managedwnd}

    @see wxAuiMDIParentFrame, wxAuiMDIChildFrame, wxMDIAnyClientWindow
*/

class WXDLLIMPEXP_AUI wxAuiMDIClientWindow : public wxAuiMDIClientWindowBase
{
public:
    /**
        Default constructor.

        Objects of this class are only created by wxAuiMDIParentFrame which uses
        the default constructor and calls CreateClient() immediately
        afterwards.
     */
    wxAuiMDIClientWindow();
    /**
        Constructor, creating the window.

        @param parent
            The wxAuiMDIParentFrame parent.
        @param style
            The window style. Only wxHSCROLL and wxVSCROLL bits are meaningful
            here.

    */
    wxAuiMDIClientWindow(wxAuiMDIParentFrame *parent, long style = 0);

    /**
        Called by wxAuiMDIParentFrame immediately after creating the client
        window.

        This function may be overridden in the derived class but the base class
        version must usually be called first to really create the window.

        See wxAuiMDIClientWindow() for further details.
    */
    virtual bool CreateClient(wxAuiMDIParentFrame *parent,
                              long style = wxVSCROLL | wxHSCROLL);

    /**
        Select the expected wxAuiNotebook tab and activates the 
        corresponding wxAuiMDIChildFrame.
    */
    virtual int SetSelection(size_t page);
    /**
        Get the current active child.
    */
    virtual wxAuiMDIChildFrame* GetActiveChild();
    /**
        Change the active child by the given one and select the
        corresponding wxAuiNotebook tab.
    */
    virtual void SetActiveChild(wxAuiMDIChildFrame* pChildFrame);

protected:

    void PageChanged(int oldSelection, int newSelection);
    void OnPageClose(wxAuiNotebookEvent& evt);
    void OnPageChanged(wxAuiNotebookEvent& evt);
    void OnSize(wxSizeEvent& evt);
};
