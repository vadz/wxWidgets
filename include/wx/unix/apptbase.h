///////////////////////////////////////////////////////////////////////////////
// Name:        wx/unix/apptbase.h
// Purpose:     declaration of wxAppTraits for Unix systems
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.06.2003
// RCS-ID:      $Id$
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIX_APPTBASE_H_
#define _WX_UNIX_APPTBASE_H_

#include "wx/hashmap.h"

struct wxEndProcessData;
struct wxExecuteData;
class wxFDIOManager;
class wxFDIOHandler;
class wxSelectDispatcher;

// ----------------------------------------------------------------------------
// wxAppTraits: the Unix version adds extra hooks needed by Unix code
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxAppTraits : public wxAppTraitsBase
{
public:
    // wxExecute() support methods
    // ---------------------------

    // wait for the process termination, return whatever wxExecute() must
    // return
    //
    // base class implementation handles all cases except wxEXEC_SYNC without
    // wxEXEC_NOEVENTS one which is implemented at the GUI level
    virtual int WaitForChild(wxExecuteData& execData);

    /** Common code for GUI and console apps to wait for process
        termination for synchronous execution. */
    int WaitForChildSync(wxExecuteData& execData);

    // Integrate the monitoring of the given fd with the port-specific event
    // loop: when this fd is written to, OnReadWaiting() of the provided
    // handler will be called.
    //
    // The lifetime of the handler must be long enough, call
    // RemoveProcessCallback() before destroying it.
    virtual void AddProcessCallback(wxFDIOHandler& handler, int fd);

    // Remove the callback previously installed by AddProcessCallback().
    virtual void RemoveProcessCallback(int fd);

    /*
        Wrapper for calling the platform specific, AddProcessCallback, so that
        there is more control over the handling of callbacks that might still
        be occurring after we don't want them anymore, but the fd is still
        closing.

        Additional parameter dispatcher indicates that this specific
        wxFDIODispatcher will be used instead of the current event loop, so the
        callback needs to be set differently in that case. This is needed for
        the wxEXEC_NOEVENTS case of wxExecute.
     */
    void
    RegisterProcessCallback(wxFDIOHandler& handler,
                            int fd,
                            wxFDIODispatcher *dispatcher = NULL);

    /** Disables the callback and removes the fd from the hash that
        keeps track of which fds have callbacks. */
    void UnRegisterProcessCallback(int fd);

    // Check if we have an handler for the events on the given FD.
    bool HasCallbackForFD(int fd) const
    {
        return m_fdHandlers.find(fd) != m_fdHandlers.end();
    }

#if wxUSE_SOCKETS
    // return a pointer to the object which should be used to integrate
    // monitoring of the file descriptors to the event loop (currently this is
    // used for the sockets only but should be used for arbitrary event loop
    // sources in the future)
    //
    // this object may be different for the console and GUI applications
    //
    // the pointer is not deleted by the caller as normally it points to a
    // static variable
    virtual wxFDIOManager *GetFDIOManager();
#endif // wxUSE_SOCKETS

protected:
    // Information we keep for each FD.
    struct FDHandlerData
    {
        FDHandlerData()
        {
            handler = NULL;
            dispatcher = NULL;
        }

        FDHandlerData(wxFDIOHandler* handler_, wxFDIODispatcher* dispatcher_)
            : handler(handler_), dispatcher(dispatcher_)
        {
        }

        wxFDIOHandler* handler;
        wxFDIODispatcher* dispatcher;
    };

    WX_DECLARE_HASH_MAP(int, FDHandlerData, wxIntegerHash, wxIntegerEqual, FDHandlers);
    FDHandlers m_fdHandlers;
};

#endif // _WX_UNIX_APPTBASE_H_

