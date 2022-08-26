//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Functions for dealing with resources and the international DLL.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#ifdef BUILDING_VB7TO8
#define DLLNAME L"vb7to8ui.dll"
#else
#if HOSTED
#define DLLNAME VBHDLLNAME
#else
#define DLLNAME L"VBC7ui.dll"
#endif
#endif // BUILDING_VB7TO8

#if HOSTED
extern VBComModule _Module;
#else
extern CComModule _Module;
#endif

#ifdef BUILDING_VBC
extern LCID g_watsonLcid;
#else
LCID g_watsonLcid;
#endif

static BOOL g_bAttemptedLoad = FALSE;

#define BUFLEN 40
UINT GetOEMCodePageFromLCID(LCID id);
UINT GetANSICodePageFromLCID(LCID id);

// SafeGetUserDefaultUILanguage -- only on win2k/xp
LANGID SafeGetUserDefaultUILanguage()
{
    // Test if the platform has the GetUserDefaultUILanguage() API.
    // This is supported by Windows 2000/XP only.
    typedef LANGID (GET_USER_DEFAULT_UI_LANGUAGE)(VOID);

    HINSTANCE hKernel32 = LoadLibraryW(L"Kernel32.dll");

    if (hKernel32 != NULL)
    {
        GET_USER_DEFAULT_UI_LANGUAGE* pGetUserDefaultUILanguage =
            (GET_USER_DEFAULT_UI_LANGUAGE*)GetProcAddress(hKernel32, "GetUserDefaultUILanguage");
        FreeLibrary(hKernel32);

        if (pGetUserDefaultUILanguage != NULL)
        {
            LANGID uiLangID = (*pGetUserDefaultUILanguage)();
            if (uiLangID != 0)
            {
                return uiLangID;
            }
        }
    }
    else
    {
        VSFAIL("Expected to load full path to kernel32.dll");
    }

    // return zero if not available
    return 0;
}

// Gets the last error and turns it into an hresult.
HRESULT HrOfLastError()
{
    long lErr = GetLastError();
    return lErr ? HRESULT_FROM_WIN32(lErr) : E_OUTOFMEMORY;
}

