//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class SequentialNameGenerator
{
public:
    SequentialNameGenerator(
        Compiler * pCompiler,
        _In_ NorlsAllocator * pAllocator);

    void Init(_In_z_ const WCHAR *BaseName);
    void Clear();
    STRING *GetNextName();
    void CommitNewNames();
    void AbortNewNames();
    
    ULONG GetHighestSuffix() 
    {
         return HighestSuffix; 
    }

    void SetHighestSuffix(ULONG Highest) 
    {
         HighestSuffix = Highest; 
    }

private :
    struct NameSuffix : public CSingleLink<NameSuffix>
    {
        ULONG Value;
    };

protected:
    STRING * GetName(_In_ NameSuffix * Suffix);

private:
    Compiler *m_Compiler;
    NorlsAllocator *m_nra;

    STRING *m_BaseName;

    ULONG HighestSuffix;
    CSingleList<NameSuffix> m_ReuseQueue;
    CSingleList<NameSuffix> m_NewList;
};
