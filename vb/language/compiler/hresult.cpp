//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Macros and functions for generating custom HRESULTs
//
//  We setup this ErrorInfo stuff so that any external caller will have
//  extended error info to report
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//---------------------------------------------------------------------------
// Macros and definitions
//---------------------------------------------------------------------------

// Debug macro to validate an ERRID
#define DebValidateErrid(errid) VSASSERT(errid > 0 && errid <= 0xFFFF, "")

// Copied From C#.
// VWWhidbey 572451: For FACILITY_URT, LegacyActivationShim::LoadStringRCEx needs the error code 
// match the index of the string in mscorrc.dll
#define MSG_FOR_URT_HR(hr) (0x6000 + hr)


//---------------------------------------------------------------------------
// Global variables
//---------------------------------------------------------------------------

// The following is used to signal low memory (initialized in GetHrInfoOOM)
static WCHAR g_wszOutOfMemory[64];

// Static structure containing an out of memory HRINFO to deal with
// error reporting in low memory situations.
static HRINFO g_hrinfoOOM =
{
    E_OUTOFMEMORY,        // m_hr
    NULL,
    g_wszOutOfMemory,        // m_bstrDescription
    NULL,                // m_wszHelpFile
    0,                // m_dwHelpContext
};

//
// Standard errors.
//

//---------------------------------------------------------------------------
// Given an ERRID, record rich error information (IErrorInfo), set it as the
// current error info, and return a fabricated HRESULT.
//---------------------------------------------------------------------------
HRESULT HrMake(ERRID errid)
{
    return HrMakeWithError(errid, NOERROR);
}

//---------------------------------------------------------------------------
// Cover function for HrMakeReplArgs.
//---------------------------------------------------------------------------
HRESULT __cdecl HrMakeRepl(ERRID errid, ...)
{
    va_list ap;

    // Get stack pointer and delegate to HrMakeReplArgs
    va_start(ap, errid);
    return HrMakeReplArgs(errid, ap);
}

//---------------------------------------------------------------------------
// Given a resource id of a string, and optional replacement strings, record
// rich error information (IErrorInfo), set it as the current error info,
// and return an HRESULT.
//---------------------------------------------------------------------------
HRESULT HrMakeReplArgs(ERRID errid, va_list ap)
{
    return HrMakeReplArgsWithError(errid, NOERROR, ap);
}

//
// Errors with extra info
//

//---------------------------------------------------------------------------
// Given an ERRID and an HRESULT, record rich error information (IErrorInfo),
// set it as the current error info, and return a fabricated HRESULT.
//---------------------------------------------------------------------------
HRESULT HrMakeWithError(ERRID errid, HRESULT hr)
{
    return HrMakeReplWithError(errid, hr);
}


//---------------------------------------------------------------------------
// Cover function for HrMakeReplArgsWithError.
//---------------------------------------------------------------------------
HRESULT __cdecl HrMakeReplWithError(ERRID errid, HRESULT hr, ...)
{
    va_list ap;

    // Get stack pointer and delegate to HrMakeReplArgs
    va_start(ap, hr);
    return HrMakeReplArgsWithError(errid, hr, ap);
}


#define DEFAULTBUFFERSIZE 520
// A version of HrMakeWithError that takes a va_list
//---------------------------------------------------------------------------
// Given a resource id of a string, an error and optional replacement strings, record
// rich error information (IErrorInfo), set it as the current error info,
// and return an HRESULT.
//---------------------------------------------------------------------------
HRESULT HrMakeReplArgsWithError(ERRID errid, HRESULT hrErr, va_list ap)
{
    HRESULT hr;
    WCHAR wszBuffer[DEFAULTBUFFERSIZE];
    StringBuffer sbuf(wszBuffer, DEFAULTBUFFERSIZE);
    HRINFO hrinfo;

    DebValidateErrid(errid);

    // Get the string for this error.
    if (hrErr != NOERROR)
    {
        GetHrInfo(hrErr, &hrinfo);
    }
    else
    {
        memset(&hrinfo, 0, sizeof(hrinfo));

        // Get our error source
        hrinfo.m_hr = HROFERRID(errid);

        IfNullGo(hrinfo.m_bstrSource = SysAllocString(WIDE("Visual Basic")));
        hrinfo.m_bstrDescription = NULL;
    }

    // Create the new error message.
    IfFailGo(ResLoadStringReplArgs(
        errid,
        &sbuf,
        hrinfo.m_bstrDescription,
        ap));

    if (hrinfo.m_bstrDescription != g_wszOutOfMemory)
    {
        // Before we put a new string into it, release the old hrinfo.m_bstrDescription.
        SysFreeString(hrinfo.m_bstrDescription);
    }
    IfNullGo(hrinfo.m_bstrDescription = SysAllocString(sbuf.GetString()));

    hr = PropagateHrInfo(hrinfo);

Error:

    ReleaseHrInfo(&hrinfo);

    return hr;
}


