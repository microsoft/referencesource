//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Fault reporting helpers
//
//      HRESULT errors returned from the DocObject's interface methods are most often ignored by the browser.
//      If this happens during activation in particular, the user is usually left with a blank browser 
//      window or a generic HTML error page. (On occasion infinite relaunching between PresentationHost and 
//      IExplore would occur because IE falls back on shell activation, but PH just launches the browser when
//      shell-activated...) 
//
//      To make such problems and their causes easier to track down and potentially address eventually, 
//      a Watson report is sent (and the process terminated) when an error is detected that would otherwise 
//      lead the browser to abandoning activation. Many of the possible errors are likely to be caused by 
//      external factors and so are not really bugs in our code. Triggering Watson and "crashing" the process
//      may then seem like an overkill. However, leaving various yet-unknown activation & deployment problems 
//      unreported and having no effective means to offer end users help (as with a Watson response page) is 
//      much worse.
//
//      Regression-paranoia-induced crippling in SP1: To minimize the amount of code changed, the HRESULT 
//          checking and Watson triggering is initially done only at a handful of Cleanup labels, mostly in 
//          high-level functions (most of which are entry points from the browser). This will leave us without
//          exact call stacks, and the crash dumps won't capture the failure at the earliest point it's 
//          detected, but we'll at least get failure hit counts for the major entry points. And the failure
//          address logging done by the 











#pragma once

// A SEH exception is raised in case the custom Watson reporting fails (shim\WatsonReporting.cxx).
// This is the "application-defined" exception code.
const int ACTIVATION_PROBLEM_FALLBACK_EXCEPTION_CODE = 42;

const int ACTIVATION_PROBLEM_HRESULT_EXCEPTION_CODE = 43;

const HRESULT E_BAD_CONFIGURATION = HRESULT_FROM_WIN32(ERROR_BAD_CONFIGURATION);

__declspec(noreturn)
void ReportActivationFaultAndTerminate(HRESULT hr);
__declspec(noreturn)
void ReportFaultAndTerminate(HRESULT hr, int exceptionCode);

inline void HANDLE_ACTIVATION_FAULT(HRESULT hr)    
{                                          
    if(FAILED(hr))                     
    {                                       
       ReportActivationFaultAndTerminate(hr);
    }                                       
}

// Saves the return address, which is the value of the instruction pointer after the call, in a circular static
// buffer. Useful for reconstructing failure code paths from crash dumps. See the 
__declspec(noinline)
void __cdecl LogIPAddress();
//[Compilation error here? Define LOG_FAILURE_ADDRESSES.]
