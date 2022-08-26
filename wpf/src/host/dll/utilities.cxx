//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the utility functions of PresentationHost.
//
//  History:
//      2002/06/19-murrayw
//          Created
//      2003/06/30-Microsoft
//          Ported ByteRangeDownloader to WCP
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "Utilities.hxx"
#include "HostServices.h"

HRESULT TaskAllocString(__in LPCWSTR pstrSrc, __deref_out_ecount(1) LPWSTR *ppstrDest)
{
    HRESULT hr = S_OK;
    CK_PARG(ppstrDest);

    LPWSTR pstr;
    size_t cb;
    size_t cCh;

    CKHR(StringCchLengthW(pstrSrc, STRSAFE_MAX_CCH, &cCh));    
    CKHR(SizeTAdd(cCh, 1, &cb) );
    CKHR(SizeTMult(cb, sizeof(WCHAR), &cb));
    
    pstr = (LPWSTR) CoTaskMemAlloc(cb);
    CK_ALLOC(pstr);
    *ppstrDest = pstr;

    memcpy(pstr, pstrSrc, cb);

Cleanup:
    return hr;
}

// Loads the satellite .dll.mui resource library. Free it with FreeMUILibrary().
HINSTANCE LoadResourceDLL()
{
    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE))
    {
        EventWriteWpfHostUm_LoadingResourceDLLStart();
    }

    // We need to get the actual path of the DLL regardless of where the process
    // that loaded it was started from.
    WCHAR wzFileName[MAX_PATH];
    HMODULE hThisDll = GetModuleHandle(TEXT(TARGETNAME));
    if (!hThisDll || !GetModuleFileName(hThisDll, wzFileName, MAX_PATH))
    {
        ASSERT(false); return 0;
    }
    // Load the MUI resources explicitly.  On Longhorn and beyond hResourceInstance will be equal to g_hInstance
    HINSTANCE hResDLL = LoadMUILibrary(wzFileName, MUI_LANGUAGE_NAME, 0);

    if(hResDLL && ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE))
    {
        wchar_t filepath[MAX_PATH];
        GetModuleFileName(hResDLL, filepath, ARRAYSIZE(filepath));
        EventWriteWpfHostUm_LoadingResourceDLLEnd(filepath);
    }

    return hResDLL;
}
    
size_t LoadResourceString(int id, __out_ecount(cchBufSize) wchar_t *pBuf, size_t cchBufSize)
{
    HINSTANCE hResLib = LoadResourceDLL();
    if(!hResLib)
        { ASSERT(0); return false; }
    size_t cch = LoadString(hResLib, id, pBuf, (int)cchBufSize);
    FreeMUILibrary(hResLib);
    return cch;
}

BSTR LoadResourceString(int id)
{
    const size_t MaxStringLen = 4097; // according to MSDN Library
    wchar_t buf[MaxStringLen+1];
    if(!LoadResourceString(id, buf, MaxStringLen+1))
        return 0;
    return SysAllocString(buf);
}

bool FileExists(const wchar_t *filePath)
{
    ASSERT(filePath);
    WIN32_FIND_DATA findData;
    HANDLE h = FindFirstFile(filePath, &findData);
    if(h == INVALID_HANDLE_VALUE)
        return false;
    FindClose(h);
    return true;
}

bool FileExists(const wchar_t *dir, const wchar_t *shortFilename)
{
    ASSERT(dir != 0 && shortFilename != 0);
    wchar_t path[MAX_PATH];
#pragma prefast(suppress:25025, "We don't know of a better API to use in place of PathCombine. The OACR spreadsheet and MSDN do not suggest any either.")
    if (!PathCombine(path, dir, shortFilename))
        return false;
    return FileExists(path);
}

