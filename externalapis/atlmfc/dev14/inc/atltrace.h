// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLTRACE_H__
#define __ATLTRACE_H__

#pragma once

#include <atldef.h>
#include <atlconv.h>
#include <crtdbg.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>

//
// Tracing mechanism doesn't require AtlTraceTool anymore
// Output from the tracing is passed to _CrtDbgReportW
// If you need to override reporting functionality then you should use CRT Debug Routines
//

#pragma pack(push,_ATL_PACKING)
namespace ATL
{

template<unsigned int traceCategory = 0x80000 /* TraceCategories::TraceUser  */, unsigned int traceLevel = 0>
class CTraceCategoryEx
{
public:
	enum {
		TraceGeneral = 0x000001,
		TraceCom = 0x000002,
		TraceQI = 0x000004,
		TraceRegistrar = 0x000008,
		TraceRefcount = 0x000010,
		TraceWindowing = 0x000020,
		TraceControls = 0x000040,
		TraceHosting = 0x000080,
		TraceDBClient = 0x000100,
		TraceDBProvider = 0x000200,
		TraceSnapin = 0x000400,
		TraceNotImpl = 0x000800,
		TraceAllocation = 0x001000,
		TraceException = 0x002000,
		TraceTime = 0x004000,
		TraceCache = 0x008000,
		TraceStencil = 0x010000,
		TraceString = 0x020000,
		TraceMap = 0x040000,
		TraceUtil = 0x080000,
		TraceSecurity = 0x100000,
		TraceSync = 0x200000,
		TraceISAPI = 0x400000,
		TraceUser = 0x80000
	} TraceCategories;

	explicit CTraceCategoryEx(_In_z_ LPCTSTR pszCategoryName = nullptr) throw();

#ifdef _DEBUG
	static unsigned int GetLevel() throw()
	{
		return traceLevel;
	}
	
	static unsigned int GetCategory()
	{
		return traceCategory;
	}
#endif

