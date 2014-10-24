/////////////////////////////////////////////////////////////////////////////
// Name:        aui/tabdocmdi.h
// Purpose:     interface of wxAuiDocMDIParentFrame, wxAuiDocMDIChildFrame
// Author:      wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/**
    @class wxAuiDocMDIParentFrame

    wxAuiDocMDIParentFrame inherits from wxAuiDocMDIParentFrameBase which is a typedef of
    wxDocParentFrameAny<wxAuiMDIParentFrame>.

    The wxAuiDocMDIParentFrame class is part of the wxAUI class framework.
    It provides a default top-level frame for applications using the document/view
    framework. This class can only be used for wxAUI MDI parent frames.

    It cooperates with the the wxView, wxDocument, wxDocManager and wxDocTemplate
    classes.

    @library{wxaui}
    @category{docview}

    @see @ref wxauioverview @ref overview_docview, @ref page_samples_auidocview, wxAuiMDIParentFrame
*/

class WXDLLIMPEXP_AUI wxAuiDocMDIParentFrame : public wxAuiDocMDIParentFrameBase
{
public:
    //@{
    /**
        Constructor.
    */
    wxAuiDocMDIParentFrame();
    wxAuiDocMDIParentFrame(wxDocManager *manager,
                           wxFrame *parent,
                           wxWindowID id,
                           const wxString& title,
                           const wxPoint& pos = wxDefaultPosition,
                           const wxSize& size = wxDefaultSize,
                           long style = wxDEFAULT_FRAME_STYLE,
                           const wxString& name = wxFrameNameStr);
    //@}

    /**
        Creates the window.
    */
    bool Create(wxDocManager *manager,
                wxFrame *frame,
                wxWindowID id,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE,
                const wxString& name = wxFrameNameStr);

    /**
        Extend event processing to search the document manager's event table.
    */
    virtual bool TryBefore(wxEvent& event);

    wxDocManager *GetDocumentManager(void) const;

protected:
    void OnCloseWindow(wxCloseEvent& event);
    /// Defined if wxUSE_MENUS is set to 1.
    virtual void DoHandleMenu(wxCommandEvent &event);
};


/**
    @class wxAuiDocMDIChildFrame

    wxAuiDocMDIChildFrame inherits from wxAuiDocMDIChildFrameBase which is a typedef of
    wxDocChildFrameAny<wxAuiMDIChildFrame, wxAuiMDIParentFrame>.

    The wxAuiDocMDIChildFrame class is part of the wxAUI class framework.
    It provides a default frame for displaying documents on separate windows.
    This class can only be used for wxAUI MDI child frames.

    The class is part of the document/view framework supported by wxWidgets,
    and cooperates with the the wxView, wxDocument, wxDocManager and wxDocTemplate
    classes.

    @library{wxaui}
    @category{docview}

    @see @ref wxauioverview @ref overview_docview, @ref page_samples_auidocview, wxAuiMDIChildFrame
*/

class WXDLLIMPEXP_AUI wxAuiDocMDIChildFrame : public wxAuiDocMDIChildFrameBase
{
public:
    //@{
    /**
        Constructor.
    */
    wxAuiDocMDIChildFrame();
    wxAuiDocMDIChildFrame(wxDocument *doc,
                          wxView *view,
                          wxAuiMDIParentFrame *parent,
                          wxWindowID id,
                          const wxString& title,
                          const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize,
                          long style = wxDEFAULT_FRAME_STYLE,
                          const wxString& name = wxFrameNameStr);
    //@}

    /**
        Creates the window.
    */
    bool Create(wxDocument *doc,
                wxView *view,
                wxAuiMDIParentFrame *parent,
                wxWindowID id,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE,
                const wxString& name = wxFrameNameStr);

protected:
    virtual void DoActivate(wxActivateEvent& evt) { evt.Skip(false); }
};
