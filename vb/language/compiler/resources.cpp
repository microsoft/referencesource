//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Functions for dealing with resources and the international DLL.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

HINSTANCE g_hinstDll;

//---------------------------------------------------------------------------
// Cover for LoadString that loads from either the main DLL, or the
// international dll based on the resource ID value.
//---------------------------------------------------------------------------
// 


HRESULT ResLoadString(
    RESID resid,
    _Out_z_cap_(cchBuf)WCHAR * wszBuf,
    unsigned cchBuf)
{
    VB_ENTRY();

    // VS 523071
    // Exceptions should not escape this function (callers expect an HRESULT return value)
   wszBuf[0] = NULL;

    HINSTANCE hinst = FLOCALIZEDRESID(resid) ? Compiler::GetResourceDll() : g_hinstDll;

    int cchCopied = LoadString(hinst, resid, wszBuf, cchBuf);

    if (cchCopied == 0)
    {
        return GetLastHResultError();
    }

    VSASSERT(
        cchCopied > 0 && (unsigned)cchCopied < cchBuf - 1,
        "The resource string is likely larger than the expected maximum. It may be necessary to increase the maximum.");

    VB_EXIT();
}


//----------------------------------------------------------------------------
// Here is a version of LoadString which loads the string into a stringbuffer.
// The idea is that you pass in the buffer that will be updated to contain the string.
// And, as a convenience, the function also returns a WCHAR* pointer to that buffer.
//----------------------------------------------------------------------------
WCHAR *ResLoadString(RESID resid, StringBuffer *buf)
{
    if (buf==NULL)
    {
        VSFAIL("please pass a non-NULL StringBuffer to ResLoadString");
        return L"";
    }

    HRESULT hr;
    WCHAR wszLoad[CCH_MAX_LOADSTRING];

    IfFailGo(ResLoadString(resid, wszLoad, DIM(wszLoad)));
    buf->AppendString(wszLoad);
    return buf->GetString();

    Error:
    VSFAIL("Failed to load string");
    return L"";
}

//---------------------------------------------------------------------------
// Load a string and replace supplied string arguments. It requires at least 1
// (WCHAR *) be passed in as a 3rd arg.
//---------------------------------------------------------------------------
HRESULT __cdecl ResLoadStringRepl(
    RESID resid,
    StringBuffer * pbuf,
    ...)
{
    va_list ap;

    va_start(ap, pbuf);

    // ResLoadStringReplArgs() below expects to be passed the first WCHAR*
    // after cbBuf as its 4th arg. But, it also expects it to be the first
    // entry in the va_list passed as the 5th arg.
    // The way this was previously implemented didn't work on the Alpha.
    // This gets the first va_arg to pass as the 4th argument then resets the
    // va_list and passes the whole thing in as the 5th arg. v-mfork 7/14
    // Get first var arg.
    WCHAR * wszZero = va_arg(ap, WCHAR*);

    //reset ap to start right after cbBuf.
    va_end(ap);
    va_start(ap, pbuf);

    return ResLoadStringReplArgs(resid, pbuf, wszZero, ap);
}

//---------------------------------------------------------------------------
// Replace the |1 |2, etc. tokens in a given String. It requires at least 1
// (WCHAR *) be passed in as a 3rd arg.
//---------------------------------------------------------------------------
HRESULT __cdecl ResStringRepl(
    _In_z_ const WCHAR * wszIn,
    StringBuffer * pbuf,
    ...)
{
    va_list ap;

    va_start(ap, pbuf);

    // ResLoadStringReplArgs() below expects to be passed the first WCHAR*
    // after cbBuf as its 4th arg. But, it also expects it to be the first
    // entry in the va_list passed as the 5th arg.
    // The way this was previously implemented didn't work on the Alpha.
    // This gets the first va_arg to pass as the 4th argument then resets the
    // va_list and passes the whole thing in as the 5th arg. v-mfork 7/14
    // Get first var arg.
    WCHAR * wszZero = va_arg(ap, WCHAR*);

    //reset ap to start right after cbBuf.
    va_end(ap);
    va_start(ap, pbuf);

    return ResStringReplArgs((WCHAR *)wszIn, pbuf, wszZero, ap);
}

//---------------------------------------------------------------------------
// Replace strings in a buffer producing another buffer.  The placeholders
// in the string are identified by a bar character followed by a argument
// number (1-9).  For example, |2 would be replaced with the second argument
// passed to this function.  An argument is always specified as a WCHAR*.
// A placeholder number can appear more than one place in the string, and
// more importantly arguments can appear in a different order than the
// argument list for different locales (e.g. why we don't use % and sprintf).
//---------------------------------------------------------------------------
HRESULT ResStringReplArgs(
    _In_z_ WCHAR * wszIn,
    StringBuffer * pbuf,
    _In_opt_z_ WCHAR * wszRepl0,
    va_list ap)
{
    int cPlaceHoldersRead, cwch = 0;
    WCHAR* rgwszPlaceHolder[10], *wszCur = wszIn;

    cPlaceHoldersRead = 1;
    rgwszPlaceHolder[0] = wszRepl0;

    // In case this is used incorrectly, don't crash.
    WCHAR wszBadField[32];
    wszBadField[0] = 0;

    for (; *wszIn; wszIn++)
    {
        VSASSERT(wszIn[0] != WIDE('|') || wszIn[1], "Expected character following '|'");

        if (*wszIn == WIDE('|') && wszIn[1] >= WIDE('0') && wszIn[1] <= WIDE('9'))
        {
            if (cwch != 0)
            {
                pbuf->AppendWithLength(wszCur, cwch);
                cwch = 0;
            }
            wszCur = wszIn + 2;
            wszIn++;

            int iPlaceHolder = min(*wszIn - WIDE('0'), 9);

            while (cPlaceHoldersRead <= iPlaceHolder)
            {
                rgwszPlaceHolder[cPlaceHoldersRead++] = va_arg(ap, WCHAR*);
            }

            VSASSERT(rgwszPlaceHolder[iPlaceHolder], "No replacement string.");

            pbuf->AppendString( rgwszPlaceHolder[iPlaceHolder] );
        }
        else
        {
            cwch++;
        }
    }

    if (cwch != 0)
    {
        pbuf->AppendWithLength(wszCur, cwch);
    }

    return NOERROR;
}

//---------------------------------------------------------------------------
// A version of ResLoadStringRepl that takes a va_list.
//---------------------------------------------------------------------------
HRESULT ResLoadStringReplArgs(
    RESID resid,
    _In_z_ StringBuffer * pbuf,
    _In_opt_z_ WCHAR * wszRepl0,
    va_list ap)
{
    HRESULT hr;
    WCHAR wszLoad[CCH_MAX_LOADSTRING];

    IfFailGo(ResLoadString(resid, wszLoad, DIM(wszLoad)));
    IfFailGo(ResStringReplArgs(wszLoad, pbuf, wszRepl0, ap));

    Error:
    return hr;
}