	operator unsigned int() throw()
	{
#ifdef _DEBUG
		return traceCategory;
#else
		return 0;
#endif
	}
};

// Backward compatibility
class CTraceCategory : public CTraceCategoryEx<>
{
public:
	CTraceCategory(_In_z_ LPCTSTR pszCategoryName = nullptr) : CTraceCategoryEx(pszCategoryName)
	{
	}
};

__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceGeneral> atlTraceGeneral(_T("atlTraceGeneral"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceCom> atlTraceCOM(_T("atlTraceCOM"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceQI> atlTraceQI(_T("atlTraceQI"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceRegistrar> atlTraceRegistrar(_T("atlTraceRegistrar"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceRefcount> atlTraceRefcount(_T("atlTraceRefcount"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceWindowing> atlTraceWindowing(_T("atlTraceWindowing"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceControls> atlTraceControls(_T("atlTraceControls"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceHosting> atlTraceHosting(_T("atlTraceHosting"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceDBClient> atlTraceDBClient(_T("atlTraceDBClient"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceDBProvider> atlTraceDBProvider(_T("atlTraceDBProvider"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceSnapin> atlTraceSnapin(_T("atlTraceSnapin"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceNotImpl> atlTraceNotImpl(_T("atlTraceNotImpl"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceAllocation> atlTraceAllocation(_T("atlTraceAllocation"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceException> atlTraceException(_T("atlTraceException"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceTime> atlTraceTime(_T("atlTraceTime"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceCache> atlTraceCache(_T("atlTraceCache"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceStencil> atlTraceStencil(_T("atlTraceStencil"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceString> atlTraceString(_T("atlTraceString"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceMap> atlTraceMap(_T("atlTraceMap"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceUtil> atlTraceUtil(_T("atlTraceUtil"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceSecurity> atlTraceSecurity(_T("atlTraceSecurity"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceSync> atlTraceSync(_T("atlTraceSync"));
__declspec(selectany) CTraceCategoryEx<CTraceCategoryEx<>::TraceISAPI> atlTraceISAPI(_T("atlTraceISAPI"));

#ifdef _DEBUG

class CTrace
{
private:
	static errno_t BeginErrorCheck()
	{
		return errno;
	}

	static wchar_t* GetCategoryName(unsigned int nCategory)
	{
		for(unsigned int i = 0; i < m_nLastCategory; i++) 
		{
			if (m_nMap[i].nCategory == nCategory)
			{
				return m_nMap[i].categryName;
			}
		}

		return nullptr;
	}

	static void __cdecl TraceV(
			_In_opt_z_ const char *pszFileName,
			_In_ int nLine,
			_In_ unsigned int dwCategory,
			_In_ unsigned int nLevel,
			_In_z_ LPCWSTR pwszMessage)
	{
		if (CTrace::m_nLevel == CTrace::DisableTracing || CTrace::m_nLevel < nLevel || (CTrace::m_nCategory & dwCategory) == 0)
		{
			return;
		}

		wchar_t wszCategory[TraceBufferSize] = {'\0'};
		int categoryLength = 0;
		const wchar_t *const pwszCategoryName = GetCategoryName(dwCategory);
		if (pwszCategoryName != nullptr)
		{
			if ((categoryLength = swprintf_s(wszCategory, TraceBufferSize, L"%ls - ", pwszCategoryName)) == -1)
			{
				return;
			}
		}
		else
		{
			if ((categoryLength = swprintf_s(wszCategory, TraceBufferSize, L"%u - ", dwCategory)) == -1)
			{
				return;
			}
		}

		int chCount = (int)wcslen(pwszMessage) + categoryLength + 1;
		CHeapPtr<wchar_t> wszBuf;
		if (!wszBuf.Allocate(chCount))
		{
			return;
		}

		wszBuf[0] = '\0';

		if (swprintf_s(wszBuf, chCount, L"%ls%ls", wszCategory, pwszMessage) == -1)
		{
			return;
		}

		wchar_t fileName[_MAX_PATH] = {'\0'};	
#if _MSC_VER < 1900
		if (swprintf_s(fileName, _MAX_PATH, L"%S", pszFileName) == -1)
#else
		if (swprintf_s(fileName, _MAX_PATH, L"%hs", pszFileName) == -1)
#endif
		{
			return;
		}

		_CrtDbgReportW(_CRT_WARN, fileName, nLine, nullptr, L"%ls", static_cast<const wchar_t*>(wszBuf));
	}

public:
	enum {
		DefaultTraceLevel = 0,
		MaxLengthOfCategoryName = 0x80,
		MaxCategoryArray = sizeof(unsigned int) * 8, // 32 category names possible
		TraceBufferSize = 1024,
		DisableTracing = 0xffffffff,
		EnableAllCategories = DisableTracing
	};

	static unsigned int GetLevel()
	{
		return m_nLevel;
	}

	static void SetLevel(_In_ unsigned int nLevel)
	{
		m_nLevel = nLevel;
	}

	static unsigned int GetCategories()
	{
		return m_nCategory;
	}

	static void SetCategories(_In_ unsigned int nCategory)
	{
		m_nCategory = nCategory;
	}

	static void __cdecl TraceV(
		_In_opt_z_ const char *pszFileName,
		_In_ int nLine,
		_In_ unsigned int dwCategory,
		_In_ unsigned int nLevel,
		_In_z_ LPCSTR pszFmt, 
		_In_ va_list args)
	{
		int cchNeeded = _vscprintf(pszFmt, args);
		if (cchNeeded < 0)
		{
			return;
		}

		CHeapPtr<char> szBuf;
		if (!szBuf.Allocate(cchNeeded + 1))
		{
			return;
		}

		szBuf[0] = '\0';

		if (_vsnprintf_s(szBuf, cchNeeded + 1, cchNeeded, pszFmt, args) == -1)
		{
			return;
		}

		CHeapPtr<wchar_t> wszBuf;
		if (!wszBuf.Allocate(cchNeeded + 1))
		{
			return;
		}

		wszBuf[0] = '\0';

		if (::MultiByteToWideChar(CP_ACP, 0, szBuf, -1, wszBuf, cchNeeded + 1) == 0)
		{
			return;
		}

		TraceV(pszFileName, nLine, dwCategory, nLevel, wszBuf);
	}

	static void __cdecl TraceV(
		_In_opt_z_ const char *pszFileName,
		_In_ int nLine,
		_In_ unsigned int dwCategory,
		_In_ unsigned int nLevel,
		_In_z_ LPCWSTR pwszFmt,
		_In_ va_list args)
	{
#if _MSC_VER < 1900
		int cchNeeded = _vscwprintf(pwszFmt, args);
#else
		// Explicitly request the legacy wide format specifiers mode from the CRT,
		// for compatibility with previous versions.  While the CRT supports two
		// modes, the ATL and MFC functions that accept format strings only support
		// legacy mode format strings.
		int cchNeeded = __stdio_common_vswprintf(
			_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS |
			_CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR |
			_CRT_INTERNAL_PRINTF_LEGACY_WIDE_SPECIFIERS,
			NULL, 0, pwszFmt, NULL, args);
#endif
		if (cchNeeded < 0)
		{
			return;
		}

		CHeapPtr<wchar_t> wszBuf;
		if (!wszBuf.Allocate(cchNeeded + 1))
		{
			return;
		}

		wszBuf[0] = '\0';

#if _MSC_VER < 1900
		if (_vsnwprintf_s(wszBuf, cchNeeded + 1, cchNeeded, pwszFmt, args) == -1)
#else
		// Explicitly request the legacy wide format specifiers mode from the CRT,
		// for compatibility with previous versions.  While the CRT supports two
		// modes, the ATL and MFC functions that accept format strings only support
		// legacy mode format strings.
		int const vsnwprintf_result = __stdio_common_vsnwprintf_s(
			_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS |
			_CRT_INTERNAL_PRINTF_LEGACY_WIDE_SPECIFIERS,
			wszBuf, cchNeeded + 1, cchNeeded, pwszFmt, NULL, args);
		if (vsnwprintf_result < 0)
#endif
		{
			return;
		}

		TraceV(pszFileName, nLine, dwCategory, nLevel, wszBuf);
	}

	static void RegisterCategory(_In_z_ LPCTSTR pszCategory, unsigned int nCategory)
	{
		if (pszCategory == nullptr)
		{
			return;
		}

		if (m_nLastCategory >= MaxCategoryArray) 
		{
			ATLASSERT(false && "Too many categories defined");
			return;
		}

		m_nMap[m_nLastCategory].nCategory = nCategory;
#ifdef _UNICODE
		wcscpy_s(m_nMap[m_nLastCategory].categryName, MaxLengthOfCategoryName - 1, pszCategory);
#else
		wchar_t buffer[MaxLengthOfCategoryName] = { 0 };	
#if _MSC_VER < 1900
		swprintf_s(buffer, MaxLengthOfCategoryName - 1, L"%S", pszCategory);
#else
		swprintf_s(buffer, MaxLengthOfCategoryName - 1, L"%hs", pszCategory);
#endif
		wcscpy_s(m_nMap[m_nLastCategory].categryName, MaxLengthOfCategoryName - 1, buffer);
#endif
	
		m_nLastCategory++;
	}

	static bool IsTracingEnabled(
		_In_ DWORD_PTR dwCategory,
		_In_ UINT nLevel)
	{
		return CTrace::m_nLevel != CTrace::DisableTracing && CTrace::m_nLevel <= nLevel && (CTrace::m_nCategory & dwCategory) != 0;
	}
protected:
	typedef struct {
		unsigned int nCategory;
		wchar_t categryName[MaxLengthOfCategoryName];
	} CategoryMap;

	static unsigned int m_nLevel;
	static unsigned int m_nCategory;
	static unsigned int m_nLastCategory;
	static CategoryMap m_nMap[MaxCategoryArray];
};

__declspec(selectany) unsigned int CTrace::m_nLevel = static_cast<unsigned int>(CTrace::DefaultTraceLevel);
__declspec(selectany) unsigned int CTrace::m_nCategory = static_cast<unsigned int>(CTrace::EnableAllCategories);
__declspec(selectany) unsigned int CTrace::m_nLastCategory = 0;
__declspec(selectany) CTrace::CategoryMap CTrace::m_nMap[CTrace::MaxCategoryArray] = { 0 };

inline bool IsTracingEnabled(
	_In_ DWORD_PTR dwCategory,
	_In_ UINT nLevel)
{
	return CTrace::IsTracingEnabled(dwCategory, nLevel);
}

class CPreserveLastError
{
public:
	CPreserveLastError()
	{
		m_dwLastError = GetLastError();
	}

	~CPreserveLastError()
	{
		SetLastError(m_dwLastError);
	}

private:
	DWORD m_dwLastError;
};

class CTraceFileAndLineInfo
{
public:
	CTraceFileAndLineInfo(
			_In_z_ const char *pszFileName,
			_In_ int nLineNo)
		: m_pszFileName(pszFileName), m_nLineNo(nLineNo)
	{
	}

#pragma warning(push)
#pragma warning(disable : 4793)
	void __cdecl operator()(
		_In_ int dwCategory,
		_In_ UINT nLevel,
		_In_z_ const char *pszFmt, 
		...) const
	{
		CPreserveLastError ple;
		va_list ptr; va_start(ptr, pszFmt);
		ATL::CTrace::TraceV(m_pszFileName, m_nLineNo, dwCategory, nLevel, pszFmt, ptr);
		va_end(ptr);
	}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4793)
	void __cdecl operator()(
		_In_ int dwCategory,
		_In_ UINT nLevel,
		_In_z_ const wchar_t *pszFmt, 
		...) const
	{
		CPreserveLastError ple;
		va_list ptr; va_start(ptr, pszFmt);
		ATL::CTrace::TraceV(m_pszFileName, m_nLineNo, dwCategory, nLevel, pszFmt, ptr);
		va_end(ptr);
	}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4793)
	void __cdecl operator()(
		_In_z_ const char *pszFmt, 
		...) const
	{
		CPreserveLastError ple;
		va_list ptr; va_start(ptr, pszFmt);
		ATL::CTrace::TraceV(m_pszFileName, m_nLineNo, atlTraceGeneral, 0, pszFmt, ptr);
		va_end(ptr);
	}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4793)
	void __cdecl operator()(
		_In_z_ const wchar_t *pszFmt, 
		...) const
	{
		CPreserveLastError ple;
		va_list ptr; va_start(ptr, pszFmt);
		ATL::CTrace::TraceV(m_pszFileName, m_nLineNo, atlTraceGeneral, 0, pszFmt, ptr);
		va_end(ptr);
	}
#pragma warning(pop)

private:
	/* unimplemented */
	CTraceFileAndLineInfo &__cdecl operator=(_In_ const CTraceFileAndLineInfo &right);

	const char *const m_pszFileName;
	const int m_nLineNo;
};

template<unsigned int traceCategory, unsigned int traceLevel>
inline CTraceCategoryEx<traceCategory, traceLevel>::CTraceCategoryEx(_In_z_ LPCTSTR pszCategoryName) throw()
{
	CTrace::RegisterCategory(pszCategoryName, traceCategory);
}

#else // _DEBUG

inline bool IsTracingEnabled(
	_In_ DWORD_PTR,
	_In_ UINT)
{
	return false;
}

template<unsigned int traceCategory, unsigned int traceLevel>
inline CTraceCategoryEx<traceCategory, traceLevel>::CTraceCategoryEx(_In_z_ LPCTSTR pszCategoryName) throw()
{
	(void)pszCategoryName;
}

#endif  // _DEBUG


#ifdef _DEBUG

#ifndef _ATL_NO_DEBUG_CRT
class CNoUIAssertHook
{
public:
	CNoUIAssertHook()
	{
		ATLASSERT( s_pfnPrevHook == NULL );
		s_pfnPrevHook = _CrtSetReportHook(CrtHookProc);
	}
	~CNoUIAssertHook()
	{
		_CrtSetReportHook(s_pfnPrevHook);
		s_pfnPrevHook = NULL;
	}

private:
	static int __cdecl CrtHookProc(
		_In_ int eReportType,
		_In_z_ char* pszMessage,
		_Inout_ int* pnRetVal)
	{

		if (eReportType == _CRT_ASSERT)
		{
			::OutputDebugStringA( "ASSERTION FAILED\n" );
			::OutputDebugStringA( pszMessage );
			//If caller doesn't want retVal, so be it.
			if (pnRetVal != NULL)
			{
				*pnRetVal = 1;
			}
			return TRUE;
		}

		if (s_pfnPrevHook != NULL)
		{
			return s_pfnPrevHook(eReportType, pszMessage, pnRetVal);
		}
		else
		{
			return FALSE;
		}
	}

private:
	static _CRT_REPORT_HOOK s_pfnPrevHook;
};

__declspec( selectany ) _CRT_REPORT_HOOK CNoUIAssertHook::s_pfnPrevHook = NULL;

#define DECLARE_NOUIASSERT() ATL::CNoUIAssertHook _g_NoUIAssertHook;

#endif  // _ATL_NO_DEBUG_CRT

#ifndef ATLTRACE
#define ATLTRACE ATL::CTraceFileAndLineInfo(__FILE__, __LINE__)
#define ATLTRACE2 ATLTRACE
#endif

#pragma warning(push)
#pragma warning(disable : 4793)
inline void __cdecl AtlTrace(_In_z_ _Printf_format_string_ LPCSTR pszFormat, ...)
{
	CPreserveLastError ple;
	va_list ptr;
	va_start(ptr, pszFormat);
	ATL::CTrace::TraceV(NULL, -1, atlTraceGeneral, 0, pszFormat, ptr);
	va_end(ptr);
}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4793)
inline void __cdecl AtlTrace(_In_z_ _Printf_format_string_ LPCWSTR pszFormat, ...)
{
	CPreserveLastError ple;
	va_list ptr;
	va_start(ptr, pszFormat);
	ATL::CTrace::TraceV(NULL, -1, atlTraceGeneral, 0, pszFormat, ptr);
	va_end(ptr);
}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4793)
inline void __cdecl AtlTrace2(
	_In_ int dwCategory,
	_In_ UINT nLevel,
	_In_z_ _Printf_format_string_ LPCSTR pszFormat, ...)
{
	CPreserveLastError ple;
	va_list ptr;
	va_start(ptr, pszFormat);
	ATL::CTrace::TraceV(NULL, -1, dwCategory, nLevel, pszFormat, ptr);
	va_end(ptr);
}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4793)
inline void __cdecl AtlTrace2(
	_In_ int dwCategory,
	_In_ UINT nLevel,
	_In_z_ _Printf_format_string_ LPCWSTR pszFormat, ...)
{
	CPreserveLastError ple;
	va_list ptr;
	va_start(ptr, pszFormat);
	ATL::CTrace::TraceV(NULL, -1, dwCategory, nLevel, pszFormat, ptr);
	va_end(ptr);
}
#pragma warning(pop)

#define ATLTRACENOTIMPL(funcname)  do { ATLTRACE(ATL::atlTraceNotImpl, 0, _T("ATL: %Ts not implemented.\n"), funcname); return E_NOTIMPL; } while(0)

#else // !DEBUG

#pragma warning(push)
#pragma warning(disable : 4793)
inline void __cdecl AtlTraceNull(...)
{
}
inline void __cdecl AtlTrace(
	_In_z_ _Printf_format_string_ LPCSTR, ...)
{
}
inline void __cdecl AtlTrace2(
	_In_ DWORD_PTR,
	_In_ UINT,
	_In_z_ _Printf_format_string_ LPCSTR, ...)
{
}
inline void __cdecl AtlTrace(
	_In_z_ _Printf_format_string_ LPCWSTR, ...)
{
}
inline void __cdecl AtlTrace2(
	_In_ DWORD_PTR,
	_In_ UINT,
	_In_z_ _Printf_format_string_ LPCWSTR, ...)
{
}
#pragma warning(pop)

#ifndef ATLTRACE

#define ATLTRACE            __noop
#define ATLTRACE2           __noop
#endif //ATLTRACE
#define ATLTRACENOTIMPL(funcname)   return E_NOTIMPL
#define DECLARE_NOUIASSERT()

#endif //!_DEBUG

// Macro was kept for backward compatibility with WTL
#ifdef _DEBUG
#define DECLARE_TRACE_CATEGORY( name ) extern ::ATL::CTraceCategory name;
#else
#define DECLARE_TRACE_CATEGORY( name ) const ::ATL::CTraceCategory name;
#endif


};  // namespace ATL
#pragma pack(pop)

#endif  // __ATLTRACE_H__
