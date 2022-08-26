// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLCONV_H__
#define __ATLCONV_H__

#pragma once

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (push)
#pragma warning(disable: 4127) // unreachable code
#pragma warning(disable: 4987) // nonstandard extension used: 'throw(...)'
#endif //!_ATL_NO_PRAGMA_WARNINGS

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#include <atldef.h>
#include <stddef.h>
#include <atlalloc.h>

#ifndef __wtypes_h__

#if !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_IX86)
#define _X86_
#endif

#if !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_AMD64)
#define _AMD64_
#endif

#if !defined(_X86_) && !defined(_M_IX86) && !defined(_AMD64_) && defined(_M_IA64)
#if !defined(_IA64_)
#define _IA64_
#endif // !_IA64_
#endif

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>

#if !defined(OLE2ANSI)

typedef WCHAR OLECHAR;
typedef _Null_terminated_ OLECHAR  *LPOLESTR;
typedef _Null_terminated_ const OLECHAR  *LPCOLESTR;
#define OLESTR(str) L##str

#else

typedef char      OLECHAR;
typedef LPSTR     LPOLESTR;
typedef LPCSTR    LPCOLESTR;
#define OLESTR(str) str

#endif	// !OLE2ANSI
#endif	// __wtypes_h__

#ifndef _OLEAUTO_H_
typedef LPWSTR BSTR;// must (semantically) match typedef in oleauto.h

extern "C"
{
__declspec(dllimport) _Ret_maybenull_z_ BSTR __stdcall SysAllocString(_In_opt_z_ const OLECHAR *);
__declspec(dllimport) _Ret_maybenull_z_ BSTR __stdcall SysAllocStringLen(
	_In_reads_z_(nLen) const OLECHAR *, 
	_In_ UINT nLen);
__declspec(dllimport) INT  __stdcall SysReAllocStringLen(
	_Outptr_result_maybenull_z_ BSTR*, 
	_In_reads_opt_z_(nLen) const OLECHAR *, 
	_In_ UINT nLen);
__declspec(dllimport) void __stdcall SysFreeString(_In_opt_z_ BSTR);
}
#endif

// we use our own implementation of InterlockedExchangePointer because of problems with the one in system headers
#ifdef _M_IX86
#undef InterlockedExchangePointer
inline void* WINAPI InterlockedExchangePointer(
	_Inout_ void** pp, 
	_In_opt_ void* pNew) throw()
{
	return( reinterpret_cast<void*>(static_cast<LONG_PTR>(
		::InterlockedExchange(reinterpret_cast<LONG*>(pp), 
			static_cast<LONG>(reinterpret_cast<LONG_PTR>(pNew))))) );
}
#endif

#define ATLCONV_DEADLAND_FILL _SECURECRT_FILL_BUFFER_PATTERN

#pragma pack(push,_ATL_PACKING)
namespace ATL
{

inline UINT WINAPI _AtlGetConversionACP() throw()
{
#ifdef _CONVERSION_DONT_USE_THREAD_LOCALE
	return CP_ACP;
#else
	return CP_THREAD_ACP;
#endif
}

template <class _CharType>
inline void AtlConvAllocMemory(
	_Inout_ _Outptr_result_buffer_(nLength) _CharType** ppBuff,
	_In_ int nLength,
	_In_reads_(nFixedBufferLength) _CharType* pszFixedBuffer,
	_In_ int nFixedBufferLength)
{
	ATLENSURE_THROW(ppBuff != NULL, E_INVALIDARG);
	ATLENSURE_THROW(nLength >= 0, E_INVALIDARG);
	ATLENSURE_THROW(pszFixedBuffer != NULL, E_INVALIDARG);

	//if buffer malloced, try to realloc.
	if (*ppBuff != pszFixedBuffer)
	{
		if( nLength > nFixedBufferLength )
		{
			_CharType* ppReallocBuf = static_cast< _CharType* >( _recalloc(*ppBuff, nLength,sizeof( _CharType ) ) );
			if (ppReallocBuf == NULL) 
			{
				AtlThrow( E_OUTOFMEMORY );
			}
			*ppBuff = ppReallocBuf;
		} else
		{
			free(*ppBuff);
			*ppBuff=pszFixedBuffer;
		}

	} else //Buffer is not currently malloced.
	{
		if( nLength > nFixedBufferLength )
		{
			*ppBuff = static_cast< _CharType* >( calloc(nLength,sizeof( _CharType ) ) );
		} else
		{			
			*ppBuff=pszFixedBuffer;
		}
	}

	if (*ppBuff == NULL)
	{
		AtlThrow( E_OUTOFMEMORY );
	}
}

template <class _CharType>
inline void AtlConvFreeMemory(
	_Pre_maybenull_ _Post_invalid_ _CharType* pBuff,
	_Pre_notnull_ _Pre_writable_size_(nFixedBufferLength) _CharType* pszFixedBuffer,
	_In_ int nFixedBufferLength)
{
	(nFixedBufferLength);
	if( pBuff != pszFixedBuffer )
	{
		free( pBuff );
	} 	
#ifdef _DEBUG
	else
	{		
		memset(pszFixedBuffer,ATLCONV_DEADLAND_FILL,nFixedBufferLength*sizeof(_CharType));
	}
#endif
}

template< int t_nBufferLength = 128 >
class CW2WEX
{
public:
	CW2WEX(_In_z_ LPCWSTR psz) throw(...) :
		m_psz( m_szBuffer )
	{
		Init( psz );
	}
	CW2WEX(
			_In_z_ LPCWSTR psz, 
			_In_ UINT nCodePage) throw(...) :
		m_psz( m_szBuffer )
	{
		(void)nCodePage;  // Code page doesn't matter

		Init( psz );
	}
	~CW2WEX() throw()
	{
		AtlConvFreeMemory(m_psz,m_szBuffer,t_nBufferLength);
	}

	_Ret_z_ operator LPWSTR() const throw()
	{
		return( m_psz );
	}

private:
	void Init(_In_z_ LPCWSTR psz) throw(...)
	{
		if (psz == NULL)
		{
			m_psz = NULL;
			return;
		}
		int nLength = static_cast<int>(wcslen( psz ))+1;
		AtlConvAllocMemory(&m_psz,nLength,m_szBuffer,t_nBufferLength);
		ATLASSUME(m_psz != NULL);
		Checked::memcpy_s( m_psz, nLength*sizeof( wchar_t ), psz, nLength*sizeof( wchar_t ));
	}

public:
	LPWSTR m_psz;
	wchar_t m_szBuffer[t_nBufferLength];

private:
	CW2WEX(_In_ const CW2WEX&) throw();
	CW2WEX& operator=(_In_ const CW2WEX&) throw();
};
typedef CW2WEX<> CW2W;

template< int t_nBufferLength = 128 >
class CA2AEX
{
public:
	CA2AEX(_In_z_ LPCSTR psz) throw(...) :
		m_psz( m_szBuffer )
	{
		Init( psz );
	}
	CA2AEX(
			_In_z_ LPCSTR psz, 
			_In_ UINT nCodePage) throw(...) :
		m_psz( m_szBuffer )
	{
		(void)nCodePage;  // Code page doesn't matter

		Init( psz );
	}
	~CA2AEX() throw()
	{
		AtlConvFreeMemory(m_psz,m_szBuffer,t_nBufferLength);
	}

