// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLWINVERAPI_H__
#define __ATLWINVERAPI_H__

#pragma once

#ifdef _ATL_ALL_WARNINGS
#pragma warning( push )
#endif

#pragma warning(disable: 4786) // identifier was truncated in the debug information
#pragma warning(disable: 4127) // constant expression

#include <atldef.h>
#include <windows.h>
#include <ole2.h>
#include <sdkddkver.h>

// This file contains declarations of wrappers for methods used
// in ATL that are only available in later versions of Windows.

// When the minimum supported version of Windows is increased, the
// implementations of these methods just call the real Windows APIs.

// Minimum supported versions of Windows:
// Windows XP SP2 for x86 and x64, Windows 8 for ARM

#if defined(_M_IX86) || defined(_M_X64)
#define _ATL_NTDDI_MIN NTDDI_WINXPSP2
#else
#define _ATL_NTDDI_MIN NTDDI_WIN8
#endif

// Use this macro for loading a local cached function from a DLL that is known to be loaded (e.g. KERNEL32)
#define IFDYNAMICGETCACHEDFUNCTION(libraryname, functionname, functionpointer) \
	static volatile auto functionpointer##_cache = reinterpret_cast<decltype(::functionname)*>(NULL); \
	auto functionpointer = reinterpret_cast<decltype(::functionname)*>(functionpointer##_cache); \
	if (functionpointer == reinterpret_cast<decltype(::functionname)*>(NULL)) \
	{ \
		HINSTANCE hLibrary = GetModuleHandleW(libraryname); \
		if (hLibrary != NULL) \
		{ \
			functionpointer = reinterpret_cast<decltype(::functionname)*>(::GetProcAddress(hLibrary, #functionname)); \
			functionpointer##_cache = reinterpret_cast<decltype(::functionname)*>(::EncodePointer((PVOID)functionpointer)); \
		} \
	} \
	else \
	{ \
		functionpointer = reinterpret_cast<decltype(::functionname)*>(::DecodePointer((PVOID)functionpointer)); \
	} \
	if (functionpointer != reinterpret_cast<decltype(::functionname)*>(NULL))


#pragma pack(push,_ATL_PACKING)
namespace ATL
{
	inline BOOL _AtlInitializeCriticalSectionEx(__out LPCRITICAL_SECTION lpCriticalSection, __in DWORD dwSpinCount, __in DWORD Flags)
	{
	#if (_ATL_NTDDI_MIN >= NTDDI_VISTA) || !defined(_ATL_USE_WINAPI_FAMILY_DESKTOP_APP)
		return InitializeCriticalSectionEx(lpCriticalSection, dwSpinCount, Flags);
	#else

	#if (NTDDI_VERSION >= NTDDI_VISTA)
		// use InitializeCriticalSectionEx if it is available (only on Vista+)...
		IFDYNAMICGETCACHEDFUNCTION(L"kernel32.dll", InitializeCriticalSectionEx, pfInitializeCriticalSectionEx)
		{
			return (*pfInitializeCriticalSectionEx)(lpCriticalSection, dwSpinCount, Flags);
		}
	#else
		UNREFERENCED_PARAMETER(Flags);
	#endif

		// ...otherwise fall back to using InitializeCriticalSectionAndSpinCount.
		return InitializeCriticalSectionAndSpinCount(lpCriticalSection, dwSpinCount);
	#endif
	}

	int _AtlLCMapStringEx(_In_opt_ LPCWSTR lpLocaleName, _In_ DWORD dwMapFlags, _In_ LPCWSTR lpSrcStr, _In_ int cchSrc, _Out_opt_ LPWSTR lpDestStr, _In_ int cchDest, _In_opt_ LPNLSVERSIONINFO lpVersionInformation, _In_opt_ LPVOID lpReserved, _In_opt_ LPARAM sortHandle);

	// Needed for downlevel NLS APIs
	LCID _AtlDownlevelLocaleNameToLCID(LPCWSTR localeName);
	int  _AtlDownlevelLCIDToLocaleName(LCID lcid, LPWSTR outLocaleName, int cchLocaleName);

}	// namespace ATL
#pragma pack(pop)

#ifdef _ATL_ALL_WARNINGS
#pragma warning( pop )
#endif

#endif	// __ATLWINVERAPI_H__
