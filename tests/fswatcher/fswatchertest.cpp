///////////////////////////////////////////////////////////////////////////////
// Name:        tests/fswatcher/fswatchertest.cpp
// Purpose:     wxFileSystemWatcher unit test
// Author:      Bartosz Bekier
// Created:     2009-06-11
// Copyright:   (c) 2009 Bartosz Bekier
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "testprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/timer.h"
#endif

#if wxUSE_FSWATCHER

#include "wx/evtloop.h"
#include "wx/filename.h"
#include "wx/filefn.h"
#include "wx/fswatcher.h"
#include "wx/scopedptr.h"
#include "wx/stdpaths.h"
#include "wx/vector.h"

#include "testfile.h"

// ----------------------------------------------------------------------------
// local functions
// ----------------------------------------------------------------------------

// class generating file system events
class EventGenerator
{
public:
    static EventGenerator& Get()
    {
        if (!ms_instance)
            ms_instance = new EventGenerator(GetWatchDir());

        return *ms_instance;
    }

    EventGenerator(const wxFileName& path) : m_base(path)
    {
        m_old = wxFileName();
        m_file = RandomName();
        m_new = RandomName();
    }

    // operations
    bool CreateFile()
    {
        return CreateFile(m_file);
    }

    bool CreateFile(const wxFileName& filename)
    {
        wxFile file(filename.GetFullPath(), wxFile::write);
        return file.IsOpened() && filename.FileExists();
    }

    bool RenameFile()
    {
        REQUIRE(m_file.FileExists());

        wxLogDebug("Renaming %s=>%s", m_file.GetFullPath(), m_new.GetFullPath());

        bool ret = wxRenameFile(m_file.GetFullPath(), m_new.GetFullPath());
        if (ret)
        {
            m_old = m_file;
            m_file = m_new;
            m_new = RandomName();
        }

        return ret;
    }

    bool DeleteFile()
    {
        REQUIRE(m_file.FileExists());

        bool ret =  wxRemoveFile(m_file.GetFullPath());
        if (ret)
        {
            m_old = m_file;
            m_file = m_new;
            m_new = RandomName();
        }

        return ret;
    }

    bool TouchFile()
    {
        return m_file.Touch();
    }

    bool ReadFile()
    {
        wxFile f(m_file.GetFullPath());
        REQUIRE(f.IsOpened());

        char buf[1];
        ssize_t count = f.Read(buf, sizeof(buf));
        CHECK(count > 0);

        return true;
    }

    bool ModifyFile()
    {
        return ModifyFile(m_file);
    }

    bool ModifyFile(const wxFileName& filename)
    {
        REQUIRE(filename.FileExists());

        wxFile file(filename.GetFullPath(), wxFile::write_append);
        REQUIRE(file.IsOpened());

        CHECK(file.Write("Words of Wisdom, Lloyd. Words of wisdom\n"));
        return file.Close();
    }

    // helpers
    wxFileName RandomName(int length = 10)
    {
        return RandomName(m_base, length);
    }

    // static helpers
    static const wxFileName& GetWatchDir()
    {
        static wxFileName dir;

        if (dir.DirExists())
            return dir;

        wxString tmp = wxStandardPaths::Get().GetTempDir();
        dir.AssignDir(tmp);

        // XXX look for more unique name? there is no function to generate
        // unique filename, the file always get created...
        dir.AppendDir("fswatcher_test");
        REQUIRE(!dir.DirExists());
        REQUIRE(dir.Mkdir());

        return dir;
    }

    static void RemoveWatchDir()
    {
        wxFileName dir = GetWatchDir();
        REQUIRE(dir.DirExists());

        // just to be really sure we know what we remove
        REQUIRE( dir.GetDirs().Last() == "fswatcher_test" );

        CHECK( dir.Rmdir(wxPATH_RMDIR_RECURSIVE) );
    }

    static wxFileName RandomName(const wxFileName& base, int length = 10)
    {
        static int ALFA_CNT = 'z' - 'a';

        wxString s;
        for (int i = 0 ; i < length; ++i)
        {
            char c = 'a' + (rand() % ALFA_CNT);
            s += c;
        }

        return wxFileName(base.GetFullPath(), s);
    }

public:
    wxFileName m_base;     // base dir for doing operations
    wxFileName m_file;     // current file name
    wxFileName m_old;      // previous file name
    wxFileName m_new;      // name after renaming

protected:
    static EventGenerator* ms_instance;
};

