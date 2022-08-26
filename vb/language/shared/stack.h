//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// A stack implemented on top of a dynamic array.
//-------------------------------------------------------------------------------------------------

template <class TYPE>
class DynamicArray;

template<class TYPE>
class Stack
{
public:
    NEW_CTOR_SAFE()

    // Construction.
    Stack()
    {
    }

    // Makes sure that this goes away.
    ~Stack()
    {
    }

    // Throw away all used memory.
    void Destroy()
    {
        m_daValues.Destroy();
    }

    // Reset the stack without throwing away
    // its memory.
    //
    void Reset()
    {
        m_daValues.Reset();
    }

    // Get the number of elements in the stack.
    ULONG Count()
    {
        return m_daValues.Count();
    }

    // Returns true if the stack is empty or false if it is not.
    bool Empty()
    {
        return !m_daValues.Count();
    }

    // Get the top element of the stack.
    TYPE &Top()
    {
        return m_daValues.End();
    }

    // Returns any element in the stack. This is zero based, so the
    // first element to be pushed onto the stack is the 0'th element,
    // the next is the 1st, and the last is n-1.
    TYPE &Element(unsigned iElement)
    {
        VSASSERT(iElement < Count(),"Stack element out of range");
        return m_daValues.Element(iElement);
    }

    // Pop the top element off the stack.
    void Pop()
    {
        m_daValues.Shrink(1);
    }

    // Push a new element onto the stack.
    void Push(const TYPE &t)
    {
        m_daValues.Add() = t;
    }

    void CopyTo(_Inout_ Stack<TYPE> *dest)
    {
        for (ULONG i = 0; i < this->Count(); i++)
        {
            dest->Push(this->Element(i));
        }
    }

private:
    DynamicArray<TYPE> m_daValues;
};

