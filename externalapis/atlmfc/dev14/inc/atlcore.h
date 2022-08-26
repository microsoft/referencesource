// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLCORE_H__
#define __ATLCORE_H__

#pragma once

#ifdef _ATL_ALL_WARNINGS
#pragma warning( push )
#endif

#pragma warning(disable: 4786) // identifier was truncated in the debug information
#pragma warning(disable: 4127) // constant expression

#include <atldef.h>
#include <windows.h>
#include <ole2.h>
#ifdef _ATL_USE_WINAPI_FAMILY_PHONE_APP
#include <oleauto.h>
#endif // _ATL_USE_WINAPI_FAMILY_PHONE_APP

#include <limits.h>
#include <tchar.h>
#include <mbstring.h>

#include <atlchecked.h>
#include <atlsimpcoll.h>
#include <atlwinverapi.h>

#pragma pack(push,_ATL_PACKING)
namespace ATL
{

namespace details
{
	// Copy the necessary machinery from the STL to avoid dragging in STL headers.

	template<class T> struct add_lvalue_reference { typedef T& type; };
	template<> struct add_lvalue_reference<void> { typedef void type; };
	template<> struct add_lvalue_reference<const void> { typedef const void type; };
	template<> struct add_lvalue_reference<volatile void> { typedef volatile void type; };
	template<> struct add_lvalue_reference<const volatile void> { typedef const volatile void type; };

	template<class T> struct add_rvalue_reference { typedef T&& type; };
	template<> struct add_rvalue_reference<void> { typedef void type; };
	template<> struct add_rvalue_reference<const void> { typedef const void type; };
	template<> struct add_rvalue_reference<volatile void> { typedef volatile void type; };
	template<> struct add_rvalue_reference<const volatile void> { typedef const volatile void type; };

	template <class Ty, class... Args>
	struct is_nothrow_constructible
	{
		static constexpr bool value = __is_nothrow_constructible(Ty, Args...);
	};

	template <class Ty>
	using is_nothrow_move_constructible = is_nothrow_constructible<Ty, typename add_rvalue_reference<Ty>::type>;

	template <class To, class From>
	struct is_nothrow_assignable
	{
		static constexpr bool value = {__is_nothrow_assignable(To, From)}; // TRANSITION, VSO#362116
	};

	template <class Ty>
	using is_nothrow_move_assignable = is_nothrow_assignable<typename add_lvalue_reference<Ty>::type, typename add_rvalue_reference<Ty>::type>;

#ifdef _ATL_NO_EXCEPTIONS
#define _ATL_NOEXCEPT(...) throw()
#else
#define _ATL_NOEXCEPT(...) noexcept(__VA_ARGS__)
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Checking out the string len
inline int AtlStrLen(_In_opt_z_ const wchar_t *str)
{
	if (str == NULL)
		return 0;
	return static_cast<int>(::wcslen(str));
}


inline int AtlStrLen(_In_opt_z_ const char *str)
{
	if (str == NULL)
		return 0;
	return static_cast<int>(::strlen(str));
}

/////////////////////////////////////////////////////////////////////////////
// Verify that a null-terminated string points to valid memory
inline BOOL AtlIsValidString(
	_In_reads_z_(nMaxLength) LPCWSTR psz,
	_In_ size_t nMaxLength = INT_MAX)
{
	(nMaxLength);
	return (psz != NULL);
}

// Verify that a null-terminated string points to valid memory
inline BOOL AtlIsValidString(
	_In_reads_z_(nMaxLength) LPCSTR psz,
	_In_ size_t nMaxLength = UINT_MAX)
{
	(nMaxLength);
	return (psz != NULL);
}

// Verify that a pointer points to valid memory
inline BOOL AtlIsValidAddress(
	_In_reads_bytes_opt_(nBytes) const void* p,
	_In_ size_t nBytes,
	_In_ BOOL bReadWrite = TRUE)
{
	(bReadWrite);
	(nBytes);
	return (p != NULL);
}

template<typename T>
inline void AtlAssertValidObject(
	_Inout_opt_ const T *pOb)
{
	ATLASSERT(pOb);
	ATLASSERT(AtlIsValidAddress(pOb, sizeof(T)));
	if(pOb)
		pOb->AssertValid();
}
#ifdef _DEBUG
#define ATLASSERT_VALID(x) ATL::AtlAssertValidObject(x)
#else
#define ATLASSERT_VALID(x) __noop;
#endif

// COM Sync Classes
class CComCriticalSection
{
public:
	CComCriticalSection() throw()
	{
		memset(&m_sec, 0, sizeof(CRITICAL_SECTION));
	}

