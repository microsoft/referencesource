//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the main entry point of the PresentationShim used for hosting
//     Windows Client Applications in the browser
//
//  History:
//     2002/06/12-murrayw
//          Created
//     2003/06/03-kusumav
//          Ported to WCP
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//     2009/08/xx-Microsoft,Microsoft
//          PresentationHost 'super-shim' refactoring. Defined ProcessCommandLine and renamed 
//              WinMain->DelegatedWinMain. Compiling the 'shim implementation' code into PHDLL.
//              See overview in DLL\PresentationHostDll.nativeproj.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "Main.hxx"
#include "LoRights.hxx"
#include "HostShim.hxx"
#include "ClassFactory.hxx"
#include "Version.hxx"
#include "WatsonReporting.hxx"
#include "ShimUtilities.hxx"
#include "..\inc\registry.hxx"

#define EVENT_SIZE 64
#define NO_HOST_TIMEOUT_SECONDS (10 * 60)

//******************************************************************************
//
//   global variables
//
//******************************************************************************

//CComModule _Module; - now using the one defined in DllMain.cxx
long        g_ServerRefCount;
int         g_exitCode = 0;
DWORD       g_dwCommandLineFlags;
LPWSTR      g_pswzDebugSecurityZoneURL;
HANDLE      g_hNoHostTimer_shim;


//******************************************************************************
//
// DelegatedWinMain - invoked by PresentationHost.exe (shim)
//
//******************************************************************************

