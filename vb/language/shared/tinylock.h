//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Smallest possible implementation of an exclusive lock.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class CTinyLock
{
private:
    volatile DWORD m_dwOwner;          // Owning thread (non-zero means locked)

    void Spin (long *piSpinCount)
    {
        if ((*piSpinCount)-- < 0)
        {
            InterlockedIncrement (&m_iSleeps);
            Snooze();
            *piSpinCount = m_iSpinStart;
        }
        else if (*piSpinCount == (m_iSpinStart - 1))
        {
            InterlockedIncrement (&m_iSpins);
        }
    }

public:
    static LONG m_iSpins;
    static LONG m_iSleeps;
    static LONG m_iSpinStart;

public:
    CTinyLock() : m_dwOwner(0)
    {
        if (m_iSpinStart == -1)
        {
            SYSTEM_INFO si;
            GetSystemInfo (&si);
            if (si.dwNumberOfProcessors > 1)
                m_iSpinStart = 4000;            // Hard-coded spin count...
            else
                m_iSpinStart = 0;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // CTinyLock::Acquire -- lock this object, returning true if this is the
    // first lock, false otherwise.  If true is returned, Release() must be
    // called to unlock the object.

    bool Acquire ()
    {
        long iSpin = m_iSpinStart;
        DWORD dwOld, dwTid = GetCurrentThreadId();

        while (true)
        {
            dwOld = (DWORD)InterlockedCompareExchange ((LONG*) & m_dwOwner, (LONG)dwTid, 0);
            if (dwOld == 0 || dwOld == dwTid)
                break;
            Spin (&iSpin);
        }

        return dwOld == 0;
    }

    ////////////////////////////////////////////////////////////////////////////
    // CTinyLock::Release -- release the lock held by this thread.  This should
    // NOT be called if Acquire() returned false, indicating that this thread
    // already had this object locked.

    void Release ()
    {
        VSASSERT (m_dwOwner == GetCurrentThreadId(), "Invalid");
        m_dwOwner = 0;
    }

    ////////////////////////////////////////////////////////////////////////////
    // CTinyLock::LockedByMe -- tests the lock to see if this thread currently
    // has it locked.
    bool LockedByMe ()
    {
        return m_dwOwner == GetCurrentThreadId();
    }

    int CountLockedByMe ()
    {
        return m_dwOwner == GetCurrentThreadId() ? 1 : 0;
    }

    ////////////////////////////////////////////////////////////////////////////
    // CTinyLock::IsLocked -- tests the lock to see if it is locked or not (by
    // any thread)

    bool IsLocked ()
    {
        return m_dwOwner != 0;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Snooze
    //
    // This is a friendly way to give up a time slice.  It uses Sleep(1), which
    // allows lower-priority threads to execute, and then calls PeekMessage, which
    // allows sent messages to be processed by this thread (allowing COM/RPC messages
    // to go through, etc.)
    static inline void Snooze ()
    {
// #ifndef FEATURE_PAL
//         MSG msg;
//        Sleep(1);
//        PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE);
//#else
        SwitchToThread();
//#endif
    }
};

////////////////////////////////////////////////////////////////////////////////
// CTinyGate -- handles scope-locking for a CTinyLock object

class CTinyGate
{
private:
    CTinyLock *m_pLock;
    bool m_fLocked;

public:
    CTinyGate(CTinyLock *p) : m_pLock(p)
    {
        m_fLocked = m_pLock && m_pLock->Acquire();
    }

    ~CTinyGate()
    {
        Release ();
    }

    void Release()
    {
        if (m_fLocked)
        {
            m_pLock->Release();
            m_fLocked = false;
        }
    }

    void Acquire()
    {
        VSASSERT(!m_fLocked, "Invalid");
        m_fLocked = m_pLock && m_pLock->Acquire();
    }

    bool HasLocked() const
    {
        return m_fLocked;
    }
};

