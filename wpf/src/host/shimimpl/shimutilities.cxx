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
#include "ShimUtilities.hxx"
#include "..\inc\registry.hxx"

//******************************************************************************
//
// ParseCommandLine()
//
//    Parses the Windows command line which was passed to WinMain.
//    This function determines if the -Embedding switch has been given.
//
//      Valid usage:
//
//          <file name>
//          The path of the file to be activated. Can also be a URI.
//         
//          -event <event name>
//          Open the event with this name and signal it when the ClassFactory
//          is ready to go. Will terminate if there was an error opening
//          the event (e.g. if it has not already been created.)
//
//          -debug
//          When activating an application, do not commit it to the store. Only
//          works when activating a local file.
//
//          -embedding
//          Register the ClassFactory and wait for a CreateInstance call.
//          If the -event or -debug flags are specified, it is not necessary to
//          specify the -embedding flag, since that flag is set internally.
//
//          -DebugSecurityZoneURL
//          URL that is used to give site of origin access to app while debugging only
//
//          -LaunchDotApplication
//          URL of a .application to launch via click once.  This argument is used from
//          Firefox to launch .application with the correct security checks in place.
//
//
//      Scenarios:
//
//          Shell handler
//          PresentationHost.exe <file name>
//          XpsViewer.exe <file name>
//
//          MIME-handler (can also be used when debugging PresentationHost itself)
//          PresentationHost.exe -embedding
//          XpsViewer.exe -embedding
//
//          VS F5 debugging
//          PresentationHost.exe -event <event name> -debug
//
//******************************************************************************

HRESULT ParseCommandLine(__out_ecount(1) LPDWORD pdwFlags, 
                         __out_ecount(nFileNameSize) LPWSTR pwzFileName,
                         size_t nFileNameSize,
                         __out_ecount(nEventNameSize) LPWSTR pwzEventName,
                         size_t nEventNameSize,
                         __out_ecount(nDebugSecurityZoneURLSize) LPWSTR pwzDebugSecurityZoneURL,
                         size_t nDebugSecurityZoneURLSize,
                         __out_ecount_opt(nDotApplicationURLSize) LPWSTR pwzDotApplicationURL,
                         size_t nDotApplicationURLSize
                         )
{
    HRESULT hr = S_OK;

    if (pdwFlags)
    {
        *pdwFlags = FLAGS_NONE;
    }
    if (pwzFileName)
    {
        *pwzFileName = L'\0';
    }
    if (pwzEventName)
    {
        *pwzEventName = L'\0';
    }
    if(pwzDebugSecurityZoneURL)
    {
        *pwzDebugSecurityZoneURL = L'\0';
    }
    if (pwzDotApplicationURL)
    {
        *pwzDotApplicationURL = L'\0';
    }

    LPWSTR* argv = NULL;

    CK_PARG(pdwFlags);
    CK_PARG(pwzEventName);
    CK_PARG(pwzFileName);
    CK_PARG(pwzDebugSecurityZoneURL);

    // Get a WCHAR version of the command line (the one passed in WinMain is not)
    int argc;
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    UINT argcUnsigned;	
    CKHR( IntToUInt(argc, &argcUnsigned) );

    __bound UINT nArgIndex = 1;
    while(nArgIndex < argcUnsigned)
    {
        LPWSTR arg = argv[nArgIndex];

        // Check for "-<flag>" or "/<flag>" and set appropriate internal flag
        if ((*arg == L'-') || (*arg == L'/'))
        {
            arg++;

            if (!_wcsicmp(arg, EMBEDDINGFLAG))
            {
                *pdwFlags |= FLAG_EMBEDDING;
            }
            else if (!_wcsicmp(arg, DEBUGFLAG))
            {
                *pdwFlags |= FLAG_DEBUG | FLAG_EMBEDDING;
            }
            else if ((!_wcsicmp(arg, DEBUGSECURITYZONEURLFLAG)))
            {
                *pdwFlags |= FLAG_DEBUGSECURITYZONEURL ;
                // The next parameter must be the name of the Site of origin; if it isn't that's an error.
                if (nArgIndex + 1 < argcUnsigned)
                {
                    arg = argv[++nArgIndex];
                    CKHR(StringCchCopy(pwzDebugSecurityZoneURL, nDebugSecurityZoneURLSize, arg));
                    CKHR(((*pwzDebugSecurityZoneURL == L'-') || (*pwzDebugSecurityZoneURL == L'/')) ? E_INVALIDARG : S_OK);
                }
            }
            else if (!_wcsicmp(arg, EVENTFLAG))
            {
                *pdwFlags |= FLAG_EVENT | FLAG_EMBEDDING;

                // The next parameter must be the name of the event; if it isn't that's an error.
                if (nArgIndex + 1 < argcUnsigned)
                {
                    arg = argv[++nArgIndex];
                    CKHR(StringCchCopy(pwzEventName, nEventNameSize, arg));
                    CKHR(((*pwzEventName == L'-') || (*pwzEventName == L'/')) ? E_INVALIDARG : S_OK);
                }
            }
#ifdef APPLICATION_SHIM            
            else if (!_wcsicmp(arg, LAUNCHDOTAPPLICATIONFLAG) && pwzDotApplicationURL)
            {
                *pdwFlags |= FLAG_LAUNCHDOTAPPLICATION;
                if (nArgIndex + 1 < argcUnsigned)
                {
                    arg = argv[++nArgIndex];
                    CKHR(StringCchCopy(pwzDotApplicationURL, nDotApplicationURLSize, arg));
                    CKHR(((*pwzDotApplicationURL == L'-') || (*pwzDotApplicationURL == L'/')) ? E_INVALIDARG : S_OK);
                }
            }
            else if (!_wcsicmp(arg, REGISTERSERVERFLAG))
            {
                *pdwFlags |= FLAG_REGISTERSERVER;
            }
            else if (!_wcsicmp(arg, UNREGISTERSERVERFLAG))
            {
                *pdwFlags |= FLAG_UNREGISTERSERVER;
            }
#endif
            else
            {
                CKHR(E_INVALIDARG);
            }
        }
        else
        {
            if ((*pdwFlags == FLAGS_NONE || *pdwFlags & FLAG_DEBUG) && *pwzFileName == NULL)
            {
                // Only use this arg for the file name. We could return E_INVALIDARG
                // if there are more arguments, but we should try to run with what we have.
                //added check for filename to be null since we only use the first filename that we get

                CKHR(StringCchCat(pwzFileName, nFileNameSize, arg));
            }
            else
            {
                // A file name is not valid unless either no flags are specified or the debug flag is.
                CKHR(E_INVALIDARG);
            }
        }
        ++nArgIndex;
    }

Cleanup:
    if (argv)
    {
        LocalFree(argv);
    }
    return hr;
}

