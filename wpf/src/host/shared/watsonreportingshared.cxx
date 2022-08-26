//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Fault reporting helpers. See .hxx.
//
//  History:
//      2007/07/26   Microsoft     Created
//      2007/09/20   Microsoft     Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "SharedUtilities.hxx"
#include "WatsonReportingShared.hxx"
#include <WinNT.h>
#include "ErrorRep.h"

// winnt.h & intrin.h define these functions differently and compilation fails.
#define _interlockedbittestandset _interlockedbittestandsetX
#define _interlockedbittestandreset _interlockedbittestandresetX
#define _interlockedbittestandset64 _interlockedbittestandset64X
#define _interlockedbittestandreset64 _interlockedbittestandreset64X
#include <intrin.h>


bool ReportFault(EXCEPTION_POINTERS *pExcpInfo)
{
    // To save the DLL dependency, LoadLibrary() is used.
    EFaultRepRetVal (WINAPI *pfReportFault)(LPEXCEPTION_POINTERS pep, DWORD dwMode);
    HMODULE hLib = LoadLibrary(L"Faultrep.dll");
    if(hLib && GetProcAddress(hLib, "ReportFault", &pfReportFault))
    {
        EFaultRepRetVal res = (*pfReportFault)(pExcpInfo, 0);
        return res == frrvErr /*"The function failed but the client was launched"*/ ||
            res == frrvErrNoDW || res == frrvOk || 
            res == frrvOkHeadless || res == frrvOkManifest || res == frrvOkQueued;
    }
    return false;
}

__declspec(noreturn)
void ReportFaultAndTerminate(HRESULT hr, int exceptionCode)
{
    ASSERT(FAILED(hr));
    // If we are handling a remote call from the browser, the RPC layer is going to swallow the exception.
    // To prevent this, call ReportFault() directly. 
    __try
    {
        ULONG_PTR ulhr = hr;
        RaiseException(exceptionCode, 0, 1/*# of args*/, &ulhr);
    }
    __except(ReportFault(GetExceptionInformation()) ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        ExitProcess(hr);
    }
}

__declspec(noreturn)
void ReportActivationFaultAndTerminate(HRESULT hr)
{
    ReportFaultAndTerminate(hr, ACTIVATION_PROBLEM_HRESULT_EXCEPTION_CODE);
}

//
// Failure IP address logging. Useful for deconstructing failure code paths from crash dumps.
// The 




void* g_IPLog[32];
int g_IPLogPtr;

#pragma intrinsic(_ReturnAddress)

__declspec(noinline)
void _cdecl LogIPAddress()
{
    g_IPLogPtr = (g_IPLogPtr + 1) % ARRAYSIZE(g_IPLog);
    g_IPLog[g_IPLogPtr] = _ReturnAddress();
}