	_Ret_z_ operator LPSTR() const throw()
	{
		return( m_psz );
	}

private:
	void Init(_In_z_ LPCSTR psz) throw(...)
	{
		if (psz == NULL)
		{
			m_psz = NULL;
			return;
		}
		int nLength = static_cast<int>(strlen( psz ))+1;
		AtlConvAllocMemory(&m_psz,nLength,m_szBuffer,t_nBufferLength);		
		Checked::memcpy_s( m_psz, nLength*sizeof( char ), psz, nLength*sizeof( char ));
	}

public:
	LPSTR m_psz;
	char m_szBuffer[t_nBufferLength];

private:
	CA2AEX(_In_ const CA2AEX&) throw();
	CA2AEX& operator=(_In_ const CA2AEX&) throw();
};
typedef CA2AEX<> CA2A;

template< int t_nBufferLength = 128 >
class CA2CAEX
{
public:
	CA2CAEX(_In_z_ LPCSTR psz) throw(...) :
		m_psz( psz )
	{
	}
	CA2CAEX(
			_In_z_ LPCSTR psz, 
			_In_ UINT nCodePage) throw(...) :
		m_psz( psz )
	{
		(void)nCodePage;
	}
	~CA2CAEX() throw()
	{
	}
	
	_Ret_z_ operator LPCSTR() const throw()
	{
		return( m_psz );
	}
	
public:
	LPCSTR m_psz;

private:
	CA2CAEX(_In_ const CA2CAEX&) throw();
	CA2CAEX& operator=(_In_ const CA2CAEX&) throw();
};
typedef CA2CAEX<> CA2CA;

template< int t_nBufferLength = 128 >
class CW2CWEX
{
public:
	CW2CWEX(_In_z_ LPCWSTR psz) throw(...) :
		m_psz( psz )
	{
	}
	CW2CWEX(
			_In_z_ LPCWSTR psz, 
			_In_ UINT nCodePage) throw(...) :
		m_psz( psz )
	{		
		UNREFERENCED_PARAMETER(nCodePage);
	}
	~CW2CWEX() throw()
	{
	}

	_Ret_z_ operator LPCWSTR() const throw()
	{
		return( m_psz );
	}

public:
	LPCWSTR m_psz;

private:
	CW2CWEX(_In_ const CW2CWEX&) throw();
	CW2CWEX& operator=(_In_ const CW2CWEX&) throw();
};
typedef CW2CWEX<> CW2CW;

template< int t_nBufferLength = 128 >
class CA2WEX
{
public:
	CA2WEX(_In_z_ LPCSTR psz) throw(...) :
		m_psz( m_szBuffer )
	{
		Init( psz, _AtlGetConversionACP() );
	}
	CA2WEX(
			_In_z_ LPCSTR psz, 
			_In_ UINT nCodePage) throw(...) :
		m_psz( m_szBuffer )
	{
		Init( psz, nCodePage );
	}
	~CA2WEX() throw()
	{
		AtlConvFreeMemory(m_psz,m_szBuffer,t_nBufferLength);
	}

	_Ret_z_ operator LPWSTR() const throw()
	{
		return( m_psz );
	}

private:
	void Init(
		_In_z_ LPCSTR psz, 
		_In_ UINT nCodePage) throw(...)
	{
		if (psz == NULL)
		{
			m_psz = NULL;
			return;
		}
		int nLengthA = static_cast<int>(strlen( psz ))+1;
		int nLengthW = nLengthA;

		AtlConvAllocMemory(&m_psz,nLengthW,m_szBuffer,t_nBufferLength);

		BOOL bFailed=(0 == ::MultiByteToWideChar( nCodePage, 0, psz, nLengthA, m_psz, nLengthW ) );
		if (bFailed)
		{
			if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
			{
				nLengthW = ::MultiByteToWideChar( nCodePage, 0, psz, nLengthA, NULL, 0);
ATLPREFAST_SUPPRESS(6102)                
				AtlConvAllocMemory(&m_psz,nLengthW,m_szBuffer,t_nBufferLength);
ATLPREFAST_UNSUPPRESS()
				bFailed=(0 == ::MultiByteToWideChar( nCodePage, 0, psz, nLengthA, m_psz, nLengthW ) );
			}			
		}
		if (bFailed)
		{
			AtlConvFreeMemory(m_psz, m_szBuffer, t_nBufferLength);
			AtlThrowLastWin32();
		}		
	}

public:
	LPWSTR m_psz;
	wchar_t m_szBuffer[t_nBufferLength];

private:
	CA2WEX(_In_ const CA2WEX&) throw();
	CA2WEX& operator=(_In_ const CA2WEX&) throw();
};
typedef CA2WEX<> CA2W;

template< int t_nBufferLength = 128 >
class CW2AEX
{
public:
	CW2AEX(_In_z_ LPCWSTR psz) throw(...) :
		m_psz( m_szBuffer )
	{
		Init( psz, _AtlGetConversionACP() );
	}
	CW2AEX(
			_In_z_ LPCWSTR psz, 
			_In_ UINT nCodePage) throw(...) :
		m_psz( m_szBuffer )
	{
		Init( psz, nCodePage );
	}
	~CW2AEX() throw()
	{		
		AtlConvFreeMemory(m_psz,m_szBuffer,t_nBufferLength);
	}

