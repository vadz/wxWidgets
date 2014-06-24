///////////////////////////////////////////////////////////////////////////////
// Name:        tests/vectors/vectors.cpp
// Purpose:     wxVector<T> unit test
// Author:      Anton Dyachenko
// Created:     2014-06-11
// Copyright:   (c) 2014 Anton Dyachenko
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "testprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif // WX_PRECOMP

#include "wx/zipstrm.h"
#include "wx/wfstream.h"

// ----------------------------------------------------------------------------
// simple class capable of detecting leaks of its objects
// ----------------------------------------------------------------------------

class AesZipTestCase : public CppUnit::TestCase
{
public:
    AesZipTestCase() {}

private:
    CPPUNIT_TEST_SUITE( AesZipTestCase );
        CPPUNIT_TEST( Decrypt );
    CPPUNIT_TEST_SUITE_END();

    void Decrypt();

    DECLARE_NO_COPY_CLASS(AesZipTestCase)
};

wxString GetPassword(const wxString& zip)
{
    return "aesziptestarchive";
}

void AesZipTestCase::Decrypt()
{
    wxByte buffer[1024];
    wxFileInputStream file("aesziptestarchive.zip");
    //file.Read(buffer, sizeof(buffer));
    wxZipInputStream input(file);
    input.SetPasswordProvider(GetPassword, "aesziptestarchive.zip");
    auto entry = input.GetNextEntry();
    input.OpenEntry(*entry);
    input.Read(buffer, sizeof(buffer));
    int a = 0;
}

// register in the unnamed registry so that these tests are run by default
CPPUNIT_TEST_SUITE_REGISTRATION( AesZipTestCase );

// also include in its own registry so that these tests can be run alone
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( AesZipTestCase, "AesZipTestCase" );
