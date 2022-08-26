//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Declarations that are exposed by VsPackage to Debugger and Compiler components.
//
//-------------------------------------------------------------------------------------------------

#pragma once

// STRID_EE* string resources are used by Debugger.
#include "..\VsPackage\IDEResources.h"

// The following 2 functions are used by CDebugParsedExpression::EvaluateSync from Debugger.
namespace IDEHelpers
{
    BSTR TrimWhitespaces(LPCOLESTR lpszText);
    
    inline
    bool IsBlankOrLineBreak(WCHAR wch)
    {
        return IsBlank(wch) || wch == UCH_CR || wch == UCH_LF;
    }
};