int PASCAL DelegatedWinMain(__in HINSTANCE hInstance, 
                            __in_opt HINSTANCE hPrevInstance, 
                            __in_opt LPSTR lpCommandLine, 
                            __in int nCmdShow)
{
    HRESULT hr = S_OK;

    // The heap setting and DEPPolicy are not set here since these are setup by PH.EXE shim.
    // Thus, we have nothing to do here in that respect.

    EventRegisterMicrosoft_Windows_WPF();
    EventWriteWpfHostUm_WinMainStart();

    BOOL bCoInitialized = FALSE;

    WCHAR wzFileName[PATH_BUFFER_SIZE + 1];
    WCHAR wzEventName[EVENT_SIZE + 1];
    WCHAR wzDebugSecurityZoneURL[DEBUGSECURITYZONEURL_BUFFER_SIZE + 1];
    WCHAR wzDotApplicationURL[INTERNET_MAX_URL_LENGTH];
    
    PLUID_AND_ATTRIBUTES pDisabledPrivileges = NULL;
    DWORD dwDisabledPrivilegesCount = 0;

    g_dwCommandLineFlags = FLAGS_NONE;

    // Check for the command line flags
    CKHR(ParseCommandLine(&g_dwCommandLineFlags, wzFileName, PATH_BUFFER_SIZE, wzEventName, EVENT_SIZE, wzDebugSecurityZoneURL, DEBUGSECURITYZONEURL_BUFFER_SIZE, wzDotApplicationURL, INTERNET_MAX_URL_LENGTH));

    // Overloading PresentationHost.exe to assist in launching .application files 
    if (*wzDotApplicationURL && (g_dwCommandLineFlags & FLAG_LAUNCHDOTAPPLICATION))
    {
        CKHR(CoInitialize(NULL));
        bCoInitialized = TRUE;
        LaunchClickOnceApplication(wzDotApplicationURL);
        goto Cleanup;
    }

    // For the shell open scenario, invoke the browser and exit
    if (*wzFileName && (FLAGS_NONE == g_dwCommandLineFlags))
    {
        CKHR(InvokeBrowser(wzFileName));
        g_exitCode = ERROR_SUCCESS;
        goto Cleanup;
    }
    if(*wzDebugSecurityZoneURL && (g_dwCommandLineFlags & FLAG_DEBUGSECURITYZONEURL))
    {
        g_pswzDebugSecurityZoneURL = wzDebugSecurityZoneURL;
    }

    // The code below checks to see if the process is running with admin privileges.
    // If so, it will start a new instance of the process with the same arguments but
    // with reduced privileges. We don't do this in case a debugger is attached or if
    // we detect the process was created with a high integrity level (or above) by
    // running IE elevated as a way to achieve full-trust operation.
    BOOL isEmbedding = (g_dwCommandLineFlags & FLAG_EMBEDDING);
    BOOL isDebug = (g_dwCommandLineFlags & FLAG_DEBUG) || IsDebuggerPresent();

    if (isEmbedding && !isDebug && !IsPresentationHostHighIntegrity())
    {
        if (ERROR_SUCCESS != GetDisabledPrivileges(&pDisabledPrivileges, &dwDisabledPrivilegesCount))
        {
            g_exitCode = ERROR_COULD_NOT_LAUNCH_RESTRICTED;
            goto Cleanup;
        }

        Assert(dwDisabledPrivilegesCount != 0);
        Assert(pDisabledPrivileges != NULL);

        if (!IsCurrentProcessRestricted(pDisabledPrivileges, dwDisabledPrivilegesCount)) // checks ShouldProcessBeRestricted
        {
            if (!LaunchRestrictedProcess(NULL, pDisabledPrivileges, dwDisabledPrivilegesCount))
            {
                ReportActivationFaultAndTerminate(WPFHostError2HResult(ERROR_COULD_NOT_LAUNCH_RESTRICTED));
            }
            g_exitCode = ERROR_SUCCESS;
            goto Cleanup;
        }

        // If we aren't exiting then we should clean up
        if (pDisabledPrivileges != NULL)
        {
            delete [] pDisabledPrivileges;
            pDisabledPrivileges = NULL;
        }
    }

    (void)CheckRegistryAccess();
    (void)CheckAppDataFolderAccess();
    // The HRESULT is ignored. If the process exits at this point, IE will fall back on shell activation, 
    // which will lead to infinite relaunching... So, if the registry or file ACL prolem is detected, 
    // we send the Watson report and let ClickOnce deployment fail, which usually leads to the error page.

    // Set up termination thread if we don't connect to the host
    if (!isDebug)
    {
        g_exitCode = ERROR_NO_HOST_INIT_FAILED;
        DWORD dwTimeoutSeconds = 0;
        CKHR(GetRegistryDWORD(HKEY_LOCAL_MACHINE, RegKey_WPF_Hosting, RegValue_NoHostTimeoutSeconds, dwTimeoutSeconds, NO_HOST_TIMEOUT_SECONDS));
        g_hNoHostTimer_shim = TerminateIfNoHost(dwTimeoutSeconds);
        CHECK_BOOL_FROM_WIN32(g_hNoHostTimer_shim);
        g_exitCode = 0;
    }

    CKHR(CoInitialize(NULL));
    bCoInitialized = TRUE;
    
    if (!(g_dwCommandLineFlags & FLAG_EMBEDDING))
    {
        // Nothing to do
        g_exitCode = ERROR_NO_COMMAND_SPECIFIED;
        goto Cleanup;
    }

    CKHR(CClassFactory::RegisterClassObjects());

    // Signal the event if one was specified
    if (g_dwCommandLineFlags & FLAG_EVENT)
    {
        HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, wzEventName);
        BOOL success = hEvent != NULL && SetEvent(hEvent);
        if (hEvent)
        {
            CloseHandle(hEvent);
        }

        if (!success)
        {
            g_exitCode = ERROR_NO_EVENT;
            goto Cleanup;
        }
    }

    // For the VS debug scenario, invoke the browser
    if (*wzFileName && (g_dwCommandLineFlags & FLAG_DEBUG))
    {
        CKHR(InvokeBrowser(wzFileName));
    }

    EventWriteWpfHostUm_EnteringMessageLoop();

    //The message loop will only exit when the main object gets destroyed.
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0))
    {
        // Forward all messages; ForwardTranslateAccelerator is responsible for determining
        // which messages are relevant to it.
        CVersion* pVersion = CHostShim::GetValidVersion();
        if (pVersion && pVersion->IsAttached() && (pVersion->ForwardTranslateAccelerator(&msg) == NOERROR))
        {
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

Cleanup:
    CClassFactory::RevokeAllClassObjects();

    if (pDisabledPrivileges != NULL)
    {
        delete [] pDisabledPrivileges;
        pDisabledPrivileges = NULL;
    }
    
    if (bCoInitialized)
    {
        /*
        CoEEShutDownCOM() is called to cleanly release RCWs (COM Runtime Callable Wrappers).
        AppVerifier finds that "A COM Proxy was called from the wrong context". This presumably happens
        because the Finalizer thread fails to switch to the object's right context and calls 
        IUnknown->Release() directly. See WOSB 1795776.

        Microsoft says, "You should also call GetModuleHandle() on mscorwks.dll just to see if it is loaded. [...]
        CoEEShutdownCOM will cause mscorwks.dll to be loaded even if no managed code has been run."

        This patch may not be needed once PresentationHost is migrated to the DevDiv depot and starts
        using the MSVCRT version matching the compiler.
        */
        if ((GetModuleHandle(L"clr.dll") != NULL) || (GetModuleHandle(L"mscorwks.dll") != NULL))
        {
            LegacyActivationShim::CoEEShutDownCOM();
        }

        CoUninitialize();
    }

    // The only cases above where hr is set to failure code is while starting the app
    // and if registration failed. If we successfully started the app, return 0
    int retCode = FAILED(hr) ? hr : g_exitCode;

    EventWriteWpfHostUm_WinMainEnd((DWORD)retCode);

    return retCode;
}

// This is the export which is invoked by the shim implementation of
// PresentationHost.exe to call into the actual v4+ PresentationHost
// implementation.
extern "C" int __stdcall ProcessCommandLine(__in HINSTANCE hInstance, 
                   __in_opt LPSTR lpCommandLine, 
                   __in int nCmdShow)
{
    return DelegatedWinMain(hInstance, NULL, lpCommandLine, nCmdShow);
}
