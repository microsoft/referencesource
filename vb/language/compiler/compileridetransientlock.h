//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Transient Lock.  Mainly used for the ErrorTable.  The decisicion on whether or not locking 
//  is necessary cannot always be made as compile time.  However it's really awkward to write
//  code like the following
//
//  CompilerIdeLock lock = MaybeCreateLock();
//  ...
//  if ( DidCreateLock() ) {
//      lock.Unlock();
//  }
//  DoOperationThatRequiresUnlocked();
//
//  This class fixes the problem by allowing the creator to specify the type of lock and fake
//  Lock/Unlock when it's not needed
//
//-------------------------------------------------------------------------------------------------

#pragma once

DECLARE_ENUM(TransientLockMode)
    Synchronized,
    Faked
END_ENUM(TransientLockMode)

class CompilerIdeTransientLock
{
public:
    NEW_CTOR_SAFE()

    CompilerIdeTransientLock(CompilerIdeCriticalSection& cs, TransientLockModeEnum mode) : 
      m_cs(cs),
      m_mode(mode),
      m_isLocked(false)
    {
        Lock();
    }

    ~CompilerIdeTransientLock()
    {
        if ( m_isLocked )
        {
            Unlock();
        }
    }

    void Lock()
    {
        ThrowIfTrue(m_isLocked);
        switch ( m_mode )
        {
        case TransientLockMode::Synchronized:
            m_cs.Lock();
            break;
        case TransientLockMode::Faked:
            break;
        default:
            ThrowIfFalse(false);
            break;
        }
        m_isLocked =  true;
    }

    void Unlock()
    {
        ThrowIfFalse(m_isLocked);        
        switch ( m_mode )
        {
        case TransientLockMode::Synchronized:
            m_cs.Unlock();
            break;
        case TransientLockMode::Faked:
            break;
        default:
            ThrowIfFalse(false);
            break;
        }
        m_isLocked = false;
    }

private:
    // Do not autogenerate
    CompilerIdeTransientLock();
    CompilerIdeTransientLock(const CompilerIdeTransientLock&);
    CompilerIdeTransientLock& operator=(const CompilerIdeTransientLock&);

    CompilerIdeCriticalSection& m_cs;
    TransientLockModeEnum m_mode;
    bool m_isLocked;

};
