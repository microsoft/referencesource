//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// AutoPtr 
//
//-------------------------------------------------------------------------------------------------

#pragma once

template <class T>
class AutoPtr
{
public:
    explicit AutoPtr(T * pMem = NULL) :
        m_pMem(NULL)
    {
        Attach(pMem);
    }

    virtual ~AutoPtr()
    {
        Destroy();
    }

    operator T *() const
    {
        return m_pMem;
    }

    T* operator ->()
    {
        return m_pMem;
    }
    
    T** operator&()
    {
        VSASSERT(!m_pMem, "Memory leak detected");
        return &m_pMem;
    }

    void Attach(T * pMem)
    {
        Destroy();
        m_pMem = pMem;
    }

    T * Detach()
    {
        T * pRet = m_pMem;
        m_pMem = NULL;
        return pRet;
    }

    void Destroy()
    {
        DoDestroy(m_pMem);
        Detach();
    }

protected:
    virtual void DoDestroy(T * pMem)
    {
        if (pMem)
        {
            delete pMem;
        }
    }

private:

    T * m_pMem;
};

