//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// AutoArray 
//
//-------------------------------------------------------------------------------------------------

#pragma once

template <class T>
class AutoArray :
    public AutoPtr<T>
{
public:
    AutoArray(T * pArray) :
        AutoPtr(pArray)
    {
    }

    virtual __override
    ~AutoArray()
    {
        Destroy();
    }

protected:

    void DoDestroy(T * pMem)
    {
        if (pMem)
        {
            delete [] pMem;
        }
    }
};
