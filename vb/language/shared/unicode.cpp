//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  This module provides handy functions which can be used to manipulate
//  WCHAR (UNICODE) and char (ANSI) strings.  Most of this deals with
//  conversion and dealing with the lack of UNICODE support in Win95.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

// case insensitive,  default lcid, compare entire string
bool WszEqNoCase(
    _In_z_ const WCHAR * wsz1,
    _In_z_ const WCHAR * wsz2)
{
    return(CSTR_EQUAL == 
        CompareStringW(
            LOCALE_USER_DEFAULT,
            NORM_IGNORECASE | NORM_IGNOREWIDTH,
            wsz1,
            -1,
            wsz2,
            -1));
}

// case insensitive,  default lcid,  compare first 'cch' characters
bool WszEqLenNoCase(
    _In_count_(cch)const WCHAR * wsz1,
    int cch,
    _In_z_ const WCHAR * wsz2)
{
    return(CSTR_EQUAL == 
        CompareStringW(
            LOCALE_USER_DEFAULT,
            NORM_IGNORECASE | NORM_IGNOREWIDTH,
            wsz1,
            cch,
            wsz2,
            -1));
}

#if ID_TEST

// Copy a unicode string into an ansi buffer
char * WszCpyToSz(
    _Out_z_cap_x_(strlen(wszSrc)+ 1)char * szDst,
    _In_z_ const WCHAR * wszSrc)
{
    VSASSERT(szDst != NULL && wszSrc != NULL, "");
    int cchSrc = (int)wcslen(wszSrc);
    int cchRet = WideCharToMultiByte(
        CP_ACP,
        0,
        wszSrc,
        cchSrc + 1,
        szDst,
        cchSrc * 2 + 1,
        NULL,
        NULL);
    VSASSERT(cchRet != 0, "WideCharToMultiByte failed");
    return szDst;
}

// Copy an ansi string into a unicode buffer
WCHAR * SzCpyToWsz(
    _Out_z_cap_x_((strlen(szSrc)+ 1)* sizeof(WCHAR))WCHAR * wszDst,
    _In_z_ const char * szSrc)
{
    VSASSERT(wszDst != NULL && szSrc != NULL, "");
    MultiByteToWideChar(CP_ACP, 0, szSrc, -1, wszDst, (int)(strlen(szSrc)+1));
    return wszDst;
}

#endif

// Converts binary stream into hex-encoded Unicode string.
// So an input stream of { 0x44, 0x55, 0x66 } would be converted
// to the string L"445566"
void BinaryToUnicode(
    BYTE * pbSrc,  // Binary stream to convert
    UINT cbSrc,  // Count of bytes in binary stream
    _Out_z_cap_x_(" At least cbSrc ")LPWSTR pwszDst /* Where to store converted string */)
{
    BYTE *pbLim = pbSrc + cbSrc;

// Convert value 0..15 into character 0..f
#define TOHEX(a) ((a)>=10 ? L'a'+(a)-10 : L'0'+(a))

    while (pbSrc < pbLim)
    {
        *pwszDst++ = TOHEX(*pbSrc >> 4);
        *pwszDst++ = TOHEX(*pbSrc & 0x0f);
        pbSrc++;
    }

#undef TOHEX

    *pwszDst = L'\0';
}

