///////////////////////////////////////////////////////////////////////////////
// Name:        wx/unix/private/executeiohandler.h
// Purpose:     IO handler class for the FD used by wxExecute() under Unix
// Author:      Rob Bresalier
// Created:     2013-01-06
// RCS-ID:      $Id$
// Copyright:   (c) 2013 Rob Bresalier
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIX_PRIVATE_EXECUTEIOHANDLER_H_
#define _WX_UNIX_PRIVATE_EXECUTEIOHANDLER_H_

#include "wx/private/fdiohandler.h"
#include "wx/process.h"

class wxStreamTempInputBuffer;

/** This class is used to handle callbacks from the OS that indicate data
    is available on stdout or stderr. */
class wxExecuteIOHandler : public wxFDIOHandler
{
public:
    void Init(
        int flags,
        int fd,
        wxProcess *process,
        wxStreamTempInputBuffer *buf,
        bool stderrHandlerFlag)
    {
        m_shutDownCallbackFlag = false;
        m_flags = flags;
        m_fd = fd;
        m_process = process;
        m_buf = buf;
        m_stderrHandlerFlag = stderrHandlerFlag;
    }

    bool IsShutDownFlagSet() const { return m_shutDownCallbackFlag; }

    // Called when the associated descriptor is available for reading.
    virtual void OnReadWaiting();

    // These methods are never called as we only monitor the associated FD for
    // reading, but we still must implement them as they're pure virtual in the
    // base class.
    virtual void OnWriteWaiting() { }
    virtual void OnExceptionWaiting() { }

    /** Disables future callbacks. */
    void DisableCallback();

private:
    /** If true, it means to shut down and de-register the
        callback function. */
    bool m_shutDownCallbackFlag;

    int m_flags;
    int m_fd;
    wxProcess *m_process;
    wxStreamTempInputBuffer *m_buf;

    bool m_stderrHandlerFlag;
};

#endif // _WX_UNIX_PRIVATE_EXECUTEIOHANDLER_H_