BSTR GetFileVersion(const wchar_t *filePath)
{
    BSTR result = 0;
    DWORD dummy;
    DWORD verInfoSize = GetFileVersionInfoSize(filePath, &dummy);
    if(verInfoSize)
    {
        void *pVerInfo = new char[verInfoSize];
        if(pVerInfo && GetFileVersionInfo(filePath, 0, verInfoSize, pVerInfo))
        {
            // Prepare the FileVersion sub-block path for VerQueryValue()
            void *pLangId;
            UINT langId, langIdSize, verStrSize;
            wchar_t fileVerQueryPath[80];
            if(VerQueryValue(pVerInfo, L"\\VarFileInfo\\Translation", &pLangId, &langIdSize)
                && langIdSize == 4)
            {
                langId = *(UINT*)pLangId;
                StringCbPrintf(fileVerQueryPath, sizeof fileVerQueryPath,
                    L"\\StringFileInfo\\%02X%02X%02X%02X\\FileVersion",
                    (langId & 0xff00)>>8, langId & 0xff,
                    (langId & 0xff000000)>>24, (langId & 0xff0000)>>16);            
            }
            else 
            {
                StringCbPrintf(fileVerQueryPath, sizeof fileVerQueryPath,
                    L"\\StringFileInfo\\%04X04B0\\FileVersion", GetUserDefaultLangID());
            }

            wchar_t *pwszVerInfo;
            if(VerQueryValue(pVerInfo, fileVerQueryPath, (void**)&pwszVerInfo, &verStrSize))
            {
                result = SysAllocString(pwszVerInfo);
            }
        }
        delete [] pVerInfo;
    }
    return result;
}

// Returns whether the given message is for keyboard tab navigation.
// -1 = tab backwards, 0 = none, 1 = tab forward
// (Note: F6 is for higher-level tabbing, switching b/w different panes.)
int IsTabCycler(const MSG &msg)
{
    int nDir = 0;
    if (msg.message == WM_KEYDOWN)
    {
        if (msg.wParam == VK_TAB || msg.wParam == VK_F6)
        {
            if (GetKeyState(VK_CONTROL) >= 0)
            {
                nDir = (GetKeyState(VK_SHIFT) < 0) ? -1 : 1;
            }
        }
    }
    return nDir;
}



//*********************************************************************************
//                              COM Helpers
//*********************************************************************************
//+---------------------------------------------------------------
//
// Function:    IsSameObject
//
// Synopsis:    Checks for COM identity
//
// Arguments:   pUnkLeft, pUnkRight
//
//              pUnkLeft and pUnk1 can come from diff objects, likewise with pUnkRight and pUnk2, 
//              hence the multiple checks.
//
//              The table for comparision is
//                  pUnkLeft, pUnkRight
//                  pUnkLeft, pUnk2
//                  pUnk1, pUnkRight
//                  pUnk1, pUnk2
//
//+---------------------------------------------------------------

BOOL IsSameObject(IUnknown *pUnkLeft, IUnknown *pUnkRight)
{
    IUnknown *pUnk1 = NULL;
    IUnknown *pUnk2 = NULL;

    //The quick check
    if (pUnkLeft == pUnkRight)
        return TRUE;

    if (pUnkLeft == NULL || pUnkRight == NULL)
        return FALSE;

    //Release the QI-ed interface pointers but don't NULL the pointers
    if (SUCCEEDED(pUnkLeft->QueryInterface(IID_IUnknown, (LPVOID *)&pUnk1)))
    {
        pUnk1->Release();
    }
    if (SUCCEEDED(pUnkRight->QueryInterface(IID_IUnknown, (LPVOID *)&pUnk2)))
    {
        pUnk2->Release();
    }

    if ( (pUnk1 == pUnkRight) ||
         (pUnk1 == pUnk2) ||
         (pUnkLeft == pUnk2))
    {
            return TRUE;
    }

    return FALSE;
}


//*********************************************************************************
//                              Menu Helpers
//*********************************************************************************
WORD GetMenuResourceId(MimeType mimeType)
{
    switch (mimeType)
    {
    case MimeType_Application:
    case MimeType_Markup:
        return MENU_XAPP;    
    case MimeType_Xps:
    case MimeType_Unknown:
    default:
        ASSERT(0);
        return 0;
    }
}

HMENU GetMenuFromID(__in HMENU hMenu, UINT menuID)
{
    HMENU hmenuRet = NULL;
    
    if (!hMenu)
        return NULL;

    MENUITEMINFO mii;
    ZeroMemory(&mii, sizeof(mii));

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_SUBMENU;
    
    if (GetMenuItemInfo(hMenu, menuID, FALSE, &mii))
        hmenuRet = mii.hSubMenu;
    
    return hmenuRet;
}

void UpdateMenuItems(__in HMENU hMenu, UINT cCmds, __in_ecount(cCmds) OLECMD *prgCmds)
{
    if (prgCmds == NULL)
        return;
    
    UINT mfEnable = 0;
    for (UINT i = 0; i < cCmds; i++)
    {
        if ((prgCmds[i].cmdf & (OLECMDF_ENABLED | OLECMDF_SUPPORTED)) == (OLECMDF_ENABLED | OLECMDF_SUPPORTED))
        {
            mfEnable = MF_ENABLED;
        }
        else
        {
            mfEnable = MF_GRAYED;
        }
        EnableMenuItem(hMenu, prgCmds[i].cmdID, (MF_BYCOMMAND | mfEnable));
    }

}

