
//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Uses the standard VB Common heap for its operations
//
//-------------------------------------------------------------------------------------------------

#pragma once

class VBAllocWrapper
{
public:
    template <class T> T * Allocate()
    {
        return new (zeromemory) T();
    }

    template <class T, class R1>
    T * Allocate(R1 argument1)
    {
        return new (zeromemory) T(argument1);
    }

    template <class T, class R1, class R2>
    T * Allocate(
        R1 argument1,
        R2 argument2)
    {
        return new (zeromemory) T(argument1, argument2);
    }

    template <class T, class R1, class R2, class R3>
    T * Allocate(
        R1 argument1,
        R2 argument2,
        R3 argument3)
    {
        return new (zeromemory) T(argument1, argument2, argument3);
    }

    template <class T, class R1, class R2, class R3, class R4>
    T * Allocate(
        R1 argument1,
        R2 argument2,
        R3 argument3,
        R4 argument4)
    {
        return new (zeromemory) T(argument1, argument2, argument3, argument4);
    }

    template <class T, class R1, class R2, class R3, class R4, class R5>
    T * Allocate(
        R1 argument1,
        R2 argument2,
        R3 argument3,
        R4 argument4,
        R5 argument5)
    {
        return new (zeromemory) T(argument1, argument2, argument3, argument4, argument5);
    }

    template <class T>
    T * AllocateArray(unsigned long count)
    {
        return new (zeromemory) T[count];
    }

    template <class T>
    void DeAllocate(T * pData)
    {
        delete pData;
    }

    template <class T>
    void DeAllocateArray(T * pArray)
    {
        delete [] pArray;
    }

};