//
// Errors with GetLastError()
//

// Given an ERRID and the last error, record rich error information.
//---------------------------------------------------------------------------
// Given an ERRID, record rich error information (IErrorInfo), set it as the
// current error info, and return a fabricated HRESULT.
//---------------------------------------------------------------------------
HRESULT HrMakeWithLast(ERRID errid)
{
    return HrMakeWithError(errid, GetLastHResultError());
}


// Given an ERRID, an HRESULT and optional replacement string, record rich
// error information.
//---------------------------------------------------------------------------
// Cover function for HrMakeReplArgs.
//---------------------------------------------------------------------------
HRESULT __cdecl HrMakeReplWithLast(ERRID errid, ...)
{
    va_list ap;

    // Get stack pointer and delegate to HrMakeReplArgs
    va_start(ap, errid);
    return HrMakeReplArgsWithLast(errid, ap);
}

// A version of HrMakeWithError that takes a va_list
//---------------------------------------------------------------------------
// Given a resource id of a string, and optional replacement strings, record
// rich error information (IErrorInfo), set it as the current error info,
// and return an HRESULT.
//---------------------------------------------------------------------------
HRESULT HrMakeReplArgsWithLast(ERRID errid, va_list ap)
{
    return HrMakeReplArgsWithError(errid, GetLastHResultError(), ap);
}

//
// Other stuff
//

//---------------------------------------------------------------------------
// Return an HRINFO structure associated with the out of memory error.  This
// can never fail, even in low memory.
//---------------------------------------------------------------------------
static HRINFO* GetHrInfoOOM()
{
    // If the description string has not been loaded
    if (g_wszOutOfMemory[0] == WIDE('\0'))
    {
        HRESULT hr = S_OK;

        // Try to load the localized string.
        hr = ResLoadString(STRID_OutOfMemory, g_wszOutOfMemory, DIM(g_wszOutOfMemory));

        if (FAILED(hr))
        {
            // Something has gone horribly wrong.  Make sure we have a fully-formed
            // error string
            wcscpy_s(g_wszOutOfMemory, _countof(g_wszOutOfMemory), WIDE("Out of memory"));
        }
    }

    // Return pointer to static structure
    return &g_hrinfoOOM;
}


