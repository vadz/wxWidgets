/////////////////////////////////////////////////////////////////////////////
// Name:        src/unix/utilsunx.cpp
// Purpose:     generic Unix implementation of many wx functions (for wxBase)
// Author:      Vadim Zeitlin
// Id:          $Id$
// Copyright:   (c) 1998 Robert Roebling, Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#include "wx/utils.h"

#define USE_PUTENV (!defined(HAVE_SETENV) && defined(HAVE_PUTENV))

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/app.h"
    #include "wx/wxcrtvararg.h"
    #if USE_PUTENV
        #include "wx/module.h"
        #include "wx/hashmap.h"
    #endif
#endif

#include "wx/apptrait.h"

#include "wx/process.h"
#include "wx/unix/private/unixprocess.h"
#include "wx/thread.h"

#include "wx/cmdline.h"

#include "wx/wfstream.h"

#include "wx/private/selectdispatcher.h"
#include "wx/private/fdiodispatcher.h"
#include "wx/unix/execute.h"
#include "wx/unix/private.h"

#ifdef wxHAS_GENERIC_PROCESS_CALLBACK
#include "wx/private/fdiodispatcher.h"
#endif

#include "wx/evtloop.h"
#include "wx/mstream.h"

#include <pwd.h>
#include <sys/wait.h>       // waitpid()

#ifdef HAVE_SYS_SELECT_H
#   include <sys/select.h>
#endif

#define HAS_PIPE_STREAMS (wxUSE_STREAMS && wxUSE_FILE)

#if HAS_PIPE_STREAMS

#include "wx/private/pipestream.h"
#include "wx/private/streamtempinput.h"

#endif // HAS_PIPE_STREAMS

// not only the statfs syscall is called differently depending on platform, but
// one of its incarnations, statvfs(), takes different arguments under
// different platforms and even different versions of the same system (Solaris
// 7 and 8): if you want to test for this, don't forget that the problems only
// appear if the large files support is enabled
#ifdef HAVE_STATFS
    #ifdef __BSD__
        #include <sys/param.h>
        #include <sys/mount.h>
    #else // !__BSD__
        #include <sys/vfs.h>
    #endif // __BSD__/!__BSD__

    #define wxStatfs statfs

    #ifndef HAVE_STATFS_DECL
        // some systems lack statfs() prototype in the system headers (AIX 4)
        extern "C" int statfs(const char *path, struct statfs *buf);
    #endif
#endif // HAVE_STATFS

#ifdef HAVE_STATVFS
    #include <sys/statvfs.h>

    #define wxStatfs statvfs
#endif // HAVE_STATVFS

#if defined(HAVE_STATFS) || defined(HAVE_STATVFS)
    // WX_STATFS_T is detected by configure
    #define wxStatfs_t WX_STATFS_T
#endif

// SGI signal.h defines signal handler arguments differently depending on
// whether _LANGUAGE_C_PLUS_PLUS is set or not - do set it
#if defined(__SGI__) && !defined(_LANGUAGE_C_PLUS_PLUS)
    #define _LANGUAGE_C_PLUS_PLUS 1
#endif // SGI hack

#include <stdarg.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>          // for O_WRONLY and friends
#include <time.h>           // nanosleep() and/or usleep()
#include <ctype.h>          // isspace()
#include <sys/time.h>       // needed for FD_SETSIZE

#ifdef HAVE_UNAME
    #include <sys/utsname.h> // for uname()
#endif // HAVE_UNAME

// Used by wxGetFreeMemory().
#ifdef __SGI__
    #include <sys/sysmp.h>
    #include <sys/sysinfo.h>   // for SAGET and MINFO structures
#endif

#ifdef HAVE_SETPRIORITY
    #include <sys/resource.h>   // for setpriority()
#endif

// Keep track of open PIDs
WX_DECLARE_HASH_MAP(int, wxEndProcessData *, wxIntegerHash, wxIntegerEqual, ChildProcessesOpenedHash);
static ChildProcessesOpenedHash childProcessesOpenedHash;

// ----------------------------------------------------------------------------
// conditional compilation
// ----------------------------------------------------------------------------

// many versions of Unices have this function, but it is not defined in system
// headers - please add your system here if it is the case for your OS.
// SunOS < 5.6 (i.e. Solaris < 2.6) and DG-UX are like this.
#if !defined(HAVE_USLEEP) && \
    ((defined(__SUN__) && !defined(__SunOs_5_6) && \
                         !defined(__SunOs_5_7) && !defined(__SUNPRO_CC)) || \
     defined(__osf__) || defined(__EMX__))
    extern "C"
    {
        #ifdef __EMX__
            /* I copied this from the XFree86 diffs. AV. */
            #define INCL_DOSPROCESS
            #include <os2.h>
            inline void usleep(unsigned long delay)
            {
                DosSleep(delay ? (delay/1000l) : 1l);
            }
        #else // Unix
            int usleep(unsigned int usec);
        #endif // __EMX__/Unix
    };

    #define HAVE_USLEEP 1
#endif // Unices without usleep()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// sleeping
// ----------------------------------------------------------------------------

void wxSleep(int nSecs)
{
    sleep(nSecs);
}

void wxMicroSleep(unsigned long microseconds)
{
#if defined(HAVE_NANOSLEEP)
    timespec tmReq;
    tmReq.tv_sec = (time_t)(microseconds / 1000000);
    tmReq.tv_nsec = (microseconds % 1000000) * 1000;

    // we're not interested in remaining time nor in return value
    (void)nanosleep(&tmReq, NULL);
#elif defined(HAVE_USLEEP)
    // uncomment this if you feel brave or if you are sure that your version
    // of Solaris has a safe usleep() function but please notice that usleep()
    // is known to lead to crashes in MT programs in Solaris 2.[67] and is not
    // documented as MT-Safe
    #if defined(__SUN__) && wxUSE_THREADS
        #error "usleep() cannot be used in MT programs under Solaris."
    #endif // Sun

    usleep(microseconds);
#elif defined(HAVE_SLEEP)
    // under BeOS sleep() takes seconds (what about other platforms, if any?)
    sleep(microseconds * 1000000);
#else // !sleep function
    #error "usleep() or nanosleep() function required for wxMicroSleep"
#endif // sleep function
}

void wxMilliSleep(unsigned long milliseconds)
{
    wxMicroSleep(milliseconds*1000);
}

// ----------------------------------------------------------------------------
// process management
// ----------------------------------------------------------------------------

int wxKill(long pid, wxSignal sig, wxKillError *rc, int flags)
{
    int err = kill((pid_t) (flags & wxKILL_CHILDREN) ? -pid : pid, (int)sig);
    if ( rc )
    {
        switch ( err ? errno : 0 )
        {
            case 0:
                *rc = wxKILL_OK;
                break;

            case EINVAL:
                *rc = wxKILL_BAD_SIGNAL;
                break;

            case EPERM:
                *rc = wxKILL_ACCESS_DENIED;
                break;

            case ESRCH:
                *rc = wxKILL_NO_PROCESS;
                break;

            default:
                // this goes against Unix98 docs so log it
                wxLogDebug(wxT("unexpected kill(2) return value %d"), err);

                // something else...
                *rc = wxKILL_ERROR;
        }
    }

    return err;
}

