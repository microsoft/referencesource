// UniApi.h - Unicode API wrappers
//-----------------------------------------------------------------
// Microsoft Confidential
// Copyright 1998 Microsoft Corporation.  All Rights Reserved.
//
// November 8, 1998 [paulde]
//
//
#ifdef _MSC_VER
#pragma once
#endif

#ifndef _UNIAPI_H_
#define _UNIAPI_H_

//$
#ifndef UNIAPI_NOATL
#include "atlbase.h"
#endif

//---------------------------------------------------------------
// Initialization
BOOL    WINAPI W_IsUnicodeSystem();

//---------------------------------------------------------------
// API Wrappers
//---------------------------------------------------------------
#if defined(_M_IA64) || defined(_M_AMD64) || defined (FEATURE_PAL)

#define W_FindFirstFile FindFirstFileW
#define W_FindNextFile          FindNextFileW
#define W_GetFileAttributes     GetFileAttributesW
#define W_SetFileAttributes     SetFileAttributesW
#define W_GetFullPathName       GetFullPathNameW
#define W_CreateFile            CreateFileW
#define W_GetCurrentDirectory   GetCurrentDirectoryW
#define W_GetTempPath           GetTempPathW
#define W_GetTempFileName       GetTempFileNameW
#define W_DeleteFile            DeleteFileW
#define W_GetWindowTextLength   GetWindowTextLengthW
#define W_GetWindowText         GetWindowTextW
#define W_RegisterClass         RegisterClassW
#define W_CreateWindowEx        CreateWindowExW
#define W_CreateWindow(C,W,S,x,y,w,h,p,m,i,L) CreateWindowExW(0L,C,W,S,x,y,w,h,p,m,i,L)
#define W_CreateDialogParam     CreateDialogParamW
#define W_CreateDialog(i,t,p,f) CreateDialogParamW(i,t,p,f,NULL)
#define W_CreateDialogIndirect(i,t,p,f) CreateDialogIndirectParamW(i,t,p,f,NULL)
#define W_CopyFile(s,d,f)       CopyFileW(s,d,f)

#else

HANDLE  WINAPI W_FindFirstFile      (__in_z PCWSTR pFileName, WIN32_FIND_DATAW * pfd);
BOOL    WINAPI W_FindNextFile       (HANDLE h, WIN32_FIND_DATAW * pfd);
DWORD   WINAPI W_GetFileAttributes  (__in_z PCWSTR pFileName);
BOOL    WINAPI W_SetFileAttributes  (__in_z PCWSTR pFileName, DWORD dwAttr);
DWORD   WINAPI W_GetFullPathName    (__in_z PCWSTR pFileName, DWORD cch, __out_ecount(cch) PWSTR pBuffer, __deref_opt_out PWSTR * ppFilePart);
HANDLE  WINAPI W_CreateFile         (__in_z PCWSTR pFileName, DWORD dwAccess, DWORD dwShare, SECURITY_ATTRIBUTES * psa,
                                     DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplate );
DWORD   WINAPI W_GetCurrentDirectory(DWORD cch, __out_ecount(cch) PWSTR pBuffer);
DWORD   WINAPI W_GetTempPath        (DWORD cch, __out_ecount(cch) PWSTR pBuffer);
UINT    WINAPI W_GetTempFileName    (__in_z PCWSTR pPathName, __in_z PCWSTR pPrefix, UINT uUnique, __out_ecount(MAX_PATH) PWSTR pTempFileName);
BOOL    WINAPI W_DeleteFile         (__in_z PCWSTR pPathName);
BOOL    WINAPI W_CopyFile           (__in_z PCWSTR pExistingPathName, __in_z PCWSTR pNewPathName, BOOL bFailIfExists);

#ifndef FEATURE_CORESYSTEM
int     WINAPI W_GetWindowTextLength (HWND hwnd); // per docs, always large enough but not always exact
int     WINAPI W_GetWindowText       (HWND hwnd, __out_ecount(cch) PWSTR psz, int cch);

ATOM    WINAPI W_RegisterClass  (CONST WNDCLASSW * pWndClass);

// To see if window is really Unicode after creation, use the IsWindowUnicode() Win32 API
HWND    WINAPI W_CreateWindowEx (DWORD dwExStyle, PCWSTR pClassName, PCWSTR pWindowName,
                                 DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
                                 HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, PVOID lpParam);
#define W_CreateWindow(C,W,S,x,y,w,h,p,m,i,L) W_CreateWindowEx(0L,C,W,S,x,y,w,h,p,m,i,L)

HWND    WINAPI W_CreateDialogParam (HINSTANCE hInstance, PCWSTR pTemplateName, HWND hWndParent, 
                                    DLGPROC pDialogFunc, LPARAM dwInitParam);
#define W_CreateDialog(i,t,p,f) W_CreateDialogParam(i,t,p,f,NULL)

HWND    WINAPI W_CreateDialogIndirectParam (HINSTANCE hInstance, const DLGTEMPLATE * pTemplate,
                                            HWND hWndParent, DLGPROC pDialogFunc, LPARAM dwInitParam);
#define W_CreateDialogIndirect(i,t,p,f) W_CreateDialogIndirectParam(i,t,p,f,NULL)
#endif

#endif

//---------------------------------------------------------------
// Additional utility functions
//---------------------------------------------------------------

