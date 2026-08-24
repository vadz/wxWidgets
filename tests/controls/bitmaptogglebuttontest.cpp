///////////////////////////////////////////////////////////////////////////////
// Name:        tests/controls/bitmaptogglebuttontest.cpp
// Purpose:     wxBitmapToggleButton unit test
// Author:      Steven Lamerton
// Created:     2010-07-17
// Copyright:   (c) 2010 Steven Lamerton
///////////////////////////////////////////////////////////////////////////////

#include "testprec.h"

#if wxUSE_TOGGLEBTN


#include "wx/tglbtn.h"

#ifdef wxHAS_BITMAPTOGGLEBUTTON

#ifndef WX_PRECOMP
    #include "wx/app.h"
#endif // WX_PRECOMP

#include "testableframe.h"
#include "wx/uiaction.h"
#include "wx/artprov.h"

class BitmapToggleButtonTestCase
{
public:
    BitmapToggleButtonTestCase();
    ~BitmapToggleButtonTestCase();

protected:
    wxBitmapToggleButton* m_button;

    wxDECLARE_NO_COPY_CLASS(BitmapToggleButtonTestCase);
};

BitmapToggleButtonTestCase::BitmapToggleButtonTestCase()
{
    m_button = new wxBitmapToggleButton(wxTheApp->GetTopWindow(), wxID_ANY,
                                        wxArtProvider::GetIcon(wxART_INFORMATION,
                                                               wxART_OTHER,
                                                               wxSize(32, 32)));
    m_button->Update();
    m_button->Refresh();
}

BitmapToggleButtonTestCase::~BitmapToggleButtonTestCase()
{
    wxDELETE(m_button);
}

TEST_CASE_METHOD(BitmapToggleButtonTestCase, "BitmapToggleButton::Click", "[bitmaptogglebutton]")
{
#if wxUSE_UIACTIONSIMULATOR
    if ( !EnableUITests() )
        return;

    EventCounter clicked(m_button, wxEVT_TOGGLEBUTTON);

    wxUIActionSimulator sim;

    const wxPoint pos = m_button->GetScreenPosition();

    //We move in slightly to account for window decorations
    sim.MouseMove(pos + wxPoint(10, 10));
    wxYield();

    sim.MouseClick();
    wxYield();

    CHECK(clicked.GetCount() == 1);
    CHECK(m_button->GetValue());

    clicked.Clear();

    // Change the mouse position to prevent the second click from being
    // recognized as double click.
    sim.MouseMove(pos + wxPoint(20, 20));
    wxYield();

    sim.MouseClick();
    wxYield();

    CHECK(clicked.GetCount() == 1);
    CHECK(!m_button->GetValue());
#endif // wxUSE_UIACTIONSIMULATOR
}

TEST_CASE_METHOD(BitmapToggleButtonTestCase, "BitmapToggleButton::Value", "[bitmaptogglebutton]")
{
    EventCounter clicked(m_button, wxEVT_BUTTON);

    m_button->SetValue(true);

    CHECK(m_button->GetValue());

    m_button->SetValue(false);

    CHECK(!m_button->GetValue());

    CHECK( clicked.GetCount() == 0 );
}

#endif // wxHAS_BITMAPTOGGLEBUTTON

#endif // wxUSE_TOGGLEBTN