EventGenerator* EventGenerator::ms_instance = 0;


// Abstract base class from which concrete event tests inherit.
//
// This class provides the common test skeleton which various virtual hooks
// that should or can be reimplemented by the derived classes.
class FSWTesterBase : public wxEvtHandler
{
public:
    FSWTesterBase(int types = wxFSW_EVENT_ALL) :
        eg(EventGenerator::Get()),
        m_eventTypes(types)
    {
        Bind(wxEVT_FSWATCHER, &FSWTesterBase::OnFileSystemEvent, this);

        // wxFileSystemWatcher can be created only once the event loop is
        // running, so we can't do it from here and will do it from inside the
        // loop when this event handler is invoked.
        CallAfter(&FSWTesterBase::OnIdleInit);
    }

    virtual ~FSWTesterBase()
    {
        if (m_loop.IsRunning())
            m_loop.Exit();

        for ( size_t n = 0; n < m_events.size(); ++n )
            delete m_events[n];
    }

    void Exit()
    {
        m_loop.Exit();
    }

    // sends idle event, so we get called in a moment
    void SendIdle()
    {
        wxIdleEvent* e = new wxIdleEvent();
        QueueEvent(e);
    }

    void Run()
    {
        m_loop.Run();
    }

    void OnIdleInit()
    {
        // Now that the event loop is running, we can construct the watcher.
        m_watcher.reset(new wxFileSystemWatcher());
        m_watcher->SetOwner(this);

        Init();

        GenerateEvent();

        // Check the result when the next idle event comes: note that we can't
        // use CallAfter() here, unfortunately, because OnIdleCheckResult()
        // would then be called immediately, from the same event loop iteration
        // as we're called from, because the idle/pending events are processed
        // for as long as there any. Instead, we need to return to the event
        // loop itself to give it a chance to dispatch wxFileSystemWatcherEvent
        // and wait until our handler for it calls SendIdle() which will then
        // end up calling OnIdleCheckResult() afterwards.
        Bind(wxEVT_IDLE, &FSWTesterBase::OnIdleCheckResult, this);
    }

    void OnIdleCheckResult(wxIdleEvent& WXUNUSED(event))
    {
        Unbind(wxEVT_IDLE, &FSWTesterBase::OnIdleCheckResult, this);

        CheckResult();
        Exit();
    }

    virtual void Init()
    {
        wxFileName dir = EventGenerator::GetWatchDir();
        REQUIRE( m_watcher->Add(dir, m_eventTypes) );
    }

    virtual void CheckResult()
    {
        REQUIRE( !m_events.empty() );

        const wxFileSystemWatcherEvent* const e = m_events.front();

        // this is our "reference event"
        const wxFileSystemWatcherEvent expected = ExpectedEvent();

        CHECK( e->GetChangeType() == expected.GetChangeType() );

        CHECK( e->GetEventType() == wxEVT_FSWATCHER );

        // XXX this needs change
        CHECK( e->GetEventCategory() == wxEVT_CATEGORY_UNKNOWN );

        CHECK( e->GetPath() == expected.GetPath() );
        CHECK( e->GetNewPath() == expected.GetNewPath() );


        // In addition to checking that we got the event we expected, also
        // check that we didn't get any extraneous events.
        wxString desc;
        size_t ignored = 0;
        for ( size_t n = 1; n < m_events.size(); ++n )
        {
            const wxFileSystemWatcherEvent* const e2 = m_events[n];

#ifdef __WXMSW__
            // Under MSW extra modification events are sometimes reported after a
            // rename and we just can't get rid of them, so ignore them in this
            // test if they do happen.
            if ( e->GetChangeType() == wxFSW_EVENT_RENAME )
            {
                if ( e2->GetChangeType() == wxFSW_EVENT_MODIFY &&
                        e2->GetPath() == e->GetNewPath() )
                {
                    // This is a modify event for the new file, ignore it.
                    ignored++;
                    continue;
                }
            }
#endif // __WXMSW__

            desc += wxString::Format("Extra event %zu: type=%#04x, path=\"%s\"",
                                     n - ignored,
                                     e2->GetChangeType(),
                                     e2->GetPath().GetFullPath());
            if ( e2->GetNewPath() != e2->GetPath() )
            {
                desc += wxString::Format(" -> \"%s\"",
                                         e2->GetNewPath().GetFullPath());
            }

            desc += "\n";
        }

        INFO(desc);
        CHECK( m_events.size() - ignored == 1 );
    }