// Callback used to find and load the satellite DLL that contains the compiler's
// localizable strings.
//
// This routine currently assumes that the resource DLL for a specific
// locale will be located in a subdirectory names that LCID directly
// under the current location of VBC/VB7TO8.  We may want to consider linking some
// defaults directly into VBC/VB7TO8.
//
// The searching order is:
//  1)  User Locale
//  1a) User Locale with SUBLANG_DEFAULT
//  2)  System Locale
//  2a) System Locale with SUBLANG_DEFAULT
//  3)  English!
//  4)  The same directory VBC/VB7TO8 is being run from
//  5)  First satalite DLL found in resources LCID dirs if none of the above
HRESULT __stdcall DelayLoadUICallback(HINSTANCE * phinst)
{
    WCHAR wszCurrentDir[MAX_PATH]; // Used with GetModuleFileName
    WCHAR wszUIDllName[1000];
    WCHAR wszFullPathUIDllName[2000];

    // Init out parmameter
    *phinst = NULL;

#if HOSTED && UIDLL_FRAMEWORKLOOKUP

    BSTR bstrFrameworkPath = ::GetNDPSystemPath();

    HRESULT hrNdpPathCopy = StringCchCopyW(
        wszCurrentDir,
        sizeof(wszCurrentDir) / sizeof(WCHAR),
        bstrFrameworkPath);

    ::SysFreeString(bstrFrameworkPath);

    if (FAILED(hrNdpPathCopy))
    {
        return hrNdpPathCopy;
    }

#else // HOSTED && UIDLL_FRAMEWORKLOOKUP

    // Get the base path and file name of the dll to load.
    if (!GetModuleFileName(
#if HOSTED
        //VB Hosted is not an executable file; needs to provide the module's HINSTANCE.
        g_hinstDll,
#else // HOSTED
        NULL,
#endif // HOSTED
        wszCurrentDir, sizeof(wszCurrentDir) / sizeof(WCHAR)))
    {
        return HrOfLastError();
    }

    *PathFindName(wszCurrentDir) = 0;

#endif // HOSTED && UIDLL_FRAMEWORKLOOKUP

    HRESULT hrStringCopy = StringCchCopyW(
        wszUIDllName,
        sizeof(wszUIDllName) / sizeof(WCHAR),
        DLLNAME);

    if (FAILED(hrStringCopy))
    {
        return hrStringCopy;
    }

    //
    // Set up the order to search.
    //

    LANGID userDefaultUILanguage = SafeGetUserDefaultUILanguage();

    LCID lcidUser = GetUserDefaultLCID();
    LCID lcidSystem = GetSystemDefaultLCID();
    LCID lcid[] =
    {
        MAKELCID(userDefaultUILanguage, SORT_DEFAULT),
        ConvertDefaultLocale(
            MAKELCID(MAKELANGID(PRIMARYLANGID(userDefaultUILanguage),
            SUBLANG_NEUTRAL),
            SORT_DEFAULT)),
        lcidUser,
        MAKELCID(
            MAKELANGID(PRIMARYLANGID(lcidUser),
            SUBLANG_DEFAULT),
            SORTIDFROMLCID(lcidUser)),
        MAKELCID(
            MAKELANGID(PRIMARYLANGID(lcidUser),
            SUBLANG_NEUTRAL),
            SORTIDFROMLCID(lcidUser)),
        lcidSystem,
        MAKELCID(
            MAKELANGID(PRIMARYLANGID(lcidSystem),
            SUBLANG_DEFAULT),
            SORTIDFROMLCID(lcidSystem)),
        MAKELCID(
            MAKELANGID(PRIMARYLANGID(lcidSystem),
            SUBLANG_NEUTRAL),
            SORTIDFROMLCID(lcidSystem)),
        0x409,  // even though we embed 0x0409 in the exe, we still look for it on disk in case
                // LOC ships bug fixes for the resources.
    };
    UINT uiConsoleOutputCP = GetConsoleOutputCP();

    LCID lcidUsed = -1;

    //
    // Loop through each LCID, create the path and check to see if a DLL at that
    // path exists.
    //

    for (int i = 0; i < sizeof(lcid) / sizeof(LCID); i++)
    {
        // Skip null lcids
        if (lcid[i] == 0)
            continue;

        // Check if its the same as any LCID already checked,
        // which is very possible
        //
        for (int n = 0; n < i; n++)
        {
            if (lcid[n] == lcid[i])
            {
                goto NextLcid;
            }
        }

#if !HOSTED // the Hosted Expression Compiler returns all errors as strings. Codepage doesn't matter
        if (lcid[i] != 0x409 && 
            uiConsoleOutputCP != CP_UTF8 && 
            uiConsoleOutputCP != GetOEMCodePageFromLCID(lcid[i]) && 
            uiConsoleOutputCP != GetANSICodePageFromLCID(lcid[i]))
        {
            goto NextLcid;
        }
#endif

        if (SUCCEEDED(StringCchPrintfW(
                            wszFullPathUIDllName,
                            sizeof(wszFullPathUIDllName) / sizeof(WCHAR),
                            L"%s%u\\%s",
                            wszCurrentDir,
                            lcid[i],
                            wszUIDllName)) &&
            (0xFFFFFFFF != GetFileAttributes(wszFullPathUIDllName)))
        {
#ifdef BUILDING_VBC
            // Ensure can get a .NET 4.5 string (SecurityCriticalAsync, introduced in Dev11).
            // This is necessary because .NET 4.5 may overlay .NET 4.0 after .NET 4.0 language
            // packs are installed, leaving only .NET 4.0 localized resource DLLs.
            // The old langauge pack resources (4.0) are missing some entries (errors, etc.)
            // introduced in .NET 4.5.  Skip .NET 4.0 resources even if they match for locale.
            // LoadLibraryEx is safe here--it is a fully-qualified path and is
            // loaded as a data file and thus cannot be spoofed.
            *phinst = LoadLibraryEx(wszFullPathUIDllName, NULL, LOAD_LIBRARY_AS_DATAFILE);
            if (*phinst)
            {
                WCHAR buf[CCH_MAX_LOADSTRING];
                if (LoadString(*phinst, 37060, buf, DIM(buf)) > 0) // 37060 is the most recent message to have been added as of this checkin
                {
#endif
                    // Jump to the load
                    lcidUsed = lcid[i];
                    goto LoadDll;
#ifdef BUILDING_VBC
                }
                FreeLibrary(*phinst);
                *phinst = NULL;
            }
#endif
        }

NextLcid:
        ;
    }

    // None of the default choice exists, so now look to see if the DLL exists
    // in the same directory as VBC/VB7TO8.
    //
    if (SUCCEEDED(StringCchPrintfW(
                        wszFullPathUIDllName,
                        sizeof(wszFullPathUIDllName) / sizeof(WCHAR),
                        L"%s%s",
                        wszCurrentDir,
                        wszUIDllName)) &&
        (0xFFFFFFFF != GetFileAttributes(wszFullPathUIDllName)))
    {
        // Jump to the load
        goto LoadDll;
    }

    {
        // Look for the first resource dll in any LCID subdirectory.
        if (SUCCEEDED(StringCchPrintfW(
                            wszFullPathUIDllName,
                            sizeof(wszFullPathUIDllName) / sizeof(WCHAR),
                            L"%s*.*",
                            wszCurrentDir)))
        {
            WIN32_FIND_DATA wfd;
            HANDLE hDirs = FindFirstFile(wszFullPathUIDllName, &wfd);

            if (hDirs != INVALID_HANDLE_VALUE)
            {
                while (FindNextFile(hDirs, &wfd))
                {
                    // We are only interested in directories, since at this level, that should
                    // be the only thing in this directory, i.e, LCID sub dirs
                    //
                    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        // Skip current and previous dirs, "." and ".."
                        if (!lstrcmp(wfd.cFileName, L".") || !lstrcmp(wfd.cFileName, L".."))
                        {
                            continue;
                        }

                        // Does this dir have a copy of the dll?
                        if (SUCCEEDED(StringCchPrintfW(
                                            wszFullPathUIDllName,
                                            sizeof(wszFullPathUIDllName) / sizeof(WCHAR),
                                            L"%s%s\\%s",
                                            wszCurrentDir,
                                            wfd.cFileName,
                                            wszUIDllName)) &&
                            (0xFFFFFFFF != GetFileAttributes(wszFullPathUIDllName)))
                        {
                            // Got it!
                            FindClose(hDirs);
                            goto LoadDll;
                        }
                    }
                }

                FindClose(hDirs);
            }
        }
    }