//---------------------------------------------------------------------------
// Fill an HRINFO structure with information about an HRESULT that we did
// not generate.  Use FormatMessage to get this information.
//---------------------------------------------------------------------------
static void GetSysHrInfo(HRESULT hrInfo, _Out_ HRINFO* phrinfo)
{
    HRESULT hr = NOERROR;
    WCHAR wszDescription[1024];
    StringBuffer sbuf(wszDescription, 1024);
    WCHAR wszNum[16];
    BSTR bstrDescription = NULL;
    BSTR bstrHelpFile = NULL;

    // Get the description from the system
    wszDescription[0] = WIDE('\0');
    FormatMessage(FORMAT_MESSAGE_MAX_WIDTH_MASK | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
        (ULONG)hrInfo, GetSystemDefaultLangID(), wszDescription, sizeof(wszDescription) / sizeof(WCHAR), NULL);

    // If the system does not have a string for this error
    if (*wszDescription == WIDE('\0'))
    {
        // Format the error number
        _ultow_s((ULONG)hrInfo, wszNum, _countof(wszNum), 16);

        // Format the number.
        IfFailGo(ResLoadStringRepl(STRID_SystemErrorFormat1, &sbuf, wszNum));

        // For the special but common case of STATUS_ACCESS_VIOLATION, add a little
        // text telling the user the compiler is horked.
        // 


        if (hrInfo == (HRESULT)STATUS_ACCESS_VIOLATION)
        {
            WCHAR wszHorked[512];
            if (SUCCEEDED(ResLoadString(STRID_CompilerIsHorked, wszHorked, DIM(wszHorked))))
            {
                sbuf.AppendString(L" (");
                sbuf.AppendString(wszHorked);
                sbuf.AppendChar(L')');
            }
        }

    }
    else
    {
        sbuf.AppendString(wszDescription);
    }

    // Alloc description string
    IfNullGo(bstrDescription = SysAllocString(sbuf.GetString()));

    // Take ownership of strings
    phrinfo->m_hr = hrInfo;
    phrinfo->m_bstrSource = NULL;
    phrinfo->m_bstrDescription = bstrDescription;
    phrinfo->m_bstrHelpFile = bstrHelpFile;
    phrinfo->m_dwHelpContext = 0;
    bstrDescription = NULL;
    bstrHelpFile = NULL;

Error:
    SysFreeString(bstrDescription);
    SysFreeString(bstrHelpFile);

    if (FAILED(hr))
        CopyMemory(phrinfo, GetHrInfoOOM(), sizeof(*phrinfo));
}


//---------------------------------------------------------------------------
// Fill an HRINFO structure with information about a particular HRESULT.
//---------------------------------------------------------------------------
void GetHrInfo(HRESULT hrInfo, _Out_ HRINFO* phrinfo)
{
    HRESULT hr(S_OK);
    IErrorInfo* pei = NULL;
    BSTR bstrSource = NULL;
    BSTR bstrDescription = NULL;
    BSTR bstrHelpFile = NULL;

    ZeroMemory(phrinfo, sizeof(*phrinfo));

    #define FIXED_SIZE_CHAR_COUNT 512
    int msgLength = 0;


    // Obtain ownership of the current IErrorInfo.
    if (SUCCEEDED(GetErrorInfo(0, &pei)) &&
        pei != NULL &&
        SUCCEEDED(pei->GetDescription(&bstrDescription)) &&
        bstrDescription != NULL)
    {
        // Get help file path
        IfFailGo(pei->GetHelpFile(&bstrHelpFile));

        // Get help context
        DWORD dwHelpContext;
        IfFailGo(pei->GetHelpContext(&dwHelpContext));

        // Get source of error
        IfFailGo(pei->GetSource(&bstrSource));

        // Take ownership of the strings
        phrinfo->m_hr = hrInfo;
        phrinfo->m_bstrSource = bstrSource;
        phrinfo->m_bstrDescription = bstrDescription;
        phrinfo->m_bstrHelpFile = bstrHelpFile;
        phrinfo->m_dwHelpContext = dwHelpContext;
        bstrSource = NULL;
        bstrDescription = NULL;
        bstrHelpFile = NULL;
    }
#ifndef FEATURE_CORESYSTEM
    else if(HRESULT_FACILITY(hrInfo) == FACILITY_URT)
    {
        WCHAR strBuf[FIXED_SIZE_CHAR_COUNT];
        if(SUCCEEDED(LegacyActivationShim::LoadStringRCEx(-1, MSG_FOR_URT_HR(hrInfo), strBuf, FIXED_SIZE_CHAR_COUNT, true, &msgLength))) 
        {
            phrinfo->m_bstrDescription = SysAllocString(strBuf);
            phrinfo->m_hr = hrInfo;
        }
    }        
#endif
    else
    {
        // Try to get information about the error from the system
        GetSysHrInfo(hrInfo, phrinfo);
    }

Error:
    RELEASE(pei);
    SysFreeString(bstrSource);
    SysFreeString(bstrDescription);
    SysFreeString(bstrHelpFile);
    if (FAILED(hr))
        GetSysHrInfo(hrInfo, phrinfo);
    VSASSERT(phrinfo->m_hr!=S_OK, "");
    VSASSERT(phrinfo->m_bstrDescription, "");
}