	~CComCriticalSection()
	{
	}

	_Success_(1) _Acquires_lock_(this->m_sec) HRESULT Lock() throw()
	{
		EnterCriticalSection(&m_sec);
		return S_OK;
	}
	_Success_(1) _Releases_lock_(this->m_sec) HRESULT Unlock() throw()
	{
		LeaveCriticalSection(&m_sec);
		return S_OK;
	}
	HRESULT Init() throw()
	{
		HRESULT hRes = S_OK;
		if (!_AtlInitializeCriticalSectionEx(&m_sec, 0, 0))
		{
			hRes = HRESULT_FROM_WIN32(GetLastError());
		}

		return hRes;
	}

	HRESULT Term() throw()
	{
		DeleteCriticalSection(&m_sec);
		return S_OK;
	}
	CRITICAL_SECTION m_sec;
};

class CComAutoCriticalSection :
	public CComCriticalSection
{
public:
	CComAutoCriticalSection()
	{
		HRESULT hr = CComCriticalSection::Init();
		if (FAILED(hr))
			AtlThrow(hr);
	}
	~CComAutoCriticalSection() throw()
	{
		CComCriticalSection::Term();
	}
private :
	HRESULT Init(); // Not implemented. CComAutoCriticalSection::Init should never be called
	HRESULT Term(); // Not implemented. CComAutoCriticalSection::Term should never be called
};

class CComSafeDeleteCriticalSection :
	public CComCriticalSection
{
public:
	CComSafeDeleteCriticalSection(): m_bInitialized(false)
	{
	}

	~CComSafeDeleteCriticalSection() throw()
	{
		if (!m_bInitialized)
		{
			return;
		}
		m_bInitialized = false;
		CComCriticalSection::Term();
	}

	HRESULT Init() throw()
	{
		ATLASSERT( !m_bInitialized );
		HRESULT hr = CComCriticalSection::Init();
		if (SUCCEEDED(hr))
		{
			m_bInitialized = true;
		}
		return hr;
	}

	HRESULT Term() throw()
	{
		if (!m_bInitialized)
		{
			return S_OK;
		}
		m_bInitialized = false;
		return CComCriticalSection::Term();
	}

	_Success_(1) _Acquires_lock_(this->m_sec)
	HRESULT Lock()
	{
		// CComSafeDeleteCriticalSection::Init or CComAutoDeleteCriticalSection::Init
		// not called or failed.
		// m_critsec member of CComObjectRootEx is now of type
		// CComAutoDeleteCriticalSection. It has to be initialized
		// by calling CComObjectRootEx::_AtlInitialConstruct
		ATLASSUME(m_bInitialized);
		return CComCriticalSection::Lock();
	}

private:
	bool m_bInitialized;
};

class CComAutoDeleteCriticalSection :
	public CComSafeDeleteCriticalSection
{
private:
	// CComAutoDeleteCriticalSection::Term should never be called
	HRESULT Term() throw();
};

class CComFakeCriticalSection
{
public:
	HRESULT Lock() throw()
	{
		return S_OK;
	}
	HRESULT Unlock() throw()
	{
		return S_OK;
	}
	HRESULT Init() throw()
	{
		return S_OK;
	}
	HRESULT Term() throw()
	{
		return S_OK;
	}
};

/////////////////////////////////////////////////////////////////////////////
// Module

// Used by any project that uses ATL
struct _ATL_BASE_MODULE70
{
	UINT cbSize;
	HINSTANCE m_hInst;
	HINSTANCE m_hInstResource;
	DWORD dwAtlBuildVer;
	const GUID* pguidVer;
	CComCriticalSection m_csResource;
	CSimpleArray<HINSTANCE> m_rgResourceInstance;
};
typedef _ATL_BASE_MODULE70 _ATL_BASE_MODULE;

class CAtlBaseModule :
	public _ATL_BASE_MODULE
{
public :
	static bool m_bInitFailed;
	CAtlBaseModule() throw();
	~CAtlBaseModule() throw ();

	HINSTANCE GetModuleInstance() throw()
	{
		return m_hInst;
	}
	HINSTANCE GetResourceInstance() throw()
	{
		return m_hInstResource;
	}
	HINSTANCE SetResourceInstance(_In_ HINSTANCE hInst) throw()
	{
		return static_cast< HINSTANCE >(InterlockedExchangePointer((void**)&m_hInstResource, hInst));
	}

	bool AddResourceInstance(_In_ HINSTANCE hInst) throw();
	bool RemoveResourceInstance(_In_ HINSTANCE hInst) throw();
	HINSTANCE GetHInstanceAt(_In_ int i) throw();
};

__declspec(selectany) bool CAtlBaseModule::m_bInitFailed = false;
extern CAtlBaseModule _AtlBaseModule;

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

/////////////////////////////////////////////////////////////////////////////
// String resource helpers

#pragma warning(push)
#pragma warning(disable: 4200)
	struct ATLSTRINGRESOURCEIMAGE
	{
		WORD nLength;
		WCHAR achString[];
	};
#pragma warning(pop)	// C4200

inline const ATLSTRINGRESOURCEIMAGE* _AtlGetStringResourceImage(
	_In_ HINSTANCE hInstance,
	_In_ HRSRC hResource,
	_In_ UINT id) throw()
{
	const ATLSTRINGRESOURCEIMAGE* pImage;
	const ATLSTRINGRESOURCEIMAGE* pImageEnd;
	ULONG nResourceSize;
	HGLOBAL hGlobal;
	UINT iIndex;

	hGlobal = ::LoadResource( hInstance, hResource );
	if( hGlobal == NULL )
	{
		return( NULL );
	}

	pImage = (const ATLSTRINGRESOURCEIMAGE*)::LockResource( hGlobal );
	if( pImage == NULL )
	{
		return( NULL );
	}

	nResourceSize = ::SizeofResource( hInstance, hResource );
	pImageEnd = (const ATLSTRINGRESOURCEIMAGE*)(LPBYTE( pImage )+nResourceSize);
	iIndex = id&0x000f;

	while( (iIndex > 0) && (pImage < pImageEnd) )
	{
		pImage = (const ATLSTRINGRESOURCEIMAGE*)(LPBYTE( pImage )+(sizeof( ATLSTRINGRESOURCEIMAGE )+(pImage->nLength*sizeof( WCHAR ))));
		iIndex--;
	}
	if( pImage >= pImageEnd )
	{
		return( NULL );
	}
	if( pImage->nLength == 0 )
	{
		return( NULL );
	}

	return( pImage );
}

inline const ATLSTRINGRESOURCEIMAGE* AtlGetStringResourceImage(
	_In_ HINSTANCE hInstance,
	_In_ UINT id) throw()
{
	HRSRC hResource;
	/*
		The and operation (& static_cast<WORD>(~0)) protects the expression from being greater
		than WORD - this would cause a runtime error when the application is compiled with /RTCc flag.
	*/
	hResource = ::FindResourceW(hInstance, MAKEINTRESOURCEW( (((id>>4)+1) & static_cast<WORD>(~0)) ), (LPWSTR) RT_STRING);
	if( hResource == NULL )
	{
		return( NULL );
	}

	return _AtlGetStringResourceImage( hInstance, hResource, id );
}

inline const ATLSTRINGRESOURCEIMAGE* AtlGetStringResourceImage(
	_In_ HINSTANCE hInstance,
	_In_ UINT id,
	_In_ WORD wLanguage) throw()
{
	HRSRC hResource;
	/*
		The and operation (& static_cast<WORD>(~0)) protects the expression from being greater
		than WORD - this would cause a runtime error when the application is compiled with /RTCc flag.
	*/
	hResource = ::FindResourceExW(hInstance, (LPWSTR) RT_STRING, MAKEINTRESOURCEW( (((id>>4)+1) & static_cast<WORD>(~0)) ), wLanguage);
	if( hResource == NULL )
	{
		return( NULL );
	}

	return _AtlGetStringResourceImage( hInstance, hResource, id );
}

inline const ATLSTRINGRESOURCEIMAGE* AtlGetStringResourceImage(_In_ UINT id) throw()
{
	const ATLSTRINGRESOURCEIMAGE* p = NULL;
	HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);

