//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation a sequential name generator
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

SequentialNameGenerator::SequentialNameGenerator(
    Compiler * pCompiler,
    _In_ NorlsAllocator * pAllocator) :
    m_Compiler(pCompiler), 
        m_nra(pAllocator)
{
    m_BaseName = NULL;
    HighestSuffix = 0;
}

void SequentialNameGenerator::Init(_In_z_ const WCHAR * BaseName)
{
    m_BaseName = m_Compiler->AddString(BaseName);
    Clear();
}

void SequentialNameGenerator::Clear()
{
    m_ReuseQueue.Clear();
    m_NewList.Clear();
    HighestSuffix = 0;
}

STRING * SequentialNameGenerator::GetNextName()
{
    if (m_ReuseQueue.NumberOfEntries() == 0)
    {
        NameSuffix *Suffix = (NameSuffix *)m_nra->Alloc(sizeof(NameSuffix));
        Suffix->Value = HighestSuffix;
        ++HighestSuffix;
        return GetName(Suffix);
    }
    else
    {
        NameSuffix *Suffix = m_ReuseQueue.GetFirst();
        m_ReuseQueue.Remove(Suffix);
        return GetName(Suffix);
    }
}

STRING * SequentialNameGenerator::GetName(_In_ NameSuffix * Suffix)
{
    WCHAR Digits[MaxStringLengthForIntToStringConversion] = {0}; // e.g. long + trailing NULL
    IfFailThrow(StringCchPrintfW(Digits, DIM(Digits), L"%lu", Suffix->Value));

    m_NewList.InsertLast(Suffix);  // Track the newly added entry for transactional rollback purposes

    return m_Compiler->ConcatStrings(m_BaseName, WIDE("_"), Digits);
}

void SequentialNameGenerator::CommitNewNames()
{
    m_NewList.Clear();  // Make all allocated name since last commit final.
}

void SequentialNameGenerator::AbortNewNames()
{
    m_ReuseQueue.Splice(&m_NewList); // All allocated name since last commit need to be reused.
}