//---------------------------------------------------------------------------
// Free the contents of an HRINFO structure.  Call this on HRINFO structure
// returned from GetHrInfo.
//---------------------------------------------------------------------------
void ReleaseHrInfo(_In_ HRINFO* phrinfo)
{
    // In this case it wasn't a bstr
    if (phrinfo->m_bstrDescription != g_wszOutOfMemory)
        SysFreeString(phrinfo->m_bstrDescription);

    SysFreeString(phrinfo->m_bstrSource);
    SysFreeString(phrinfo->m_bstrHelpFile);

#if DEBUG
    FillMemory(phrinfo, sizeof(*phrinfo), 0xFF);
#endif DEBUG
}

void CopyHrInfo(const HRINFO& source, _Out_ HRINFO* pDest)
{
    if ( source.m_bstrDescription != g_wszOutOfMemory )
    {
        pDest->m_bstrDescription = ::SysAllocString(source.m_bstrDescription);
    }
    else
    {
        pDest->m_bstrDescription = g_wszOutOfMemory;
    }

    pDest->m_bstrHelpFile = ::SysAllocString(source.m_bstrHelpFile);
    pDest->m_bstrSource = ::SysAllocString(source.m_bstrSource);
    pDest->m_hr = source.m_hr;
    pDest->m_dwHelpContext = source.m_dwHelpContext;
}

HRESULT PropagateHrInfo(HRESULT hr, const WCHAR* description)
{
    HRINFO info;

    ZeroMemory(&info, sizeof(HRINFO));
    info.m_hr = hr;
    info.m_bstrDescription = ::SysAllocString(description);
    info.m_bstrSource = SysAllocString(WIDE("Visual Basic"));
    hr = PropagateHrInfo(info);

    ::ReleaseHrInfo(&info);
    return hr;
}

//---------------------------------------------------------------------------
// Takes an HRINFO and makes it the current thread's error information.
//---------------------------------------------------------------------------

HRESULT PropagateHrInfo(HRINFO &hrinfo)
{
    HRESULT hr = NOERROR;

    ICreateErrorInfo *pcerrinfo = NULL;
    IErrorInfo *perrinfo = NULL;

    // Get a standard errinfo object.
    IfFailGo(CreateErrorInfo(&pcerrinfo));

    // Fill in its members.
    IfFailGo(pcerrinfo->SetGUID(GUID_NULL));
    IfFailGo(pcerrinfo->SetSource(hrinfo.m_bstrSource));
    IfFailGo(pcerrinfo->SetDescription(hrinfo.m_bstrDescription));
    IfFailGo(pcerrinfo->SetHelpFile(hrinfo.m_bstrHelpFile));
    IfFailGo(pcerrinfo->SetHelpContext(hrinfo.m_dwHelpContext));

    // Set it.
    IfFailGo(pcerrinfo->QueryInterface(IID_IErrorInfo, (LPVOID FAR*) &perrinfo));

#pragma prefast(suppress: 26010, "perrinfo is not a buffer")
    SetErrorInfo(0, perrinfo);

    hr = hrinfo.m_hr;

Error:
    RELEASE(perrinfo);
    RELEASE(pcerrinfo);

    return hr;
}


//---------------------------------------------------------------------------
// Show an error in a messagebox.  Only do this if we have no choice
// or if an error occurs when the IDE won't show it.
//---------------------------------------------------------------------------

bool HrShowError(HRESULT hr)
{
#ifndef FEATURE_CORESYSTEM
    HRESULT hrString;
    WCHAR wszTitle[64];
    HRINFO hrinfo;

    hrString = ResLoadString(STRID_MicrosoftVisualBasic, wszTitle, DIM(wszTitle));

    if (FAILED(hrString))
    {
        wcscpy_s(wszTitle, _countof(wszTitle), L"Microsoft Visual Basic");
    }

    GetHrInfo(hr, &hrinfo);

    if (MessageBox(NULL, hrinfo.m_bstrDescription, wszTitle, MB_OK | MB_ICONHAND | MB_SYSTEMMODAL))
    {
        ReleaseHrInfo(&hrinfo);
        return true;
    }

    ReleaseHrInfo(&hrinfo);
#endif
    return false;
}