	_Ret_z_ operator LPSTR() const throw()
	{
		return( m_psz );
	}

private:
	void Init(
		_In_z_ LPCWSTR psz, 
		_In_ UINT nConvertCodePage) throw(...)
	{
		if (psz == NULL)
		{
			m_psz = NULL;
			return;
		}
		int nLengthW = static_cast<int>(wcslen( psz ))+1;
		int nLengthA = nLengthW*4;
		
		AtlConvAllocMemory(&m_psz,nLengthA,m_szBuffer,t_nBufferLength);

		BOOL bFailed=(0 == ::WideCharToMultiByte( nConvertCodePage, 0, psz, nLengthW, m_psz, nLengthA, NULL, NULL ));
		if (bFailed)
		{
			if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
			{
				nLengthA = ::WideCharToMultiByte( nConvertCodePage, 0, psz, nLengthW, NULL, 0, NULL, NULL );
ATLPREFAST_SUPPRESS(6102)
                AtlConvAllocMemory(&m_psz,nLengthA,m_szBuffer,t_nBufferLength);
ATLPREFAST_UNSUPPRESS()
				bFailed=(0 == ::WideCharToMultiByte( nConvertCodePage, 0, psz, nLengthW, m_psz, nLengthA, NULL, NULL ));
			}			
		}
		if (bFailed)
		{
			AtlConvFreeMemory(m_psz, m_szBuffer, t_nBufferLength);
			AtlThrowLastWin32();
		}
	}

public:
	LPSTR m_psz;
	char m_szBuffer[t_nBufferLength];

private:
	CW2AEX(_In_ const CW2AEX&) throw();
	CW2AEX& operator=(_In_ const CW2AEX&) throw();
};
typedef CW2AEX<> CW2A;

#ifdef _UNICODE

#define CW2T CW2W
#define CW2TEX CW2WEX
#define CW2CT CW2CW
#define CW2CTEX CW2CWEX
#define CT2W CW2W
#define CT2WEX CW2WEX
#define CT2CW CW2CW
#define CT2CWEX CW2CWEX

#define CA2T CA2W
#define CA2TEX CA2WEX
#define CA2CT CA2W
#define CA2CTEX CA2WEX
#define CT2A CW2A
#define CT2AEX CW2AEX
#define CT2CA CW2A
#define CT2CAEX CW2AEX

#else  // !_UNICODE

#define CW2T CW2A
#define CW2TEX CW2AEX
#define CW2CT CW2A
#define CW2CTEX CW2AEX
#define CT2W CA2W
#define CT2WEX CA2WEX
#define CT2CW CA2W
#define CT2CWEX CA2WEX

#define CA2T CA2A
#define CA2TEX CA2AEX
#define CA2CT CA2CA
#define CA2CTEX CA2CAEX
#define CT2A CA2A
#define CT2AEX CA2AEX
#define CT2CA CA2CA
#define CT2CAEX CA2CAEX

#endif  // !_UNICODE

#define COLE2T CW2T
#define COLE2TEX CW2TEX
#define COLE2CT CW2CT
#define COLE2CTEX CW2CTEX
#define CT2OLE CT2W
#define CT2OLEEX CT2WEX
#define CT2COLE CT2CW
#define CT2COLEEX CT2CWEX

};  // namespace ATL
#pragma pack(pop)

#pragma pack(push,8)

#ifndef _ATL_EX_CONVERSION_MACROS_ONLY

#ifndef _DEBUG
	#define USES_CONVERSION int _convert; (_convert); UINT _acp = ATL::_AtlGetConversionACP() /*CP_THREAD_ACP*/; (_acp); LPCWSTR _lpw; (_lpw); LPCSTR _lpa; (_lpa)
#else
	#define USES_CONVERSION int _convert = 0; (_convert); UINT _acp = ATL::_AtlGetConversionACP() /*CP_THREAD_ACP*/; (_acp); LPCWSTR _lpw = NULL; (_lpw); LPCSTR _lpa = NULL; (_lpa)
#endif

#endif	// _ATL_EX_CONVERSION_MACROS_ONLY

#ifndef _DEBUG
	#define USES_CONVERSION_EX int _convert_ex; (_convert_ex); UINT _acp_ex = ATL::_AtlGetConversionACP(); (_acp_ex); LPCWSTR _lpw_ex; (_lpw_ex); LPCSTR _lpa_ex; (_lpa_ex); USES_ATL_SAFE_ALLOCA
#else
	#define USES_CONVERSION_EX int _convert_ex = 0; (_convert_ex); UINT _acp_ex = ATL::_AtlGetConversionACP(); (_acp_ex); LPCWSTR _lpw_ex = NULL; (_lpw_ex); LPCSTR _lpa_ex = NULL; (_lpa_ex); USES_ATL_SAFE_ALLOCA
#endif

#ifdef _WINGDI_
	ATLAPI_(LPDEVMODEA) AtlDevModeW2A(_Inout_opt_ LPDEVMODEA lpDevModeA, _In_ const DEVMODEW* lpDevModeW);
#endif

/////////////////////////////////////////////////////////////////////////////
// Global UNICODE<>ANSI translation helpers
ATLPREFAST_SUPPRESS(6054)
_Ret_maybenull_z_ _Post_writable_byte_size_(nChars) inline LPWSTR WINAPI AtlA2WHelper(
	_Out_writes_opt_z_(nChars) LPWSTR lpw, 
	_In_opt_z_ LPCSTR lpa, 
	_In_ int nChars, 
	_In_ UINT acp) throw()
{	
	ATLASSERT(lpa != NULL);
	ATLASSERT(lpw != NULL);
	if (lpw == NULL || lpa == NULL)
		return NULL;
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	*lpw = '\0';
	int ret = MultiByteToWideChar(acp, 0, lpa, -1, lpw, nChars);
	if(ret == 0)
	{
		ATLASSERT(FALSE);
		return NULL;
	}		
	return lpw;
}
ATLPREFAST_UNSUPPRESS()

ATLPREFAST_SUPPRESS(6054)
_Ret_maybenull_z_ _Post_writable_byte_size_(nChars) inline LPSTR WINAPI AtlW2AHelper(
	_Out_writes_opt_z_(nChars) LPSTR lpa, 
	_In_opt_z_ LPCWSTR lpw, 
	_In_ int nChars, 
	_In_ UINT acp) throw()
{
	ATLASSERT(lpw != NULL);
	ATLASSERT(lpa != NULL);
	if (lpa == NULL || lpw == NULL)
		return NULL;
	// verify that no illegal character present
	// since lpa was allocated based on the size of lpw
	// don't worry about the number of chars
	*lpa = '\0';
	int ret = WideCharToMultiByte(acp, 0, lpw, -1, lpa, nChars, NULL, NULL);
	if(ret == 0)
	{
		ATLASSERT(FALSE);
		return NULL;
	}
	return lpa;
}
ATLPREFAST_UNSUPPRESS()

_Ret_maybenull_z_ _Post_writable_byte_size_(nChars) inline LPWSTR WINAPI AtlA2WHelper(
	_Out_writes_opt_z_(nChars) LPWSTR lpw, 
	_In_opt_z_ LPCSTR lpa, 
	_In_ int nChars) throw()
{
	return AtlA2WHelper(lpw, lpa, nChars, CP_ACP);
}

_Ret_maybenull_z_ _Post_writable_byte_size_(nChars) inline LPSTR WINAPI AtlW2AHelper(
	_Out_writes_opt_z_(nChars) LPSTR lpa, 
	_In_opt_z_ LPCWSTR lpw, 
	_In_ int nChars) throw()
{
	return AtlW2AHelper(lpa, lpw, nChars, CP_ACP);
}

#ifndef _CONVERSION_DONT_USE_THREAD_LOCALE
	#ifdef ATLA2WHELPER
		#undef ATLA2WHELPER
		#undef ATLW2AHELPER
	#endif
	#define ATLA2WHELPER AtlA2WHelper
	#define ATLW2AHELPER AtlW2AHelper
#else
	#ifndef ATLA2WHELPER
		#define ATLA2WHELPER AtlA2WHelper
		#define ATLW2AHELPER AtlW2AHelper
	#endif
#endif

#ifndef _ATL_EX_CONVERSION_MACROS_ONLY