// Shutdown or reboot the PC
bool wxShutdown(int flags)
{
    flags &= ~wxSHUTDOWN_FORCE;

    wxChar level;
    switch ( flags )
    {
        case wxSHUTDOWN_POWEROFF:
            level = wxT('0');
            break;

        case wxSHUTDOWN_REBOOT:
            level = wxT('6');
            break;

        case wxSHUTDOWN_LOGOFF:
            // TODO: use dcop to log off?
            return false;

        default:
            wxFAIL_MSG( wxT("unknown wxShutdown() flag") );
            return false;
    }

    return system(wxString::Format("init %c", level).mb_str()) == 0;
}

// ----------------------------------------------------------------------------
// wxStream classes to support IO redirection in wxExecute
// ----------------------------------------------------------------------------

#if HAS_PIPE_STREAMS

bool wxPipeInputStream::CanRead() const
{
    if ( m_lasterror == wxSTREAM_EOF )
        return false;

    // check if there is any input available
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    const int fd = m_file->fd();

    fd_set readfds;

    wxFD_ZERO(&readfds);
    wxFD_SET(fd, &readfds);

    switch ( select(fd + 1, &readfds, NULL, NULL, &tv) )
    {
        case -1:
            wxLogSysError(_("Impossible to get child process input"));
            // fall through

        case 0:
            return false;

        default:
            wxFAIL_MSG(wxT("unexpected select() return value"));
            // still fall through

        case 1:
            // input available -- or maybe not, as select() returns 1 when a
            // read() will complete without delay, but it could still not read
            // anything
            return !Eof();
    }
}

size_t wxPipeOutputStream::OnSysWrite(const void *buffer, size_t size)
{
    // We need to suppress error logging here, because on writing to a pipe
    // which is full, wxFile::Write reports a system error. However, this is
    // not an extraordinary situation, and it should not be reported to the
    // user (but if really needed, the program can recognize it by checking
    // whether LastRead() == 0.) Other errors will be reported below.
    size_t ret;
    {
        wxLogNull logNo;
        ret = m_file->Write(buffer, size);
    }

    switch ( m_file->GetLastError() )
    {
       // pipe is full
#ifdef EAGAIN
       case EAGAIN:
#endif
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
       case EWOULDBLOCK:
#endif
           // do not treat it as an error
           m_file->ClearLastError();
           // fall through

       // no error
       case 0:
           break;

       // some real error
       default:
           wxLogSysError(_("Can't write to child process's stdin"));
           m_lasterror = wxSTREAM_WRITE_ERROR;
    }

    return ret;
}

#endif // HAS_PIPE_STREAMS

// ----------------------------------------------------------------------------
// wxShell
// ----------------------------------------------------------------------------

static wxString wxMakeShellCommand(const wxString& command)
{
    wxString cmd;
    if ( !command )
    {
        // just an interactive shell
        cmd = wxT("xterm");
    }
    else
    {
        // execute command in a shell
        cmd << wxT("/bin/sh -c '") << command << wxT('\'');
    }

    return cmd;
}

bool wxShell(const wxString& command)
{
    return wxExecute(wxMakeShellCommand(command), wxEXEC_SYNC) == 0;
}

bool wxShell(const wxString& command, wxArrayString& output)
{
    wxCHECK_MSG( !command.empty(), false, wxT("can't exec shell non interactively") );

    return wxExecute(wxMakeShellCommand(command), output);
}

namespace
{

// helper class for storing arguments as char** array suitable for passing to
// execvp(), whatever form they were passed to us
class ArgsArray
{
public:
    ArgsArray(const wxArrayString& args)
    {
        Init(args.size());

        for ( int i = 0; i < m_argc; i++ )
        {
            m_argv[i] = wxStrdup(args[i]);
        }
    }

#if wxUSE_UNICODE
    ArgsArray(wchar_t **wargv)
    {
        int argc = 0;
        while ( wargv[argc] )
            argc++;

        Init(argc);

        for ( int i = 0; i < m_argc; i++ )
        {
            m_argv[i] = wxSafeConvertWX2MB(wargv[i]).release();
        }
    }
#endif // wxUSE_UNICODE

    ~ArgsArray()
    {
        for ( int i = 0; i < m_argc; i++ )
        {
            free(m_argv[i]);
        }

        delete [] m_argv;
    }

    operator char**() const { return m_argv; }

private:
    void Init(int argc)
    {
        m_argc = argc;
        m_argv = new char *[m_argc + 1];
        m_argv[m_argc] = NULL;
    }

    int m_argc;
    char **m_argv;

    wxDECLARE_NO_COPY_CLASS(ArgsArray);
};

} // anonymous namespace

// ----------------------------------------------------------------------------
// wxExecute implementations
// ----------------------------------------------------------------------------
void wxUpdateListOfOpenChildProcesses(wxExecuteData& execData)
{
    wxEndProcessData *endProcData;

    endProcData = &(execData.endProcData);

    if ( !(execData.flags & wxEXEC_SYNC) )
    {   // For asynchronous execution, we need a pointer on the heap otherwise
        // the pointer that we are passed is on the stack of wxExecute().
        // However for sync execution we need endProcData to be on the stack
        // so that we can retrieve the exit code.
        endProcData = new wxEndProcessData;

        // Save flag that this is an async process so that the wxEndProcessData
        // data will get deleted in wxHandleProcessTermination()
        endProcData->async = true;
    }

    execData.endProcDataPtr = endProcData;

    endProcData->process = execData.process;
    endProcData->pid = execData.pid;

    // Add the pid of this process to a list of pids that
    // we are keeping track of, so that when we get the SIGCHLD
    // signal, we can go through our list of open PIDS to check
    // each one for termination.
    childProcessesOpenedHash[endProcData->pid] = endProcData;
}

#if defined(__DARWIN__)
bool wxMacLaunch(char **argv);
#endif

long wxExecute(const wxString& command, int flags, wxProcess *process,
        const wxExecuteEnv *env)
{
    ArgsArray argv(wxCmdLineParser::ConvertStringToArgs(command,
                                                        wxCMD_LINE_SPLIT_UNIX));

    return wxExecute(argv, flags, process, env);
}

#if wxUSE_UNICODE

long wxExecute(wchar_t **wargv, int flags, wxProcess *process,
        const wxExecuteEnv *env)
{
    ArgsArray argv(wargv);

    return wxExecute(argv, flags, process, env);
}

#endif // wxUSE_UNICODE

