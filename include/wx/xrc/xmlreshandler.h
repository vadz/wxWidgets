/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xmlreshandler.cpp
// Purpose:     XML resource handler
// Author:      Steven Lamerton
// Created:     2011/01/26
// RCS-ID:
// Copyright:   (c) 2011 Steven Lamerton
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


#ifndef _WX_XMLRESHANDLER_H_
#define _WX_XMLRESHANDLER_H_

#include "wx/defs.h"

#if wxUSE_XRC

#include "wx/string.h"
#include "wx/artprov.h"
#include "wx/colour.h"
#include "wx/animate.h"
#include "wx/filesys.h"
#include "wx/imaglist.h"

class WXDLLIMPEXP_FWD_XML wxXmlNode;
class WXDLLIMPEXP_FWD_XML wxXmlResource;

class WXDLLIMPEXP_FWD_CORE wxXmlResourceHandlerImplBase;

class WXDLLIMPEXP_CORE wxXmlResourceHandler : public wxObject
{
DECLARE_ABSTRACT_CLASS(wxXmlResourceHandler)
public:
    // Constructor.
    wxXmlResourceHandler();

    // Destructor.
    ~wxXmlResourceHandler();

    // This one is called from CreateResource after variables
    // were filled.
    virtual wxObject *DoCreateResource() = 0;

    // Returns true if it understands this node and can create
    // a resource from it, false otherwise.
    virtual bool CanHandle(wxXmlNode *node) = 0;

    // Add a style flag (e.g. wxMB_DOCKABLE) to the list of flags
    // understood by this handler.
    void AddStyle(const wxString& name, int value);

    // Add styles common to all wxWindow-derived classes.
    void AddWindowStyles();

    void SetParentResource(wxXmlResource *res);
    void SetImpl(wxXmlResourceHandlerImplBase* impl);

    void ReportError(wxXmlNode *context, const wxString& message);
    void ReportError(const wxString& message);
    void ReportParamError(const wxString& param, const wxString& message);

    wxObject *CreateResource(wxXmlNode *node, wxObject *parent,
                             wxObject *instance);
    bool IsOfClass(wxXmlNode *node, const wxString& classname) const;
    wxString GetNodeContent(const wxXmlNode *node);
    bool HasParam(const wxString& param);

    //Various getters, see wxXmlResourceHandlerImpl for more details
    wxXmlNode *GetParamNode(const wxString& param);
    wxString GetParamValue(const wxString& param);
    wxString GetParamValue(const wxXmlNode* node);
    int GetStyle(const wxString& param = wxT("style"), int defaults = 0);
    wxString GetText(const wxString& param, bool translate = true);
    int GetID();
    wxString GetName();
    bool GetBool(const wxString& param, bool defaultv = false);
    long GetLong(const wxString& param, long defaultv = 0);
    float GetFloat(const wxString& param, float defaultv = 0);
    wxColour GetColour(const wxString& param,
                       const wxColour& defaultv = wxNullColour);
    wxSize GetSize(const wxString& param = wxT("size"),
                   wxWindow *windowToUse = NULL);
    wxPoint GetPosition(const wxString& param = wxT("pos"));
    wxCoord GetDimension(const wxString& param, wxCoord defaultv = 0,
                         wxWindow *windowToUse = NULL);
    wxDirection GetDirection(const wxString& param, wxDirection dir = wxLEFT);
    wxBitmap GetBitmap(const wxString& param = wxT("bitmap"),
                       const wxArtClient& defaultArtClient = wxART_OTHER,
                       wxSize size = wxDefaultSize);
    wxBitmap GetBitmap(const wxXmlNode* node,
                       const wxArtClient& defaultArtClient = wxART_OTHER,
                       wxSize size = wxDefaultSize);
    wxIcon GetIcon(const wxString& param = wxT("icon"),
                   const wxArtClient& defaultArtClient = wxART_OTHER,
                   wxSize size = wxDefaultSize);
    wxIcon GetIcon(const wxXmlNode* node,
                   const wxArtClient& defaultArtClient = wxART_OTHER,
                   wxSize size = wxDefaultSize);
    wxIconBundle GetIconBundle(const wxString& param,
                               const wxArtClient& defaultArtClient = wxART_OTHER);
    wxImageList *GetImageList(const wxString& param = wxT("imagelist"));

#if wxUSE_ANIMATIONCTRL
    wxAnimation GetAnimation(const wxString& param = wxT("animation"));
#endif

    wxFont GetFont(const wxString& param = wxT("font"), wxWindow* parent = NULL);
    bool GetBoolAttr(const wxString& attr, bool defaultv);
    void SetupWindow(wxWindow *wnd);
    void CreateChildren(wxObject *parent, bool this_hnd_only = false);
    void CreateChildrenPrivately(wxObject *parent, wxXmlNode *rootnode = NULL);
    wxObject *CreateResFromNode(wxXmlNode *node,
                                wxObject *parent, wxObject *instance = NULL);

#if wxUSE_FILESYSTEM
    wxFileSystem& GetCurFileSystem();
#endif