// These resource string load funcs never convert the string, even on Win9x
// -- you get resource data verbatim.
int     WINAPI W_LoadString   (HINSTANCE hinst, UINT id, __out_ecount(cch) PWSTR psz, int cch);
BSTR    WINAPI W_LoadBSTR     (HINSTANCE hInst, UINT id); // Free with SysFreeString
PWSTR   WINAPI W_LoadVSString (HINSTANCE hInst, UINT id); // Free with VSFree

// Specify size of available buffer as template parameter
template<size_t cch> int WINAPI W_LoadString (HINSTANCE hinst, UINT id, _Out_cap_(cch) WCHAR (&psz)[cch])
{
    return W_LoadString (hinst, id, psz, cch);
}


#if !defined(NUMBER_OF)
#define NUMBER_OF(x) (sizeof(x) / sizeof((x)[0]))
#endif // !defined(NUMBER_OF)

// Returns length-prefixed non-null-terminated resource image for string
PCWSTR  WINAPI W_GetStringResourceImage (HINSTANCE hinst, UINT id);

// file/directory functions
int WINAPI W_Access(PCWSTR pPathName, int mode);                // _wchdir/_chdir
int WINAPI W_Rename(PCWSTR pOldPathName, PCWSTR pNewPathName);  // _wrename/_rename
int WINAPI W_Remove(PCWSTR pPathName);                          // _wremove/_remove
int WINAPI W_ChDir(PCWSTR pPathName);                           // _wchdir/_chdir
int WINAPI W_MkDir(PCWSTR pPathName);                           // _wmkdir/_mkdir
int WINAPI W_RmDir(PCWSTR pPathName);                           // _wrmdir/_rmdir

// Walk a file's path and get its actual letter case; also expand all short names
// to long names.  Returns non-zero on success, 0 on failure (output buffer too small
// to hold the result, error accessing the path, or out of memory).  In case of failure 
// the contents of the pszOut buffer are undefined.  cchOut specifies the size of the 
// pszOut buffer in characters.
BOOL    WINAPI W_GetActualFileCase(__in_z PCWSTR pszName, __out_ecount(cchOut) PWSTR pszOut, DWORD cchOut);

//---------------------------------------------------------
void inline W_ReplaceChar(__inout_opt __inout_z PWSTR psz, WCHAR chOld, WCHAR chNew)
{
    if (psz)
    {
        WCHAR ch;
        while (0 != (ch = *psz))
        {
            if (ch == chOld)
                *psz = chNew;
            psz++;
        }
    }
}

//--------------

#ifndef UNIAPI_NOATL
void inline W_AddTrailingChar(ATL::CComBSTR &bstr, WCHAR wch)
{
  int len = bstr.Length();
  if(len > 0)
  {
    if(bstr[len - 1] != wch)
    {
      WCHAR wchBuf[2] = {wch, 0};
      bstr += wchBuf;
    }
  }
}

void inline W_AddTrailingSlash(ATL::CComBSTR &bstrFolder)
{
  W_AddTrailingChar(bstrFolder, L'\\');
}

void inline W_RemoveTrailingSlash(ATL::CComBSTR &bstrFolder)
{
  int len = bstrFolder.Length();
  if(len > 0)
  {
    if(bstrFolder[len - 1] == L'\\')
    {
	  ATL::CComBSTR bstrTmp(bstrFolder);
      bstrTmp[len - 1] = 0;
      bstrFolder = bstrTmp;
    }
  }
}

//---------------------------------------------------------

int inline BSTRcmp(BSTR bstr1, BSTR bstr2)
{
  if ((bstr1 == NULL) && (bstr2 == NULL))
    return 0;
  else if ((bstr1 == NULL) && (bstr2 != NULL))
    return -1;
  else if ((bstr1 != NULL) && (bstr2 == NULL))
    return 1;
  else 
    return wcscmp(bstr1, bstr2);
}

int inline BSTRicmp(BSTR bstr1, BSTR bstr2)
{
  if ((bstr1 == NULL) && (bstr2 == NULL))
    return 0;
  else if ((bstr1 == NULL) && (bstr2 != NULL))
    return -1;
  else if ((bstr1 != NULL) && (bstr2 == NULL))
    return 1;
  else 
    return _wcsicmp(bstr1, bstr2);
}

//------------------------------------------

#ifndef FEATURE_CORESYSTEM

// GetWindowText
BOOL W_GetWindowText(HWND hwnd, ATL::CComBSTR &bstrText);
BOOL WINAPI W_GetWindowTextTrimSpaces(HWND hwnd, ATL::CComBSTR &bstrText);



BOOL inline W_GetDlgItemText(HWND hwnd, UINT uiID, ATL::CComBSTR &bstrText)
{
  HWND hwndItem = GetDlgItem(hwnd, uiID);
  //_ASSERTE(hwndItem); // commented because we can't rely on _ASSERTE being defined

  return W_GetWindowText(hwndItem, bstrText);
}

BOOL inline W_GetDlgItemTextTrimSpaces(HWND hwnd, UINT uiID, ATL::CComBSTR &bstrText)
{
  HWND hwndItem = GetDlgItem(hwnd, uiID);
  //_ASSERTE(hwndItem); // commented because we can't rely on _ASSERTE being defined

  return W_GetWindowTextTrimSpaces(hwndItem, bstrText);
}
#endif
#endif // UNIAPI_NOATL

//----------------------------------

#endif // _UNIAPI_H_