#if HOSTED
    //Need to load the "VBHosted" dll as the resource dll if no resource dll was provided

    *phinst = g_hinstDll;
    goto DllLoaded;
#endif

    //
    // We did not find it.  Return an error.
    //
    {
        HRESULT hr = E_FAIL;
        CComBSTR bstrErrorMessage;
        ICreateErrorInfo *pcerrinfo = NULL;
        IErrorInfo *perrinfo = NULL;
        WCHAR wszLoadString[256];

        if(!LoadString(NULL,
                IDS_ERR_INTLLIBNOTFOUND,
                wszLoadString,
                sizeof(wszLoadString) / sizeof(WCHAR)))
        {
            return HrOfLastError();
        }
        bstrErrorMessage = wszLoadString;
        bstrErrorMessage += wszUIDllName;

        // Get a standard errinfo object.
        hr = CreateErrorInfo(&pcerrinfo);

        // Fill in its members.
        if (SUCCEEDED(hr))
        {
            hr = pcerrinfo->SetDescription(bstrErrorMessage);

            // Set it.
            if (SUCCEEDED(hr))
            {
                hr = pcerrinfo->QueryInterface(IID_IErrorInfo, (LPVOID FAR*) &perrinfo);

                if (SUCCEEDED(hr))
                {
#pragma prefast(suppress: 26010, "No buffers here")
                    SetErrorInfo(0, perrinfo);
                }
            }
        }

        if (perrinfo)
        {
            perrinfo->Release();
        }

        if (pcerrinfo)
        {
            pcerrinfo->Release();
        }

        return hr;
    }

LoadDll:

    // Finally, attempt to load the library
    if (!*phinst)
    {
        // LoadLibraryEx is safe here--it is a fully-qualified path and is
        // loaded as a data file and thus cannot be spoofed.
        *phinst = LoadLibraryEx(wszFullPathUIDllName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    }
    g_watsonLcid = lcidUsed;

#if HOSTED
DllLoaded:
#endif

    if (!*phinst)
    {
        return HrOfLastError();
    }
    else
    {
        _Module.m_hInstResource = *phinst;
        return NOERROR;
    }
}

int LoadUIString(
    UINT uID,
    _Out_bytecap_(nBufferMax)LPWSTR lpBuffer,
    int nBufferMax)
{
    if (!g_bAttemptedLoad)
    {
        g_bAttemptedLoad = TRUE;

        HINSTANCE hinstResources = NULL;

        // Try loading it once
        if(FAILED(DelayLoadUICallback(&hinstResources)))
            _Module.m_hInstResource = NULL;
    }

    HINSTANCE hinst = FLOCALIZEDRESID(uID) ?
        _Module.GetResourceInstance() :
        _Module.GetModuleInstance();

    // ::LoadString takes number of TCHARS, not number of bytes
    nBufferMax /= sizeof(WCHAR);
    return LoadString(hinst, uID, lpBuffer, nBufferMax);
}

UINT GetOEMCodePageFromLCID(LCID id)
{
    // GetLocaleInfo won't overflow buffer, and 40 WCHAR is considered large enough
    // to hold the integer code page value.
    WCHAR szBuffer[BUFLEN];
    GetLocaleInfo(id, LOCALE_IDEFAULTCODEPAGE, szBuffer, BUFLEN);
    return _wtoi(szBuffer);
}

UINT GetANSICodePageFromLCID(LCID id)
{
    // GetLocaleInfo won't overflow buffer, and 40 WCHAR is considered large enough
    // to hold the integer code page value.
    WCHAR szBuffer[BUFLEN];
    GetLocaleInfo(id, LOCALE_IDEFAULTANSICODEPAGE, szBuffer, BUFLEN);
    return _wtoi(szBuffer);
}
