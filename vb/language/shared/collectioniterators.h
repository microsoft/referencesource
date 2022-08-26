//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once

template <class T>
class DestroyableIterator : public virtual IIterator<T>
{
public:
    virtual
    void Destroy() = 0;
};


template <class T, class W, class Alloc = VBAllocWrapper>
class AllocDestroyableIterator : public virtual DestroyableIterator<T>
{
public:
    AllocDestroyableIterator(const Alloc & alloc) :
        m_alloc(alloc),
        m_Wrapped()
    {
    }

    template <class R1>
    AllocDestroyableIterator(
        const Alloc &alloc,
        R1 argument1) :
        m_alloc(alloc),
        m_Wrapped(argument1)
    {
    }

    template <class R1, class R2>
    AllocDestroyableIterator(
        const Alloc &alloc,
        R1 argument1,
        R2 argument2) :
        m_alloc(Alloc),
        m_Wrapped(argument1, argument2)
    {
    }

    template <class R1, class R2, class R3>
    AllocDestroyableIterator(
        const Alloc &alloc,
        R1 argument1,
        R2 argument2,
        R3 argument3) :
        m_alloc(alloc),
        m_Wrapped(argument1, argument2, argument3)
    {
    }

    template <class R1, class R2, class R3, class R4>
    AllocDestroyableIterator(
        const Alloc &alloc,
        R1 argument1,
        R2 argument2,
        R3 argument3,
        R4 argument4) :
        m_alloc(alloc),
        m_Wrapped(argument1, argument2, argument3, argument4)
    {
    }

    template <class R1, class R2, class R3, class R4, class R5>
    AllocDestroyableIterator(
        const Alloc &alloc,
        R1 argument1,
        R2 argument2,
        R3 argument3,
        R4 argument4,
        R5 argument5) :
        m_alloc(alloc),
        m_Wrapped(argument1, argument2, argument3, argument4, argument5)
    {
    }

    bool MoveNext()
    {
        return m_Wrapped.MoveNext();
    }

    T Current()
    {
        return m_Wrapped.Current();
    }

    void Remove()
    {
        m_Wrapped.Remove();
    }

    void Destroy()
    {
        m_alloc.DeAllocate(this);
    }
private:
    W m_Wrapped;
    Alloc m_alloc;
};


//====================================================================
// Allows for the easy creation of iterators that operate on constant
// collections.  It's OK to cast away const here as long as the
// MoveNext and Current methods on the base iterator don't actually
// change anything.  Most don't
//====================================================================
template <class T,class L, class I>
class ConstIteratorWrapper : public virtual IConstIterator<T>
{
public:
    ConstIteratorWrapper(const L* list)
        : m_it(const_cast<L*>(list))
    {
    }

    ConstIteratorWrapper(const L& list)
        : m_it(const_cast<L&>(list))
    {
    }

    ConstIteratorWrapper(const ConstIteratorWrapper<T,L,I> &src)
        : m_it(src.m_it)
    {
    }

    virtual
    bool MoveNext() 
    {
         return m_it.MoveNext(); 
    }

    virtual
    T Current() 
    {
         return m_it.Current(); 
    }

private:
    I m_it;
};