    virtual void GenerateEvent() = 0;

    virtual wxFileSystemWatcherEvent ExpectedEvent() = 0;


protected:
    EventGenerator& eg;
    wxEventLoop m_loop;    // loop reference

    wxScopedPtr<wxFileSystemWatcher> m_watcher;

    int m_eventTypes;  // Which event-types to watch. Normally all of them

    wxVector<wxFileSystemWatcherEvent*> m_events;

private:
    void OnFileSystemEvent(wxFileSystemWatcherEvent& evt)
    {
        wxLogDebug("--- %s ---", evt.ToString());
        m_events.push_back(wxDynamicCast(evt.Clone(), wxFileSystemWatcherEvent));

        // test finished
        SendIdle();
    }
};

// FSWTesterBase assumes that some events will actually be generated, however
// it's also useful to have test cases checking that the events are not
// generated and this class is used for them. The derived class should just
// override GenerateEvent() to perform whatever actions which, contrary to the
// name of the method, should be checked to not generate any events.
//
// The template parameter T here is a FSWTesterBase-derived class to inherit
// from, i.e. this class uses CRTP.
template <class T>
class FSWNoEventTester : public T,
                         private wxTimer
{
public:
    FSWNoEventTester()
    {
        // We need to use an inactivity timer as we never get any file
        // system events in this test, so we consider that the test is
        // finished when this 1s timeout expires instead of, as usual,
        // stopping after getting the file system events.
        StartOnce(200);
    }

    virtual void CheckResult() wxOVERRIDE
    {
        REQUIRE( this->m_events.empty() );
    }

    virtual wxFileSystemWatcherEvent ExpectedEvent() wxOVERRIDE
    {
        FAIL( "Shouldn't be called" );

        return wxFileSystemWatcherEvent(wxFSW_EVENT_ERROR);
    }

    virtual void Notify() wxOVERRIDE
    {
        this->SendIdle();
    }
};

// ----------------------------------------------------------------------------
// test fixture
// ----------------------------------------------------------------------------

class FileSystemWatcherTestCase
{
public:
    FileSystemWatcherTestCase()
    {
        // Before each test, remove the dir if it exists.
        // It would exist if the previous test run was aborted.
        wxString tmp = wxStandardPaths::Get().GetTempDir();
        wxFileName dir;
        dir.AssignDir(tmp);
        dir.AppendDir("fswatcher_test");
        dir.Rmdir(wxPATH_RMDIR_RECURSIVE);
        EventGenerator::Get().GetWatchDir();
    }

    ~FileSystemWatcherTestCase()
    {
        EventGenerator::Get().RemoveWatchDir();
    }
};

// ----------------------------------------------------------------------------
// TestEventCreate
// ----------------------------------------------------------------------------

TEST_CASE_METHOD(FileSystemWatcherTestCase,
                 "wxFileSystemWatcher::EventCreate", "[fsw]")
{
    class EventTester : public FSWTesterBase
    {
    public:
        virtual void GenerateEvent() wxOVERRIDE
        {
            CHECK(eg.CreateFile());
        }

        virtual wxFileSystemWatcherEvent ExpectedEvent() wxOVERRIDE
        {
            wxFileSystemWatcherEvent event(wxFSW_EVENT_CREATE);
            event.SetPath(eg.m_file);
            event.SetNewPath(eg.m_file);
            return event;
        }
    };

    EventTester tester;

    tester.Run();
}

// ----------------------------------------------------------------------------
// TestEventDelete
// ----------------------------------------------------------------------------

