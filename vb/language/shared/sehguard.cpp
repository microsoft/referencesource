//-------------------------------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Seh guard
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

void __cdecl SehTranslatorFunctionNoWatson(unsigned int code, struct _EXCEPTION_POINTERS* exceptionPtrs)
{
    throw SehException(code);
}

SehGuard::SehGuard()
{
    m_previous = _set_se_translator(SehTranslatorFunctionNoWatson);
}

SehGuard::~SehGuard()
{
    _set_se_translator(m_previous);
}