// wxExecute: the real worker function
long wxExecute(char **argv, int flags, wxProcess *process,
        const wxExecuteEnv *env)
{
    static bool wxExecuteExecutedAlready = false;

    // for the sync execution, we return -1 to indicate failure, but for async
    // case we return 0 which is never a valid PID
    //
    // we define this as a macro, not a variable, to avoid compiler warnings
    // about "ERROR_RETURN_CODE value may be clobbered by fork()"
    #define ERROR_RETURN_CODE ((flags & wxEXEC_SYNC) ? -1 : 0)

    wxCHECK_MSG( *argv, ERROR_RETURN_CODE, wxT("can't exec empty command") );

#if wxUSE_THREADS
    // fork() doesn't mix well with POSIX threads: on many systems the program
    // deadlocks or crashes for some reason. Probably our code is buggy and
    // doesn't do something which must be done to allow this to work, but I
    // don't know what yet, so for now just warn the user (this is the least we
    // can do) about it
    wxASSERT_MSG( wxThread::IsMain(),
                    wxT("wxExecute() can be called only from the main thread") );
#endif // wxUSE_THREADS

#if defined(__WXCOCOA__) || ( defined(__WXOSX_MAC__) && wxOSX_USE_COCOA_OR_CARBON )
    // wxMacLaunch() only executes app bundles and only does it asynchronously.
    // It returns false if the target is not an app bundle, thus falling
    // through to the regular code for non app bundles.
    if ( !(flags & wxEXEC_SYNC) && wxMacLaunch(argv) )
    {
        // we don't have any PID to return so just make up something non null
        return -1;
    }
#endif // __DARWIN__

    if ( !wxExecuteExecutedAlready )
    {
        if ( wxTheApp != NULL )
        {
            // Setup the signal handler for SIGCHLD, as this is the
            // mechanism that is used to detect that the child process
            // has terminated.
            wxTheApp->SetSignalHandler(SIGCHLD, wxCheckChildProcessTermination);

            wxExecuteExecutedAlready = true;
        }
        else
        {
            wxFAIL_MSG("wxTheApp is not initialized, therefore we can't wait for child termination!");
        }
    }

    // this struct contains all information which we use for housekeeping
    wxExecuteData execData;
    wxStreamTempInputBuffer bufOut, bufErr;

    execData.flags = flags;
    execData.process = process;

    // Used for wxEXEC_NOEVENTS case.
    wxSelectDispatcher dispatcher;

    if ( flags & wxEXEC_NOEVENTS )
    {
        execData.dispatcher = &dispatcher;
    }

    // create pipes
    if ( !execData.pipeEndProcDetect.Create() )
    {
        wxLogError( _("Failed to execute '%s'\n"), *argv );

        return ERROR_RETURN_CODE;
    }

    // pipes for inter process communication
    wxPipe pipeIn,      // stdin
           pipeOut,     // stdout
           pipeErr;     // stderr

    if ( process && process->IsRedirected() )
    {
        if ( !pipeIn.Create() || !pipeOut.Create() || !pipeErr.Create() )
        {
            wxLogError( _("Failed to execute '%s'\n"), *argv );

            return ERROR_RETURN_CODE;
        }
    }

    // priority: we need to map wxWidgets priority which is in the range 0..100
    // to Unix nice value which is in the range -20..19. As there is an odd
    // number of elements in our range and an even number in the Unix one, we
    // have to do it in this rather ugly way to guarantee that:
    //  1. wxPRIORITY_{MIN,DEFAULT,MAX} map to -20, 0 and 19 respectively.
    //  2. The mapping is monotonously increasing.
    //  3. The mapping is onto the target range.
    int prio = process ? process->GetPriority() : 0;
    if ( prio <= 50 )
        prio = (2*prio)/5 - 20;
    else if ( prio < 55 )
        prio = 1;
    else
        prio = (2*prio)/5 - 21;

    // fork the process
    //
    // NB: do *not* use vfork() here, it completely breaks this code for some
    //     reason under Solaris (and maybe others, although not under Linux)
    //     But on OpenVMS we do not have fork so we have to use vfork and
    //     cross our fingers that it works.
#ifdef __VMS
   pid_t pid = vfork();
#else
   pid_t pid = fork();
#endif
   if ( pid == -1 )     // error?
    {
        wxLogSysError( _("Fork failed") );

        return ERROR_RETURN_CODE;
    }
    else if ( pid == 0 )  // we're in child
    {
        // NB: we used to close all the unused descriptors of the child here
        //     but this broke some programs which relied on e.g. FD 1 being
        //     always opened so don't do it any more, after all there doesn't
        //     seem to be any real problem with keeping them opened

#if !defined(__VMS) && !defined(__EMX__)
        if ( flags & wxEXEC_MAKE_GROUP_LEADER )
        {
            // Set process group to child process' pid.  Then killing -pid
            // of the parent will kill the process and all of its children.
            setsid();
        }
#endif // !__VMS

#if defined(HAVE_SETPRIORITY)
        if ( prio && setpriority(PRIO_PROCESS, 0, prio) != 0 )
        {
            wxLogSysError(_("Failed to set process priority"));
        }
#endif // HAVE_SETPRIORITY

        // redirect stdin, stdout and stderr
        if ( pipeIn.IsOk() )
        {
            if ( dup2(pipeIn[wxPipe::Read], STDIN_FILENO) == -1 ||
                 dup2(pipeOut[wxPipe::Write], STDOUT_FILENO) == -1 ||
                 dup2(pipeErr[wxPipe::Write], STDERR_FILENO) == -1 )
            {
                wxLogSysError(_("Failed to redirect child process input/output"));
            }

            pipeIn.Close();
            pipeOut.Close();
            pipeErr.Close();
        }

        // Close all (presumably accidentally) inherited file descriptors to
        // avoid descriptor leaks. This means that we don't allow inheriting
        // them purposefully but this seems like a lesser evil in wx code.
        // Ideally we'd provide some flag to indicate that none (or some?) of
        // the descriptors do not need to be closed but for now this is better
        // than never closing them at all as wx code never used FD_CLOEXEC.

        // Note that while the reading side of the end process detection pipe
        // can be safely closed, we should keep the write one opened, it will
        // be only closed when the process terminates resulting in a read
        // notification to the parent
        const int fdEndProc = execData.pipeEndProcDetect.Detach(wxPipe::Write);
        execData.pipeEndProcDetect.Close();

        // TODO: Iterating up to FD_SETSIZE is both inefficient (because it may
        //       be quite big) and incorrect (because in principle we could
        //       have more opened descriptions than this number). Unfortunately
        //       there is no good portable solution for closing all descriptors
        //       above a certain threshold but non-portable solutions exist for
        //       most platforms, see [http://stackoverflow.com/questions/899038/
        //          getting-the-highest-allocated-file-descriptor]
        for ( int fd = 0; fd < (int)FD_SETSIZE; ++fd )
        {
            if ( fd != STDIN_FILENO  &&
                 fd != STDOUT_FILENO &&
                 fd != STDERR_FILENO &&
                 fd != fdEndProc )
            {
                close(fd);
            }
        }


        // Process additional options if we have any
        if ( env )
        {
            // Change working directory if it is specified
            if ( !env->cwd.empty() )
                wxSetWorkingDirectory(env->cwd);

            // Change environment if needed.
            //
            // NB: We can't use execve() currently because we allow using
            //     non full paths to wxExecute(), i.e. we want to search for
            //     the program in PATH. However it just might be simpler/better
            //     to do the search manually and use execve() envp parameter to
            //     set up the environment of the child process explicitly
            //     instead of doing what we do below.
            if ( !env->env.empty() )
            {
                wxEnvVariableHashMap oldenv;
                wxGetEnvMap(&oldenv);

                // Remove unwanted variables
                wxEnvVariableHashMap::const_iterator it;
                for ( it = oldenv.begin(); it != oldenv.end(); ++it )
                {
                    if ( env->env.find(it->first) == env->env.end() )
                        wxUnsetEnv(it->first);
                }

                // And add the new ones (possibly replacing the old values)
                for ( it = env->env.begin(); it != env->env.end(); ++it )
                    wxSetEnv(it->first, it->second);
            }
        }

        execvp(*argv, argv);

        fprintf(stderr, "execvp(");
        for ( char **a = argv; *a; a++ )
            fprintf(stderr, "%s%s", a == argv ? "" : ", ", *a);
        fprintf(stderr, ") failed with error %d!\n", errno);

        // there is no return after successful exec()
        _exit(-1);

        // some compilers complain about missing return - of course, they
        // should know that exit() doesn't return but what else can we do if
        // they don't?
        //
        // and, sure enough, other compilers complain about unreachable code
        // after exit() call, so we can just always have return here...
#if defined(__VMS) || defined(__INTEL_COMPILER)
        return 0;
#endif
    }
    else // we're in parent
    {
        // save it for WaitForChild() use
        execData.pid = pid;

        // We update the information about open child processes right away
        // to avoid a race condition.
        // The SIGCHLD handler will interrupt the code, and then will trigger
        // idle processing where we will call wxCheckChildProcessTermination()
        // We don't want the call to wxCheckChildProcessTermination() to occur
        // before we've added the new opened PID.  wxCheckChildProcessTermination()
        // will get called during an event loop.
        // By adding this information as soon as we
        // know what the PID is, then we can be sure that this information
        // is properly stored before we go back into any event loop where
        // the signal handler could get processed.
        wxUpdateListOfOpenChildProcesses(execData);

        if (execData.process)
        {
            execData.process->SetPid(pid);  // and also in the wxProcess

            // Save endProcDataPtr so we can refer to it when we want to
            // check to terminate a synchronous event loop.
            process->GetUnixProcess()->SetEndProcDataPtr(execData.endProcDataPtr);
        }


        // prepare for IO redirection

#if HAS_PIPE_STREAMS

        if ( process && process->IsRedirected() )
        {
            if ( execData.flags & wxEXEC_SYNC )
            {
                // For synchronous wxExecute, we need to enable notification so
                // that we can receive the stdout/stderr callbacks during the
                // temporary event loop.
                process->GetUnixProcess()->EnableNotifications(true);
            }

            // Avoid deadlocks which could result from trying to write to the
            // child input pipe end while the child itself is writing to its
            // output end and waiting for us to read from it.
            if ( !pipeIn.MakeNonBlocking(wxPipe::Write) )
            {
                // This message is not terrible useful for the user but what
                // else can we do? Also, should we fail here or take the risk
                // to continue and deadlock? Currently we choose the latter but
                // it might not be the best idea.
                wxLogSysError(_("Failed to set up non-blocking pipe, "
                                "the program might hang."));
#if wxUSE_LOG
                wxLog::FlushActive();
#endif
            }

            wxOutputStream *inStream =
                new wxPipeOutputStream(pipeIn.Detach(wxPipe::Write));

            const int fdOut = pipeOut.Detach(wxPipe::Read);
            wxPipeInputStream *outStream = new wxPipeInputStream(fdOut);

            const int fdErr = pipeErr.Detach(wxPipe::Read);
            wxPipeInputStream *errStream = new wxPipeInputStream(fdErr);

            process->SetPipeStreams(outStream, inStream, errStream);

            execData.bufOut = &bufOut;
            execData.bufErr = &bufErr;

            execData.bufOut->Init(outStream);
            execData.bufErr->Init(errStream);

            execData.fdOut = fdOut;
            execData.fdErr = fdErr;
        }
#endif // HAS_PIPE_STREAMS

        if ( pipeIn.IsOk() )
        {
            pipeIn.Close();
            pipeOut.Close();
            pipeErr.Close();
        }

        // we want this function to work even if there is no wxApp so ensure
        // that we have a valid traits pointer
        wxConsoleAppTraits traitsConsole;
        wxAppTraits *traits = wxTheApp ? wxTheApp->GetTraits() : NULL;
        if ( !traits )
            traits = &traitsConsole;

#if wxUSE_STREAMS
        if ( process && process->IsRedirected() && process->GetUnixProcess()->IsNotifyEnabled() )
        {
            // If enabled, register and activate the callbacks for output/error
            wxExecuteIOHandler& handlerStdout = process->GetUnixProcess()->GetStdoutHandler();
            handlerStdout.Init(
                execData.flags,   // int flags
                execData.fdOut,   // int fd
                process,          // wxProcess *process
                execData.bufOut,  // wxStreamTempInputBuffer *buf
                false             // bool stderrHandlerFlag
            );

            wxExecuteIOHandler& handlerStderr = process->GetUnixProcess()->GetStderrHandler();
            handlerStderr.Init(
                execData.flags,   // int flags
                execData.fdErr,   // int fd
                process,          // wxProcess *process
                execData.bufErr,  // wxStreamTempInputBuffer *buf
                true              // bool stderrHandlerFlag
            );

            // Register the callback for stdout.
            traits->RegisterProcessCallback(handlerStdout, execData.fdOut, execData.dispatcher);

            // Register the callback for stderr.
            traits->RegisterProcessCallback(handlerStderr, execData.fdErr, execData.dispatcher);
        }
#endif // wxUSE_STREAMS

        return traits->WaitForChild(execData);
    }

#if !defined(__VMS) && !defined(__INTEL_COMPILER)
    return ERROR_RETURN_CODE;
#endif
}

