/////////////////////////////////////////////////////////////////////////////
// Name:        src/gtk1/utilsgtk.cpp
// Purpose:
// Author:      Robert Roebling
// Id:          $Id$
// Copyright:   (c) 1998 Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#include "wx/utils.h"

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/intl.h"
    #include "wx/log.h"
#endif

#include "wx/apptrait.h"
#include "wx/gtk1/private/timer.h"
#include "wx/evtloop.h"
#include "wx/process.h"

#include "wx/unix/execute.h"

#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>   // for WNOHANG
#include <unistd.h>

#include "glib.h"
#include "gdk/gdk.h"
#include "gtk/gtk.h"
#include "gtk/gtkfeatures.h"
#include "gdk/gdkx.h"

#ifdef HAVE_X11_XKBLIB_H
    /* under HP-UX and Solaris 2.6, at least, XKBlib.h defines structures with
     * field named "explicit" - which is, of course, an error for a C++
     * compiler. To be on the safe side, just redefine it everywhere. */
    #define explicit __wx_explicit

    #include "X11/XKBlib.h"

    #undef explicit
#endif // HAVE_X11_XKBLIB_H

//-----------------------------------------------------------------------------
// data
//-----------------------------------------------------------------------------

extern GtkWidget *wxGetRootWindow();

//----------------------------------------------------------------------------
// misc.
//----------------------------------------------------------------------------

void wxBell()
{
    gdk_beep();
}

/* Don't synthesize KeyUp events holding down a key and producing
   KeyDown events with autorepeat. */
#ifdef HAVE_X11_XKBLIB_H
bool wxSetDetectableAutoRepeat( bool flag )
{
    Bool result;
    XkbSetDetectableAutoRepeat( GDK_DISPLAY(), flag, &result );
    return result;       /* true if keyboard hardware supports this mode */
}
#else
bool wxSetDetectableAutoRepeat( bool WXUNUSED(flag) )
{
    return false;
}
#endif

// ----------------------------------------------------------------------------
// display characterstics
// ----------------------------------------------------------------------------

void *wxGetDisplay()
{
    return GDK_DISPLAY();
}

void wxDisplaySize( int *width, int *height )
{
    if (width) *width = gdk_screen_width();
    if (height) *height = gdk_screen_height();
}

void wxDisplaySizeMM( int *width, int *height )
{
    if (width) *width = gdk_screen_width_mm();
    if (height) *height = gdk_screen_height_mm();
}

void wxGetMousePosition( int* x, int* y )
{
    gdk_window_get_pointer( NULL, x, y, NULL );
}

bool wxColourDisplay()
{
    return true;
}

int wxDisplayDepth()
{
    return gdk_window_get_visual( wxGetRootWindow()->window )->depth;
}

wxWindow* wxFindWindowAtPoint(const wxPoint& pt)
{
    return wxGenericFindWindowAtPoint(pt);
}


// ----------------------------------------------------------------------------
// subprocess routines
// ----------------------------------------------------------------------------

namespace
{

WX_DECLARE_HASH_MAP(int, int, wxIntegerHash, wxIntegerEqual, FDSourceIDs);
FDSourceIDs gs_sourceIDs;

}

extern "C" {
static
void GTK_EndProcessDetector(gpointer data, gint source,
                            GdkInputCondition WXUNUSED(condition) )
{
    wxFDIOHandler* const handler = static_cast<wxFDIOHandler *>(data);

    wxOnReadWaiting(handler, fd);
}

void wxGUIAppTraits::AddProcessCallback(wxFDIOHandler& handler, int fd)
{
    int tag = gdk_input_add(fd,
                            GDK_INPUT_READ,
                            GTK_EndProcessDetector,
                            (gpointer)&handler);

    gs_sourceIDs[fd] = tag;
}

void wxGUIAppTraits::RemoveProcessCallback(int fd)
{
    // child exited, end waiting

    // Don't close the fd because wxProcess will close it in its
    // destructor.  Also we don't want to close it so that we can
    // keep using the output/error streams in the wxProcess.
    // close(fd);

    const FDSourceIDs::iterator it = gs_sourceIDs.find(fd);
    wxCHECK_RET( it != gs_sourceIDs.end(), "No such FD" );

    gdk_input_remove(it->second);
    gs_sourceIDs.erase(it);
}

#if wxUSE_TIMER

wxTimerImpl* wxGUIAppTraits::CreateTimerImpl(wxTimer *timer)
{
    return new wxGTKTimerImpl(timer);
}

#endif // wxUSE_TIMER

// ----------------------------------------------------------------------------
// wxPlatformInfo-related
// ----------------------------------------------------------------------------

wxPortId wxGUIAppTraits::GetToolkitVersion(int *verMaj, int *verMin) const
{
    if ( verMaj )
        *verMaj = gtk_major_version;
    if ( verMin )
        *verMin = gtk_minor_version;

    return wxPORT_GTK;
}

wxEventLoopBase* wxGUIAppTraits::CreateEventLoop()
{
    return new wxEventLoop;
}

