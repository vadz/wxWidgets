///////////////////////////////////////////////////////////////////////////////
// Name:        tests/controls/htmllboxtest.cpp
// Purpose:     wxSimpleHtmlListBoxNameStr unit test
// Author:      Vadim Zeitlin
// Created:     2010-11-27
// Copyright:   (c) 2010 Vadim Zeitlin <vadim@wxwidgets.org>
///////////////////////////////////////////////////////////////////////////////

#include "testprec.h"

#if wxUSE_HTML


#ifndef WX_PRECOMP
    #include "wx/app.h"
#endif // WX_PRECOMP

#include "wx/htmllbox.h"
#include "itemcontainertest.h"

class HtmlListBoxTestCase : public ItemContainerTestCase
{
public:
    HtmlListBoxTestCase();
    ~HtmlListBoxTestCase();

protected:
    virtual wxItemContainer *GetContainer() const override { return m_htmllbox; }
    virtual wxWindow *GetContainerWindow() const override { return m_htmllbox; }

    wxSimpleHtmlListBox* m_htmllbox;

    wxDECLARE_NO_COPY_CLASS(HtmlListBoxTestCase);
};

wxITEM_CONTAINER_TESTS(HtmlListBoxTestCase, "HtmlListBox",
                       "[htmllistbox][item-container]");

HtmlListBoxTestCase::HtmlListBoxTestCase()
{
    m_htmllbox = new wxSimpleHtmlListBox(wxTheApp->GetTopWindow(), wxID_ANY);
}

HtmlListBoxTestCase::~HtmlListBoxTestCase()
{
    wxDELETE(m_htmllbox);
}

#endif //wxUSE_HTML