TEST_CASE_METHOD(FileSystemWatcherTestCase,
                 "wxFileSystemWatcher::EventDelete", "[fsw]")
{
    class EventTester : public FSWTesterBase
    {
    public:
        virtual void GenerateEvent() wxOVERRIDE
        {
            CHECK(eg.DeleteFile());
        }

        virtual wxFileSystemWatcherEvent ExpectedEvent() wxOVERRIDE
        {
            wxFileSystemWatcherEvent event(wxFSW_EVENT_DELETE);
            event.SetPath(eg.m_old);

            // CHECK maybe new path here could be NULL or sth?
            event.SetNewPath(eg.m_old);
            return event;
        }
    };

    // we need to create a file now, so we can delete it
    EventGenerator::Get().CreateFile();

    EventTester tester;
    tester.Run();
}

// kqueue-based implementation doesn't collapse create/delete pairs in
// renames and doesn't detect neither modifications nor access to the
// files reliably currently so disable these tests
//
// FIXME: fix the code and reenable them
#ifndef wxHAS_KQUEUE

// ----------------------------------------------------------------------------
// TestEventRename
// ----------------------------------------------------------------------------

TEST_CASE_METHOD(FileSystemWatcherTestCase,
                 "wxFileSystemWatcher::EventRename", "[fsw]")
{
    class EventTester : public FSWTesterBase
    {
    public:
        virtual void GenerateEvent() wxOVERRIDE
        {
            CHECK(eg.RenameFile());
        }

        virtual wxFileSystemWatcherEvent ExpectedEvent() wxOVERRIDE
        {
            wxFileSystemWatcherEvent event(wxFSW_EVENT_RENAME);
            event.SetPath(eg.m_old);
            event.SetNewPath(eg.m_file);
            return event;
        }
    };

    // need a file to rename later
    EventGenerator::Get().CreateFile();

    EventTester tester;
    tester.Run();
}

// ----------------------------------------------------------------------------
// TestEventModify
// ----------------------------------------------------------------------------

TEST_CASE_METHOD(FileSystemWatcherTestCase,
                 "wxFileSystemWatcher::EventModify", "[fsw]")
{
    class EventTester : public FSWTesterBase
    {
    public:
        virtual void GenerateEvent() wxOVERRIDE
        {
            CHECK(eg.ModifyFile());
        }

        virtual wxFileSystemWatcherEvent ExpectedEvent() wxOVERRIDE
        {
            wxFileSystemWatcherEvent event(wxFSW_EVENT_MODIFY);
            event.SetPath(eg.m_file);
            event.SetNewPath(eg.m_file);
            return event;
        }
    };

    // we need to create a file to modify
    EventGenerator::Get().CreateFile();

    EventTester tester;
    tester.Run();
}

// MSW implementation doesn't detect file access events currently
#ifndef __WINDOWS__

// ----------------------------------------------------------------------------
// TestEventAccess
// ----------------------------------------------------------------------------

TEST_CASE_METHOD(FileSystemWatcherTestCase,
                 "wxFileSystemWatcher::EventAccess", "[fsw]")
{
    class EventTester : public FSWTesterBase
    {
    public:
        virtual void GenerateEvent() wxOVERRIDE
        {
            CHECK(eg.ReadFile());
        }

        virtual wxFileSystemWatcherEvent ExpectedEvent() wxOVERRIDE
        {
            wxFileSystemWatcherEvent event(wxFSW_EVENT_ACCESS);
            event.SetPath(eg.m_file);
            event.SetNewPath(eg.m_file);
            return event;
        }
    };

    // we need to create a file to read from it and write sth to it
    EventGenerator::Get().CreateFile();
    EventGenerator::Get().ModifyFile();

    EventTester tester;
    tester.Run();
}

#endif // __WINDOWS__

#endif // !wxHAS_KQUEUE

#ifdef wxHAS_INOTIFY
// ----------------------------------------------------------------------------
// TestEventAttribute
// ----------------------------------------------------------------------------
TEST_CASE_METHOD(FileSystemWatcherTestCase,
                 "wxFileSystemWatcher::EventAttribute", "[fsw]")
{
    class EventTester : public FSWTesterBase
    {
    public:
        virtual void GenerateEvent() wxOVERRIDE
        {
            CHECK(eg.TouchFile());
        }

        virtual wxFileSystemWatcherEvent ExpectedEvent() wxOVERRIDE
        {
            wxFileSystemWatcherEvent event(wxFSW_EVENT_ATTRIB);
            event.SetPath(eg.m_file);
            event.SetNewPath(eg.m_file);
            return event;
        }
    };

    // we need to create a file to touch
    EventGenerator::Get().CreateFile();

    EventTester tester;
    tester.Run();
}

