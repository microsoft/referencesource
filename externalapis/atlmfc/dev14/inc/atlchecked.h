// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLCHECKED_H__
#define __ATLCHECKED_H__

#pragma once

#include <atldef.h>
#include <atlexcept.h>
#include <malloc.h>
#include <string.h>
#include <mbstring.h>
#include <wchar.h>
#include <tchar.h>
#include <stdlib.h>


#pragma pack(push,_ATL_PACKING)
namespace ATL
{

inline errno_t AtlCrtErrorCheck(_In_ errno_t nError)
{
	switch(nError)
	{
	case ENOMEM:
		AtlThrow(E_OUTOFMEMORY);
		break;
	case EINVAL:
	case ERANGE:
		AtlThrow(E_INVALIDARG);
		break;
	case 0:
	case STRUNCATE:
		break;
	default:
		AtlThrow(E_FAIL);
		break;
	}
	return nError;
}

/////////////////////////////////////////////////////////////////////////////
// Secure (Checked) CRT functions

namespace Checked
{

#ifdef _AFX
#define ATLMFC_CRT_ERRORCHECK(expr) AFX_CRT_ERRORCHECK(expr)
#else
#define ATLMFC_CRT_ERRORCHECK(expr) ATL_CRT_ERRORCHECK(expr)
#endif

inline void __cdecl memcpy_s(
	_Out_writes_bytes_to_(_S1max,_N)  void *_S1,
	_In_ size_t _S1max,
	_In_reads_bytes_(_N) const void *_S2,
	_In_ size_t _N)
{
	ATLMFC_CRT_ERRORCHECK(::memcpy_s(_S1, _S1max, _S2, _N));
}

inline void __cdecl wmemcpy_s(
	_Out_writes_to_(_N1,_N) wchar_t *_S1,
	_In_ size_t _N1,
	_In_reads_(_N) const wchar_t *_S2,
	_In_ size_t _N)
{
	ATLMFC_CRT_ERRORCHECK(::wmemcpy_s(_S1, _N1, _S2, _N));
}

inline void __cdecl memmove_s(
	_Out_writes_bytes_to_(_S1max,_N) void *_S1,
	_In_ size_t _S1max,
	_In_reads_bytes_(_N) const void *_S2,
	_In_ size_t _N)
{
	ATLMFC_CRT_ERRORCHECK(::memmove_s(_S1, _S1max, _S2, _N));
}

inline void __cdecl strcpy_s(
	_Out_writes_z_(_S1max) char *_S1,
	_In_ size_t _S1max,
	_In_z_ const char *_S2)
{
	ATLMFC_CRT_ERRORCHECK(::strcpy_s(_S1, _S1max, _S2));
}

inline void __cdecl wcscpy_s(
	_Out_writes_z_(_S1max) wchar_t *_S1,
	_In_ size_t _S1max,
	_In_z_ const wchar_t *_S2)
{
	ATLMFC_CRT_ERRORCHECK(::wcscpy_s(_S1, _S1max, _S2));
}

inline void __cdecl tcscpy_s(
	_Out_writes_z_(_SizeInChars) TCHAR * _Dst,
	_In_ size_t _SizeInChars,
	_In_z_ const TCHAR * _Src)
{
	ATLMFC_CRT_ERRORCHECK(::_tcscpy_s(_Dst, _SizeInChars, _Src));
}

inline errno_t __cdecl strncpy_s(
	_Out_writes_z_(_SizeInChars) char *_Dest,
	_In_ size_t _SizeInChars,
	_In_z_ const char *_Source,
	_In_ size_t _Count)
{
	return ATLMFC_CRT_ERRORCHECK(::strncpy_s(_Dest, _SizeInChars, _Source,_Count));
}

inline errno_t __cdecl wcsncpy_s(
	_Out_writes_z_(_SizeInChars) wchar_t *_Dest,
	_In_ size_t _SizeInChars,
	_In_z_ const wchar_t *_Source,
	_In_ size_t _Count)
{
	return ATLMFC_CRT_ERRORCHECK(::wcsncpy_s(_Dest, _SizeInChars, _Source,_Count));
}

inline errno_t __cdecl tcsncpy_s(
	_Out_writes_z_(_SizeInChars) TCHAR *_Dest,
	_In_ size_t _SizeInChars,
	_In_z_ const TCHAR *_Source,
	_In_ size_t _Count)
{
	return ATLMFC_CRT_ERRORCHECK(::_tcsncpy_s(_Dest, _SizeInChars, _Source,_Count));
}

inline void __cdecl strcat_s(
	_Inout_updates_z_(_SizeInChars) char * _Dst,
	_In_ size_t _SizeInChars,
	_In_z_ const char * _Src)
{
	ATLMFC_CRT_ERRORCHECK(::strcat_s(_Dst, _SizeInChars, _Src));
}

inline void __cdecl wcscat_s(
	_Inout_updates_z_(_SizeInChars) wchar_t * _Dst,
	_In_ size_t _SizeInChars,
	_In_z_ const wchar_t * _Src)
{
	ATLMFC_CRT_ERRORCHECK(::wcscat_s(_Dst, _SizeInChars, _Src));
}

inline void __cdecl tcscat_s(
	_Inout_updates_z_(_SizeInChars) TCHAR * _Dst,
	_In_ size_t _SizeInChars,
	_In_z_ const TCHAR * _Src)
{
	ATLMFC_CRT_ERRORCHECK(::_tcscat_s(_Dst, _SizeInChars, _Src));
}

inline void __cdecl strlwr_s(
	_Inout_updates_z_(_SizeInChars) char * _Str,
	_In_ size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_strlwr_s(_Str, _SizeInChars));
}

inline void __cdecl wcslwr_s(
	_Inout_updates_z_(_SizeInChars) wchar_t * _Str,
	_In_ size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_wcslwr_s(_Str, _SizeInChars));
}

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
inline void __cdecl mbslwr_s(
	_Inout_updates_z_(_SizeInChars) unsigned char * _Str,
	_In_ size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_mbslwr_s(_Str, _SizeInChars));
}
#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

