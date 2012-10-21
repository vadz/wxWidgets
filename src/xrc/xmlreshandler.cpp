/////////////////////////////////////////////////////////////////////////////
// Name:wx/xrc/xmlreshandler.cpp
// Purpose: XML resource handler
// Author:  Steven Lamerton
// Created: 2011/01/26
// RCS-ID:
// Copyright:   (c) 2011 Steven Lamerton
// Licence: wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_XRC

#include "wx/xrc/xmlreshandler.h"
#include "wx/xrc/xmlres.h"

IMPLEMENT_ABSTRACT_CLASS(wxXmlResourceHandler, wxObject)
IMPLEMENT_ABSTRACT_CLASS(wxXmlResourceHandlerImplBase, wxObject)

wxXmlResourceHandler::wxXmlResourceHandler()
{
    m_impl = NULL;
}

wxXmlResourceHandler::~wxXmlResourceHandler()
{
    wxDELETE(m_impl);
}

void wxXmlResourceHandler::SetParentResource(wxXmlResource *res)
{
    m_resource = res;
}

void wxXmlResourceHandler::SetImpl(wxXmlResourceHandlerImplBase* impl)
{
    m_impl = impl;
}

wxXmlResourceHandlerImplBase* wxXmlResourceHandler::GetImpl() const
{
    if(!m_impl)
    {
        wxFAIL_MSG(wxT("Implementation not found"));
    }
    return m_impl;
}

void wxXmlResourceHandler::AddStyle(const wxString& name, int value)
{
    m_styleNames.Add(name);
    m_styleValues.Add(value);
}

void wxXmlResourceHandler::AddWindowStyles()
{
    XRC_ADD_STYLE(wxCLIP_CHILDREN);

    // the border styles all have the old and new names, recognize both for now
    XRC_ADD_STYLE(wxSIMPLE_BORDER); XRC_ADD_STYLE(wxBORDER_SIMPLE);
    XRC_ADD_STYLE(wxSUNKEN_BORDER); XRC_ADD_STYLE(wxBORDER_SUNKEN);
    XRC_ADD_STYLE(wxDOUBLE_BORDER); XRC_ADD_STYLE(wxBORDER_DOUBLE); // deprecated
    XRC_ADD_STYLE(wxBORDER_THEME);
    XRC_ADD_STYLE(wxRAISED_BORDER); XRC_ADD_STYLE(wxBORDER_RAISED);
    XRC_ADD_STYLE(wxSTATIC_BORDER); XRC_ADD_STYLE(wxBORDER_STATIC);
    XRC_ADD_STYLE(wxNO_BORDER);     XRC_ADD_STYLE(wxBORDER_NONE);
    XRC_ADD_STYLE(wxBORDER_DEFAULT);

    XRC_ADD_STYLE(wxTRANSPARENT_WINDOW);
    XRC_ADD_STYLE(wxWANTS_CHARS);
    XRC_ADD_STYLE(wxTAB_TRAVERSAL);
    XRC_ADD_STYLE(wxNO_FULL_REPAINT_ON_RESIZE);
    XRC_ADD_STYLE(wxFULL_REPAINT_ON_RESIZE);
    XRC_ADD_STYLE(wxVSCROLL);
    XRC_ADD_STYLE(wxHSCROLL);
    XRC_ADD_STYLE(wxALWAYS_SHOW_SB);
    XRC_ADD_STYLE(wxWS_EX_BLOCK_EVENTS);
    XRC_ADD_STYLE(wxWS_EX_VALIDATE_RECURSIVELY);
    XRC_ADD_STYLE(wxWS_EX_TRANSIENT);
    XRC_ADD_STYLE(wxWS_EX_CONTEXTHELP);
    XRC_ADD_STYLE(wxWS_EX_PROCESS_IDLE);
    XRC_ADD_STYLE(wxWS_EX_PROCESS_UI_UPDATES);
}

//We simply pass everything through to the implementation

wxObject *wxXmlResourceHandler::CreateResource(wxXmlNode *node, wxObject *parent, wxObject *instance)
{
    return GetImpl()->CreateResource(node, parent, instance);
}

void wxXmlResourceHandler::CreateChildren(wxObject *parent, bool this_hnd_only)
{
    GetImpl()->CreateChildren(parent, this_hnd_only);
}

void wxXmlResourceHandler::CreateChildrenPrivately(wxObject *parent, wxXmlNode *rootnode)
{
    GetImpl()->CreateChildrenPrivately(parent, rootnode);
}

bool wxXmlResourceHandler::IsOfClass(wxXmlNode *node, const wxString& classname) const
{
    return GetImpl()->IsOfClass(node, classname);
}

wxString wxXmlResourceHandler::GetNodeContent(const wxXmlNode *node)
{
    return GetImpl()->GetNodeContent(node);
}

bool wxXmlResourceHandler::HasParam(const wxString& param)
{
    return GetImpl()->HasParam(param);
}

wxXmlNode *wxXmlResourceHandler::GetParamNode(const wxString& param)
{
    return GetImpl()->GetParamNode(param);
}

