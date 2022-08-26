// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLCOMMEM_H__
#define __ATLCOMMEM_H__

#pragma once

#ifndef __ATLMEM_H__
	#error ATLComMem.h requires atlmem.h to be included first
#endif	// __ATLMEM_H__

#include <atlcomcli.h>

#pragma pack(push,_ATL_PACKING)
namespace ATL
{

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

class CComHeap :
	public IAtlMemMgr
{
// IAtlMemMgr
public:
	virtual _Ret_maybenull_ _Post_writable_byte_size_(nBytes) _ATL_DECLSPEC_ALLOCATOR void* Allocate(_In_ size_t nBytes) throw()
	{
#ifdef _WIN64
		if( nBytes > INT_MAX )
		{
			return( NULL );
		}
#endif
		return( ::CoTaskMemAlloc( ULONG( nBytes ) ) );
	}
	virtual void Free(_In_opt_ void* p) throw()
	{
		::CoTaskMemFree( p );
	}
	virtual _Ret_maybenull_ _Post_writable_byte_size_(nBytes) _ATL_DECLSPEC_ALLOCATOR void* Reallocate(
		_In_opt_ void* p,
		_In_ size_t nBytes) throw()
	{
#ifdef _WIN64
		if( nBytes > INT_MAX )
		{
			return( NULL );
		}
#endif
		return( ::CoTaskMemRealloc( p, ULONG( nBytes ) ) );
	}
	virtual size_t GetSize(_In_opt_ void* p) throw()
	{
		CComPtr< IMalloc > pMalloc;

		HRESULT hr = ::CoGetMalloc( 1, &pMalloc );
		if (FAILED(hr))
			return 0;

		return( pMalloc->GetSize( p ) );
	}
};

#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

/////////////////////////////////////////////////////////////////////////////
// OLE task memory allocation support

inline LPWSTR AtlAllocTaskWideString(
	_In_opt_z_ LPCWSTR lpszString) throw()
{
	if (lpszString == NULL)
	{
		return NULL;
	}

	size_t nSize = 0;
	HRESULT hr = ::ATL::AtlAdd<size_t>(&nSize, wcslen(lpszString), 1);
	if (FAILED(hr))
	{
		return NULL;
	}

	hr = ::ATL::AtlMultiply(&nSize, nSize, sizeof(wchar_t));
	if (FAILED(hr))
	{
		return NULL;
	}

	LPWSTR lpszResult = (LPWSTR)CoTaskMemAlloc(nSize);
	if (lpszResult == NULL)
	{
		return NULL;
	}

	if(0 != memcpy_s(lpszResult, nSize, lpszString, nSize))
	{
		CoTaskMemFree(lpszResult);
		return NULL;
	}

	return lpszResult;
}

inline LPWSTR AtlAllocTaskWideString(
	_In_opt_z_ LPCSTR lpszString) throw()
{
	if (lpszString == NULL)
	{
		return NULL;
	}

	size_t strLen = strlen(lpszString);
	if (strLen > ::ATL::AtlLimits<int>::_Max)
	{
		return NULL;
	}

	int cch = 0;
	HRESULT hr = ::ATL::AtlAdd<int>(&cch, static_cast<int>(strLen), 1);
	if (FAILED(hr))
	{
		return NULL;
	}

	size_t cb = 0;
	hr = ::ATL::AtlMultiply<size_t>(&cb, cch, sizeof(wchar_t));
	if (FAILED(hr))
	{
		return NULL;
	}

	LPWSTR lpszResult = (LPWSTR)CoTaskMemAlloc(cb);
	if (lpszResult != NULL)
	{
		int nRet = MultiByteToWideChar(CP_ACP, 0, lpszString, -1, lpszResult, cch);
		ATLASSERT(nRet != 0);
		if (nRet == 0)
		{
ATLPREFAST_SUPPRESS(6102)
            CoTaskMemFree(lpszResult);
ATLPREFAST_UNSUPPRESS()
			lpszResult = NULL;
		}
	}
	return lpszResult;
}

inline LPSTR AtlAllocTaskAnsiString(
	_In_opt_z_ LPCWSTR lpszString) throw()
{
	if (lpszString == NULL)
	{	
		return NULL;
	}

	size_t nStrLen = wcslen(lpszString);
	if (nStrLen > ::ATL::AtlLimits<int>::_Max)
	{
		return NULL;
	}
	
	int nBytes = 0;
	HRESULT hr = ::ATL::AtlAdd<int>(&nBytes, static_cast<int>(nStrLen), 1);
	if (FAILED(hr))
	{
		return NULL;
	}

	hr = ::ATL::AtlMultiply<int>(&nBytes, nBytes, sizeof(wchar_t));
	if (FAILED(hr))
	{
		return NULL;
	}	

	LPSTR lpszResult = (LPSTR)CoTaskMemAlloc(nBytes);
	if (lpszResult != NULL)
	{
		int nRet = WideCharToMultiByte(CP_ACP, 0, lpszString, -1, lpszResult, nBytes, NULL, NULL);
		ATLASSERT(nRet != 0);
		if (nRet == 0)
		{
ATLPREFAST_SUPPRESS(6102)
            CoTaskMemFree(lpszResult);
ATLPREFAST_UNSUPPRESS()
			lpszResult = NULL;
		}
	}
	return lpszResult;
}

inline LPSTR AtlAllocTaskAnsiString(
	_In_opt_z_ LPCSTR lpszString) throw()
{
	if (lpszString == NULL)
	{
		return NULL;
	}

	size_t nSize = 0;
	HRESULT hr = ::ATL::AtlAdd<size_t>(&nSize, strlen(lpszString), 1);
	if (FAILED(hr))
	{
		return NULL;
	}

	LPSTR lpszResult = (LPSTR)CoTaskMemAlloc(nSize);
	if (lpszResult == NULL)
	{
		return NULL;
	}

	if(0 != memcpy_s(lpszResult, nSize, lpszString, nSize))
	{
		CoTaskMemFree(lpszResult);
		return NULL;
	}
	return lpszResult;
}

#ifdef _UNICODE
	#define AtlAllocTaskString(x) AtlAllocTaskWideString(x)
#else
	#define AtlAllocTaskString(x) AtlAllocTaskAnsiString(x)
#endif

#define AtlAllocTaskOleString(x) AtlAllocTaskWideString(x)

}	// namespace ATL
#pragma pack(pop)

#endif	// __ATLCOMMEM_H__
