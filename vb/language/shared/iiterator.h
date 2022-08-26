//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once


template <class T>
class IIterator : public virtual IConstIterator<T>
{
public:
    virtual
    ~IIterator()
    {
    }
    
    virtual
    void Remove() = 0;
};

template <class T>
class Iterator : public virtual IIterator<T>
{
public:
    Iterator()
    {
    }

    explicit Iterator(RefCountedPtr<IIterator<T> > spIterator)
        : m_spIterator(spIterator)
    {

    }

    template <class U>
    Iterator(const U& it)
    {
        m_spIterator.Attach(new (zeromemory) U(it));
    }

    virtual __override
    bool MoveNext()
    {
        if ( m_spIterator )
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
        ThrowIfFalse2(m_spIterator, "Current called on an empty iterator");
        return m_spIterator->Current();
    }
    
    virtual __override
    void Remove()
    {
        ThrowIfFalse2(m_spIterator, "Remove called on an empty iterator");
        m_spIterator->Remove();
    }

private:
    // Allow default copy constructor and assignment operators to
    // be generated because RefCountedPtr is copy safe
    RefCountedPtr<IIterator<T> > m_spIterator;
};