	for (int i = 1; hInst != NULL && p == NULL; hInst = _AtlBaseModule.GetHInstanceAt(i++))
	{
		p = AtlGetStringResourceImage(hInst, id);
	}
	return p;
}

inline const ATLSTRINGRESOURCEIMAGE* AtlGetStringResourceImage(
	_In_ UINT id,
	_In_ WORD wLanguage) throw()
{
	const ATLSTRINGRESOURCEIMAGE* p = NULL;
	HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);

	for (int i = 1; hInst != NULL && p == NULL; hInst = _AtlBaseModule.GetHInstanceAt(i++))
	{
		p = AtlGetStringResourceImage(hInst, id, wLanguage);
	}
	return p;
}

ATLPREFAST_SUPPRESS(6054)
inline int AtlLoadString(
	_In_ UINT nID,
	_Out_writes_to_(nBufferMax, return + 1) LPTSTR lpBuffer,
	_In_ int nBufferMax) throw()
{
	HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);
	int nRet = 0;

	for (int i = 1; hInst != NULL && nRet == 0; hInst = _AtlBaseModule.GetHInstanceAt(i++))
	{
		nRet = LoadString(hInst, nID, lpBuffer, nBufferMax);
	}
	return nRet;
}
ATLPREFAST_UNSUPPRESS()

