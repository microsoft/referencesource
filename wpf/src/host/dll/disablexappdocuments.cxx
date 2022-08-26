//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     This file has all the code that detects whether the registry keys
//     to disable xps documents and xapps are set.
//  History:
//     2005/06/09 - akaza Created the file 
//     2007/09/20 - Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------
#include "Precompiled.hxx"
#include "DisableXappDocuments.hxx"
#include "UrlMonInterop.hxx"
#include "OleDocument.hxx"
#include "HostSupport.h"
#include "..\inc\registry.hxx"
    
// EXPLICIT definition because IE versions lesser than 7 will not have this value and will probabaly
// not recognize it.

#define URLMON_OPTION_FOR_BROWSERAPPSDOCUMENTS          0x10000010
#define URLACTION_WEB_BROWSER_APPLICATIONS_INTERNAL     0x00002400
#define URLACTION_XPS_DOCUMENTS_INTERNAL                0x00002401
#define URLACTION_LOOSE_XAML_INTERNAL                   0x00002402

// generic function that checks presence or absence of a reg key in HKLM 
// returns true if the key is present and its value is 1 , false otherwise
// pRegkeyLocationToCheck: The key in HKLM to open
// pRegKeyToExtract: The specific key to look for
BOOL IsDisableKeySet(LPCTSTR pRegkeyLocationToCheck, LPCTSTR pRegKeyToExtract)
{
    LONG    lRet = ERROR_SUCCESS;
    HKEY    hKey = NULL;
    DWORD   dwUnrestricted = 0;
    DWORD   dwSize = sizeof(dwUnrestricted);
    

    // throw exception in case pointer is null
    Assert((pRegkeyLocationToCheck!=NULL) &&
              (pRegKeyToExtract!=NULL));

    //  If it doesn't exist or if key exists and is 0 then return false
    //  if it exists and is set to 1, then return true
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        pRegkeyLocationToCheck,
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);

    if (ERROR_SUCCESS == lRet)
    {
        lRet = RegQueryValueEx(hKey,
                               pRegKeyToExtract,
                               NULL,
                               NULL,
                               (LPBYTE)&dwUnrestricted,
                               &dwSize);
        RegCloseKey(hKey);
    }
    return (ERROR_SUCCESS == lRet && dwUnrestricted == 1);
}



BOOL IsMimeTypeDisabled(__in_ecount(INTERNET_MAX_URL_LENGTH+1) LPOLESTR pUrl, __in_ecount(1) COleDocument* pOleDoc)
{
    MimeType mimeType = pOleDoc->GetCurrentMime();
    DWORD dwAction = URLACTION_WEB_BROWSER_APPLICATIONS_INTERNAL;

    // Get the mime type
    switch(mimeType)
    {
    case MimeType_Application:
        if (IsDisableKeySet(RegKey_WPF_Features, RegValue_XBAPDisallow))
            return TRUE;
        break;

    case MimeType_Xps:
        if (IsDisableKeySet(RegKey_WPF_Features, RegValue_XPSDocumentsDisallow))
            return TRUE;
        dwAction = URLACTION_XPS_DOCUMENTS_INTERNAL;
        break;

    case MimeType_Markup:
        if (IsDisableKeySet(RegKey_WPF_Features, RegValue_LooseXamlDisallow))
            return TRUE;
        dwAction = URLACTION_LOOSE_XAML_INTERNAL;
        break;
    }

    // The fact that the previous call returned false could mean one of two things
    // either the key was available and value was set to zero (which should not happen)
    // or the key does not exist. 
    // The key being set to zero could mean that we decided to disable this reg key or user did it
    // to allow xapps to work. We should in the most case not encounter this.
    // If the key was not present it could mean one of two things , either it was never there or 
    // we are on IE7. On IE 7 we want to run processUrlAction The following call does that for us

    return !ProcessURLActionOnIE7(dwAction,pUrl,pOleDoc);
}



// action indicates the mimetype we are dealing with
BOOL ProcessURLActionOnIE7(DWORD action, __in_ecount(INTERNET_MAX_URL_LENGTH+1) LPOLESTR pUrl, __in_ecount(1) COleDocument* pOleDoc)
{
    HRESULT                     hr = S_OK;
    DWORD                       dwResult = 0;
    LPBYTE                      dummyPtr = new BYTE[sizeof(DWORD)];

    Assert(pUrl != NULL);

    Assert(pOleDoc != NULL);

    // The following code calls into UrlMkGetSessionOption  with the flag for URLMON_OPTION_USE_BROWSERAPPSDOCUMENTS
    // if the return is E_INVALIDARGS then we are on a version of IE lesser than 7 else we get NOERROR.
    // In the case where we get NOERROR we call ProcessURLAction with the appropriate flag for XAPPs or XPS
    // depending on the value of the parameter passed in.            

    hr = UrlMkGetSessionOption(URLMON_OPTION_FOR_BROWSERAPPSDOCUMENTS, dummyPtr, sizeof(DWORD), &dwResult, 0);
    if (dummyPtr)
    {
        delete [] dummyPtr;
    }
    // this is the case where we are on a lower version of IE than version 7

    if (E_INVALIDARG == hr)
    {
        return TRUE;
    }

    // in this case we call ProcessURLAction depending on whether it is an application or a document

    BOOL fAllow = FALSE;
    if (NOERROR == hr)
    {
        // at this time COM is already initialized so I do not 
        // call CoInitializeEx
        //  we do not really care about the return hr for this call since it returns 
        //  either S_OK for allow or S_FALSE for ~S_ALLOW or E_OUTOFMEMORY

        hr = UrlmonInterop::ProcessUrlActionWrapper(action, pUrl, pOleDoc,fAllow);
    }
    return fAllow;
    
}
