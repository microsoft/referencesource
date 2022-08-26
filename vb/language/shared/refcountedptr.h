//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Ref Counted Pointer.
//
// Only safe for single threaded operations
//
//-------------------------------------------------------------------------------------------------

#pragma once

template <class T>
class RefCountedPtr
{
private:
    struct Data
    {
        NEW_CTOR_SAFE()

        Data() :
            pData(NULL),
            Count(0)
        {
        }
            
        ~Data()
        {
            if ( pData )
            {
                delete pData;
            }
        }

        T *pData;
        SafeInt<unsigned int> Count;
    };

public:

    RefCountedPtr() 
        : m_pData(NULL)
    {

    }

    explicit RefCountedPtr(T* pData)
    {
        m_pData = new Data();
        m_pData->pData = pData;
        m_pData->Count = 1;
    }

    RefCountedPtr(const RefCountedPtr<T> &other) 
        : m_pData(NULL)
    {
        // Increment count before assigning the pointer.  Pointer assignment is
        // exception safe but incrementing the count is not.  If incrementing the 
        // count overflows then the count would be N and the number of references
        // would be N+1 resulting in a double free.
        if ( other.m_pData )
        {
            other.m_pData->Count +=1;
            m_pData = other.m_pData;
        }
    }
    
    ~RefCountedPtr()
    {
        Release();
    }

    operator T*() const
    {
        return m_pData ? m_pData->pData : NULL;
    }

    T& operator *() const
    {
        VSASSERT(m_pData, "Accessing NULL Pointer");
        return *(m_pData->pData);
    }

    T* operator ->()
    {
        VSASSERT(m_pData, "Accessing NULL Pointer");
        return m_pData->pData; 
    }

    const T* operator ->() const
    {
        VSASSERT(m_pData, "Accessing NULL Pointer");
        return m_pData->pData; 
    }

    const RefCountedPtr<T>& operator=(const RefCountedPtr<T>& other)
    {
        RefCountedPtr<T> temp(other);
        TemplateUtil::Swap(&m_pData, &temp.m_pData);
        return *this;
    }

    const RefCountedPtr<T>& operator=(const T& other)
    {
        RefCountedPtr<T> temp(new T(other));
        TemplateUtil::Swap(&m_pData, &temp.m_pData);
        return *this;
    }

    const RefCountedPtr<T>& operator=(T* ptr)
    {
        RefCountedPtr<T> temp(ptr);
        TemplateUtil::Swap(&m_pData, &temp.m_pData);
        return *this;
    }

    void Attach( _In_ T *pData)
    {
        Release();
        RefCountedPtr<T> temp(pData);
        *this = temp;
    }

    void Release()
    {
        if ( m_pData )
        {
            VSASSERT(m_pData->Count.Value() != 0, "RefCountedPtr count error");

            m_pData->Count -= 1;
            if ( 0 == m_pData->Count.Value() )
            {   
                delete m_pData;
            }
            m_pData = NULL;
        }
    }

    T* GetData() const
    {
        return (m_pData) ? m_pData->pData : NULL;
    }

private:

    void RemoveReference()
    {
    }

    Data* m_pData;
};
