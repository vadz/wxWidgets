///////////////////////////////////////////////////////////////////////////////
// Name:        tests/controls/togglebuttontest.cpp
// Purpose:     wxToggleButton unit test
// Author:      Steven Lamerton
// Created:     2010-07-14
// Copyright:   (c) 2010 Steven Lamerton
///////////////////////////////////////////////////////////////////////////////

#include "testprec.h"

#if wxUSE_TOGGLEBTN


#ifndef WX_PRECOMP
    #include "wx/app.h"
#endif // WX_PRECOMP

#include "testableframe.h"
#include "wx/uiaction.h"
#include "wx/tglbtn.h"

class ToggleButtonTestCase
{
public:
    ToggleButtonTestCase();
    ~ToggleButtonTestCase();

protected:
    wxToggleButton* m_button;

    wxDECLARE_NO_COPY_CLASS(ToggleButtonTestCase);
};

ToggleButtonTestCase::ToggleButtonTestCase()
{
    m_button = new wxToggleButton(wxTheApp->GetTopWindow(), wxID_ANY, "wxToggleButton");
}

ToggleButtonTestCase::~ToggleButtonTestCase()
{
    wxDELETE(m_button);
}

TEST_CASE_METHOD(ToggleButtonTestCase, "ToggleButton::Click", "[togglebutton]")
{
#if wxUSE_UIACTIONSIMULATOR
    if ( !EnableUITests() )
        return;

    EventCounter clicked(m_button, wxEVT_TOGGLEBUTTON);

    wxUIActionSimulator sim;

    //We move in slightly to account for window decorations
    sim.MouseMove(m_button->GetScreenPosition() + wxPoint(10, 10));
    wxYield();

    sim.MouseClick();
    wxYield();

    CHECK(clicked.GetCount() == 1);
    CHECK(m_button->GetValue());
    clicked.Clear();

    sim.MouseClick();
    wxYield();

    CHECK(clicked.GetCount() == 1);
    CHECK(!m_button->GetValue());
#endif
}

TEST_CASE_METHOD(ToggleButtonTestCase, "ToggleButton::Value", "[togglebutton]")
{
    EventCounter clicked(m_button, wxEVT_BUTTON);

    m_button->SetValue(true);

    CHECK(m_button->GetValue());

    m_button->SetValue(false);

    CHECK(!m_button->GetValue());

    CHECK( clicked.GetCount() == 0 );
}

#endif //wxUSE_TOGGLEBTN
