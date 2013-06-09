/////////////////////////////////////////////////////////////////////////////
// Name:        wx/unix/execute.h
// Purpose:     private details of wxExecute() implementation
// Author:      Vadim Zeitlin
// Id:          $Id$
// Copyright:   (c) 1998 Robert Roebling, Julian Smart, Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIX_EXECUTE_H
#define _WX_UNIX_EXECUTE_H

#include "wx/unix/pipe.h"

class wxFDIODispatcher;
class wxFDIOHandler;
class WXDLLIMPEXP_FWD_BASE wxProcess;
class wxStreamTempInputBuffer;
class wxSelectDispatcher;

struct wxEndProcessData
{
    wxEndProcessData()
    {
        pid =
        tag =
        exitcode = -1;
        process = NULL;
        async = false;
        syncEventLoopPtr = NULL;
        childProcessTerminatedFlag = false;
    }

    // For async processes:  Handles the termination and notifies the user.
    //
    // For sync processes:  Checks if stdout/stderr and child process have all terminated before
    // stopping the event loop and informing the application about termination.
    void CheckHandleTermination();

    int pid;                // pid of the process
    int tag;                // port dependent value
    wxProcess *process;     // if !NULL: notified on process termination
    int exitcode;           // the exit code
    bool async;             // if true, delete us on process termination
    wxEventLoopBase *syncEventLoopPtr;  // Used for ending the event loop
                            // for synchronous processing.
    bool childProcessTerminatedFlag;
};

// struct in which information is passed from wxExecute() to wxAppTraits
// methods
struct wxExecuteData
{
    wxExecuteData()
    {
        flags =
        pid = 0;

        process = NULL;

#if wxUSE_STREAMS
        bufOut =
        bufErr = NULL;

        fdOut =
        fdErr = wxPipe::INVALID_FD;
#endif // wxUSE_STREAMS

        endProcDataPtr = &endProcData;

        dispatcher = NULL;
    }

    // get the FD corresponding to the read end of the process end detection
    // pipe and close the write one
    int GetEndProcReadFD()
    {
        const int fd = pipeEndProcDetect.Detach(wxPipe::Read);
        pipeEndProcDetect.Close();
        return fd;
    }


    // wxExecute() flags
    int flags;

    // the pid of the child process
    int pid;

    // the associated process object or NULL
    wxProcess *process;

    // pipe used for end process detection
    wxPipe pipeEndProcDetect;

    // For synchronous execution, this stores the endProcData on the
    // stack.
    wxEndProcessData endProcData;

    // This pointer can be used to reference the endProcData for both
    // sync and async.  For sync, it points to the element inside of
    // this wxExecuteData.  For async, it points to an endProcData that
    // was created on the heap.
    wxEndProcessData *endProcDataPtr;

    // Used for wxEXEC_NOEVENTS case.
    // Set to NULL when wxEXEC_NOEVENTS is not specified.
    wxFDIODispatcher *dispatcher;

#if wxUSE_STREAMS
    // the input buffer bufOut is connected to stdout, this is why it is
    // called bufOut and not bufIn
    wxStreamTempInputBuffer *bufOut,
                            *bufErr;

    // the corresponding FDs, -1 if not redirected
    int fdOut,
        fdErr;
#endif // wxUSE_STREAMS
};

// this function is called when the process terminates from port specific
// callback function and is common to all ports (src/unix/utilsunx.cpp)
extern WXDLLIMPEXP_BASE void wxHandleProcessTermination(wxEndProcessData *proc_data);

// This function is called in order to check if any of our child processes
// have terminated, and if so to call their OnTerminate() function.  It will
// get called as a result of receiving SIGCHLD, it will also get called during
// the event loop for robustness.  Additionally, for synchronous cases of
// wxExecute, it will get called during their wait loops.
extern WXDLLIMPEXP_BASE void wxCheckChildProcessTermination(int sig);

// Call wxFDIOHandler::OnReadWaiting() on the given handler if this FD is still
// being monitored.
extern WXDLLIMPEXP_BASE void wxOnReadWaiting(wxFDIOHandler* handler, int fd);

#endif // _WX_UNIX_EXECUTE_H