// ----------------------------------------------------------------------------
// TestSingleWatchtypeEvent: Watch only wxFSW_EVENT_ACCESS
// ----------------------------------------------------------------------------

TEST_CASE_METHOD(FileSystemWatcherTestCase,
                 "wxFileSystemWatcher::SingleWatchtypeEvent", "[fsw]")
{
    class EventTester : public FSWTesterBase
    {
    public:
        // We could pass wxFSW_EVENT_CREATE or MODIFY instead, but not RENAME or
        // DELETE as the event path fields would be wrong in CheckResult()
        EventTester() : FSWTesterBase(wxFSW_EVENT_ACCESS) {}

        virtual void GenerateEvent() wxOVERRIDE
        {
            // As wxFSW_EVENT_ACCESS is passed to the ctor only ReadFile() will
            // generate an event. Without it they all will, and the test fails
            CHECK(eg.CreateFile());
            CHECK(eg.ModifyFile());
            CHECK(eg.ReadFile());
        }

        virtual wxFileSystemWatcherEvent ExpectedEvent() wxOVERRIDE
        {
            wxFileSystemWatcherEvent event(wxFSW_EVENT_ACCESS);
            event.SetPath(eg.m_file);
            event.SetNewPath(eg.m_file);
            return event;
        }
    };

    EventTester tester;
    tester.Run();
}
#endif // wxHAS_INOTIFY

// ----------------------------------------------------------------------------
// TestTrees
// ----------------------------------------------------------------------------

TEST_CASE_METHOD(FileSystemWatcherTestCase,
                 "wxFileSystemWatcher::Tree", "[fsw]")
{
    class TreeEventTester : public FSWTesterBase
    {
    public:
        virtual void Init() wxOVERRIDE
        {
            wxFileName dir = EventGenerator::GetWatchDir();
            REQUIRE( m_watcher->AddTree(dir, m_eventTypes) );
        }

        virtual void GenerateEvent() wxOVERRIDE
        {
            CHECK(eg.CreateFile());
        }

        virtual wxFileSystemWatcherEvent ExpectedEvent() wxOVERRIDE
        {
            wxFileSystemWatcherEvent event(wxFSW_EVENT_CREATE);
            event.SetPath(eg.m_file);
            event.SetNewPath(eg.m_file);
            return event;
        }

#ifdef __WXOSX__
        virtual void CheckResult() wxOVERRIDE
        {
            // Monitoring trees is buggy under macOS and the generated events
            // don't use correct paths currently, so just check that an event
            // was generated, without checking anything else.
            REQUIRE( !m_events.empty() );

            const wxFileSystemWatcherEvent * const e = m_events.front();

            const wxFileSystemWatcherEvent expected = ExpectedEvent();

            CHECK( e->GetChangeType() == expected.GetChangeType() );
            CHECK( e->GetEventType() == wxEVT_FSWATCHER );
        }
#endif // __WXOSX__
    } tester;

    tester.Run();
}

