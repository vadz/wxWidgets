/////////////////////////////////////////////////////////////////////////////
// Name:        src/aui/tabdocmdi.cpp
// Purpose:     Frame classes for MDI document/view applications
// Author:      Kinaou Hervé
// Created:     2013-06-16
// RCS-ID:      $Id:$
// Copyright:   (c) wxWidgets development team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#if wxUSE_AUI
#if wxUSE_MDI

#include "wx/aui/tabdocmdi.h"

/*
 * Docview Aui MDI parent frame
 */

IMPLEMENT_CLASS(wxAuiDocMDIParentFrame, wxAuiMDIParentFrame)

BEGIN_EVENT_TABLE(wxAuiDocMDIParentFrame, wxAuiMDIParentFrame)
    EVT_CLOSE(wxAuiDocMDIParentFrame::OnCloseWindow)
END_EVENT_TABLE()

wxAuiDocMDIParentFrame::wxAuiDocMDIParentFrame(wxDocManager *manager,
                                               wxFrame *parent,
                                               wxWindowID id,
                                               const wxString& title,
                                               const wxPoint& pos,
                                               const wxSize& size,
                                               long style,
                                               const wxString& name)
{
    Create(manager, parent, id, title, pos, size, style, name);
}

bool wxAuiDocMDIParentFrame::Create(wxDocManager *manager,
                                    wxFrame *frame,
                                    wxWindowID id,
                                    const wxString& title,
                                    const wxPoint& pos,
                                    const wxSize& size,
                                    long style,
                                    const wxString& name)
{
    if (wxAuiDocMDIParentFrameBase::Create(manager, frame, id, title, pos, size, style, name))
    {
        GetClientWindow()->Connect(GetClientWindow()->GetId(),
            wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED,
            wxAuiNotebookEventHandler(wxAuiDocMDIParentFrame::OnNotebookPageChanged),
            NULL,
            this);

        return true;
    }

    return false;
}

// Extend event processing to search the view's event table
bool wxAuiDocMDIParentFrame::TryBefore(wxEvent& event)
{
#if wxUSE_MENUS
    // Catch the events of kind wxID_CLOSE[_ALL] to send the appropriate tabmdi event
    if (event.GetEventType() == wxEVT_COMMAND_MENU_SELECTED)
    {
        if (event.GetId() == wxID_CLOSE)
            event.SetId(wxWINDOWCLOSE);
        else if (event.GetId() == wxID_CLOSE_ALL)
            event.SetId(wxWINDOWCLOSEALL);
    }
#endif //wxUSE_MENUS

    return wxAuiDocMDIParentFrameBase::TryBefore(event);
}

void wxAuiDocMDIParentFrame::OnNotebookPageChanged(wxAuiNotebookEvent &event)
{
    wxWindow *selWnd = GetClientWindow()->GetPage(event.GetSelection());
    if (selWnd && selWnd->IsKindOf(CLASSINFO(wxAuiDocMDIChildFrame)))
    {
        wxAuiDocMDIChildFrame *selFrm = wxDynamicCast(selWnd, wxAuiDocMDIChildFrame);
        if (selFrm && selFrm->GetView())
            selFrm->GetView()->Activate(true);
    }
    event.Skip(true);
}

void wxAuiDocMDIParentFrame::OnCloseWindow(wxCloseEvent& event)
{
    wxCommandEvent eventCloseAll(wxEVT_COMMAND_MENU_SELECTED, wxWINDOWCLOSEALL);
    eventCloseAll.Skip();
    DoHandleMenu(eventCloseAll);

    if (eventCloseAll.GetSkipped())
    {
        GetClientWindow()->Disconnect(GetClientWindow()->GetId(),
            wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED,
            wxAuiNotebookEventHandler(wxAuiDocMDIParentFrame::OnNotebookPageChanged),
            NULL,
            this);

        wxAuiMDIParentFrame::OnCloseWindow(event);
    }
}


#if wxUSE_MENUS
void wxAuiDocMDIParentFrame::DoHandleMenu(wxCommandEvent& event)
{
    switch (event.GetId())
    {
    case wxWINDOWCLOSEALL:
        while (GetActiveChild())
        {
            if (!GetActiveChild()->Close())
            {
                event.Skip(false);
                return; // failure
            }
        }
        break;
    default:
        wxAuiMDIParentFrame::DoHandleMenu(event);
    }
}
#endif // wxUSE_MENUS


/*
 * Default document child frame for Aui MDI children
 */

IMPLEMENT_CLASS(wxAuiDocMDIChildFrame, wxAuiMDIChildFrame)

wxAuiDocMDIChildFrame::wxAuiDocMDIChildFrame(wxDocument *doc,
                                             wxView *view,
                                             wxAuiMDIParentFrame *parent,
                                             wxWindowID id,
                                             const wxString& title,
                                             const wxPoint& pos,
                                             const wxSize& size,
                                             long style,
                                             const wxString& name)
{
    Create(doc, view, parent, id, title, pos, size, style, name);
}

bool wxAuiDocMDIChildFrame::Create(wxDocument *doc,
                                   wxView *view,
                                   wxAuiMDIParentFrame *parent,
                                   wxWindowID id,
                                   const wxString& title,
                                   const wxPoint& pos,
                                   const wxSize& size,
                                   long style,
                                   const wxString& name)
{
    return wxAuiDocMDIChildFrameBase::Create(doc, view, parent, id, title, pos, size, style, name);
}

#endif
    //wxUSE_AUI
#endif
    // wxUSE_MDI