inline HINSTANCE AtlFindResourceInstance(
	_In_z_ LPCTSTR lpName,
	_In_z_ LPCTSTR lpType,
	_In_ WORD wLanguage = 0) throw()
{
	ATLASSERT(lpType != RT_STRING);	// Call AtlFindStringResourceInstance to find the string
	_Analysis_assume_(lpType != NULL);
	if (lpType == RT_STRING)
		return NULL;

	if (ATL_IS_INTRESOURCE(lpType))
	{
		/* Prefast false warnings caused by bad-shaped definition of MAKEINTRESOURCE macro from PSDK */
		if (lpType == ATL_RT_ICON)
		{
			lpType = ATL_RT_GROUP_ICON;
		}
		else if (lpType == ATL_RT_CURSOR)
		{
			lpType = ATL_RT_GROUP_CURSOR;
		}
	}

	HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);
	HRSRC hResource = NULL;

	for (int i = 1; hInst != NULL; hInst = _AtlBaseModule.GetHInstanceAt(i++))
	{
		hResource = ::FindResourceEx(hInst, lpType, lpName, wLanguage);
		if (hResource != NULL)
		{
			return hInst;
		}
	}

	return NULL;
}

inline HINSTANCE AtlFindResourceInstance(
	_In_ UINT nID,
	_In_z_ LPCTSTR lpType,
	_In_ WORD wLanguage = 0) throw()
{
	/*
		The and operation (& static_cast<WORD>(~0)) protects the expression from being greater
		than WORD - this would cause a runtime error when the application is compiled with /RTCc flag.
	*/
	return AtlFindResourceInstance(MAKEINTRESOURCE(nID & static_cast<WORD>(~0)), lpType, wLanguage);
}

inline HINSTANCE AtlFindStringResourceInstance(
	_In_ UINT nID,
	_In_ WORD wLanguage = 0) throw()
{
	const ATLSTRINGRESOURCEIMAGE* p = NULL;
	HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);

	for (int i = 1; hInst != NULL && p == NULL; hInst = _AtlBaseModule.GetHInstanceAt(i++))
	{
		p = AtlGetStringResourceImage(hInst, nID, wLanguage);
		if (p != NULL)
			return hInst;
	}

	return NULL;
}

#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

/*
Needed by both atlcomcli and atlsafe, so needs to be in here
*/
inline HRESULT AtlSafeArrayGetActualVartype(
	_In_ SAFEARRAY *psaArray,
	_Out_ VARTYPE *pvtType)
{
	HRESULT hrSystem=::SafeArrayGetVartype(psaArray, pvtType);

	if(FAILED(hrSystem))
	{
		return hrSystem;
	}

	/*
	When Windows has a SAFEARRAY of type VT_DISPATCH with FADF_HAVEIID,
	it returns VT_UNKNOWN instead of VT_DISPATCH. We patch the value to be correct
	*/
	if(pvtType && *pvtType==VT_UNKNOWN)
	{
		if(psaArray && ((psaArray->fFeatures & FADF_HAVEIID)!=0))
		{
			if(psaArray->fFeatures & FADF_DISPATCH)
			{
				*pvtType=VT_DISPATCH;
			}
		}
	}

	return hrSystem;
}
template <typename _CharType>
inline _CharType* AtlCharNext(_In_ const _CharType* p) throw()
{
	ATLASSUME(p != NULL);	// Too expensive to check separately here
	if (*p == '\0')  // ::CharNextA won't increment if we're at a \0 already
		return const_cast<_CharType*>(p+1);
	else
		return ::CharNextA(p);
}