#undef ERROR_RETURN_CODE

// ----------------------------------------------------------------------------
// file and directory functions
// ----------------------------------------------------------------------------

const wxChar* wxGetHomeDir( wxString *home  )
{
    *home = wxGetUserHome();
    wxString tmp;
    if ( home->empty() )
        *home = wxT("/");
#ifdef __VMS
    tmp = *home;
    if ( tmp.Last() != wxT(']'))
        if ( tmp.Last() != wxT('/')) *home << wxT('/');
#endif
    return home->c_str();
}

wxString wxGetUserHome( const wxString &user )
{
    struct passwd *who = (struct passwd *) NULL;

    if ( !user )
    {
        wxChar *ptr;

        if ((ptr = wxGetenv(wxT("HOME"))) != NULL)
        {
            return ptr;
        }

        if ((ptr = wxGetenv(wxT("USER"))) != NULL ||
             (ptr = wxGetenv(wxT("LOGNAME"))) != NULL)
        {
            who = getpwnam(wxSafeConvertWX2MB(ptr));
        }

        // make sure the user exists!
        if ( !who )
        {
            who = getpwuid(getuid());
        }
    }
    else
    {
      who = getpwnam (user.mb_str());
    }

    return wxSafeConvertMB2WX(who ? who->pw_dir : 0);
}

