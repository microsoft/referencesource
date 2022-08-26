// awconv.h - ANSI/Unicode conversion
//-----------------------------------------------------------------
// Microsoft Confidential
// Copyright 1998 Microsoft Corporation.  All Rights Reserved.
//
// June 1, 1998 [paulde]
//
//---------------------------------------------------------------
// cb  == count of bytes
// cch == count of characters

#pragma once
#ifndef __AWCONV_H__
#define __AWCONV_H__
#include "unistr.h"

// To avoid conflicts between malloc.h and vsmem.h, declare _alloca here
//#include "malloc.h"
extern "C"  void * __cdecl _alloca(size_t);
#pragma intrinsic(_alloca)
#if !defined(_M_CEE)
#pragma intrinsic(strlen)
#endif

// for docs, see comment section below

#define cbWideForAnsiSize(cbAnsi)    ((cbAnsi)*sizeof(WCHAR))
#define cchWideForAnsiSize(cbAnsi)   (cbAnsi)
#define cbAnsiForWideSize(cbWide)    (cbWide)

// Win64Fix (Microsoft): strlen returns size_t which is 64-bit long.
// In this particular case I think int is sufficient and I won't need to fix ann the calls to the StrLenA
inline int cbWideForAnsiStr  (_In_z_ LPCSTR  sz) { return ((int)strlen(sz) + 1) * sizeof(WCHAR); }
inline int cchWideForAnsiStr (_In_z_ LPCSTR  sz) { return ((int)strlen(sz) + 1); }
inline int cbAnsiForWideStr  (_In_z_ LPCWSTR sz) { return ((int)wcslen(sz) * sizeof(WCHAR)) + 1; }

PWSTR   WINAPI CopyWFromAN  (_Out_z_cap_(cchdest) PWSTR dest, size_t cchdest, _In_opt_z_ PCSTR  src);
PSTR    WINAPI CopyAFromWN  (_Out_z_cap_(cchdest) PSTR  dest, size_t cchdest, _In_opt_z_ PCWSTR src);

HRESULT WINAPI StrDupW      (_In_opt_z_ PCWSTR psz, _Deref_out_z_ PWSTR * ppszNew); // free with VSFree
HRESULT WINAPI StrDupA      (_In_opt_z_ PCSTR  psz, _Deref_out_z_ PSTR  * ppszNew); // free with VSFree
HRESULT WINAPI StrDupAFromW (_In_opt_z_ PCWSTR psz, _Deref_out_z_ PSTR  * ppszNew); // free with VSFree
HRESULT WINAPI StrDupWFromA (_In_opt_z_ PCSTR  psz, _Deref_out_z_ PWSTR * ppszNew); // free with VSFree

// cchMax is the max number of chars to duplicate into the new string,
// not including a trailing 0
HRESULT WINAPI StrNDupW (_In_opt_ _Pre_count_(cchMax) PCWSTR pszSrc, unsigned int cchMax,
                         _Deref_out_z_ PWSTR * ppszCopy);
BSTR    WINAPI SysNDup  (_In_opt_ _Pre_count_(cchMax) PCWSTR psz, unsigned int cchMax); // free with SysFreeString

#ifndef UNICODE

// Same as StrDupW, but conflicts with shlwapi.h which defines 
// StrDup as an A/W macro and breaks us.
HRESULT WINAPI StrDup       (PCWSTR psz, _Out_opt_ PWSTR * ppszNew); // free with VSFree

#endif //UNICODE

BSTR    WINAPI SysAllocStringFromA (_In_z_ PCSTR psz);

//---------------------------------------------------------------
// WIDESTR/ANSISTR
//
// WARNING:
// These macros use _alloca, which causes stack growth at EACH invocation.
// DO NOT use WIDESTR and ANSISTR in a loop.
// DO NOT use WIDESTR and ANSISTR with long strings.
//

#ifndef WIDESTR

#define WIDESTR(x)  ((HIWORD((UINT_PTR)(x)) != 0) ? ( (strlen(x) > (UINT_MAX / 2) - 1) ? NULL : CopyWFromAN((LPWSTR) _alloca((strlen(x)+1) * 2), strlen(x)+1, (x)) ) : (LPWSTR)(x))
#endif

#ifndef ANSISTR
#define ANSISTR(x)  ((HIWORD((UINT_PTR)(x)) != 0) ? CopyAFromWN((LPSTR) _alloca((StrLen(x)*2)+1), (StrLen(x)*2)+1, (x)) : (LPSTR)(x))  // *2 for worst case DBCS string
#endif

//---------------------------------------------------------------

#ifndef UNICODE

inline HRESULT WINAPI StrDup (PCWSTR sz, _Out_opt_ PWSTR * ppszNew)
{
    return StrDupW (sz, ppszNew);
}

#endif //UNICODE

//---------------------------------------------------------------
// cbWideForAnsiSize
//
// Returns: Max size of a WCHAR buffer required to hold the result
//          of converting cbAnsi bytes of an ANSI string to Unicode.
//
// This number is suitable for direct use as an argument to an allocator.
//
// Max count of wide characters that corresponds to an ANSI buffer of size cbAnsi
//

//---------------------------------------------------------------
// cbWideForAnsiStr
// 
// Returns: Max size of a WCHAR buffer required to hold the result
//          of converting sz to Unicode.
//
// This number is suitable for direct use as an argument to allocators
//

//---------------------------------------------------------------
//
// PWSTR CopyWFromA (PWSTR dest, LPCSTR src);
//
// Copy an ANSI string to a Unicode (wide) string. The destination
// is assumed to be large enough for the result.
//
// Returns: dest
//

//---------------------------------------------------------------
// PSTR  CopyAFromW (PSTR dest, PCWSTR src);
//
// Copy a Unicode (wide) string to an ANSI string. The destination
// is assumed to be large enough for the result.
//
// Returns: dest
//
#endif // __AWCONV_H__
