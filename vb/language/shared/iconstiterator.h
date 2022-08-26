//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once

template <class T>
class IConstIterator
{
public:
    virtual
    ~IConstIterator() 
    {
    }

    virtual
    bool MoveNext() = 0;

    virtual
    T Current() = 0;
};

template <class T>
class ConstIterator : public virtual IConstIterator<T>
{
public:
    ConstIterator()
    {
    }

    explicit ConstIterator(RefCountedPtr<IConstIterator<T> > spIterator)
        : m_spIterator(spIterator)
    {

    }

    template <class U>
    ConstIterator(const U& it)
    {
        m_spIterator.Attach(new (zeromemory) U(it));
    }

    virtual __override
    bool MoveNext()
    {
        if ( m_spIterator)
        {
            return m_spIterator->MoveNext();
        }
        else
        {
            // NULL is used to represent an empty iterator
            return false;
        }
    }

    virtual __override
    T Current()
    {
        ThrowIfFalse2(m_spIterator, "Calling current on a NULL iterator");
        return m_spIterator->Current();
    }

private:

    // Allow default copy constructor and assignment operators to
    // be generated because RefCountedPtr is copy safe
    RefCountedPtr<IConstIterator<T> > m_spIterator;
};