// ----------------------------------------------------------------------------
// network and user id routines
// ----------------------------------------------------------------------------

// private utility function which returns output of the given command, removing
// the trailing newline
static wxString wxGetCommandOutput(const wxString &cmd)
{
    // Suppress stderr from the shell to avoid outputting errors if the command
    // doesn't exist.
    FILE *f = popen((cmd + " 2>/dev/null").ToAscii(), "r");
    if ( !f )
    {
        // Notice that this doesn't happen simply if the command doesn't exist,
        // but only in case of some really catastrophic failure inside popen()
        // so we should really notify the user about this as this is not normal.
        wxLogSysError(wxT("Executing \"%s\" failed"), cmd);
        return wxString();
    }

    wxString s;
    char buf[256];
    while ( !feof(f) )
    {
        if ( !fgets(buf, sizeof(buf), f) )
            break;

        s += wxString::FromAscii(buf);
    }

    pclose(f);

    if ( !s.empty() && s.Last() == wxT('\n') )
        s.RemoveLast();

    return s;
}

// retrieve either the hostname or FQDN depending on platform (caller must
// check whether it's one or the other, this is why this function is for
// private use only)
static bool wxGetHostNameInternal(wxChar *buf, int sz)
{
    wxCHECK_MSG( buf, false, wxT("NULL pointer in wxGetHostNameInternal") );

    *buf = wxT('\0');

    // we're using uname() which is POSIX instead of less standard sysinfo()
#if defined(HAVE_UNAME)
    struct utsname uts;
    bool ok = uname(&uts) != -1;
    if ( ok )
    {
        wxStrlcpy(buf, wxSafeConvertMB2WX(uts.nodename), sz);
    }
#elif defined(HAVE_GETHOSTNAME)
    char cbuf[sz];
    bool ok = gethostname(cbuf, sz) != -1;
    if ( ok )
    {
        wxStrlcpy(buf, wxSafeConvertMB2WX(cbuf), sz);
    }
#else // no uname, no gethostname
    wxFAIL_MSG(wxT("don't know host name for this machine"));

    bool ok = false;
#endif // uname/gethostname

    if ( !ok )
    {
        wxLogSysError(_("Cannot get the hostname"));
    }

    return ok;
}

bool wxGetHostName(wxChar *buf, int sz)
{
    bool ok = wxGetHostNameInternal(buf, sz);

    if ( ok )
    {
        // BSD systems return the FQDN, we only want the hostname, so extract
        // it (we consider that dots are domain separators)
        wxChar *dot = wxStrchr(buf, wxT('.'));
        if ( dot )
        {
            // nuke it
            *dot = wxT('\0');
        }
    }

    return ok;
}

bool wxGetFullHostName(wxChar *buf, int sz)
{
    bool ok = wxGetHostNameInternal(buf, sz);

    if ( ok )
    {
        if ( !wxStrchr(buf, wxT('.')) )
        {
            struct hostent *host = gethostbyname(wxSafeConvertWX2MB(buf));
            if ( !host )
            {
                wxLogSysError(_("Cannot get the official hostname"));

                ok = false;
            }
            else
            {
                // the canonical name
                wxStrlcpy(buf, wxSafeConvertMB2WX(host->h_name), sz);
            }
        }
        //else: it's already a FQDN (BSD behaves this way)
    }

    return ok;
}

bool wxGetUserId(wxChar *buf, int sz)
{
    struct passwd *who;

    *buf = wxT('\0');
    if ((who = getpwuid(getuid ())) != NULL)
    {
        wxStrlcpy (buf, wxSafeConvertMB2WX(who->pw_name), sz);
        return true;
    }

    return false;
}

bool wxGetUserName(wxChar *buf, int sz)
{
#ifdef HAVE_PW_GECOS
    struct passwd *who;

    *buf = wxT('\0');
    if ((who = getpwuid (getuid ())) != NULL)
    {
       char *comma = strchr(who->pw_gecos, ',');
       if (comma)
           *comma = '\0'; // cut off non-name comment fields
       wxStrlcpy(buf, wxSafeConvertMB2WX(who->pw_gecos), sz);
       return true;
    }

    return false;
#else // !HAVE_PW_GECOS
    return wxGetUserId(buf, sz);
#endif // HAVE_PW_GECOS/!HAVE_PW_GECOS
}

bool wxIsPlatform64Bit()
{
    const wxString machine = wxGetCommandOutput(wxT("uname -m"));

    // the test for "64" is obviously not 100% reliable but seems to work fine
    // in practice
    return machine.Contains(wxT("64")) ||
                machine.Contains(wxT("alpha"));
}

#ifdef __LINUX__
wxLinuxDistributionInfo wxGetLinuxDistributionInfo()
{
    const wxString id = wxGetCommandOutput(wxT("lsb_release --id"));
    const wxString desc = wxGetCommandOutput(wxT("lsb_release --description"));
    const wxString rel = wxGetCommandOutput(wxT("lsb_release --release"));
    const wxString codename = wxGetCommandOutput(wxT("lsb_release --codename"));

    wxLinuxDistributionInfo ret;

    id.StartsWith("Distributor ID:\t", &ret.Id);
    desc.StartsWith("Description:\t", &ret.Description);
    rel.StartsWith("Release:\t", &ret.Release);
    codename.StartsWith("Codename:\t", &ret.CodeName);

    return ret;
}
#endif

// these functions are in src/osx/utilsexc_base.cpp for wxMac
#ifndef __DARWIN__

wxOperatingSystemId wxGetOsVersion(int *verMaj, int *verMin)
{
    // get OS version
    int major, minor;
    wxString release = wxGetCommandOutput(wxT("uname -r"));
    if ( release.empty() ||
         wxSscanf(release.c_str(), wxT("%d.%d"), &major, &minor) != 2 )
    {
        // failed to get version string or unrecognized format
        major =
        minor = -1;
    }

    if ( verMaj )
        *verMaj = major;
    if ( verMin )
        *verMin = minor;

    // try to understand which OS are we running
    wxString kernel = wxGetCommandOutput(wxT("uname -s"));
    if ( kernel.empty() )
        kernel = wxGetCommandOutput(wxT("uname -o"));

    if ( kernel.empty() )
        return wxOS_UNKNOWN;

    return wxPlatformInfo::GetOperatingSystemId(kernel);
}