TEST_CASE_METHOD(FileSystemWatcherTestCase,
                 "wxFileSystemWatcher::Trees", "[fsw]")
{
    class TreeTester : public FSWTesterBase
    {
        const size_t subdirs;
        const size_t files;

    public:
        TreeTester() : subdirs(5), files(3) {}

        void GrowTree(wxFileName dir
#ifdef __UNIX__
                      , bool withSymlinks = false
#endif
                      )
        {
            REQUIRE(dir.Mkdir());
            // Now add a subdir with an easy name to remember in WatchTree()
            dir.AppendDir("child");
            REQUIRE(dir.Mkdir());
            wxFileName child(dir);  // Create a copy to which to symlink

            // Create a branch of 5 numbered subdirs, each containing 3
            // numbered files
            for ( unsigned d = 0; d < subdirs; ++d )
            {
                dir.AppendDir(wxString::Format("subdir%u", d+1));
                REQUIRE(dir.Mkdir());

                const wxString prefix = dir.GetPathWithSep();
                const wxString ext[] = { ".txt", ".log", "" };
                for ( unsigned f = 0; f < files; ++f )
                {
                    // Just create the files.
                    wxFile(prefix + wxString::Format("file%u", f+1) + ext[f],
                           wxFile::write);
                }
#if defined(__UNIX__)
                if ( withSymlinks )
                {
                    // Create a symlink to a files, and another to 'child'
                    CHECK
                    (
                        symlink(wxString(prefix + "file1").c_str(),
                                wxString(prefix + "file.lnk").c_str()) == 0
                    );

                    CHECK
                    (
                        symlink(child.GetFullPath().c_str(),
                                wxString(prefix + "dir.lnk").c_str()) == 0
                    );
                }
#endif // __UNIX__
            }
        }

        void RmDir(wxFileName dir)
        {
            REQUIRE(dir.DirExists());

            CHECK(dir.Rmdir(wxPATH_RMDIR_RECURSIVE));
        }

        void WatchDir(wxFileName dir)
        {
            REQUIRE(m_watcher);

            // Store the initial count; there may already be some watches
            const int initial = m_watcher->GetWatchedPathsCount();

            m_watcher->Add(dir);
            CHECK( m_watcher->GetWatchedPathsCount() == initial + 1 );
        }

        void RemoveSingleWatch(wxFileName dir)
        {
            REQUIRE(m_watcher);

            const int initial = m_watcher->GetWatchedPathsCount();

            m_watcher->Remove(dir);
            CHECK( m_watcher->GetWatchedPathsCount() == initial - 1 );
        }

        void WatchTree(const wxFileName& dir)
        {
            REQUIRE(m_watcher);

            int treeitems = 1; // the trunk
#if !defined(__WINDOWS__) && !defined(wxHAVE_FSEVENTS_FILE_NOTIFICATIONS)
            // When there's no file mask, wxMSW and wxOSX set a single watch
            // on the trunk which is implemented recursively.
            // wxGTK always sets an additional watch for each subdir
            treeitems += subdirs + 1; // +1 for 'child'
#endif // !__WINDOWS__ && !wxHAVE_FSEVENTS_FILE_NOTIFICATIONS

            // Store the initial count; there may already be some watches
            const int initial = m_watcher->GetWatchedPathsCount();

            GrowTree(dir);

            m_watcher->AddTree(dir);
            const int plustree = m_watcher->GetWatchedPathsCount();

            CHECK( plustree == initial + treeitems );

            m_watcher->RemoveTree(dir);
            CHECK( m_watcher->GetWatchedPathsCount() == initial );

            // Now test the refcount mechanism by watching items more than once
            wxFileName child(dir);
            child.AppendDir("child");
            m_watcher->AddTree(child);
            // Check some watches were added; we don't care about the number
            CHECK(initial < m_watcher->GetWatchedPathsCount());
            // Now watch the whole tree and check that the count is the same
            // as it was the first time, despite also adding 'child' separately
            // Except that in wxMSW this isn't true: each watch will be a
            // single, recursive dir; so fudge the count
            int fudge = 0;
#if defined(__WINDOWS__) || defined(wxHAVE_FSEVENTS_FILE_NOTIFICATIONS)
            fudge = 1;
#endif // __WINDOWS__ || wxHAVE_FSEVENTS_FILE_NOTIFICATIONS
            m_watcher->AddTree(dir);
            CHECK( m_watcher->GetWatchedPathsCount() == plustree + fudge );
            m_watcher->RemoveTree(child);
            CHECK(initial < m_watcher->GetWatchedPathsCount());
            m_watcher->RemoveTree(dir);
            CHECK( m_watcher->GetWatchedPathsCount() == initial );
#if defined(__UNIX__)
            // Finally, test a tree containing internal symlinks
            RmDir(dir);
            GrowTree(dir, true /* test symlinks */);

            // Without the DontFollowLink() call AddTree() would now assert
            // (and without the assert, it would infinitely loop)
            wxFileName fn = dir;
            fn.DontFollowLink();
            CHECK(m_watcher->AddTree(fn));
            CHECK(m_watcher->RemoveTree(fn));

            // Regrow the tree without symlinks, ready for the next test
            RmDir(dir);
            GrowTree(dir, false);
#endif // __UNIX__
        }

        void WatchTreeWithFilespec(const wxFileName& dir)
        {
            REQUIRE(m_watcher);
            REQUIRE(dir.DirExists()); // Was built in WatchTree()

            // Store the initial count; there may already be some watches
            const int initial = m_watcher->GetWatchedPathsCount();

            // When we use a filter, both wxMSW and wxGTK implementations set
            // an additional watch for each subdir (+1 for the root dir itself
            // and another +1 for "child").
            // On OS X, if we use FSEvents then we still only have 1 watch.
#ifdef wxHAVE_FSEVENTS_FILE_NOTIFICATIONS
            const int treeitems = 1;
#else
            const int treeitems = subdirs + 2;
#endif
            m_watcher->AddTree(dir, wxFSW_EVENT_ALL, "*.txt");

            CHECK( m_watcher->GetWatchedPathsCount() == initial + treeitems );

            // RemoveTree should try to remove only those files that were added
            m_watcher->RemoveTree(dir);
            CHECK( m_watcher->GetWatchedPathsCount() == initial );
        }

        void RemoveAllWatches()
        {
            REQUIRE(m_watcher);

            m_watcher->RemoveAll();
            CHECK( m_watcher->GetWatchedPathsCount() == 0 );
        }

        virtual void GenerateEvent() wxOVERRIDE
        {
            // We don't use this function for events. Just run the tests

            wxFileName watchdir = EventGenerator::GetWatchDir();
            REQUIRE(watchdir.DirExists());

            wxFileName treedir(watchdir);
            treedir.AppendDir("treetrunk");
            CHECK(!treedir.DirExists());

            wxFileName singledir(watchdir);
            singledir.AppendDir("single");
            CHECK(!singledir.DirExists());
            CHECK(singledir.Mkdir());

            WatchDir(singledir);
            WatchTree(treedir);
            // Now test adding and removing a tree using a filespec
            // wxMSW uses the generic method to add matching files; which fails
            // as it doesn't support adding files :/ So disable the test
#ifndef __WINDOWS__
            WatchTreeWithFilespec(treedir);
#endif // __WINDOWS__

            RemoveSingleWatch(singledir);
            // Add it back again, ready to test RemoveAll()
            WatchDir(singledir);

            RemoveAllWatches();

            // Clean up
            RmDir(treedir);
            RmDir(singledir);

            Exit();
        }

        virtual wxFileSystemWatcherEvent ExpectedEvent() wxOVERRIDE
        {
            FAIL("Shouldn't be called");

            return wxFileSystemWatcherEvent(wxFSW_EVENT_ERROR);
        }

        virtual void CheckResult() wxOVERRIDE
        {
            // Do nothing. We override this to prevent receiving events in
            // ExpectedEvent()
        }
    };

    TreeTester tester;

    tester.Run();
}

