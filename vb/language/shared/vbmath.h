//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  In place overflow safe math operations 
//
//-------------------------------------------------------------------------------------------------

#pragma once

class VBMath
{
public:

    template<typename T, typename U>
    static 
        T Multiply(
        T left, 
        U right)
    {
        SafeInt<T> total = left;
        total *= right;
        return total.Value();
    }

    template<typename T>
    static 
        bool TryMultiply(
        T left, 
        T right)
    {
        return TryMultiply<T>(left, right, NULL);
    }

    template <typename T, typename U>
    static
        bool TryMultiply(
        T left,
        U right )
    {
        return TryMultiply<T,U,T>(left, right, NULL);
    }

    template<typename T>
    static
        bool TryMultiply(
        T left, 
        T right, 
        _Out_opt_ T* result)
    {
        T localResult;
        HRESULT hr = ::ATL::AtlMultiply(&localResult, left, right);
        if ( result )
        {
            *result = SUCCEEDED(hr) ? localResult : 0;
        }

        return SUCCEEDED(hr);
    }

    template <typename T, typename U, typename V>
    static
        bool TryMultiply(
        T left,
        U right,
        _Out_opt_ V* result)
    {
        SafeInt<V> safeLeft = left;
        SafeInt<V> safeRight = right;
        return TryMultiply<V>(safeLeft.Value(), safeRight.Value(), result);
    }

    template <typename T, typename U>
    static 
        T Add(
        T left, 
        U right)
    {
        SafeInt<T> total = left;
        total += right;
        return total.Value();
    }

    template <typename T>
    static 
        bool TryAdd(
        T left, 
        T right)
    {
        return TryAdd(left, right, NULL);
    }

    template <typename T, typename U>
    static 
        bool TryAdd(
        T left, 
        U right)
    {
        return TryAdd<T,U,T>(left, right, NULL);
    }

    template <typename T>
    static 
        bool TryAdd(
        T left, 
        T right, 
        _Out_opt_ T *result)
    {
        T localResult;
        HRESULT hr = ::ATL::AtlAdd(&localResult, left, right);
        if ( result )
        {
            *result = SUCCEEDED(hr) ? localResult : 0;
        }

        return SUCCEEDED(hr);
    }   

    template <typename T, typename U, typename V>
    static 
        bool TryAdd(
        T left, 
        U right, 
        _Out_opt_ V *result)
    {
        SafeInt<V> safeLeft = left;
        SafeInt<V> safeRight = right;
        return TryAdd<V>(safeLeft.Value(), safeRight.Value(), result);
    }   

    template <typename T, typename U>
    static 
        bool Divide(
        T left, 
        U right)
    {
        SafeInt<T> total = left;
        total /= right;
        return total.Value();
    }

    template <typename T, typename U>
    static
        void Convert(
        _Inout_ T *left,
        _In_ U right)
    {
        SafeInt<T> v = right;
        *left = v.Value();
    }

    template <typename T, typename U>
    static
    T Convert(
        U value)
    {
        SafeInt<T> v = value;
        return v.Value();
    }

    template <typename T, typename U>
    static T Subtract(T left, U right)
    {
        SafeInt<T> v = SafeInt<T>(left) - right;
        return v.Value();
    }

    
    // This function rounds value up to the nearest radix.  It will throw if rounding up results in an overflow
    // If you need a version that doesn't throw, you can write a new RoundUp() following the TryAdd(), etc. pattern
    template <class T>
    static inline T RoundUp(
        T value, // the value to round up.  Note that this implementation can't round up a value at MAX_VALUE for the type,
                 // e.g. if T is integer the value 4gb rounded up to the nearest 2 is something you'd think would work,
                 // since it is just the same number (4gb) but it will overflow because of the way the implementation works.
        T radix) // !!! Note, this implementation assumes that radix (what we are rounding up to) is a power of 2
    {
        T mask = (T) (radix - 1);
        SafeInt<T> valueToRoundUp = value; // Dev10 678668-allow for possible overflow, such as trying to roundup 4294967295
        valueToRoundUp += mask;
          
        return static_cast<T> (valueToRoundUp.Value() & ~mask);
    }

    template <class T>
    static inline T RoundUpAllocSize(T value)
    {
        T alignmask = sizeof(void*);
        return RoundUp(value, static_cast<T> (alignmask));
    }

    template <class T>
    static inline T RoundUpAllocSizeTo(T value, int access)
    {
        return RoundUp(value, static_cast<T> (access));
    }

};


