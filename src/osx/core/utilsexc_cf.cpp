/////////////////////////////////////////////////////////////////////////////
// Name:        src/osx/core/utilsexc_cf.cpp
// Purpose:     Execution-related utilities for Darwin
// Author:      David Elliott, Ryan Norton (wxMacExecute)
// Modified by: Stefan Csomor (added necessary wxT for unicode builds)
// Created:     2004-11-04
// RCS-ID:      $Id$
// Copyright:   (c) David Elliott, Ryan Norton
// Licence:     wxWindows licence
// Notes:       This code comes from src/osx/carbon/utilsexc.cpp,1.11
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/utils.h"
#endif //ndef WX_PRECOMP
#include "wx/unix/execute.h"
#include "wx/stdpaths.h"
#include "wx/app.h"
#include "wx/apptrait.h"
#include "wx/thread.h"
#include "wx/process.h"

#include <sys/wait.h>

#include <CoreFoundation/CFSocket.h>

namespace
{

WX_DECLARE_HASH_MAP(int, CFSocketRef, wxIntegerHash, wxIntegerEqual, FDSockets);
FDSockets gs_sockets;

}

/*!
    Called due to source signal detected by the CFRunLoop.
    This is nearly identical to the wxGTK equivalent.
 */
extern "C" void WXCF_EndProcessDetector(CFSocketRef s,
                                        CFSocketCallBackType WXUNUSED(callbackType),
                                        CFDataRef WXUNUSED(address),
                                        void const *WXUNUSED(data),
                                        void *info)
{
    wxOnReadWaiting(static_cast<wxFDIOHandler *>(info), CFSocketGetNative(s));
}

/*!
    Implements the GUI-specific AddProcessCallback() for both wxMac and
    wxCocoa using the CFSocket/CFRunLoop API which is available to both.
    Takes advantage of the fact that sockets on UNIX are just regular
    file descriptors and thus even a non-socket file descriptor can
    apparently be used with CFSocket so long as you only tell CFSocket
    to do things with it that would be valid for a non-socket fd.
 */
void wxGUIAppTraits::AddProcessCallback(wxFDIOHandler& handler, int fd)
{
    CFSocketContext context =
    {   0
    ,   &handler
    ,   NULL
    ,   NULL
    ,   NULL
    };
    CFSocketRef cfSocket = CFSocketCreateWithNative(kCFAllocatorDefault,fd,kCFSocketReadCallBack,&WXCF_EndProcessDetector,&context);
    if(cfSocket == NULL)
    {
        wxLogError(wxT("Failed to create socket for end process detection"));
        return;
    }

    /* When IO is complete and we need to stop the callbacks, we need to
       call CFSocketInvalidate().  By default, CFSocketInvalidate() will
       also close the fd, which is undesired and could cause lots of
       problems.  In order to not close the fd, we need to clear the
       kCFSocketCloseOnInvalidate flag.  See:

       https://developer.apple.com/library/mac/#documentation/CoreFOundation/Reference/CFSocketRef/Reference/reference.html
       */
    CFOptionFlags sockopt = CFSocketGetSocketFlags(cfSocket);

    /* Clear the close-on-invalidate flag. */
    sockopt &= ~kCFSocketCloseOnInvalidate;

    CFSocketSetSocketFlags(cfSocket, sockopt);

    CFRunLoopSourceRef runLoopSource = CFSocketCreateRunLoopSource(kCFAllocatorDefault, cfSocket, /*highest priority:*/0);
    if(runLoopSource == NULL)
    {
        wxLogError(wxT("Failed to create CFRunLoopSource from CFSocket for end process detection"));
        CFSocketInvalidate(cfSocket);
        CFRelease(cfSocket);
        return;
    }

    // Save the socket so that we can remove it later if asked to.
    gs_sockets[fd] = cfSocket;

    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    // Now that the run loop has the source retained we can release it.
    CFRelease(runLoopSource);
}

void wxGUIAppTraits::RemoveProcessCallback(int fd)
{
    const FDSockets::iterator it = gs_sockets.find(fd);
    wxCHECK_RET( it != gs_sockets.end(), "No such FD" );

   /*
       CFSocketInvalidate does not close the fd due to clearing of
       the kCFSocketCloseOnInvalidate when the socket (fd) was
       created.

       CFSocketInvalidate invalidates the run loop source for us which should
       cause it to release the CFSocket (thus causing it to be deallocated) and
       remove itself from the runloop which should release it and
       cause it to also be deallocated.  Of course, it's possible
       the RunLoop hangs onto one or both of them by
       retaining/releasing them within its stack frame.  However,
       that shouldn't be depended on. Assume that s is deallocated
       due to the following call.
    */
   CFSocketInvalidate(it->second);
   gs_sockets.erase(it);
}

/////////////////////////////////////////////////////////////////////////////

// NOTE: This doesn't really belong here but this was a handy file to
// put it in because it's already compiled for wxCocoa and wxMac GUI lib.
#if wxUSE_STDPATHS
static wxStandardPathsCF gs_stdPaths;
wxStandardPaths& wxGUIAppTraits::GetStandardPaths()
{
    return gs_stdPaths;
}
#endif

#if wxUSE_SOCKETS

// we need to implement this method in a file of the core library as it should
// only be used for the GUI applications but we can't use socket stuff from it
// directly as this would create unwanted dependencies of core on net library
//
// so we have this global pointer which is set from sockosx.cpp when it is
// linked in and we simply return it from here
extern WXDLLIMPEXP_BASE wxSocketManager *wxOSXSocketManagerCF;
wxSocketManager *wxGUIAppTraits::GetSocketManager()
{
    return wxOSXSocketManagerCF ? wxOSXSocketManagerCF
                                : wxGUIAppTraitsBase::GetSocketManager();
}

#endif // wxUSE_SOCKETS
