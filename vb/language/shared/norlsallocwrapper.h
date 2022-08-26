//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class NorlsAllocWrapper
{
public:
    NorlsAllocWrapper(_In_ NorlsAllocator * pNorls) : 
        m_pNorlsAlloc(pNorls)
    {
        Assume(pNorls, L"An attempt was made to create a NorlsAllocWrapper with a null norls allocator");
    }

    NorlsAllocWrapper(const NorlsAllocWrapper &src) : 
        m_pNorlsAlloc(src.m_pNorlsAlloc)
    {
    }

    template <class T>
    T * Allocate()
    {
        void * pRet = m_pNorlsAlloc->Alloc(sizeof(T));
        pRet = new (pRet) T ();
        return reinterpret_cast<T *>(pRet);
    }

    template <class T, class R1>
    T * Allocate(R1 argument1)
    {
        void * pRet = m_pNorlsAlloc->Alloc(sizeof(T));
        pRet = new (pRet) T (argument1);
        return reinterpret_cast<T *>(pRet);
    }

    template <class T, class R1, class R2>
    T * Allocate(
        R1 argument1,
        _In_ R2 argument2)
    {
        void * pRet = m_pNorlsAlloc->Alloc(sizeof(T));
        pRet = new (pRet) T (argument1, argument2);
        return reinterpret_cast<T *>(pRet);

    }

    template <class T, class R1, class R2, class R3>
    T * Allocate(
        R1 argument1,
        R2 argument2,
        R3 argument3)
    {
        void * pRet = m_pNorlsAlloc->Alloc(sizeof(T));
        pRet = new (pRet) T (argument1, argument2, argument3);
        return reinterpret_cast<T *>(pRet);

    }

    template <class T, class R1, class R2, class R3, class R4>
    T * Allocate(
        R1 argument1,
        R2 argument2,
        R3 argument3,
        R4 argument4)
    {
        void * pRet = m_pNorlsAlloc->Alloc(sizeof(T));
        pRet = new (pRet) T (argument1, argument2, argument3, argument4);
        return reinterpret_cast<T *>(pRet);

    }

    template <class T, class R1, class R2, class R3, class R4, class R5>
    T * Allocate(
        R1 argument1,
        R2 argument2,
        R3 argument3,
        R4 argument4,
        R5 argument5)
    {
        void * pRet = m_pNorlsAlloc->Alloc(sizeof(T));
        pRet = new (pRet) T (argument1, argument2, argument3, argument4, argument5);
        return reinterpret_cast<T *>(pRet);

    }

    template <class T>
    T * AllocateArray(unsigned long count)
    {
        void * pRet = NULL;

        if (VBMath::TryMultiply(sizeof(T), count))
        {
            pRet = m_pNorlsAlloc->Alloc(sizeof(T)*count);
            new (pRet) T[count];
        }

        return reinterpret_cast<T *>(pRet);
    }

    template <class T>
    void DeAllocate(T * pData)
    {
        //we do nothing
        //infact we don't even run the destructor.
        //This is because we are using the norls allocator, whic means
        //means memory is never released, and no destructors are ever run.
    }

    template <class T>
    void DeAllocateArray(T * pArray)
    {
        //we do nothing
        //infact we don't even run the destructor.
        //This is because we are using the norls allocator, whic means
        //means memory is never released, and no destructors are ever run.
    }

private:
    NorlsAllocator * m_pNorlsAlloc;
};