#define A2W(lpa) (\
	((_lpa = lpa) == NULL) ? NULL : (\
		_convert = (static_cast<int>(strlen(_lpa))+1),\
		(INT_MAX/2<_convert)? NULL :  \
		ATLA2WHELPER((LPWSTR) alloca(_convert*sizeof(WCHAR)), _lpa, _convert, _acp)))

#define W2A(lpw) (\
	((_lpw = lpw) == NULL) ? NULL : (\
		(_convert = (static_cast<int>(wcslen(_lpw))+1), \
		(_convert>INT_MAX/2) ? NULL : \
		ATLW2AHELPER((LPSTR) alloca(_convert*sizeof(WCHAR)), _lpw, _convert*sizeof(WCHAR), _acp))))

#define A2W_CP(lpa, cp) (\
	((_lpa = lpa) == NULL) ? NULL : (\
		_convert = (static_cast<int>(strlen(_lpa))+1),\
		(INT_MAX/2<_convert)? NULL : \
		ATLA2WHELPER((LPWSTR) alloca(_convert*sizeof(WCHAR)), _lpa, _convert, (cp))))

#define W2A_CP(lpw, cp) (\
	((_lpw = lpw) == NULL) ? NULL : (\
		(_convert = (static_cast<int>(wcslen(_lpw))+1), \
		(_convert>INT_MAX/2) ? NULL : \
		ATLW2AHELPER((LPSTR) alloca(_convert*sizeof(WCHAR)), _lpw, _convert*sizeof(WCHAR), (cp)))))

#endif

// The call to _alloca will not cause stack overflow if _AtlVerifyStackAvailable returns TRUE.
// Notice that nChars is never used in these conversion functions. We cannot change the behavior of
// these functions to actually use nChars because we could potentially break a lot of legacy code.
#define A2W_EX(lpa, nChars) (\
	((_lpa_ex = lpa) == NULL) ? NULL : (\
		_convert_ex = (static_cast<int>(strlen(_lpa_ex))+1),\
		FAILED(::ATL::AtlMultiply(&_convert_ex, _convert_ex, static_cast<int>(sizeof(WCHAR)))) ? NULL : \
		ATLA2WHELPER(	\
			(LPWSTR)_ATL_SAFE_ALLOCA(_convert_ex, _ATL_SAFE_ALLOCA_DEF_THRESHOLD), \
			_lpa_ex, \
			_convert_ex / sizeof(WCHAR), \
			_acp_ex)))

#define A2W_EX_DEF(lpa) A2W_EX(lpa, _ATL_SAFE_ALLOCA_DEF_THRESHOLD)

#define W2A_EX(lpw, nChars) (\
	((_lpw_ex = lpw) == NULL) ? NULL : (\
		_convert_ex = (static_cast<int>(wcslen(_lpw_ex))+1),\
		FAILED(::ATL::AtlMultiply(&_convert_ex, _convert_ex, static_cast<int>(sizeof(WCHAR)))) ? NULL : \
		ATLW2AHELPER(	\
			(LPSTR)_ATL_SAFE_ALLOCA(_convert_ex, _ATL_SAFE_ALLOCA_DEF_THRESHOLD), \
			_lpw_ex, \
			_convert_ex, \
			_acp_ex)))

#define W2A_EX_DEF(lpa) W2A_EX(lpa, _ATL_SAFE_ALLOCA_DEF_THRESHOLD)

#define A2W_CP_EX(lpa, nChars, cp) (\
	((_lpa_ex = lpa) == NULL) ? NULL : (\
		_convert_ex = (static_cast<int>(strlen(_lpa_ex))+1),\
		FAILED(::ATL::AtlMultiply(&_convert_ex, _convert_ex, static_cast<int>(sizeof(WCHAR)))) ? NULL : \
		ATLA2WHELPER(	\
			(LPWSTR)_ATL_SAFE_ALLOCA(_convert_ex, _ATL_SAFE_ALLOCA_DEF_THRESHOLD), \
			_lpa_ex, \
			_convert_ex / sizeof(WCHAR), \
			(cp))))

#define W2A_CP_EX(lpw, nChars, cp) (\
	((_lpw_ex = lpw) == NULL) ? NULL : (\
		_convert_ex = (static_cast<int>(wcslen(_lpw_ex))+1),\
		FAILED(::ATL::AtlMultiply(&_convert_ex, _convert_ex, static_cast<int>(sizeof(WCHAR)))) ? NULL : \
		ATLW2AHELPER(	\
			(LPSTR)_ATL_SAFE_ALLOCA(_convert_ex, _ATL_SAFE_ALLOCA_DEF_THRESHOLD), \
			_lpw_ex, \
			_convert_ex, \
			(cp))))

#ifndef _ATL_EX_CONVERSION_MACROS_ONLY

#define A2CW(lpa) ((LPCWSTR)A2W(lpa))
#define W2CA(lpw) ((LPCSTR)W2A(lpw))

#define A2CW_CP(lpa, cp) ((LPCWSTR)A2W_CP(lpa, (cp)))
#define W2CA_CP(lpw, cp) ((LPCSTR)W2A_CP(lpw, (cp)))

#endif	// _ATL_EX_CONVERSION_MACROS_ONLY

#define A2CW_EX(lpa, nChar) ((LPCWSTR)A2W_EX(lpa, nChar))
#define A2CW_EX_DEF(lpa) ((LPCWSTR)A2W_EX_DEF(lpa))
#define W2CA_EX(lpw, nChar) ((LPCSTR)W2A_EX(lpw, nChar))
#define W2CA_EX_DEF(lpw) ((LPCSTR)W2A_EX_DEF(lpw))

#define A2CW_CP_EX(lpa, nChar, cp) ((LPCWSTR)A2W_CP_EX(lpa, nChar, (cp)))
#define W2CA_CP_EX(lpw, nChar, cp) ((LPCSTR)W2A_CP_EX(lpw, nChar, (cp)))

inline int ocslen(_In_opt_z_ LPCOLESTR x) throw() 
{ 
	if (x == NULL)
	{
		return 0;
	}

	return static_cast<int>(wcslen(x));
}

ATLPREFAST_SUPPRESS(6054)
inline bool ocscpy_s(
	_Out_writes_(maxSize) LPOLESTR dest, 
	_In_ size_t maxSize, 
	_In_z_ LPCOLESTR src) throw() 
{ 
	return 0 == memcpy_s(dest, maxSize*sizeof(WCHAR), src, (ocslen(src)+1)*sizeof(WCHAR)); 
}
ATLPREFAST_UNSUPPRESS()

inline bool ocscat_s(
	_Inout_updates_z_(maxSize) LPOLESTR dest, 
	_In_ size_t maxSize, 
	_In_z_ LPCOLESTR src) throw() 
{ 
	return 0 == wcscat_s(dest, maxSize,src); 
}

#if defined(_UNICODE)

// in these cases the default (TCHAR) is the same as OLECHAR

