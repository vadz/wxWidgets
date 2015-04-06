///////////////////////////////////////////////////////////////////////////////
// Name:        samples/xrc/openxrc.cpp
// Purpose:     Function for opening XRC files in the XRC sample.
// Author:      Vadim Zeitlin
// Created:     2015-04-06
// Copyright:   (c) 2015 Vadim Zeitlin
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/dialog.h"
    #include "wx/frame.h"
    #include "wx/log.h"
#endif

#include "wx/choicdlg.h"
#include "wx/generic/stattextg.h"

#include "wx/xrc/xmlres.h"

// XML classes only need to be used here to implement the capability to load
// arbitrary XRC files, they are not required during normal XRC use.
#include "wx/xml/xml.h"

// Define a generic fall back XRC handler: this is used to avoid errors when
// loading XRC files using custom handlers.
class FallbackResourceHandler : public wxXmlResourceHandler
{
public:
    FallbackResourceHandler()
        : m_oldUnknownHandler(wxXmlResource::Get()->SetUnknownHandler(this))
    {
        SetImpl(new wxXmlResourceHandlerImpl(this));
        SetParentResource(wxXmlResource::Get());

        AddWindowStyles();
    }

    virtual wxObject *DoCreateResource()
    {
        wxWindow* const win = new wxGenericStaticText
                                  (
                                   m_parentAsWindow,
                                    wxID_ANY,
                                    wxString::Format("[Dummy %s]", GetClass()),
                                    GetPosition(),
                                    GetSize(),
                                    GetStyle(),
                                    GetName()
                                  );

        if ( GetBool("hidden", 0) )
            win->Hide();

        // Make the dummy window stand out.
        win->SetFont(win->GetFont().MakeBold());
        win->SetBackgroundColour(0xff00ff);

        SetupWindow(win);

        return win;
    }

    virtual bool CanHandle(wxXmlNode * WXUNUSED(node))
    {
        return true;
    }

    virtual ~FallbackResourceHandler()
    {
        wxXmlResource::Get()->SetUnknownHandler(m_oldUnknownHandler);
    }

private:
    wxXmlResourceHandler* const m_oldUnknownHandler;

    wxDECLARE_NO_COPY_CLASS(FallbackResourceHandler);
};

extern bool OpenXRCFile(wxWindow* parent, const wxString& xrcFile)
{
    wxXmlResource* const res = wxXmlResource::Get();

    if ( !res->Load(xrcFile) )
        return false;

    // Show any warnings that might have been generated when loading the file.
    wxLog::FlushActive();

    // Look at all top level children of the loaded XML file.
    const wxXmlDocument* const doc = res->GetDocument(xrcFile);
    wxCHECK_MSG( doc, false, "should have valid wxXmlDocument" );

    const wxXmlNode* const root = doc->GetRoot();
    wxCHECK_MSG( root, false, "should have valid root" );

    wxArrayString
        dialogs,
        frames,
        panels,
        all;
    for ( const wxXmlNode* n = root->GetChildren(); n; n = n->GetNext() )
    {
        const wxString cls = n->GetAttribute("class", wxString());
        const wxString name = n->GetAttribute("name", wxString());

        if ( cls == "wxDialog" )
            dialogs.push_back(name);
        else if ( cls == "wxFrame" )
            frames.push_back(name);
        else if ( cls == "wxPanel" )
            panels.push_back(name);
        else
            continue;

        all.push_back(name);
    }

    // If there is just one element we can open, do open it. Otherwise ask the
    // user to select one.
    wxString name;
    switch ( all.size() )
    {
        case 0:
            wxLogWarning("XRC file \"%s\" doesn't contain any windows that "
                         "can be opened.", xrcFile);
            return false;

        case 1:
            name = all[0];
            break;

        default:
            name = wxGetSingleChoice
                   (
                        "Choose the name of the object to open",
                        "XRC sample",
                        all,
                        parent
                   );
            if ( name.empty() )
                return false;
    }

    // Ensure that we do something even with the elements of custom classes
    // that might be present.
    FallbackResourceHandler fallbackHandler;

    if ( dialogs.Index(name) != wxNOT_FOUND )
    {
        wxDialog dlg;
        if ( res->LoadDialog(&dlg, parent, name) )
            dlg.ShowModal();
    }
    else if ( frames.Index(name) != wxNOT_FOUND )
    {
        wxFrame* const frame = res->LoadFrame(NULL, name);
        if ( frame )
            frame->Show();
    }
    else if ( panels.Index(name) != wxNOT_FOUND )
    {
        // Put the panel inside a resizeable dialog to check how it resizes.
        wxDialog dlg(parent, wxID_ANY, "XRC sample dialog",
                     wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        if ( res->LoadPanel(&dlg, name) )
            dlg.ShowModal();
    }
    else
    {
        wxLogWarning("XRC sample is confused.");
    }

    return true;
}