wxString wxXmlResourceHandler::GetParamValue(const wxString& param)
{
    return GetImpl()->GetParamValue(param);
};

wxString wxXmlResourceHandler::GetParamValue(const wxXmlNode* node)
{
    return GetImpl()->GetParamValue(node);
}

int wxXmlResourceHandler::GetStyle(const wxString& param, int defaults)
{
    return GetImpl()->GetStyle(param, defaults);
}

wxString wxXmlResourceHandler::GetText(const wxString& param, bool translate)
{
    return GetImpl()->GetText(param, translate);
}

int wxXmlResourceHandler::GetID()
{
    return GetImpl()->GetID();
}

wxString wxXmlResourceHandler::GetName()
{
    return GetImpl()->GetName();
}

bool wxXmlResourceHandler::GetBool(const wxString& param, bool defaultv)
{
    return GetImpl()->GetBool(param, defaultv);
}

long wxXmlResourceHandler::GetLong(const wxString& param, long defaultv)
{
    return GetImpl()->GetLong(param, defaultv);
}

float wxXmlResourceHandler::GetFloat(const wxString& param, float defaultv)
{
    return GetImpl()->GetFloat(param, defaultv);
}

wxColour wxXmlResourceHandler::GetColour(const wxString& param, const wxColour& defaultv)
{
    return GetImpl()->GetColour(param, defaultv);
}

wxSize wxXmlResourceHandler::GetSize(const wxString& param, wxWindow *windowToUse)
{
    return GetImpl()->GetSize(param, windowToUse);
}

wxPoint wxXmlResourceHandler::GetPosition(const wxString& param)
{
    return GetImpl()->GetPosition(param);
}

wxCoord wxXmlResourceHandler::GetDimension(const wxString& param, wxCoord defaultv,
                                           wxWindow *windowToUse)
{
    return GetImpl()->GetDimension(param, defaultv, windowToUse);
}

wxDirection wxXmlResourceHandler::GetDirection(const wxString& param, wxDirection dir)
{
    return GetImpl()->GetDirection(param, dir);
}

wxBitmap wxXmlResourceHandler::GetBitmap(const wxString& param,
                                         const wxArtClient& defaultArtClient,
                                         wxSize size)
{
    return GetImpl()->GetBitmap(param, defaultArtClient, size);
}

wxBitmap wxXmlResourceHandler::GetBitmap(const wxXmlNode* node,
                                         const wxArtClient& defaultArtClient,
                                         wxSize size)
{
    return GetImpl()->GetBitmap(node, defaultArtClient, size);
}

wxIcon wxXmlResourceHandler::GetIcon(const wxString& param,
                                     const wxArtClient& defaultArtClient,
                                     wxSize size)
{
    return GetImpl()->GetIcon(param, defaultArtClient, size);
}

wxIcon wxXmlResourceHandler::GetIcon(const wxXmlNode* node,
                                     const wxArtClient& defaultArtClient,
                                     wxSize size)
{
    return GetImpl()->GetIcon(node, defaultArtClient, size);
}

wxIconBundle wxXmlResourceHandler::GetIconBundle(const wxString& param,
                                                 const wxArtClient& defaultArtClient)
{
    return GetImpl()->GetIconBundle(param, defaultArtClient);
}

wxImageList *wxXmlResourceHandler::GetImageList(const wxString& param)
{
    return GetImpl()->GetImageList(param);
}

#if wxUSE_ANIMATIONCTRL
wxAnimation wxXmlResourceHandler::GetAnimation(const wxString& param)
{
    return GetImpl()->GetAnimation(param);
}
#endif

wxFont wxXmlResourceHandler::GetFont(const wxString& param, wxWindow* parent)
{
    return GetImpl()->GetFont(param, parent);
}

bool wxXmlResourceHandler::GetBoolAttr(const wxString& attr, bool defaultv)
{
    return GetImpl()->GetBoolAttr(attr, defaultv);
}

void wxXmlResourceHandler::SetupWindow(wxWindow *wnd)
{
    GetImpl()->SetupWindow(wnd);
}

wxObject *wxXmlResourceHandler::CreateResFromNode(wxXmlNode *node,
                                                  wxObject *parent,
                                                  wxObject *instance)
{
    return GetImpl()->CreateResFromNode(node, parent, instance);
}

#if wxUSE_FILESYSTEM
wxFileSystem& wxXmlResourceHandler::GetCurFileSystem()
{
    return GetImpl()->GetCurFileSystem();
}
#endif

void wxXmlResourceHandler::ReportError(wxXmlNode *context,
                                       const wxString& message)
{
    return GetImpl()->ReportError(context, message);
}

void wxXmlResourceHandler::ReportError(const wxString& message)
{
    return GetImpl()->ReportError(message);
}

void wxXmlResourceHandler::ReportParamError(const wxString& param,
                                            const wxString& message)
{
    return GetImpl()->ReportParamError(param, message);
}

#endif