BOOL MsgWaitForSingleObject(__in HANDLE hEvent, DWORD dwMilliSeconds, __out_ecount(1) BOOL* fWmQuit)
{
    BOOL bDone = TRUE;
    //Compiler complains about while(1) usage
    while (bDone == TRUE)
    {
        DWORD result;
        MSG msg;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                *fWmQuit = TRUE;
                PostQuitMessage((int) msg.wParam);
                return FALSE;
            }
            DispatchMessage(&msg);
        }

        result = MsgWaitForMultipleObjects(1, &hEvent, FALSE, dwMilliSeconds, QS_ALLINPUT);

        if (result == WAIT_OBJECT_0+1)
            continue;
        else
            break;
    }

    *fWmQuit = FALSE;
    //
    return TRUE;
}

// Check to see if the MimeType is not unknown and valid
BOOL IsValidMimeType(__in MimeType mime)
{
#ifdef DOCUMENT_SHIM
    return mime == MimeType_Xps;
#endif

#ifdef APPLICATION_SHIM
    return mime == MimeType_Application || mime == MimeType_Markup;
#endif
}

MimeType GetMimeTypeFromString(LPCWSTR szMimeString)
{
    if (_wcsicmp(szMimeString, CONTENT_TYPE_XPSDOCUMENT) == 0)
    {
        return MimeType_Xps;
    }
    else if (_wcsicmp(szMimeString, CONTENT_TYPE_ZIP) == 0)
    {
        return MimeType_Xps;
    }
    else if (_wcsicmp(szMimeString, CONTENT_TYPE_APPLICATION) == 0)             
    {
        return MimeType_Application;
    }
    else if (_wcsicmp(szMimeString, CONTENT_TYPE_MARKUP) == 0)
    {
        return MimeType_Markup;
    }
#ifdef DOCUMENT_SHIM
    else if (S_OK == EndsWith(szMimeString, L"+xps"))
    {
        return MimeType_Xps;
    }
#endif
    return MimeType_Unknown;
}

#ifdef DOCUMENT_SHIM
HRESULT EndsWith(LPCWSTR szTarget, LPCWSTR szMatch)
{
    HRESULT hr = (NULL != szTarget) && (NULL != szMatch) ? S_OK : E_FAIL;

    size_t cchMatch  = 0;
    size_t cchTarget = 0;
    size_t cchOffset = 0;

    if (SUCCEEDED(hr))
    {
        CKHR(StringCchLength(szMatch, MAX_PATH, &cchMatch));
        CKHR(StringCchLength(szTarget, MAX_PATH, &cchTarget));
        hr = SizeTSub(cchTarget, cchMatch, &cchOffset);
    }

    if (SUCCEEDED(hr))
    {
        hr = (_wcsicmp(szTarget + cchOffset, szMatch) == 0)
            ? S_OK : S_FALSE;
    }

Cleanup:
    return hr;
}
#endif

