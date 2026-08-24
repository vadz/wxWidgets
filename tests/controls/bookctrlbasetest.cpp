///////////////////////////////////////////////////////////////////////////////
// Name:        tests/controls/bookctrlbasetest.cpp
// Purpose:     wxBookCtrlBase unit test
// Author:      Steven Lamerton
// Created:     2010-07-02
// Copyright:   (c) 2010 Steven Lamerton
///////////////////////////////////////////////////////////////////////////////

#include "testprec.h"

#if wxUSE_BOOKCTRL

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/panel.h"
#endif // WX_PRECOMP

#include "wx/artprov.h"
#include "wx/imaglist.h"
#include "wx/bookctrl.h"
#include "bookctrlbasetest.h"
#include "testableframe.h"

void BookCtrlBaseTestCase::AddPanels()
{
    wxBookCtrlBase * const base = GetBase();

    wxSize size(32, 32);

    m_list = new wxImageList(size.x, size.y);
    m_list->Add(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_OTHER, size));
    m_list->Add(wxArtProvider::GetIcon(wxART_QUESTION, wxART_OTHER, size));
    m_list->Add(wxArtProvider::GetIcon(wxART_WARNING, wxART_OTHER, size));

    base->AssignImageList(m_list);

    Realize();

    m_panel1 = new wxPanel(base);
    m_panel2 = new wxPanel(base);
    m_panel3 = new wxPanel(base);

    base->AddPage(m_panel1, "Panel &1", false, 0);
    base->AddPage(m_panel2, "Panel 2", false, 1);
    base->AddPage(m_panel3, "Panel 3", false, 2);
}

void BookCtrlBaseTestCase::Selection()
{
    wxBookCtrlBase * const base = GetBase();

    base->SetSelection(0);

    CHECK(base->GetSelection() == 0);
    CHECK(base->GetCurrentPage() == wxStaticCast(m_panel1, wxWindow));

    base->AdvanceSelection(false);

    CHECK(base->GetSelection() == 2);
    CHECK(base->GetCurrentPage() == wxStaticCast(m_panel3, wxWindow));

    base->AdvanceSelection();

    CHECK(base->GetSelection() == 0);
    CHECK(base->GetCurrentPage() == wxStaticCast(m_panel1, wxWindow));

    base->ChangeSelection(1);

    CHECK(base->GetSelection() == 1);
    CHECK(base->GetCurrentPage() == wxStaticCast(m_panel2, wxWindow));
}

void BookCtrlBaseTestCase::Text()
{
    wxBookCtrlBase * const base = GetBase();

    const wxString expected(HasBrokenMnemonics() ? "Panel 1" : "Panel &1");
    CHECK(base->GetPageText(0) == expected);

    base->SetPageText(1, "Some other string");

    CHECK(base->GetPageText(1) == "Some other string");

    base->SetPageText(2, "string with\nline break");

    CHECK(base->GetPageText(2) == "string with\nline break");

    if ( !HasBrokenMnemonics() )
    {
        base->SetPageText(0, "With &mnemonic");
        CHECK(base->GetPageText(0) == "With &mnemonic");
    }
}

void BookCtrlBaseTestCase::PageManagement()
{
    wxBookCtrlBase * const base = GetBase();

    base->InsertPage(0, new wxPanel(base), "New Panel", true, 0);

    Realize();

    CHECK(base->GetSelection() == 0);
    CHECK(base->GetPageCount() == 4);

    // Change the selection to verify that deleting a page before the currently
    // selected one correctly updates the selection.
    base->SetSelection(2);
    CHECK(base->GetSelection() == 2);

    base->DeletePage(1);

    CHECK(base->GetPageCount() == 3);
    CHECK(base->GetSelection() == 1);

    base->RemovePage(0);

    CHECK(base->GetPageCount() == 2);
    CHECK(base->GetSelection() == 0);

    base->DeleteAllPages();

    CHECK(base->GetPageCount() == 0);
    CHECK(base->GetSelection() == -1);
}

void BookCtrlBaseTestCase::ChangeEvents()
{
    wxBookCtrlBase * const base = GetBase();

    base->SetSelection(0);

    EventCounter changing(base, GetChangingEvent());
    EventCounter changed(base, GetChangedEvent());

    base->SetSelection(1);

    CHECK(changing.GetCount() == 1);
    CHECK(changed.GetCount() == 1);

    changed.Clear();
    changing.Clear();
    base->ChangeSelection(2);

    CHECK(changing.GetCount() == 0);
    CHECK(changed.GetCount() == 0);

    base->AdvanceSelection();

    CHECK(changing.GetCount() == 1);
    CHECK(changed.GetCount() == 1);

    changed.Clear();
    changing.Clear();
    base->AdvanceSelection(false);

    CHECK(changing.GetCount() == 1);
    CHECK(changed.GetCount() == 1);
}

void BookCtrlBaseTestCase::Image()
{
    wxBookCtrlBase * const base = GetBase();

    //Check AddPanels() set things correctly
    CHECK(base->GetImageList() == m_list);
    CHECK(base->GetPageImage(0) == 0);
    CHECK(base->GetPageImage(1) == 1);
    CHECK(base->GetPageImage(2) == 2);

    base->SetPageImage(0, 2);

    CHECK(base->GetPageImage(2) == 2);
}

#endif
