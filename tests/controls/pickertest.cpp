///////////////////////////////////////////////////////////////////////////////
// Name:        tests/controls/pickertest.cpp
// Purpose:     Tests for various wxPickerBase based classes
// Author:      Steven Lamerton
// Created:     2010-08-07
// Copyright:   (c) 2010 Steven Lamerton
///////////////////////////////////////////////////////////////////////////////

#include "testprec.h"

#if wxUSE_COLOURPICKERCTRL || \
    wxUSE_DIRPICKERCTRL    || \
    wxUSE_FILEPICKERCTRL   || \
    wxUSE_FONTPICKERCTRL


#ifndef WX_PRECOMP
    #include "wx/app.h"
#endif // WX_PRECOMP

#include "wx/clrpicker.h"
#include "wx/filepicker.h"
#include "wx/fontpicker.h"
#include "pickerbasetest.h"
#include "asserthelper.h"

#if wxUSE_COLOURPICKERCTRL

class ColourPickerCtrlTestCase : public PickerBaseTestCase
{
public:
    ColourPickerCtrlTestCase();
    ~ColourPickerCtrlTestCase();

protected:
    virtual wxPickerBase *GetBase() const override { return m_colour; }

    wxColourPickerCtrl *m_colour;

    wxDECLARE_NO_COPY_CLASS(ColourPickerCtrlTestCase);
};

wxPICKER_BASE_TESTS(ColourPickerCtrlTestCase, "ColourPickerCtrl",
                    "[colourpicker][picker]");

ColourPickerCtrlTestCase::ColourPickerCtrlTestCase()
{
    m_colour = new wxColourPickerCtrl(wxTheApp->GetTopWindow(), wxID_ANY,
                                     *wxBLACK, wxDefaultPosition,
                                      wxDefaultSize, wxCLRP_USE_TEXTCTRL);
}

ColourPickerCtrlTestCase::~ColourPickerCtrlTestCase()
{
    wxDELETE(m_colour);
}

#endif //wxUSE_COLOURPICKERCTRL

#if wxUSE_DIRPICKERCTRL

class DirPickerCtrlTestCase : public PickerBaseTestCase
{
public:
    DirPickerCtrlTestCase();
    ~DirPickerCtrlTestCase();

protected:
    virtual wxPickerBase *GetBase() const override { return m_dir; }

    wxDirPickerCtrl *m_dir;

    wxDECLARE_NO_COPY_CLASS(DirPickerCtrlTestCase);
};

wxPICKER_BASE_TESTS(DirPickerCtrlTestCase, "DirPickerCtrl",
                    "[dirpicker][picker]");

DirPickerCtrlTestCase::DirPickerCtrlTestCase()
{
    m_dir = new wxDirPickerCtrl(wxTheApp->GetTopWindow(), wxID_ANY,
                                wxEmptyString, wxDirSelectorPromptStr,
                                wxDefaultPosition, wxDefaultSize,
                                wxDIRP_USE_TEXTCTRL);
}

DirPickerCtrlTestCase::~DirPickerCtrlTestCase()
{
    wxDELETE(m_dir);
}

#endif //wxUSE_DIRPICKERCTRL

#if wxUSE_FILEPICKERCTRL

class FilePickerCtrlTestCase : public PickerBaseTestCase
{
public:
    FilePickerCtrlTestCase();
    ~FilePickerCtrlTestCase();

protected:
    virtual wxPickerBase *GetBase() const override { return m_file; }

    wxFilePickerCtrl *m_file;

    wxDECLARE_NO_COPY_CLASS(FilePickerCtrlTestCase);
};

wxPICKER_BASE_TESTS(FilePickerCtrlTestCase, "FilePickerCtrl",
                    "[filepicker][picker]");

FilePickerCtrlTestCase::FilePickerCtrlTestCase()
{
    m_file = new wxFilePickerCtrl(wxTheApp->GetTopWindow(), wxID_ANY,
                                  wxEmptyString, wxFileSelectorPromptStr,
                                  wxFileSelectorDefaultWildcardStr,
                                  wxDefaultPosition, wxDefaultSize,
                                  wxFLP_USE_TEXTCTRL);
}

FilePickerCtrlTestCase::~FilePickerCtrlTestCase()
{
    wxDELETE(m_file);
}

#endif //wxUSE_FILEPICKERCTRL

#if wxUSE_FONTPICKERCTRL

class FontPickerCtrlTestCase : public PickerBaseTestCase
{
public:
    FontPickerCtrlTestCase();
    ~FontPickerCtrlTestCase();

protected:
    virtual wxPickerBase *GetBase() const override { return m_font; }

    wxFontPickerCtrl *m_font;

    wxDECLARE_NO_COPY_CLASS(FontPickerCtrlTestCase);
};

wxPICKER_BASE_TESTS(FontPickerCtrlTestCase, "FontPickerCtrl",
                    "[fontpicker][picker]");

FontPickerCtrlTestCase::FontPickerCtrlTestCase()
{
    m_font = new wxFontPickerCtrl(wxTheApp->GetTopWindow(), wxID_ANY,
                                  wxNullFont, wxDefaultPosition, wxDefaultSize,
                                  wxFNTP_USE_TEXTCTRL);
}

FontPickerCtrlTestCase::~FontPickerCtrlTestCase()
{
    wxDELETE(m_font);
}

TEST_CASE_METHOD(FontPickerCtrlTestCase, "FontPickerCtrl::ColourSelection",
                 "[fontpicker]")
{
    wxColour selectedColour(0xFF4269UL);

    CHECK( m_font->GetSelectedColour() != selectedColour );

    m_font->SetSelectedColour(selectedColour);

    INFO("Font picker did not react to color selection");
    CHECK(selectedColour == m_font->GetSelectedColour());
}
#endif //wxUSE_FONTPICKERCTRL

#endif