template <>
inline wchar_t* AtlCharNext<wchar_t>(_In_ const wchar_t* p) throw()
{
	return const_cast< wchar_t* >( p+1 );
}
template<typename CharType>
inline const CharType* AtlstrchrT(
	_In_z_ const CharType* p,
	_In_ CharType ch) throw()
{
	ATLASSERT(p != NULL);
	if(p==NULL)
	{
		return NULL;
	}
	while( *p != 0 )
	{
		if (*p == ch)
		{
			return p;
		}
		p = AtlCharNext(p);
	}
	//strchr for '\0' should succeed - the while loop terminates
	//*p == 0, but ch also == 0, so NULL terminator address is returned
	return (*p == ch) ? p : NULL;
}
//Ansi and Unicode versions of printf, used with templated CharType trait classes.
#pragma warning(push)
#pragma warning(disable : 4793)
template<typename CharType>
inline int AtlprintfT(_In_z_ _Printf_format_string_ const CharType* pszFormat,...) throw()
{
	int retval=0;
	va_list argList;
	va_start( argList, pszFormat );
	retval=vprintf(pszFormat,argList);
	va_end( argList );
	return retval;
}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4793)
template<>
inline int AtlprintfT(_In_z_ _Printf_format_string_ const wchar_t* pszFormat,... ) throw()
{
	int retval=0;
	va_list argList;
	va_start( argList, pszFormat );
#if _MSC_VER < 1900
	retval = vwprintf(pszFormat, argList);
#else
	// Explicitly request the legacy wide format specifiers mode from the CRT,
	// for compatibility with previous versions.  While the CRT supports two
	// modes, the ATL and MFC functions that accept format strings only support
	// legacy mode format strings.
	retval = __stdio_common_vfwprintf(
		_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS |
		_CRT_INTERNAL_PRINTF_LEGACY_WIDE_SPECIFIERS,
		stdout, pszFormat, NULL, argList);
#endif
	va_end( argList );
	return retval;
}
#pragma warning(pop)

inline BOOL AtlConvertSystemTimeToVariantTime(
	_In_ const SYSTEMTIME& systimeSrc,
	_Out_ double* pVarDtTm)
{
	ATLENSURE(pVarDtTm!=NULL);
	//Convert using ::SystemTimeToVariantTime and store the result in pVarDtTm then
	//convert variant time back to system time and compare to original system time.
	BOOL ok = ::SystemTimeToVariantTime(const_cast<SYSTEMTIME*>(&systimeSrc), pVarDtTm);
	SYSTEMTIME sysTime;
	::ZeroMemory(&sysTime, sizeof(SYSTEMTIME));

	ok = ok && ::VariantTimeToSystemTime(*pVarDtTm, &sysTime);
	ok = ok && (systimeSrc.wYear == sysTime.wYear &&
			systimeSrc.wMonth == sysTime.wMonth &&
			systimeSrc.wDay == sysTime.wDay &&
			systimeSrc.wHour == sysTime.wHour &&
			systimeSrc.wMinute == sysTime.wMinute &&
			systimeSrc.wSecond == sysTime.wSecond);

	return ok;
}

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

/////////////////////////////////////////////////////////////////////////////
// DLL Load Helper

inline HMODULE AtlLoadSystemLibraryUsingFullPath(_In_z_ const WCHAR *pszLibrary)
{
#if (_ATL_NTDDI_MIN > NTDDI_WIN7)
	return(::LoadLibraryExW(pszLibrary, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
#else
#ifndef _USING_V110_SDK71_
	// the LOAD_LIBRARY_SEARCH_SYSTEM32 flag for LoadLibraryExW is only supported if the DLL-preload fixes are installed, so
	// use LoadLibraryExW only if SetDefaultDllDirectories is available (only on Win8, or with KB2533623 on Vista and Win7)...
	IFDYNAMICGETCACHEDFUNCTION(L"kernel32.dll", SetDefaultDllDirectories, pfSetDefaultDllDirectories)
	{
		return(::LoadLibraryExW(pszLibrary, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
	}

	// ...otherwise fall back to using LoadLibrary from the SYSTEM32 folder explicitly.
#endif
	WCHAR wszLoadPath[MAX_PATH+1];
	UINT rc = ::GetSystemDirectoryW(wszLoadPath, _countof(wszLoadPath));
	if (rc == 0 || rc >= _countof(wszLoadPath))
	{
		return NULL;
	}

	if (wszLoadPath[rc-1] != L'\\')
	{
		if (wcscat_s(wszLoadPath, _countof(wszLoadPath), L"\\") != 0)
		{
			return NULL;
		}
	}

	if (wcscat_s(wszLoadPath, _countof(wszLoadPath), pszLibrary) != 0)
	{
		return NULL;
	}

	return(::LoadLibraryW(wszLoadPath));
#endif
}

/////////////////////////////////////////////////////////////////////////////

#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

}	// namespace ATL
#pragma pack(pop)

#ifdef _ATL_ALL_WARNINGS
#pragma warning( pop )
#endif

#endif	// __ATLCORE_H__
