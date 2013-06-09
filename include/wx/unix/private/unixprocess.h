///////////////////////////////////////////////////////////////////////////////
// Name:        wx/private/unixprocess.h
// Purpose:     Extra functionality needed in Unix wxExecute process handling.
// Author:      Rob Bresalier
// Created:     2013-04-27
// RCS-ID:      $Id$
// Copyright:   (c) 2013 Rob Bresalier
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#if wxUSE_STREAMS
    #include "wx/stream.h"
#endif

#include "wx/unix/execute.h"
#include "wx/unix/private/executeiohandler.h"

class WXDLLIMPEXP_BASE wxUnixProcess
{
public:
    wxUnixProcess()
    {
        m_notify = false;
    };

    /** Used to enable notifications of arrival of data on stdout/stderr. */
    void EnableNotifications(bool notify) { m_notify = notify; }

    /** Can be used to determine if the notifications were enabled. */
    bool IsNotifyEnabled() const { return m_notify; }

#ifdef wxUSE_STREAMS
    wxExecuteIOHandler& GetStdoutHandler() { return m_stdoutHandler; }
    wxExecuteIOHandler& GetStderrHandler() { return m_stderrHandler; }
#endif


    /** It is necessary to know the endProcDataPtr for the mechanism
        that determines when to end the temporary event loop for
        synchronous wxExecute. */
    wxEndProcessData *GetEndProcDataPtr() const { return m_endProcDataPtr; }

    /** It is necessary to set the endProcDataPtr for the mechanism
        that determines when to end the temporary event loop for
        synchronous wxExecute. */
    void SetEndProcDataPtr(wxEndProcessData *endProcDataPtr) { m_endProcDataPtr = endProcDataPtr; }

    wxExecuteIOHandler m_stdoutHandler;
    wxExecuteIOHandler m_stderrHandler;

    bool m_notify;

#ifdef __UNIX__
    wxEndProcessData *m_endProcDataPtr;
#endif

};