_ATL_INSECURE_DEPRECATE("ocscpy is not safe. Intead, use ocscpy_s")
inline OLECHAR* ocscpy(
	_Inout_ _Post_z_ LPOLESTR dest, 
	_In_z_ LPCOLESTR src) throw()
{
#pragma warning(push)
#pragma warning(disable:4996 28719)
	return wcscpy(dest, src);
#pragma warning(pop)
}

_ATL_INSECURE_DEPRECATE("ocscat is not safe. Intead, use ocscat_s")
inline OLECHAR* ocscat(
	_Inout_ _Post_z_ LPOLESTR dest, 
	_In_z_ LPCOLESTR src) throw()
{
#pragma warning(push)
#pragma warning(disable:4996 28719)
	return wcscat(dest, src);
#pragma warning(pop)
}

_Ret_z_ inline LPCOLESTR T2COLE_EX(
	_In_z_ LPCTSTR lp, 
	_In_ UINT) 
{ 
	return lp; 
}
_Ret_z_ inline LPCOLESTR T2COLE_EX_DEF(_In_z_ LPCTSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPCTSTR OLE2CT_EX(
	_In_z_ LPCOLESTR lp, 
	_In_ UINT) 
{ 
	return lp; 
}
_Ret_z_ inline LPCTSTR OLE2CT_EX_DEF(_In_z_ LPCOLESTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPOLESTR T2OLE_EX(
	_In_z_ LPTSTR lp, 
	_In_ UINT) 
{ 
	return lp; 
}
_Ret_z_ inline LPOLESTR T2OLE_EX_DEF(_In_z_ LPTSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPTSTR OLE2T_EX(
	_In_z_ LPOLESTR lp, 
	_In_ UINT) 
{ 
	return lp; 
}	
_Ret_z_ inline LPTSTR OLE2T_EX_DEF(_In_z_ LPOLESTR lp) 
{ 
	return lp; 
}	

#ifndef _ATL_EX_CONVERSION_MACROS_ONLY

_Ret_z_ inline LPCOLESTR T2COLE(_In_z_ LPCTSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPCTSTR OLE2CT(_In_z_ LPCOLESTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPOLESTR T2OLE(_In_z_ LPTSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPTSTR OLE2T(_In_z_ LPOLESTR lp) 
{ 
	return lp; 
}

#endif	 // _ATL_EX_CONVERSION_MACROS_ONLY

#else // !defined(_UNICODE)

_ATL_INSECURE_DEPRECATE("ocscpy is not safe. Intead, use ocscpy_s")
inline OLECHAR* ocscpy(
	_Inout_ _Post_z_ LPOLESTR dest, 
	_In_z_ LPCOLESTR src) throw()
{
#pragma warning(push)
#pragma warning(disable:4996)
	return (LPOLESTR) memcpy(dest, src, (ocslen(src)+1)*sizeof(WCHAR));
#pragma warning(pop)
}

_ATL_INSECURE_DEPRECATE("ocscat is not safe. Intead, use ocscat_s")
inline OLECHAR* ocscat(
	_Inout_ _Post_z_ LPOLESTR dest, 
	_In_z_ LPCOLESTR src) throw()
{
#pragma warning(push)
#pragma warning(disable:4996)
	return ocscpy(dest+ocslen(dest), src);
#pragma warning(pop)
}

#define T2COLE_EX(lpa, nChar) A2CW_EX(lpa, nChar)
#define T2COLE_EX_DEF(lpa) A2CW_EX_DEF(lpa)
#define T2OLE_EX(lpa, nChar) A2W_EX(lpa, nChar)
#define T2OLE_EX_DEF(lpa) A2W_EX_DEF(lpa)
#define OLE2CT_EX(lpo, nChar) W2CA_EX(lpo, nChar)
#define OLE2CT_EX_DEF(lpo) W2CA_EX_DEF(lpo)
#define OLE2T_EX(lpo, nChar) W2A_EX(lpo, nChar)
#define OLE2T_EX_DEF(lpo) W2A_EX_DEF(lpo)

#ifndef _ATL_EX_CONVERSION_MACROS_ONLY

#define T2COLE(lpa) A2CW(lpa)
#define T2OLE(lpa) A2W(lpa)
#define OLE2CT(lpo) W2CA(lpo)
#define OLE2T(lpo) W2A(lpo)

#endif	// _ATL_EX_CONVERSION_MACROS_ONLY

#endif // defined(_UNICODE)

_Ret_z_ inline LPOLESTR W2OLE_EX(
	_In_z_ LPWSTR lp, 
	_In_ UINT) 
{ 
	return lp; 
}
_Ret_z_ inline LPOLESTR W2OLE_EX_DEF(_In_z_ LPWSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPWSTR OLE2W_EX(
	_In_z_ LPOLESTR lp, 
	_In_ UINT) 
{ 
	return lp; 
}
_Ret_z_ inline LPWSTR OLE2W_EX_DEF(_In_z_ LPOLESTR lp) 
{ 
	return lp; 
}

#define A2OLE_EX A2W_EX
#define A2OLE_EX_DEF A2W_EX_DEF
#define OLE2A_EX W2A_EX
#define OLE2A_EX_DEF W2A_EX_DEF

_Ret_z_ inline LPCOLESTR W2COLE_EX(
	_In_z_ LPCWSTR lp,
	_In_ UINT) 
{ 
	return lp; 
}
_Ret_z_ inline LPCOLESTR W2COLE_EX_DEF(_In_z_ LPCWSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPCWSTR OLE2CW_EX(
	_In_z_ LPCOLESTR lp, 
	_In_ UINT) 
{ 
	return lp; 
}
_Ret_z_ inline LPCWSTR OLE2CW_EX_DEF(_In_z_ LPCOLESTR lp) 
{ 
	return lp; 
}

#define A2COLE_EX A2CW_EX
#define A2COLE_EX_DEF A2CW_EX_DEF
#define OLE2CA_EX W2CA_EX
#define OLE2CA_EX_DEF W2CA_EX_DEF

#ifndef _ATL_EX_CONVERSION_MACROS_ONLY

_Ret_z_ inline LPOLESTR W2OLE(_In_z_ LPWSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPWSTR OLE2W(_In_z_ LPOLESTR lp) 
{ 
	return lp; 
}

#define A2OLE A2W
#define OLE2A W2A

_Ret_z_ inline LPCOLESTR W2COLE(_In_z_ LPCWSTR lp) 
{
	return lp; 
}
_Ret_z_ inline LPCWSTR OLE2CW(_In_z_ LPCOLESTR lp) 
{ 
	return lp; 
}

#define A2COLE A2CW
#define OLE2CA W2CA

#endif	// _ATL_EX_CONVERSION_MACROS_ONLY

#if defined(_UNICODE)

#define T2A_EX W2A_EX
#define T2A_EX_DEF W2A_EX_DEF
#define A2T_EX A2W_EX
#define A2T_EX_DEF A2W_EX_DEF

_Ret_z_ inline LPWSTR T2W_EX(
	_In_z_ LPTSTR lp,
	_In_ UINT) 
{	
	return lp; 
}
_Ret_z_ inline LPWSTR T2W_EX_DEF(_In_z_ LPTSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPTSTR W2T_EX(
	_In_z_ LPWSTR lp, 
	_In_ UINT) 
{ 
	return lp; 
}
_Ret_z_ inline LPTSTR W2T_DEF(_In_z_ LPWSTR lp) 
{ 
	return lp; 
}

#define T2CA_EX W2CA_EX
#define T2CA_EX_DEF W2CA_EX_DEF
#define A2CT_EX A2CW_EX
#define A2CT_EX_DEF A2CW_EX_DEF

_Ret_z_ inline LPCWSTR T2CW_EX(
	_In_z_ LPCTSTR lp, 
	_In_ UINT) 
{ 
	return lp; 
}
_Ret_z_ inline LPCWSTR T2CW_EX_DEF(_In_z_ LPCTSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPCTSTR W2CT_EX(
	_In_z_ LPCWSTR lp, 
	_In_ UINT) 
{ 
	return lp; 
}
_Ret_z_ inline LPCTSTR W2CT_EX_DEF(_In_z_ LPCWSTR lp) 
{ 
	return lp; 
}

#ifndef _ATL_EX_CONVERSION_MACROS_ONLY

#define T2A W2A
#define A2T A2W

_Ret_z_ inline LPWSTR T2W(_In_z_ LPTSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPTSTR W2T(_In_z_ LPWSTR lp) 
{ 
	return lp; 
}

#define T2CA W2CA
#define A2CT A2CW

_Ret_z_ inline LPCWSTR T2CW(_In_z_ LPCTSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPCTSTR W2CT(_In_z_ LPCWSTR lp) 
{ 
	return lp; 
}

#endif	// _ATL_EX_CONVERSION_MACROS_ONLY
	
#else // !defined(_UNICODE)

#define T2W_EX A2W_EX
#define T2W_EX_DEF A2W_EX_DEF
#define W2T_EX W2A_EX
#define W2T_EX_DEF W2A_EX_DEF

_Ret_z_ inline LPSTR T2A_EX(
	_In_z_ LPTSTR lp, 
	_In_ UINT) 
{ 
	return lp; 
}
_Ret_z_ inline LPSTR T2A_EX_DEF(_In_z_ LPTSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPTSTR A2T_EX(
	_In_z_ LPSTR lp, 
	_In_ UINT) 
{ 
	return lp; 
}
_Ret_z_ inline LPTSTR A2T_EX_DEF(_In_z_ LPSTR lp) 
{ 
	return lp; 
}

#define T2CW_EX A2CW_EX
#define T2CW_EX_DEF A2CW_EX_DEF
#define W2CT_EX W2CA_EX
#define W2CT_EX_DEF W2CA_EX_DEF

_Ret_z_ inline LPCSTR T2CA_EX(
	_In_z_ LPCTSTR lp, 
	_In_ UINT) 
{ 
	return lp; 
}
_Ret_z_ inline LPCSTR T2CA_EX_DEF(_In_z_ LPCTSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPCTSTR A2CT_EX(
	_In_z_ LPCSTR lp, 
	_In_ UINT) 
{ 
	return lp; 
}
_Ret_z_ inline LPCTSTR A2CT_EX_DEF(_In_z_ LPCSTR lp) 
{ 
	return lp; 
}

#ifndef _ATL_EX_CONVERSION_MACROS_ONLY

#define T2W A2W
#define W2T W2A
_Ret_z_ inline LPSTR T2A(_In_z_ LPTSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPTSTR A2T(_In_z_ LPSTR lp) 
{ 
	return lp; 
}
#define T2CW A2CW
#define W2CT W2CA
_Ret_z_ inline LPCSTR T2CA(_In_z_ LPCTSTR lp) 
{ 
	return lp; 
}
_Ret_z_ inline LPCTSTR A2CT(_In_z_ LPCSTR lp) 
{ 
	return lp; 
}

#endif	// _ATL_EX_CONVERSION_MACROS_ONLY

#endif // defined(_UNICODE)

ATLPREFAST_SUPPRESS(6103)
_Check_return_ _Ret_maybenull_z_ inline BSTR A2WBSTR(
	_In_opt_z_ LPCSTR lp, 
	_In_ int nLen = -1)
{
	if (lp == NULL || nLen == 0)
		return NULL;
	USES_CONVERSION_EX;
	BSTR str = NULL;

	int nConvertedLen = MultiByteToWideChar(_acp_ex, 0, lp, nLen, NULL, 0);

	int nAllocLen = nConvertedLen;
	if (nLen == -1)
		nAllocLen -= 1;  // Don't allocate terminating '\0'
	str = ::SysAllocStringLen(NULL, nAllocLen);

	if (str != NULL)
	{
		int nResult;
		nResult = MultiByteToWideChar(_acp_ex, 0, lp, nLen, str, nConvertedLen);
		ATLASSERT(nResult == nConvertedLen);
		if (nResult != nConvertedLen)
		{
ATLPREFAST_SUPPRESS(6102)
			SysFreeString(str);
ATLPREFAST_UNSUPPRESS()
			return NULL;
		}

	}
	return str;
}
ATLPREFAST_UNSUPPRESS()

_Ret_maybenull_z_ inline BSTR OLE2BSTR(_In_opt_z_ LPCOLESTR lp) 
{
	return ::SysAllocString(lp);
}
#if defined(_UNICODE)
// in these cases the default (TCHAR) is the same as OLECHAR
_Ret_maybenull_z_ inline BSTR T2BSTR_EX(_In_opt_z_ LPCTSTR lp) 
{
	return ::SysAllocString(lp);
}
_Ret_maybenull_z_ inline BSTR A2BSTR_EX(_In_opt_z_ LPCSTR lp) 
{
	return A2WBSTR(lp);
}
_Ret_maybenull_z_ inline BSTR W2BSTR_EX(_In_opt_z_ LPCWSTR lp) 
{
	return ::SysAllocString(lp);
}

#ifndef _ATL_EX_CONVERSION_MACROS_ONLY

_Ret_maybenull_z_ inline BSTR T2BSTR(_In_opt_z_ LPCTSTR lp) 
{
	return ::SysAllocString(lp);
}
_Ret_maybenull_z_ inline BSTR A2BSTR(_In_opt_z_ LPCSTR lp) 
{
	return A2WBSTR(lp);
}
_Ret_maybenull_z_ inline BSTR W2BSTR(_In_opt_z_ LPCWSTR lp) 
{
	return ::SysAllocString(lp);
}
	
#endif	// _ATL_EX_CONVERSION_MACROS_ONLY

#else // !defined(_UNICODE)
_Ret_maybenull_z_ inline BSTR T2BSTR_EX(_In_opt_z_ LPCTSTR lp) 
{
	return A2WBSTR(lp);
}
_Ret_maybenull_z_ inline BSTR A2BSTR_EX(_In_opt_z_ LPCSTR lp) 
{
	return A2WBSTR(lp);
}
_Ret_maybenull_z_ inline BSTR W2BSTR_EX(_In_opt_z_ LPCWSTR lp) 
{
	return ::SysAllocString(lp);
}
	
#ifndef _ATL_EX_CONVERSION_MACROS_ONLY

_Ret_maybenull_z_ inline BSTR T2BSTR(_In_opt_z_ LPCTSTR lp) 
{
	return A2WBSTR(lp);
}
_Ret_maybenull_z_ inline BSTR A2BSTR(_In_opt_z_ LPCSTR lp) 
{
	return A2WBSTR(lp);
}
_Ret_maybenull_z_ inline BSTR W2BSTR(_In_opt_z_ LPCWSTR lp) 
{
	return ::SysAllocString(lp);
}

#endif	// _ATL_EX_CONVERSION_MACROS_ONLY

#endif // defined(_UNICODE)

#ifdef _WINGDI_
/////////////////////////////////////////////////////////////////////////////
// Global UNICODE<>ANSI translation helpers
inline LPDEVMODEW AtlDevModeA2W(
	_Inout_ LPDEVMODEW lpDevModeW, 
	_In_ const DEVMODEA* lpDevModeA)
{
	USES_CONVERSION_EX;
	ATLASSERT(lpDevModeW != NULL);
	if (lpDevModeA == NULL || lpDevModeW == NULL)
	{
		return NULL;
	}

	AtlA2WHelper(lpDevModeW->dmDeviceName, (LPCSTR)lpDevModeA->dmDeviceName, 32, _acp_ex);

	if(0 != memcpy_s(&lpDevModeW->dmSpecVersion, offsetof(DEVMODEW, dmFormName) - offsetof(DEVMODEW, dmSpecVersion),
		&lpDevModeA->dmSpecVersion, offsetof(DEVMODEW, dmFormName) - offsetof(DEVMODEW, dmSpecVersion)))
	{
		return NULL;
	}

	AtlA2WHelper(lpDevModeW->dmFormName, (LPCSTR)lpDevModeA->dmFormName, 32, _acp_ex);

	if(0 != memcpy_s(&lpDevModeW->dmLogPixels, sizeof(DEVMODEW) - offsetof(DEVMODEW, dmLogPixels),
		&lpDevModeA->dmLogPixels, sizeof(DEVMODEW) - offsetof(DEVMODEW, dmLogPixels)))
	{
		return NULL;
	}

	if (lpDevModeA->dmDriverExtra != 0)
	{
		// lpDevModeW holds more info
#pragma warning(push)
#pragma warning(disable:26000)
		if(0 != memcpy_s(lpDevModeW+1, lpDevModeA->dmDriverExtra, lpDevModeA+1, lpDevModeA->dmDriverExtra))
		{
			return NULL;
		}
#pragma warning(pop)
	}
	lpDevModeW->dmSize = sizeof(DEVMODEW);
	return lpDevModeW;
}

inline LPTEXTMETRICW AtlTextMetricA2W(
	_Out_ LPTEXTMETRICW lptmW, 
	_In_ LPTEXTMETRICA lptmA)
{
	USES_CONVERSION_EX;
	ATLASSERT(lptmW != NULL);
	if (lptmA == NULL || lptmW == NULL)
		return NULL;

	if(0 != memcpy_s(lptmW, sizeof(LONG) * 11, lptmA, sizeof(LONG) * 11))
	{
		return NULL;
	}

	if(0 != memcpy_s(&lptmW->tmItalic, sizeof(BYTE) * 5, &lptmA->tmItalic, sizeof(BYTE) * 5))
	{
		return NULL;
	}

	if(MultiByteToWideChar(_acp_ex, 0, (LPCSTR)&lptmA->tmFirstChar, 1, &lptmW->tmFirstChar, 1) == 0)
	{
		ATLASSERT(FALSE);
		return NULL;
	}
		
	if(MultiByteToWideChar(_acp_ex, 0, (LPCSTR)&lptmA->tmLastChar, 1, &lptmW->tmLastChar, 1) == 0)
	{
		ATLASSERT(FALSE);
		return NULL;
	}
		
	if(MultiByteToWideChar(_acp_ex, 0, (LPCSTR)&lptmA->tmDefaultChar, 1, &lptmW->tmDefaultChar, 1)== 0)
	{
		ATLASSERT(FALSE);
		return NULL;
	}
		
	if(MultiByteToWideChar(_acp_ex, 0, (LPCSTR)&lptmA->tmBreakChar, 1, &lptmW->tmBreakChar, 1) == 0)
	{
		ATLASSERT(FALSE);
		return NULL;
	}
	
	return lptmW;
}

inline LPTEXTMETRICA AtlTextMetricW2A(
	_Out_ LPTEXTMETRICA lptmA, 
	_In_ LPTEXTMETRICW lptmW)
{
	USES_CONVERSION_EX;
	ATLASSERT(lptmA != NULL);
	if (lptmW == NULL || lptmA == NULL)
	{
		return NULL;
	}

	if(0 != memcpy_s(lptmA, sizeof(LONG) * 11, lptmW, sizeof(LONG) * 11))
	{
		return NULL;
	}

	if(0 != memcpy_s(&lptmA->tmItalic, sizeof(BYTE) * 5, &lptmW->tmItalic, sizeof(BYTE) * 5))
	{
		return NULL;
	}
	
	if(WideCharToMultiByte(_acp_ex, 0, &lptmW->tmFirstChar, 1, (LPSTR)&lptmA->tmFirstChar, 1, NULL, NULL) == 0)
	{
		ATLASSERT(FALSE);
		return NULL;
	}

	if(WideCharToMultiByte(_acp_ex, 0, &lptmW->tmLastChar, 1, (LPSTR)&lptmA->tmLastChar, 1, NULL, NULL) == 0)
	{
		ATLASSERT(FALSE);
		return NULL;
	}

	if(WideCharToMultiByte(_acp_ex, 0, &lptmW->tmDefaultChar, 1, (LPSTR)&lptmA->tmDefaultChar, 1, NULL, NULL) == 0)
	{
		ATLASSERT(FALSE);
		return NULL;
	}

	if(WideCharToMultiByte(_acp_ex, 0, &lptmW->tmBreakChar, 1, (LPSTR)&lptmA->tmBreakChar, 1, NULL, NULL) == 0)
	{
		ATLASSERT(FALSE);
		return NULL;
	}

	return lptmA;
}

#ifndef ATLDEVMODEA2W
#define ATLDEVMODEA2W AtlDevModeA2W
#define ATLDEVMODEW2A AtlDevModeW2A
#define ATLTEXTMETRICA2W AtlTextMetricA2W
#define ATLTEXTMETRICW2A AtlTextMetricW2A
#endif

// Requires USES_CONVERSION_EX or USES_ATL_SAFE_ALLOCA macro before using the _EX versions of the macros
#define DEVMODEW2A_EX(lpw)\
	(((lpw) == NULL) ? NULL : ATLDEVMODEW2A((LPDEVMODEA)_ATL_SAFE_ALLOCA(sizeof(DEVMODEA)+(lpw)->dmDriverExtra, _ATL_SAFE_ALLOCA_DEF_THRESHOLD), (lpw)))
#define DEVMODEA2W_EX(lpa)\
	(((lpa) == NULL) ? NULL : ATLDEVMODEA2W((LPDEVMODEW)_ATL_SAFE_ALLOCA(sizeof(DEVMODEW)+(lpa)->dmDriverExtra, _ATL_SAFE_ALLOCA_DEF_THRESHOLD), (lpa)))
#define TEXTMETRICW2A_EX(lptmw)\
	(((lptmw) == NULL) ? NULL : ATLTEXTMETRICW2A((LPTEXTMETRICA)_ATL_SAFE_ALLOCA(sizeof(TEXTMETRICA), _ATL_SAFE_ALLOCA_DEF_THRESHOLD), (lptmw)))
#define TEXTMETRICA2W_EX(lptma)\
	(((lptma) == NULL) ? NULL : ATLTEXTMETRICA2W((LPTEXTMETRICW)_ATL_SAFE_ALLOCA(sizeof(TEXTMETRICW), _ATL_SAFE_ALLOCA_DEF_THRESHOLD), (lptma)))

#ifndef _ATL_EX_CONVERSION_MACROS_ONLY

#define DEVMODEW2A(lpw)\
	((lpw == NULL) ? NULL : ATLDEVMODEW2A((LPDEVMODEA)alloca(sizeof(DEVMODEA)+lpw->dmDriverExtra), lpw))
#define DEVMODEA2W(lpa)\
	((lpa == NULL) ? NULL : ATLDEVMODEA2W((LPDEVMODEW)alloca(sizeof(DEVMODEW)+lpa->dmDriverExtra), lpa))
#define TEXTMETRICW2A(lptmw)\
	((lptmw == NULL) ? NULL : ATLTEXTMETRICW2A((LPTEXTMETRICA)alloca(sizeof(TEXTMETRICA)), lptmw))
#define TEXTMETRICA2W(lptma)\
	((lptma == NULL) ? NULL : ATLTEXTMETRICA2W((LPTEXTMETRICW)alloca(sizeof(TEXTMETRICW)), lptma))

#endif	// _ATL_EX_CONVERSION_MACROS_ONLY

#define DEVMODEOLE DEVMODEW
#define LPDEVMODEOLE LPDEVMODEW
#define TEXTMETRICOLE TEXTMETRICW
#define LPTEXTMETRICOLE LPTEXTMETRICW

#if defined(_UNICODE)
// in these cases the default (TCHAR) is the same as OLECHAR
inline LPDEVMODEW DEVMODEOLE2T_EX(_In_opt_ LPDEVMODEOLE lp) 
{ 
	return lp; 
}
inline LPDEVMODEOLE DEVMODET2OLE_EX(_In_opt_ LPDEVMODEW lp) 
{ 
	return lp; 
}
inline LPTEXTMETRICW TEXTMETRICOLE2T_EX(_In_ LPTEXTMETRICOLE lp) 
{ 
	return lp; 
}
inline LPTEXTMETRICOLE TEXTMETRICT2OLE_EX(_In_ LPTEXTMETRICW lp) 
{ 
	return lp; 
}

#ifndef _ATL_EX_CONVERSION_MACROS_ONLY
inline LPDEVMODEW DEVMODEOLE2T(_In_ LPDEVMODEOLE lp) 
{ 
	return lp; 
}
inline LPDEVMODEOLE DEVMODET2OLE(_In_ LPDEVMODEW lp) 
{ 
	return lp; 
}
inline LPTEXTMETRICW TEXTMETRICOLE2T(_In_ LPTEXTMETRICOLE lp) 
{ 
	return lp; 
}
inline LPTEXTMETRICOLE TEXTMETRICT2OLE(_In_ LPTEXTMETRICW lp) 
{ 
	return lp; 
}
#endif	// _ATL_EX_CONVERSION_MACROS_ONLY
	
#else // !defined(_UNICODE)

#define DEVMODEOLE2T_EX(lpo) DEVMODEW2A_EX(lpo)
#define DEVMODET2OLE_EX(lpa) DEVMODEA2W_EX(lpa)
#define TEXTMETRICOLE2T_EX(lptmw) TEXTMETRICW2A_EX(lptmw)
#define TEXTMETRICT2OLE_EX(lptma) TEXTMETRICA2W_EX(lptma)

#ifndef _ATL_EX_CONVERSION_MACROS_ONLY

#define DEVMODEOLE2T(lpo) DEVMODEW2A(lpo)
#define DEVMODET2OLE(lpa) DEVMODEA2W(lpa)
#define TEXTMETRICOLE2T(lptmw) TEXTMETRICW2A(lptmw)
#define TEXTMETRICT2OLE(lptma) TEXTMETRICA2W(lptma)

#endif	// _ATL_EX_CONVERSION_MACROS_ONLY	

#endif // defined(_UNICODE)

#endif //_WINGDI_

#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////

#ifdef _WINGDI_

ATLINLINE ATLAPI_(LPDEVMODEA) AtlDevModeW2A(
	_Inout_opt_ LPDEVMODEA lpDevModeA, 
	_In_ const DEVMODEW* lpDevModeW)
{
	USES_CONVERSION_EX;
	ATLASSERT(lpDevModeA != NULL);
	if (lpDevModeW == NULL || lpDevModeA == NULL)
		return NULL;

	AtlW2AHelper((LPSTR)lpDevModeA->dmDeviceName, lpDevModeW->dmDeviceName, 32, _acp_ex);

	if(0 != memcpy_s(&lpDevModeA->dmSpecVersion, offsetof(DEVMODEA, dmFormName) - offsetof(DEVMODEA, dmSpecVersion), 
		&lpDevModeW->dmSpecVersion, offsetof(DEVMODEA, dmFormName) - offsetof(DEVMODEA, dmSpecVersion)))
	{
		return NULL;
	}

	AtlW2AHelper((LPSTR)lpDevModeA->dmFormName, lpDevModeW->dmFormName, 32, _acp_ex);

	if(0 != memcpy_s(&lpDevModeA->dmLogPixels, sizeof(DEVMODEA) - offsetof(DEVMODEA, dmLogPixels),
		&lpDevModeW->dmLogPixels, sizeof(DEVMODEA) - offsetof(DEVMODEA, dmLogPixels)))
	{
		return NULL;
	}

	if (lpDevModeW->dmDriverExtra != 0)
	{
		// lpDevModeW holds more info
#pragma warning(push)
#pragma warning(disable:26000)
		if(0 != memcpy_s(lpDevModeA+1, lpDevModeW->dmDriverExtra, lpDevModeW+1, lpDevModeW->dmDriverExtra))
		{
			return NULL;
		}
#pragma warning(pop)
	}
	
	lpDevModeA->dmSize = sizeof(DEVMODEA);
	return lpDevModeA;
}

#endif //_WINGDI

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (pop)
#endif //!_ATL_NO_PRAGMA_WARNINGS

#endif // __ATLCONV_H__
