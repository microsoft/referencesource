//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  The VB wrapper for CComModule
//  
//-------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------
//
// Wrapper class that gives ability to force cleaup sooner than CAtlModule::Term.
//
class VBComModule: public CComModule
{
public:

    VBComModule();
    ~VBComModule();

    // ForceTerminate: Execute the cleanup functions.  This function should be called before the
    // compiler allocators are destroyed because parts of the elements inside the module were
    // allocated with the compiler alloctors.  We cannot wait until the module is terminated
    // because the compiler is gone by that point. (see bug VS#546896 concerning ATL CComTypeInfoHolder Class).
    void ForceTerminate()
    {
        EnterCriticalSection(&m_csStaticDataInit);

        __try
        {
            if (m_pTermFuncs)
            {
                AtlCallTermFunc(this);
            }
        }
        __finally
        {
            LeaveCriticalSection(&m_csStaticDataInit);
        }
    }

    void LockEECompilerInit()
    {
        m_csEECompilerInitLock.Lock();
    }

    void UnlockEECompilerInit()
    {
        m_csEECompilerInitLock.Unlock();
    }

private:
    CComAutoCriticalSection m_csEECompilerInitLock;
};
