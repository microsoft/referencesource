//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Resource wrapper around VbError
//
//-------------------------------------------------------------------------------------------------

#pragma once

class VbErrorWrapper: public VbError
{
public:
    NEW_CTOR_SAFE()

    VbErrorWrapper()
    {
        Clear();
    }

    ~VbErrorWrapper()
    {
        Destroy();
    }

    void Destroy()
    {
        ::SysFreeString(ItemName);
        ::SysFreeString(FileName);
        ::SysFreeString(Description);
        ::SysFreeString(SourceText);
        Clear();
    }

    void Detach(_Inout_ VbError *pDest)
    {
        ThrowIfNull(pDest);
        ClearVbError(pDest);
        Checked::memcpy_s(pDest, sizeof(VbError), this, sizeof(VbError));
        Clear();
    }

    void Copy(_In_ const VbError *pSource)
    {
        *((VbError*)this) = *pSource;
        ItemName = ComUtil::SafeSysAllocString(pSource->ItemName);
        FileName = ComUtil::SafeSysAllocString(pSource->FileName);
        Description = ComUtil::SafeSysAllocString(pSource->Description);
        SourceText = ComUtil::SafeSysAllocString(pSource->SourceText);
    }

    static void ClearVbError(_Inout_ VbError *pError)
    {
        ZeroMemory(pError, sizeof(VbError));
    }

private:
    // Do not auto generate
    VbErrorWrapper(const VbErrorWrapper&);
    VbErrorWrapper& operator=(const VbErrorWrapper&);

    void Clear()
    {
        ClearVbError(this);
    }
};