HRESULT
IsUrlSecure(const WCHAR* pszUrl)
{
    // CoInternetParseUrl() crashes on a NULL pointer. (Fixed in Windows Vista.)
    if(!pszUrl || !*pszUrl)
    {
        ASSERT(0);
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    WCHAR szBuffer[INTERNET_MAX_SCHEME_LENGTH + 1];
    DWORD cchResult = INTERNET_MAX_SCHEME_LENGTH + 1;
    
    CKHR(CoInternetParseUrl(pszUrl, PARSE_SCHEMA, 0, szBuffer, cchResult, &cchResult, 0));
    
    if (_wcsicmp(szBuffer,URL_SCHEME_SECURE) != 0)
    {
        return S_FALSE;
    }
      
Cleanup:

    //E_FAIL indicates the parse was not successfully so don't assume secure or unsecure from caller side
    return hr;
}


//-----------------------------------------------------------------------------
// GetTopLevelBrowser 
// Performs the nessary QIing to get the top level IWebBrowser2 object
//-----------------------------------------------------------------------------
HRESULT GetTopLevelBrowser(__in_ecount(1) IOleClientSite* pOleClientSite, __deref_out_ecount(1) IWebBrowser2** pWebBrowserTop)
{
    HRESULT hr;
    IServiceProvider    * pServiceProvider  = NULL;
    CKHR(QueryService(pOleClientSite, SID_STopLevelBrowser, IID_IServiceProvider, (void**)&pServiceProvider));
    CKHR(QueryService(pServiceProvider, SID_SWebBrowserApp, IID_IWebBrowser2, (void**)pWebBrowserTop));

Cleanup:
    ReleaseInterface(pServiceProvider);
    return hr;
}

//******************************************************************************
//
// TerminateIfHostCrashesWorker()
//
// This is called on a separate thread to wait for the IE process to die in a 
// non-g----ful manner such as a crash or a kill from the TaskManager. When this
// happens, it will kill the current process. Now. In a non-gentle fashion.
//
//******************************************************************************

VOID CALLBACK TerminateIfHostCrashesCallback(PVOID pParam, BOOL bIsTimerCallback)
{
    TerminateProcess(GetCurrentProcess(), ERROR_HOST_CRASHED);
}


//******************************************************************************
//
// TerminateIfHostCrashes()
//
// This method will create a new thread for calling TerminateIfHostCrashesWorker.
// From the browser's hwnd, it will get the process ID, from the process ID, it will
// get a handle to the process. It passes the handle to TerminateIfHostCrashesWorker.
//   
//******************************************************************************

HRESULT TerminateIfHostCrashes(HWND hwndBrowser)
{
    HRESULT hr = S_OK;

    // Get the process ID from the window
    DWORD dwProcessId = 0;

    DWORD dwThreadId = GetWindowThreadProcessId(hwndBrowser, &dwProcessId);
    CKHR(dwThreadId && dwProcessId ? S_OK : E_FAIL);

    HANDLE hProc = OpenProcess(SYNCHRONIZE, FALSE, dwProcessId);
    if (!hProc)
    {
        DWORD dwError = GetLastError();
        if (dwError == ERROR_ACCESS_DENIED)
        {
            // This will occur when run on WS03 & IE6 by an admin user.
            // In this case, IE runs as admin, but our low-rights code strips the
            // admin token. The OpenProcess call is trying to synchronize a process
            // with the admin token from a user process, which fails in WS03. In 
            // XP, this is allowed. In Vista, IE7 does not run as admin.
            return S_OK;
        }
        else
        {
            CKHR(HRESULT_FROM_WIN32(dwError));
        }
    }

    HANDLE hTerminateWaitHandle = 0; // we don't care about this because we are going to Terminate.
    CHECK_BOOL_FROM_WIN32(RegisterWaitForSingleObject(
        &hTerminateWaitHandle, hProc, (WAITORTIMERCALLBACK) TerminateIfHostCrashesCallback, 
        NULL, INFINITE, WT_EXECUTELONGFUNCTION | WT_EXECUTEONLYONCE));
    
Cleanup:
    return hr;
}