TEST_CASE_METHOD(FileSystemWatcherTestCase,
                 "wxFileSystemWatcher::NoEventsAfterRemove", "[fsw]")
{
    class NoEventsAfterRemoveEventTester : public FSWNoEventTester<FSWTesterBase>
    {
    public:
        virtual void GenerateEvent() wxOVERRIDE
        {
            m_watcher->Remove(EventGenerator::GetWatchDir());
            CHECK(eg.CreateFile());
        }
    };

    NoEventsAfterRemoveEventTester tester;
    tester.Run();
}

TEST_CASE_METHOD(FileSystemWatcherTestCase,
                 "wxFileSystemWatcher::SingleFile", "[fsw]")
{
    class SingleFileTesterBase : public FSWTesterBase
    {
    public:
        virtual void Init() wxOVERRIDE
        {
            REQUIRE( eg.CreateFile() );

            REQUIRE( m_watcher->Add(eg.m_file, wxFSW_EVENT_MODIFY) );
        }
    };

    // Check that modifying the file in place results in the expected event.
    SECTION("Modify")
    {
        class FileModifyTester : public SingleFileTesterBase
        {
        public:
            virtual void GenerateEvent() wxOVERRIDE
            {
                CHECK(eg.ModifyFile());
            }

            virtual wxFileSystemWatcherEvent ExpectedEvent() wxOVERRIDE
            {
                wxFileSystemWatcherEvent event(wxFSW_EVENT_MODIFY);
                event.SetPath(eg.m_file);
                event.SetNewPath(eg.m_file);
                return event;
            }
        } tester;

        tester.Run();
    };

    // Check that modifying the file by overwriting it with a new version also
    // results in events.
    SECTION("Overwrite")
    {
        class FileOverwriteTester : public SingleFileTesterBase
        {
        public:
            virtual void GenerateEvent() wxOVERRIDE
            {
                wxTempFile temp(eg.m_file.GetFullPath());
                temp.Write("And now for something completely different.\n");
                REQUIRE(temp.Commit());
            }

            virtual wxFileSystemWatcherEvent ExpectedEvent() wxOVERRIDE
            {
                wxFileSystemWatcherEvent event(wxFSW_EVENT_MODIFY);
                event.SetPath(eg.m_file);
                event.SetNewPath(eg.m_file);
                return event;
            }
        } tester;

        tester.Run();
    }

    // Check that modifying another file in the same directory does not result
    // in any events for the file being watched.
    SECTION("Another")
    {
        class OtherModifyTester : public FSWNoEventTester<SingleFileTesterBase>
        {
        public:
            virtual void GenerateEvent() wxOVERRIDE
            {
                const wxFileName other = eg.RandomName();
                REQUIRE(eg.CreateFile(other));
                CHECK(eg.ModifyFile(other));
            }
        } tester;

        tester.Run();
    };
}