wxString wxGetOsDescription()
{
    return wxGetCommandOutput(wxT("uname -s -r -m"));
}

#endif // !__DARWIN__

unsigned long wxGetProcessId()
{
    return (unsigned long)getpid();
}

wxMemorySize wxGetFreeMemory()
{
#if defined(__LINUX__)
    // get it from /proc/meminfo
    FILE *fp = fopen("/proc/meminfo", "r");
    if ( fp )
    {
        long memFree = -1;

        char buf[1024];
        if ( fgets(buf, WXSIZEOF(buf), fp) && fgets(buf, WXSIZEOF(buf), fp) )
        {
            // /proc/meminfo changed its format in kernel 2.6
            if ( wxPlatformInfo().CheckOSVersion(2, 6) )
            {
                unsigned long cached, buffers;
                sscanf(buf, "MemFree: %ld", &memFree);

                fgets(buf, WXSIZEOF(buf), fp);
                sscanf(buf, "Buffers: %lu", &buffers);

                fgets(buf, WXSIZEOF(buf), fp);
                sscanf(buf, "Cached: %lu", &cached);

                // add to "MemFree" also the "Buffers" and "Cached" values as
                // free(1) does as otherwise the value never makes sense: for
                // kernel 2.6 it's always almost 0
                memFree += buffers + cached;

                // values here are always expressed in kB and we want bytes
                memFree *= 1024;
            }
            else // Linux 2.4 (or < 2.6, anyhow)
            {
                long memTotal, memUsed;
                sscanf(buf, "Mem: %ld %ld %ld", &memTotal, &memUsed, &memFree);
            }
        }

        fclose(fp);

        return (wxMemorySize)memFree;
    }
#elif defined(__SGI__)
    struct rminfo realmem;
    if ( sysmp(MP_SAGET, MPSA_RMINFO, &realmem, sizeof realmem) == 0 )
        return ((wxMemorySize)realmem.physmem * sysconf(_SC_PAGESIZE));
#elif defined(_SC_AVPHYS_PAGES)
    return ((wxMemorySize)sysconf(_SC_AVPHYS_PAGES))*sysconf(_SC_PAGESIZE);
//#elif defined(__FREEBSD__) -- might use sysctl() to find it out, probably
#endif

    // can't find it out
    return -1;
}

bool wxGetDiskSpace(const wxString& path, wxDiskspaceSize_t *pTotal, wxDiskspaceSize_t *pFree)
{
#if defined(HAVE_STATFS) || defined(HAVE_STATVFS)
    // the case to "char *" is needed for AIX 4.3
    wxStatfs_t fs;
    if ( wxStatfs((char *)(const char*)path.fn_str(), &fs) != 0 )
    {
        wxLogSysError( wxT("Failed to get file system statistics") );

        return false;
    }

    // under Solaris we also have to use f_frsize field instead of f_bsize
    // which is in general a multiple of f_frsize
#ifdef HAVE_STATVFS
    wxDiskspaceSize_t blockSize = fs.f_frsize;
#else // HAVE_STATFS
    wxDiskspaceSize_t blockSize = fs.f_bsize;
#endif // HAVE_STATVFS/HAVE_STATFS

    if ( pTotal )
    {
        *pTotal = wxDiskspaceSize_t(fs.f_blocks) * blockSize;
    }

    if ( pFree )
    {
        *pFree = wxDiskspaceSize_t(fs.f_bavail) * blockSize;
    }

    return true;
#else // !HAVE_STATFS && !HAVE_STATVFS
    return false;
#endif // HAVE_STATFS
}

// ----------------------------------------------------------------------------
// env vars
// ----------------------------------------------------------------------------

#if USE_PUTENV

WX_DECLARE_STRING_HASH_MAP(char *, wxEnvVars);

static wxEnvVars gs_envVars;

class wxSetEnvModule : public wxModule
{
public:
    virtual bool OnInit() { return true; }
    virtual void OnExit()
    {
        for ( wxEnvVars::const_iterator i = gs_envVars.begin();
              i != gs_envVars.end();
              ++i )
        {
            free(i->second);
        }

        gs_envVars.clear();
    }

    DECLARE_DYNAMIC_CLASS(wxSetEnvModule)
};

IMPLEMENT_DYNAMIC_CLASS(wxSetEnvModule, wxModule)

#endif // USE_PUTENV

bool wxGetEnv(const wxString& var, wxString *value)
{
    // wxGetenv is defined as getenv()
    char *p = wxGetenv(var);
    if ( !p )
        return false;

    if ( value )
    {
        *value = p;
    }

    return true;
}

static bool wxDoSetEnv(const wxString& variable, const char *value)
{
#if defined(HAVE_SETENV)
    if ( !value )
    {
#ifdef HAVE_UNSETENV
        // don't test unsetenv() return value: it's void on some systems (at
        // least Darwin)
        unsetenv(variable.mb_str());
        return true;
#else
        value = ""; // we can't pass NULL to setenv()
#endif
    }

    return setenv(variable.mb_str(), value, 1 /* overwrite */) == 0;
#elif defined(HAVE_PUTENV)
    wxString s = variable;
    if ( value )
        s << wxT('=') << value;

    // transform to ANSI
    const wxWX2MBbuf p = s.mb_str();

    char *buf = (char *)malloc(strlen(p) + 1);
    strcpy(buf, p);

    // store the string to free() it later
    wxEnvVars::iterator i = gs_envVars.find(variable);
    if ( i != gs_envVars.end() )
    {
        free(i->second);
        i->second = buf;
    }
    else // this variable hadn't been set before
    {
        gs_envVars[variable] = buf;
    }

    return putenv(buf) == 0;
#else // no way to set an env var
    return false;
#endif
}

bool wxSetEnv(const wxString& variable, const wxString& value)
{
    return wxDoSetEnv(variable, value.mb_str());
}

bool wxUnsetEnv(const wxString& variable)
{
    return wxDoSetEnv(variable, NULL);
}

// ----------------------------------------------------------------------------
// signal handling
// ----------------------------------------------------------------------------

#if wxUSE_ON_FATAL_EXCEPTION

#include <signal.h>

extern "C" void wxFatalSignalHandler(wxTYPE_SA_HANDLER)
{
    if ( wxTheApp )
    {
        // give the user a chance to do something special about this
        wxTheApp->OnFatalException();
    }

    abort();
}