inline void __cdecl tcslwr_s(
	_Inout_updates_z_(_SizeInChars) TCHAR * _Str,
	_In_ size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_tcslwr_s(_Str, _SizeInChars));
}

inline void __cdecl strupr_s(
	_Inout_updates_z_(_SizeInChars) char * _Str,
	_In_ size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_strupr_s(_Str, _SizeInChars));
}

inline void __cdecl wcsupr_s(
	_Inout_updates_z_(_SizeInChars) wchar_t * _Str,
	_In_ size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_wcsupr_s(_Str, _SizeInChars));
}

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
inline void __cdecl mbsupr_s(
	_Inout_z_bytecap_(_SizeInChars) unsigned char * _Str,
	_In_ size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_mbsupr_s(_Str, _SizeInChars));
}
#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

inline void __cdecl tcsupr_s(
	_Inout_updates_z_(_SizeInChars) TCHAR * _Str,
	_In_ size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_tcsupr_s(_Str, _SizeInChars));
}

inline void __cdecl itoa_s(
	_In_ int _Val,
	_Out_writes_z_(_SizeInChars) char *_Buf,
	_In_ size_t _SizeInChars,
	_In_ int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_itoa_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl itot_s(
	_In_ int _Val,
	_Out_writes_z_(_SizeInChars) TCHAR *_Buf,
	_In_ size_t _SizeInChars,
	_In_ int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_itot_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl ltoa_s(
	_In_ long _Val,
	_Out_writes_z_(_SizeInChars) char *_Buf,
	_In_ size_t _SizeInChars,
	_In_ int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_ltoa_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl ltot_s(
	_In_ long _Val,
	_Out_writes_z_(_SizeInChars) TCHAR *_Buf,
	_In_ size_t _SizeInChars,
	_In_ int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_ltot_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl ultoa_s(
	_In_ unsigned long _Val,
	_Out_writes_z_(_SizeInChars) char *_Buf,
	_In_ size_t _SizeInChars,
	_In_ int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_ultoa_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl ultow_s(
	_In_ unsigned long _Val,
	_Out_writes_z_(_SizeInChars) wchar_t *_Buf,
	_In_ size_t _SizeInChars,
	_In_ int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_ultow_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl ultot_s(
	_In_ unsigned long _Val,
	_Out_writes_z_(_SizeInChars) TCHAR *_Buf,
	_In_ size_t _SizeInChars,
	_In_ int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_ultot_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl i64toa_s(
	_In_ __int64 _Val,
	_Out_writes_z_(_SizeInChars) char *_Buf,
	_In_ size_t _SizeInChars,
	_In_ int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_i64toa_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl i64tow_s(
	_In_ __int64 _Val,
	_Out_writes_z_(_SizeInChars) wchar_t *_Buf,
	_In_ size_t _SizeInChars,
	_In_ int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_i64tow_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl ui64toa_s(
	_In_ unsigned __int64 _Val,
	_Out_writes_z_(_SizeInChars) char *_Buf,
	_In_ size_t _SizeInChars,
	_In_ int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_ui64toa_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl ui64tow_s(
	_In_ unsigned __int64 _Val,
	_Out_writes_z_(_SizeInChars) wchar_t *_Buf,
	_In_ size_t _SizeInChars,
	_In_ int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_ui64tow_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl gcvt_s(
	_Out_writes_z_(_SizeInChars) char *_Buffer,
	_In_ size_t _SizeInChars,
	_In_ double _Value,
	_In_ int _Ndec)
{
	ATLMFC_CRT_ERRORCHECK(::_gcvt_s(_Buffer, _SizeInChars, _Value, _Ndec));
}

inline void __cdecl tsplitpath_s(
	_In_z_ const TCHAR *_Path,
	_Out_writes_opt_z_(_Drive_len) TCHAR *_Drive,
	_In_ size_t _Drive_len,
	_Out_writes_opt_z_(_Dir_len) TCHAR *_Dir,
	_In_ size_t _Dir_len,
	_Out_writes_opt_z_(_Fname_len) TCHAR *_Fname,
	_In_ size_t _Fname_len,
	_Out_writes_opt_z_(_Ext_len) TCHAR *_Ext,
	_In_ size_t _Ext_len)
{
	ATLMFC_CRT_ERRORCHECK(::_tsplitpath_s(_Path, _Drive, _Drive_len, _Dir, _Dir_len, _Fname, _Fname_len, _Ext, _Ext_len));
}

inline void __cdecl tmakepath_s(
	_Out_writes_z_(_SizeInChars) TCHAR *_Path,
	_In_ size_t _SizeInChars,
	_In_z_ const TCHAR *_Drive,
	_In_z_ const TCHAR *_Dir,
	_In_z_ const TCHAR *_Fname,
	_In_z_ const TCHAR *_Ext)
{
	ATLMFC_CRT_ERRORCHECK(::_tmakepath_s(_Path, _SizeInChars, _Drive, _Dir, _Fname, _Ext));
}

inline size_t __cdecl strnlen(
	_In_reads_z_(_Maxsize) const char *_Str,
	_In_ size_t _Maxsize)
{
	return ::strnlen(_Str, _Maxsize);
}

inline size_t __cdecl wcsnlen(
	_In_reads_z_(_Maxsize) const wchar_t *_Wcs,
	_In_ size_t _Maxsize)
{
	return ::wcsnlen(_Wcs, _Maxsize);
}

inline size_t __cdecl tcsnlen(
	_In_reads_z_(_Maxsize) const TCHAR *_Str,
	_In_ size_t _Maxsize)
{
	return ::_tcsnlen(_Str, _Maxsize);
}

inline int get_errno() throw()
{
	int nErrNo=0;
	errno_t nErrCall=::_get_errno(&nErrNo);
	if(nErrCall)
	{
		return nErrCall;
	}
	return nErrNo;
}

inline void set_errno(_In_ int _Value)
{
	ATLMFC_CRT_ERRORCHECK(::_set_errno(_Value));
}

} // namespace Checked

} // namespace ATL
#pragma pack(pop)

#endif // __ATLCHECKED_H__

/////////////////////////////////////////////////////////////////////////////
