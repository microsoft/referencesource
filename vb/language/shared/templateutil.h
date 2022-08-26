//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Template Utilities 
//
//-------------------------------------------------------------------------------------------------

#pragma once

class TemplateUtil
{
public:
    template <class T>
    static 
    void Swap(_Inout_ T **left, _Inout_ T **right) throw()
    {
        T *temp = *right;
        *right = *left;
        *left = temp;
    }

    //-------------------------------------------------------------------------------------------------
    //
    // Useful CompileTime function that will allow you to assert an inheritance relationship between
    // two types.  Very helpful in certain template sceanrios.
    //
    // Use a static cast here because it will prevent operator consideration.
    //
    //-------------------------------------------------------------------------------------------------
    template <typename Parent, typename Child>
    static
    void CompileAssertIsChildOf()
    {
#if DEBUG
        Parent *pParent = NULL;
        Child *pChild = NULL;
        pParent = static_cast<Parent*>(pChild);
#endif
    }

    //-------------------------------------------------------------------------------------------------
    //
    // Several of our hashing algorithms require the size of a key be a multiple of the size of 
    // a pointer.  This function will prevent a compilation in DEBUG mode unless this is satisifed
    //
    //-------------------------------------------------------------------------------------------------
    template <typename T>
    static
    void CompileAssertSizeIsPointerMultiple()
    {
#if DEBUG
        COMPILE_ASSERT(0 == (sizeof(T) % sizeof(void*)));
#endif
    }

};