bool wxHandleFatalExceptions(bool doit)
{
    // old sig handlers
    static bool s_savedHandlers = false;
    static struct sigaction s_handlerFPE,
                            s_handlerILL,
                            s_handlerBUS,
                            s_handlerSEGV;

    bool ok = true;
    if ( doit && !s_savedHandlers )
    {
        // install the signal handler
        struct sigaction act;

        // some systems extend it with non std fields, so zero everything
        memset(&act, 0, sizeof(act));

        act.sa_handler = wxFatalSignalHandler;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;

        ok &= sigaction(SIGFPE, &act, &s_handlerFPE) == 0;
        ok &= sigaction(SIGILL, &act, &s_handlerILL) == 0;
        ok &= sigaction(SIGBUS, &act, &s_handlerBUS) == 0;
        ok &= sigaction(SIGSEGV, &act, &s_handlerSEGV) == 0;
        if ( !ok )
        {
            wxLogDebug(wxT("Failed to install our signal handler."));
        }

        s_savedHandlers = true;
    }
    else if ( s_savedHandlers )
    {
        // uninstall the signal handler
        ok &= sigaction(SIGFPE, &s_handlerFPE, NULL) == 0;
        ok &= sigaction(SIGILL, &s_handlerILL, NULL) == 0;
        ok &= sigaction(SIGBUS, &s_handlerBUS, NULL) == 0;
        ok &= sigaction(SIGSEGV, &s_handlerSEGV, NULL) == 0;
        if ( !ok )
        {
            wxLogDebug(wxT("Failed to uninstall our signal handler."));
        }

        s_savedHandlers = false;
    }
    //else: nothing to do

    return ok;
}

#endif // wxUSE_ON_FATAL_EXCEPTION

// ----------------------------------------------------------------------------
// wxExecute support
// ----------------------------------------------------------------------------

// This version of AddProcessCallback is only for console applications.
// For GUI applications, AddProcessCallback is specific to the platform, so
// there are specialized versions for both OSX and GTK.
void wxAppTraits::AddProcessCallback(wxFDIOHandler& data, int fd)
{
    wxFDIODispatcher::Get()->RegisterFD
                             (
                                 fd,
                                 &data,
                                 wxFDIO_INPUT
                             );
}

// This version of RemoveProcessCallback is only for console applications.
// For GUI applications, RemoveProcessCallback is specific to the platform, so
// there are specialized versions for both OSX and GTK.
void wxAppTraits::RemoveProcessCallback(int fd)
{
    wxFDIODispatcher::Get()->UnregisterFD(fd);
}

int wxAppTraits::WaitForChild(wxExecuteData& execData)
{
    if ( !(execData.flags & wxEXEC_SYNC) )
    {
        // asynchronous execution: just launch the process and return,
        return execData.pid;
    }
    //else: synchronous execution case

    // Allocate an event loop that will be used to wait for the process
    // to terminate, will handle stdout, stderr, and any other events.
    //
    // The event loop will get started in common (to console and GUI) code
    // in WaitForChildSync
    wxConsoleEventLoop loop;
    execData.endProcData.syncEventLoopPtr = &loop;

    return WaitForChildSync(execData);
}

// This function is common code for both console and GUI applications.
//
// For non wxEXEC_NOEVENTS case, it will use a temporary event loop
// to wait for the child process to terminate, and to handle stdout/stderr events.
//
// For wxEXEC_NOEVENTS case, it will use wxSelectDispatcher to block for
// only the 3 events we care about and none others.
int wxAppTraits::WaitForChildSync(wxExecuteData& execData)
{
    if ( !(execData.flags & wxEXEC_NOEVENTS) )
    {
        // endProcData.pid will be set to 0 from wxHandleProcessTermination() when
        // the process terminates.  If it has already terminated then don't enter
        // the wait loop.
        if ( execData.endProcData.pid != 0 )
        {
            // Run a temporary event loop.  The OS will call our callback functions
            // upon arrival of stdout/stderr data as well as a callback (SIGCHLD)
            // if the child process terminates.
            execData.endProcData.syncEventLoopPtr->Run();
        }
    }
    else
    {
        // If we don't want to execute any events, we still need to handle
        // arrival of data from stdout/stderr and child process termination.
        // Handling these 3 exclusively, without handling other events is
        // accomplished using wxSelectDispatcher() to only handle data on the
        // file descriptors for those 3 events.

        // we can't simply block waiting for the child to terminate as we would
        // dead lock if it writes more than the pipe buffer size (typically
        // 4KB) bytes of output -- it would then block waiting for us to read
        // the data while we'd block waiting for it to terminate
        //
        // so multiplex here waiting for any input from the child or closure of
        // the pipe used to indicate its termination

        // Register the FD for child process termination.
        wxTheApp->RegisterSignalWakeUpPipe(*execData.dispatcher);

        // Set SyncEventPtr to NULL, so that we don't try to call
        // ScheduleExit() on a non-existent event loop.  This is not the owning
        // pointer (the object is actually on the stack), so it will still get
        // deleted properly even if we set it to NULL.
        execData.endProcData.syncEventLoopPtr = NULL;

        // endProcData.pid will be set to 0 from wxHandleProcessTermination()
        // when the process terminates.  So keep dispatching events that
        // unblock select() until we have detected the process has fully
        // terminated
        while ( execData.endProcData.pid != 0 )
        {
            execData.dispatcher->Dispatch();
        }
    }

#if HAS_PIPE_STREAMS && wxUSE_SOCKETS
    wxProcess * const process = execData.process;
    if ( process && process->IsRedirected() )
    {
        // Event loop/wxSelectDispatcher has finished and the process has terminated.

        // Perform some final updates in case we did not get a chance to execute
        // a final callback.
        execData.bufOut->Update();
        execData.bufErr->Update();

        // Disable future callbacks.
        process->GetUnixProcess()->GetStdoutHandler().DisableCallback();
        process->GetUnixProcess()->GetStderrHandler().DisableCallback();
    }
    //else: no IO redirection, just block waiting for the child to exit
#endif // HAS_PIPE_STREAMS

    // The exit code will have been set in execData.endProcData.exitcode
    // by the call to wxCheckChildProcessTermination(), which will have
    // occurred during the event loop.
    return execData.endProcData.exitcode;
}

void wxHandleProcessTermination(wxEndProcessData *data)
{
    // Notify user about termination if required
    if ( data->async )
    {
        if ( data->process )
        {
            data->process->OnTerminate(data->pid, data->exitcode);
        }

        // in case of asynchronous execution we don't need this data any more
        // after the child terminates
        delete data;
    }
    else // sync execution
    {
        // let wxExecute() know that the process has terminated
        data->pid = 0;

        if ( data->syncEventLoopPtr )
        {
            // Stop the event loop for synchronous wxExecute.
            data->syncEventLoopPtr->ScheduleExit();
        }
    }
}

void
wxAppTraits::RegisterProcessCallback(wxFDIOHandler& handler,
                                     int fd,
                                     wxFDIODispatcher* dispatcher)
{
    wxCHECK_RET( !HasCallbackForFD(fd), "Already registered for this FD?" );

    m_fdHandlers[fd] = FDHandlerData(&handler, dispatcher);

    if ( dispatcher )
    {
        dispatcher->RegisterFD(fd, &handler, wxFDIO_INPUT);
    }
    else
    {
        AddProcessCallback(handler, fd);
    }
}