    wxXmlResourceHandlerImplBase* GetImpl() const;

public:
    // Variables (filled by CreateResource)
    wxXmlNode *m_node;
    wxString m_class;
    wxObject *m_parent, *m_instance;
    wxWindow *m_parentAsWindow;
    wxXmlResource *m_resource;

    wxArrayString m_styleNames;
    wxArrayInt m_styleValues;

private:
    wxXmlResourceHandlerImplBase *m_impl;

};

//Abstract base class for wxXmlResourceHandlerImpl so we don't have any
//xrc dependancies in core
class WXDLLIMPEXP_CORE wxXmlResourceHandlerImplBase : public wxObject
{
DECLARE_ABSTRACT_CLASS(wxXmlResourceHandlerImplBase)
public:
    // Constructor.
    wxXmlResourceHandlerImplBase(wxXmlResourceHandler *handler)
                               : m_handler(handler)
    {};

    // Destructor.
    virtual ~wxXmlResourceHandlerImplBase() {};

    virtual wxObject *CreateResource(wxXmlNode *node, wxObject *parent,
                                     wxObject *instance) = 0;
    virtual bool IsOfClass(wxXmlNode *node, const wxString& classname) const = 0;
    virtual wxString GetNodeContent(const wxXmlNode *node) = 0;
    virtual bool HasParam(const wxString& param) = 0;
    virtual wxXmlNode *GetParamNode(const wxString& param) = 0;
    virtual wxString GetParamValue(const wxString& param) = 0;
    virtual wxString GetParamValue(const wxXmlNode* node) = 0;
    virtual int GetStyle(const wxString& param = wxT("style"), int defaults = 0) = 0;
    virtual wxString GetText(const wxString& param, bool translate = true) = 0;
    virtual int GetID() = 0;
    virtual wxString GetName() = 0;
    virtual bool GetBool(const wxString& param, bool defaultv = false) = 0;
    virtual long GetLong(const wxString& param, long defaultv = 0) = 0;
    virtual float GetFloat(const wxString& param, float defaultv = 0) = 0;
    virtual wxColour GetColour(const wxString& param,
                               const wxColour& defaultv = wxNullColour) = 0;
    virtual wxSize GetSize(const wxString& param = wxT("size"),
                           wxWindow *windowToUse = NULL) = 0;
    virtual wxPoint GetPosition(const wxString& param = wxT("pos")) = 0;
    virtual wxCoord GetDimension(const wxString& param, wxCoord defaultv = 0,
                                 wxWindow *windowToUse = NULL) = 0;
    virtual wxDirection GetDirection(const wxString& param, wxDirection dir = wxLEFT) = 0;
    virtual wxBitmap GetBitmap(const wxString& param = wxT("bitmap"),
                               const wxArtClient& defaultArtClient = wxART_OTHER,
                               wxSize size = wxDefaultSize) = 0;
    virtual wxBitmap GetBitmap(const wxXmlNode* node,
                               const wxArtClient& defaultArtClient = wxART_OTHER,
                               wxSize size = wxDefaultSize) = 0;
    virtual wxIcon GetIcon(const wxString& param = wxT("icon"),
                           const wxArtClient& defaultArtClient = wxART_OTHER,
                           wxSize size = wxDefaultSize) = 0;
    virtual wxIcon GetIcon(const wxXmlNode* node,
                           const wxArtClient& defaultArtClient = wxART_OTHER,
                           wxSize size = wxDefaultSize) = 0;
    virtual wxIconBundle GetIconBundle(const wxString& param,
                                       const wxArtClient& defaultArtClient = wxART_OTHER) = 0;
    virtual wxImageList *GetImageList(const wxString& param = wxT("imagelist")) = 0;

#if wxUSE_ANIMATIONCTRL
    virtual wxAnimation GetAnimation(const wxString& param = wxT("animation")) = 0;
#endif

    virtual wxFont GetFont(const wxString& param = wxT("font"), wxWindow* parent = NULL) = 0;
    virtual bool GetBoolAttr(const wxString& attr, bool defaultv) = 0;
    virtual void SetupWindow(wxWindow *wnd) = 0;
    virtual void CreateChildren(wxObject *parent, bool this_hnd_only = false) = 0;
    virtual void CreateChildrenPrivately(wxObject *parent,
                                         wxXmlNode *rootnode = NULL) = 0;
    virtual wxObject *CreateResFromNode(wxXmlNode *node, wxObject *parent,
                                        wxObject *instance = NULL) = 0;

#if wxUSE_FILESYSTEM
    virtual wxFileSystem& GetCurFileSystem() = 0;
#endif
    virtual void ReportError(wxXmlNode *context, const wxString& message) = 0;
    virtual void ReportError(const wxString& message) = 0;
    virtual void ReportParamError(const wxString& param, const wxString& message) = 0;

    wxXmlResourceHandler* GetHandler() { return m_handler; }

protected:
    wxXmlResourceHandler *m_handler;
};
#endif

#endif