TEST_CASE_METHOD(FileSystemWatcherTestCase,
                 "wxFileSystemWatcher::TwoFiles", "[fsw]")
{
    class TwoFilesTesterBase : public FSWTesterBase
    {
    public:
        virtual void Init() wxOVERRIDE
        {
            m_file2 = eg.RandomName();

            REQUIRE( eg.CreateFile() );
            REQUIRE( eg.CreateFile(m_file2) );

            REQUIRE( m_watcher->Add(eg.m_file, wxFSW_EVENT_MODIFY) );
            REQUIRE( m_watcher->Add(m_file2, wxFSW_EVENT_MODIFY) );
        }

    protected:
        wxFileName m_file2;
    };

    // Check that modifying the file in place results in the expected event.
    SECTION("Modify")
    {
        class FileModifyTester : public TwoFilesTesterBase
        {
        public:
            virtual void GenerateEvent() wxOVERRIDE
            {
                CHECK(eg.ModifyFile());
            }

            virtual wxFileSystemWatcherEvent ExpectedEvent() wxOVERRIDE
            {
                wxFileSystemWatcherEvent event(wxFSW_EVENT_MODIFY);
                event.SetPath(eg.m_file);
                event.SetNewPath(eg.m_file);
                return event;
            }
        } tester;

        tester.Run();
    };

    // Check that modifying the other file results in the expected event.
    SECTION("Modify2")
    {
        class File2ModifyTester : public TwoFilesTesterBase
        {
        public:
            virtual void GenerateEvent() wxOVERRIDE
            {
                CHECK(eg.ModifyFile(m_file2));
            }

            virtual wxFileSystemWatcherEvent ExpectedEvent() wxOVERRIDE
            {
                wxFileSystemWatcherEvent event(wxFSW_EVENT_MODIFY);
                event.SetPath(m_file2);
                event.SetNewPath(m_file2);
                return event;
            }
        } tester;

        tester.Run();
    };

    // Check that modifying another file in the same directory does not result
    // in any events for the file being watched.
    SECTION("Another")
    {
        class OtherModifyTester : public FSWNoEventTester<TwoFilesTesterBase>
        {
        public:
            virtual void GenerateEvent() wxOVERRIDE
            {
                const wxFileName other = eg.RandomName();
                REQUIRE(eg.CreateFile(other));
                CHECK(eg.ModifyFile(other));
            }
        } tester;

        tester.Run();
    };
}
#endif // wxUSE_FSWATCHER