void wxAppTraits::UnRegisterProcessCallback(int fd)
{
    const FDHandlers::iterator it = m_fdHandlers.find(fd);

    wxCHECK_RET( it != m_fdHandlers.end(), "FD not registered" );

    const FDHandlerData& data = it->second;

    if ( data.dispatcher )
        data.dispatcher->UnregisterFD(fd);
    else
        RemoveProcessCallback(fd);

    m_fdHandlers.erase(it);
}

// helper function which calls waitpid() and analyzes the result
// The result of waitpid is returned so that the caller can know
// if the child process with 'pid' actually terminated or not.
int DoWaitForChild(int pid, int flags = 0, int *waitpid_rc = NULL)
{
    wxASSERT_MSG( pid > 0, "invalid PID" );

    int status, rc;

    // loop while we're getting EINTR
    for ( ;; )
    {
        rc = waitpid(pid, &status, flags);

        if ( waitpid_rc != NULL)
        {
            *waitpid_rc=rc;
        }

        if ( rc != -1 || errno != EINTR )
            break;
    }

    if ( rc != 0 )
    {
        if ( rc == -1 )
        {
            wxLogLastError(wxString::Format("waitpid(%d)", pid));
        }
        else // child did terminate
        {
            wxASSERT_MSG( rc == pid, "unexpected waitpid() return value" );

            // notice that the caller expects the exit code to be signed, e.g. -1
            // instead of 255 so don't assign WEXITSTATUS() to an int
            signed char exitcode;
            if ( WIFEXITED(status) )
                exitcode = WEXITSTATUS(status);
            else if ( WIFSIGNALED(status) )
                exitcode = -WTERMSIG(status);
            else
            {
                wxLogError("Child process (PID %d) exited for unknown reason, "
                           "status = %d", pid, status);
                exitcode = -1;
            }

            return exitcode;
        }
    }

    return -1;
}

void wxCheckChildProcessTermination(int WXUNUSED(sig))
{
    wxEndProcessData *endProcData;

    int exitcode;
    int waitpid_rc;
    bool hash_entry_deleted_flag;
    int pid;

    do
    {
        hash_entry_deleted_flag = false;

        /** Vadim Zeitlin asks:
            Why not call waitpid(-1, &status, WNOHANG) as I proposed?

            Rob Bresalier answers:
            I thought about doing it that way and perhaps I should. I'm
            just scared that while a few child processes are terminated
            that I might get a return value of 0 because of WNOHANG and
            then if I made a subsequent call it would return the PID.
            But since I would be looping as you said, if I got a return
            value of 0 that would cause an exit from the loop (and
            perhaps an error return of -1 should also cause the loop to
            stop).  Because the loop would be exited, there would be no
            subsequent call to find another PID that terminated.  This
            fear is probably unfounded, so with a little reassurance
            that this would not happen I can do it that way.  I already
            implemented my list as a hash, so it would be a small
            change.

            Vadim replies:
            Let's leave this aside for now, replacing a loop waiting for
            all children with a single call to waitpid(-1) can always be
            done later.

            See:
            https://groups.google.com/forum/?fromgroups=#!topic/wx-users/tFXaa5N-yc0
            */

        // Traverse the list of opened child processes to check which one's terminated.
        // And if it terminated, then call wxHandleProcessTermination()
        for ( ChildProcessesOpenedHash::iterator it = childProcessesOpenedHash.begin();
              it != childProcessesOpenedHash.end();
              ++it )
        {
            pid=it->first;

            // Try to execute waitpid() on the child, and see if it terminates.
            exitcode = DoWaitForChild(pid,
                                    WNOHANG,
                                    &waitpid_rc // Get return code of waitpid()
                                   );

            if (waitpid_rc == 0)
            {   // This means that this PID is still running, so move onto the
                // next PID.
                continue;
            }

            // If we are here, it means we have detected the end of this
            // process, or there was a problem with this PID.

            // Get the pointer to the endProcData before we erase the
            // iterator, so that we can use it after the iterator is erased.
            endProcData = it->second;

            // Remove this process from the hash list of child processes
            // that are still open.  Do not use the iterator after this
            // as it is now invalid.  We erase this from the list so that
            // we don't chance handling another signal from any possible
            // event loop that could be called inside of
            // wxHandleProcessTermination()
            childProcessesOpenedHash.erase(it);
            hash_entry_deleted_flag = true;

            // Save the exit code.
            endProcData->exitcode = exitcode;

            // Inform the next call to CheckHandleTermination(), and also possible
            // future calls for when stdout/stderr are done that the child process
            // has terminated.
            endProcData->childProcessTerminatedFlag = true;

            // Check if it is time to handle the termination and inform user.
            endProcData->CheckHandleTermination();

            // Since we deleted this entry, we can't trust the ++it in
            // the loop, so start the iterator loop over again.
            break;
        }
    } while( hash_entry_deleted_flag );
}

void wxExecuteIOHandler::DisableCallback()
{
    if ( !IsShutDownFlagSet() )  // Don't shutdown more than once.
    {
        wxTheApp->GetTraits()->UnRegisterProcessCallback(m_fd);

        // Inform the callback to stop itself
        m_shutDownCallbackFlag = true;
    }
}

void wxExecuteIOHandler::OnReadWaiting()
{
    bool disableCallbackAndTerminate = false;

    wxEndProcessData *endProcDataPtr;
    endProcDataPtr = this->m_process->GetUnixProcess()->GetEndProcDataPtr();

    if ( m_flags & wxEXEC_SYNC )
    {   // Sync process, process all data coming at us from the pipe
        // so that the pipe does not get full and cause a deadlock
        // situation.
        if ( m_buf )
        {
            m_buf->Update();

            if ( m_buf->Eof() )
            {
                disableCallbackAndTerminate = true;
            }
        }
    }
    else
    {
        wxFAIL_MSG("wxExecuteIOHandler::OnReadWaiting should never be called for asynchronous wxExecute.");
        disableCallbackAndTerminate = true;
    }

    if ( disableCallbackAndTerminate )
    {
        DisableCallback();
        endProcDataPtr->CheckHandleTermination();
    }
}

// Checks if stdout/stderr and child process have all terminated before
// stopping the event loop and informing the application about termination.
//
// The check on stdout/stderr is only done if notifications (that is the
// callbacks to wxProcess::OnInputAvailable() and ::OnErrorAvailable() are
// activated).
void wxEndProcessData::CheckHandleTermination()
{
    if (    process
         && process->IsRedirected()
         && process->GetUnixProcess()->IsNotifyEnabled()
       )
    {
        if ( !( process->GetUnixProcess()->GetStdoutHandler().IsShutDownFlagSet() ) )
        {   // Still waiting for stdout data, so do not terminate event loop.
            return;
        }
        if ( !( process->GetUnixProcess()->GetStderrHandler().IsShutDownFlagSet() ) )
        {   // Still waiting for stderr data, so do not terminate event loop.
            return;
        }
    }

    if ( childProcessTerminatedFlag )
    {
        // The ScheduleExit() for the event loop will get called in wxHandleProcessTermination().
        wxHandleProcessTermination(this);
    }
}

