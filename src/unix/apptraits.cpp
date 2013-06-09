///////////////////////////////////////////////////////////////////////////////
// Name:        src/unix/apptraits.cpp
// Purpose:     implementation of wxGUIAppTraits for Unix systems
// Author:      Vadim Zeitlin
// Created:     2008-03-22
// RCS-ID:      $Id$
// Copyright:   (c) 2008 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/apptrait.h"

#ifndef WX_PRECOMP
    #include "wx/utils.h"
#endif // WX_PRECOMP

#include "wx/unix/execute.h"
#include "wx/evtloop.h"

// ============================================================================
// implementation
// ============================================================================

int wxGUIAppTraits::WaitForChild(wxExecuteData& execData)
{
    const int flags = execData.flags;
    if ( !(flags & wxEXEC_SYNC) || (flags & wxEXEC_NOEVENTS) )
    {
        // async or blocking sync cases are already handled by the base class
        // just fine, no need to duplicate its code here
        return wxAppTraits::WaitForChild(execData);
    }

    // prepare to wait for the child termination: show to the user that we're
    // busy and refuse all input unless explicitly told otherwise
    wxBusyCursor bc;
    wxWindowDisabler wd(!(flags & wxEXEC_NODISABLE));

    // Allocate an event loop that will be used to wait for the process
    // to terminate, will handle stdout, stderr, and any other events.
    //
    // The event loop will get started in common (to console and GUI) code
    // in WaitForChildSync
    wxGUIEventLoop loop;
    execData.endProcData.syncEventLoopPtr = &loop;

    return WaitForChildSync(execData);
}