// Returned message buffer must be freed with LocalFree().
wchar_t* TryFormatErrorMessage(HMODULE hLib, HRESULT hr)
{
    wchar_t *pErrMsg = 0;
    if(FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 
        hLib, hr, 0, reinterpret_cast<LPWSTR>(&pErrMsg), 0, 0) != 0)
    {
        return pErrMsg;
    }
    return 0;
}

// Returned message buffer must be freed with LocalFree().
wchar_t *TryDecodeDownloadError(HRESULT hr)
{
    wchar_t *pErrMsg = 0;
    HMODULE hLib = GetModuleHandle(L"urlmon.dll");
    if(hLib)
    {
        pErrMsg = TryFormatErrorMessage(hLib, hr);
        // There's a typo in inetcore\urlmon\dll\urlerr.mc: The MessageId for this error is incorrectly 
        // entered as 0x11. Windows OOB Releases bug 1068105.
        if(!pErrMsg && hr == INET_E_INVALID_CERTIFICATE/*0x800C0019*/)
        {
            pErrMsg = TryFormatErrorMessage(hLib, 0x800C0011);
        }
    }
    if(!pErrMsg)
    {
        hLib = GetModuleHandle(L"wininet.dll");
        if(hLib)
        {
            pErrMsg = TryFormatErrorMessage(hLib, hr);
        }
    }
    return pErrMsg;
}

#ifdef APPLICATION_SHIM

HRESULT LaunchClickOnceApplication(LPCWSTR szApplicationURL)
{
    HRESULT hr;
    CComPtr<IInternetSecurityManager> spSecurityManager = NULL;
    HINSTANCE hInstance = NULL;

    // Check the .NET code running policy. (This code mostly copied from CLR's IE mime-filter.)
    // see: //depot/devdiv/private/ClickOnceNetFxSp/ndp/fx/src/Clickonce/clickoncedll/mimehandler.cpp
    CKHR(CoInternetCreateSecurityManager(NULL, &spSecurityManager, 0));

    DWORD dwUnsignedPolicy, dwSignedPolicy, dwDownloadPolicy, cbPolicy = sizeof(DWORD);

    // Look at the unsigned code policy associated with the source URL.
    CKHR(spSecurityManager->ProcessUrlAction(
                szApplicationURL, 
                URLACTION_MANAGED_UNSIGNED,
                (PBYTE) &dwUnsignedPolicy,
                cbPolicy,
                NULL,
                0,
                PUAF_NOUI,
                0));

    // Lets also retrieve the unsigned code policy associated with the source URL.
    CKHR(spSecurityManager->ProcessUrlAction(
                szApplicationURL,
                URLACTION_MANAGED_SIGNED,
                (PBYTE) &dwSignedPolicy,
                cbPolicy,
                NULL,
                0,
                PUAF_NOUI,
                0));

    /// We are adding more complex integration with IE settings,
    /// which requires we delay this decision to managed service (dfsvc.exe).
    
    // Look at the file downloads policy associated with the source URL.
    CKHR(spSecurityManager->ProcessUrlAction(
                szApplicationURL,
                URLACTION_SHELL_FILE_DOWNLOAD,
                (PBYTE) &dwDownloadPolicy,
                cbPolicy,
                NULL,
                0,
                PUAF_NOUI | PUAF_WARN_IF_DENIED,
                0));

    if (dwDownloadPolicy != URLPOLICY_ALLOW)
    {
        CKHR(CO_E_NOT_SUPPORTED);
    }

    // passed checks, go ahead and launch the app
    //

    CKHR(LegacyActivationShim::LoadLibraryShim(DFDLL, NULL /*latest version*/, NULL, &hInstance));
    fnActivateDeploymentEx pfnActivateDeploymentEx = (fnActivateDeploymentEx) GetProcAddress(hInstance, "ActivateDeploymentExW");
    CHECK_NULL_FROM_WIN32(pfnActivateDeploymentEx);

    /// We have to launch dfsvc.exe to resolve sign/unsign issues and evetually
    /// start install.
    CKHR(pfnActivateDeploymentEx(szApplicationURL, dwUnsignedPolicy, dwSignedPolicy));


Cleanup:
    if (hInstance)
    {
        FreeLibrary(hInstance);
    }
    return hr;
}
#endif

