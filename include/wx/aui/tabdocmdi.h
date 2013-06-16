/////////////////////////////////////////////////////////////////////////////
// Name:        tabdocmdi.h
// Purpose:     Frame classes for Aui MDI document/view applications
// Author:      Julian Smart
// Modified by: Kinaou Hervé
// Created:     01/02/97
// RCS-ID:      $Id:$
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_AUITABDOCMDI_H_
#define _WX_AUITABDOCMDI_H_

#if wxUSE_AUI

#include "wx/docmdi.h"
#include "wx/aui/tabmdi.h"

//-----------------------------------------------------------------------------
// classes
//-----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// An MDI document parent frame for AUI framework
// ----------------------------------------------------------------------------

typedef wxDocParentFrameAny<wxAuiMDIParentFrame> wxAuiDocMDIParentFrameBase;
template class WXDLLIMPEXP_AUI wxDocParentFrameAny<wxAuiMDIParentFrame>;

class WXDLLIMPEXP_AUI wxAuiDocMDIParentFrame : public wxAuiDocMDIParentFrameBase
{
public:
    wxAuiDocMDIParentFrame() : wxAuiDocMDIParentFrameBase() { }

    wxAuiDocMDIParentFrame(wxDocManager *manager,
                        wxFrame *parent,
                        wxWindowID id,
                        const wxString& title,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = wxDEFAULT_FRAME_STYLE,
                        const wxString& name = wxFrameNameStr);

    bool Create(wxDocManager *manager,
                wxFrame *frame,
                wxWindowID id,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE,
                const wxString& name = wxFrameNameStr);

    // Extend event processing to search the document manager's event table
    virtual bool TryBefore(wxEvent& event);

    wxDocManager *GetDocumentManager(void) const { return m_docManager; }

protected:
    void OnNotebookPageChanged(wxAuiNotebookEvent &event);
    void OnCloseWindow(wxCloseEvent& event);

#if wxUSE_MENUS
    virtual void DoHandleMenu(wxCommandEvent &event);
#endif // wxUSE_MENUS

private:
    DECLARE_CLASS(wxAuiDocMDIParentFrame)
    wxDECLARE_NO_COPY_CLASS(wxAuiDocMDIParentFrame);
    DECLARE_EVENT_TABLE()
};


// ----------------------------------------------------------------------------
// An MDI document child frame for AUI framework
// ----------------------------------------------------------------------------

typedef wxDocChildFrameAny<wxAuiMDIChildFrame, wxAuiMDIParentFrame> wxAuiDocMDIChildFrameBase;
template class WXDLLIMPEXP_AUI wxDocChildFrameAny<wxAuiMDIChildFrame, wxAuiMDIParentFrame>;

class WXDLLIMPEXP_AUI wxAuiDocMDIChildFrame : public wxAuiDocMDIChildFrameBase
{
public:
    wxAuiDocMDIChildFrame() { }

    wxAuiDocMDIChildFrame(wxDocument *doc,
                          wxView *view,
                          wxAuiMDIParentFrame *parent,
                          wxWindowID id,
                          const wxString& title,
                          const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize,
                          long style = wxDEFAULT_FRAME_STYLE,
                          const wxString& name = wxFrameNameStr);

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

private:
    DECLARE_CLASS(wxAuiDocMDIChildFrame)
    wxDECLARE_NO_COPY_CLASS(wxAuiDocMDIChildFrame);
};


#endif
    // wxUSE_AUI

#endif
    // _WX_AUITABDOCMDI_H_
