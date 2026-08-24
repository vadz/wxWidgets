///////////////////////////////////////////////////////////////////////////////
// Name:        tests/controls/simplebooktest.cpp
// Purpose:     wxSimplebook unit test
// Author:      Vadim Zeitlin
// Created:     2013-06-23
// Copyright:   (c) 2013 Vadim Zeitlin <vadim@wxwidgets.org>
///////////////////////////////////////////////////////////////////////////////

#include "testprec.h"

#if wxUSE_BOOKCTRL


#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/panel.h"
#endif // WX_PRECOMP

#include "wx/simplebook.h"
#include "bookctrlbasetest.h"

class SimplebookTestCase : public BookCtrlBaseTestCase
{
public:
    SimplebookTestCase();
    ~SimplebookTestCase();

protected:
    virtual wxBookCtrlBase *GetBase() const override { return m_simplebook; }

    virtual wxEventType GetChangedEvent() const override
        { return wxEVT_BOOKCTRL_PAGE_CHANGED; }

    virtual wxEventType GetChangingEvent() const override
        { return wxEVT_BOOKCTRL_PAGE_CHANGING; }

    wxSimplebook *m_simplebook;

    wxDECLARE_NO_COPY_CLASS(SimplebookTestCase);
};

wxBOOK_CTRL_BASE_TESTS(SimplebookTestCase, "Simplebook",
                       "[simplebook][book]");

SimplebookTestCase::SimplebookTestCase()
{
    m_simplebook = new wxSimplebook(wxTheApp->GetTopWindow(), wxID_ANY);
    AddPanels();
}

SimplebookTestCase::~SimplebookTestCase()
{
    wxDELETE(m_simplebook);
}

#endif // wxUSE_BOOKCTRL

