//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//

//  Description:
//     This file has all the Low Rights code.  This is bascially the code that detects
//     if the process is running with high privileges and the code that'll restart the
//     process with low privileges.
//
//     One key change from the IE code is that we run restricted by default.  The change
//     is in the function ShouldProcessBeRestricted().
//
//  History:
//     2005/03/10 - Created the file based on code we got from Anantha Ganjam in the IE team.
//     2007/07/06 - Microsoft - Switched to using the LSA APIs to replace the use of LookupPrivilegeValue()
//          with a more efficient version for our scenario.
//          In lieu of SDK documentation of the LSA APIs, there's this KB article:
//          http://support.microsoft.com/kb/132958.
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "LoRights.hxx"
#include "..\inc\registry.hxx"

// <NTLSA.H> Substitute
// [Begin]
// In Windows we got all these definitions (directly or indirectly) from ntlsa.h, which is not included in 
// DevDiv's SDK. Trying to transplant it fails because it's apparently incompatible with the DevDiv versions
// of other headers. ntddk.h & ntdef.h also contain most of these definitions, but they cause too many 
// redefinition errors with winnt.h. (There are multiple reports of this happening on the web. People
// try to work around by including ntddk.h in a namespace, but that by itself doesn't seem enough.)

#include <ntsecapi.h> // This gives us most of what ntlsa.h did in Windows.

extern "C"
VOID
NTAPI 
RtlInitUnicodeString (
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );


#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS  ((NTSTATUS)0x00000000L)
#endif

// from ntdef.h
#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( LSA_OBJECT_ATTRIBUTES );      \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }
extern "C" {
// from ntlsa.h
NTSTATUS
NTAPI
LsaLookupPrivilegeValue(
    __in LSA_HANDLE PolicyHandle,
    __in PLSA_UNICODE_STRING Name,
    __out PLUID Value
    );
}
//[End]

const int   cMaxAttrLen = 512;
const int   cMaxSidName = 512;

void FreeSIDArray(__inout_ecount(cSIDs) SID_AND_ATTRIBUTES *pSIDs, unsigned cSIDs)
{
    for (DWORD i = 0; i < cSIDs; i++)
    {
        delete [] static_cast<BYTE*>(pSIDs[i].Sid);
    }
    delete [] pSIDs;
}

// This is the function that launches our process with restricted tokens
BOOL LaunchRestrictedProcess(__in LPCWSTR lpwszCmdLine, __in_ecount(dwDisabledPrivilegeCount) PLUID_AND_ATTRIBUTES pDisabledPrivileges, DWORD dwDisabledPrivilegeCount)
{
    BOOL    bRet = FALSE;
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwDisabledSidCount = 0;
    PSID_AND_ATTRIBUTES     pDisabledSids = NULL;

    // lpwszCmdLine is not traced, because WinMain actually passes NULL.
    EventWriteWpfHostUm_LaunchingRestrictedProcess();

    //For the current impementation this returns the list of SIDS in g_disableSIDS
    // which happens to be Power User and Admin
    if (ERROR_SUCCESS == (dwStatus = GetDisabledSids( &pDisabledSids, &dwDisabledSidCount)))
    {
        //  if we for some reason we cannot start a restricted process
        //  the only option we have is to start process normally.
        if (ERROR_SUCCESS == (dwStatus = CreateRestrictedProcess( pDisabledSids,
                                                                  dwDisabledSidCount,
                                                                  pDisabledPrivileges,
                                                                  dwDisabledPrivilegeCount,
                                                                  lpwszCmdLine)))
        {
            bRet = TRUE;
        }
        
        FreeSIDArray(pDisabledSids, dwDisabledSidCount);
    }
    return bRet;
}

// This functions checks to see the registry if the key to restrict the process is set 
BOOL ShouldProcessBeRestricted(void)
{
    BOOL    fRet = TRUE;
    LONG    lRet = ERROR_SUCCESS;
    HKEY    hKey = NULL;
    DWORD   dwUnrestricted = 0;
    DWORD   dwSize = sizeof(dwUnrestricted);

    // We used to have an OS check here to terminate the 
    // process if we were running on version < XP SP2. 
    // We no longer need this check since we are guaranteed to 
    // be running on version >= Vista 
    // 
       
    //  Check the RunUnrestricted registry value.
    //  If it exists and is 0 then run PresentationHost with no restrictions or as the user
    //  If it doesn't exist or if it exists and is set to 1, then run IE in restricted mode
    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                         RegKey_WPF_Hosting,
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);

    if (ERROR_SUCCESS == lRet)
    {
        lRet = RegQueryValueEx( hKey,
                                RegValue_RunUnrestricted,
                                NULL,
                                NULL,
                                (LPBYTE)&dwUnrestricted,
                                &dwSize);
        if (ERROR_SUCCESS == lRet && dwUnrestricted == 1)
        {
            fRet = FALSE;
        }
        RegCloseKey(hKey);
    }
    return fRet;
}

// The logic here is essentially checking to see of the current process is already restricted
// If it is then we do not have to do additional work.
BOOL IsCurrentProcessRestricted(__in_ecount(dwDisabledPrivilegesCount) PLUID_AND_ATTRIBUTES pDisabledPrivileges, DWORD dwDisabledPrivilegesCount)
{
    static BOOL s_bInit = FALSE;
    static BOOL s_bRet = TRUE;

    if (FALSE == s_bInit)
    {
        // Checks registry to see if this process needs to be disabled 
        if (ShouldProcessBeRestricted())
        {
            DWORD   dwStatus = ERROR_SUCCESS;
            DWORD   dwDisabledSidCount = 0;
            PSID_AND_ATTRIBUTES     pDisabledSids = NULL;

            // Extract ---- SIDs (currently Admin and power user)
            if (ERROR_SUCCESS == (dwStatus = GetDisabledSids( &pDisabledSids, &dwDisabledSidCount)))
            {
                // if the current process is already restricted then we do not need to do anything more    
                if (!IsCurrentPresentationHostRestricted( pDisabledSids,
                                                          dwDisabledSidCount,
                                                          pDisabledPrivileges,
                                                          dwDisabledPrivilegesCount))
                {
                    s_bRet = FALSE;
                }

                FreeSIDArray(pDisabledSids, dwDisabledSidCount);
            }
            s_bInit = TRUE;
        }
    }
    return s_bRet;
}

// This function takes the preset list of restricted groups and priviliges and then
// checks to see if any of them exist on the current token.
// If they do then the process is not restricted and needs to be restricted.
BOOL IsCurrentPresentationHostRestricted(__in PSID_AND_ATTRIBUTES pDisabledSids,
                                         __in DWORD dwDisabledSidCount,
                                         __in PLUID_AND_ATTRIBUTES pDisabledPrivileges,
                                         __in DWORD dwDisabledPrivilegeCount)
{
    //  If we cannot open a process token for query we cannot generate a restricted token from this token
    //  If we fail here we will try to create a restricted token which is bound to fail
    //  Either case for appcompat we need to just open PresenationHost as-is. If not PresenationHost will never launch
    BOOL    fRet = TRUE;
    HANDLE  hProcToken;

    //  Check if the current PresentationHost is restricted
    //  The restrictions we put on PresenationHost are 
    //  1. Disable in Administrators and Power Users groups
    //  2. Remove a set of Privileges 
    //  If the current token has these settings then we are already in restricted PresentationHost

    // Open token for the current process , this function actually is quite generic
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcToken))
    {
        // The structure of this API is such that it returns an error to indicate success 
        // when getting size of buffer that needs to be allocated   
        DWORD   dwSize = 0;
        if (!GetTokenInformation(hProcToken, TokenGroupsAndPrivileges, NULL, 0, &dwSize) &&
            GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            BYTE userSID[SECURITY_MAX_SID_SIZE];
            PTOKEN_GROUPS_AND_PRIVILEGES pTokenGroups;
            pTokenGroups = reinterpret_cast<PTOKEN_GROUPS_AND_PRIVILEGES>(new BYTE[dwSize]);
            if (!pTokenGroups)
            {
                fRet = false; // That's a safer way to fail.
            }
            else
            {
                // This is a call to query for a token of the process with information on groups and priviliges
                if ( GetTokenInformation(hProcToken, TokenGroupsAndPrivileges, pTokenGroups, dwSize, &dwSize) && 
                    ERROR_SUCCESS == GetUserSid(hProcToken, &userSID))
                {
                    // If any of our restricted set is part of the groups or priviliges extracted then
                    // this process is not restricted and needs to be stripped.
                    if ( FALSE == CheckForDisabledSids(pTokenGroups->Sids, pTokenGroups->SidCount, pDisabledSids, dwDisabledSidCount) ||
                         FALSE == CheckForRestrictedSids(pTokenGroups->RestrictedSids, pTokenGroups->RestrictedSidCount, pDisabledSids, dwDisabledSidCount, &userSID) ||
                         FALSE == CheckForPrivileges(pTokenGroups->Privileges, pTokenGroups->PrivilegeCount, pDisabledPrivileges, dwDisabledPrivilegeCount))
                    {
                        fRet = FALSE;
                    }
                }
                delete [] reinterpret_cast<BYTE*>(pTokenGroups);
            }
        }
        CloseHandle(hProcToken);
    }
    return fRet;
}


// This is the function where the actual stripping of the token takes place, it accepts the Groups and priviliges to strip
DWORD CreateRestrictedProcess(  __in PSID_AND_ATTRIBUTES pDisabledSids,
                                __in DWORD dwDisabledSidCount,
                                __in PLUID_AND_ATTRIBUTES pDisabledPrivileges,
                                __in DWORD dwDisabledPrivilegeCount,
                                __in LPCWSTR lpwszCmdLine)
{
    //  Get the process token, add disabled SIDs for Administrators and Power Users groups
    //  Also remove the above listed privileges 
    //  Create a new PresenationHost process with this token 
    DWORD   dwStatus = ERROR_SUCCESS;
    HANDLE  hProcToken = NULL;
    HANDLE  hRestrictedToken = NULL;

    //  Ther current PresentationHost is not restriced, so we create a restricted token that 
    //  disables Administrators and Power Users groups if present and removes 
    //  a set of Privileges and adds restrictions
    if (OpenProcessToken(GetCurrentProcess(),
                         TOKEN_READ|TOKEN_WRITE|TOKEN_ASSIGN_PRIMARY|TOKEN_DUPLICATE,
                         &hProcToken))
    {

        // Extracts SID for current user, this is the priviliges that the current process will have, and 
        // even if user is admin or power user we will strip those.
        BYTE userSID[SECURITY_MAX_SID_SIZE];
        if (ERROR_SUCCESS == (dwStatus = GetUserSid(hProcToken, &userSID)))
        {
            // This is where we create a token without the priviliges and groups that we need to disable
            if (CreateRestrictedToken( hProcToken,
                                       0/*WRITE_RESTRICTED*/ ,
                                       dwDisabledSidCount,
                                       pDisabledSids,
                                       dwDisabledPrivilegeCount,
                                       pDisabledPrivileges,
                                       0, /* dwRestrictedSidCount*/
                                       NULL, /* pRestrictedSids*/ 
                                       &hRestrictedToken))
            {
                WCHAR                   wszPath[MAX_PATH];
                LPWSTR                  pwszArgs = NULL;
                STARTUPINFO             si = { sizeof(si) };
                PROCESS_INFORMATION     pi;
                GetStartupInfo(&si);
                ZeroMemory(&pi, sizeof(pi));
                if (NULL == lpwszCmdLine)
                    pwszArgs = GetCommandLineW();
                else
                    pwszArgs = (LPWSTR)lpwszCmdLine;
                if (GetModuleFileNameW(NULL, wszPath, ARRAY_SIZE(wszPath)))
                {
                    // allow access to current user sid 
                    PSID    pAccessAllowedSIDS[1] = { &userSID };
                    // Allow query and Impersonate permissions to these  users on the token
                    if (ERROR_SUCCESS == (dwStatus = AddSidsToToken(hRestrictedToken, pAccessAllowedSIDS, 1 , GENERIC_ALL /*TOKEN_QUERY|TOKEN_IMPERSONATE*/ )))
                    {

                        // Launch the process with a restricted token
                        if (CreateProcessAsUser(hRestrictedToken,
                                                wszPath,
                                                pwszArgs,
                                                NULL,
                                                NULL,
                                                FALSE,
                                                0,
                                                NULL,
                                                NULL, 
                                                &si,
                                                &pi))
                        {
                            WaitForInputIdle(pi.hProcess, 5000);
                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                        }
                        else
                        {
                            dwStatus = GetLastError();
                        }
                    }
                }
                else
                {
                    dwStatus = GetLastError();
                }
                CloseHandle(hRestrictedToken);
            }
            else
            {
                dwStatus = GetLastError();
            }
        }
        CloseHandle(hProcToken);

    }
    else
    {
        dwStatus = GetLastError();
    }

    return dwStatus;
}

// returns false if any of ---- sids are on the current token list as anything but Deny_only 
BOOL CheckForDisabledSids(__in PSID_AND_ATTRIBUTES pSids,
                          __in DWORD dwSidCount,
                          __in PSID_AND_ATTRIBUTES pDisabledSids,
                          __in DWORD dwDisabledSidCount)
{
    BOOL    fRet = TRUE;
    DWORD   dwGroupCount = 0;;
    DWORD   dwCount = 0;

    for (dwGroupCount=0; dwGroupCount<dwSidCount; dwGroupCount++)
    {
        for (dwCount=0; dwCount< dwDisabledSidCount; dwCount++)
        {
            if (EqualSid(pSids[dwGroupCount].Sid, pDisabledSids[dwCount].Sid))
            {
                if (!(pSids[dwGroupCount].Attributes & SE_GROUP_USE_FOR_DENY_ONLY))
                {
                    fRet = FALSE;
                    break;
                }
            }
        }
    }
    return fRet;
}

//check if the restricted tokens contain any of the tokens we want to strip
BOOL CheckForRestrictedSids(__in PSID_AND_ATTRIBUTES pSids,
                            __in DWORD dwSidCount,
                            __in PSID_AND_ATTRIBUTES pRestrictedSids,
                            __in DWORD dwRestrictedSidCount, 
                            __in PSID pUserSid)
{
    BOOL    fRet = TRUE;
    DWORD   dwGroupCount = 0;;
    DWORD   dwCount = 0;

    // if current user is admin or power user we need to strip
    for (dwCount = 0; dwCount < dwRestrictedSidCount; dwCount++)
    {
        if (EqualSid(pUserSid, pRestrictedSids[dwCount].Sid))
        {
            fRet = FALSE;
            break;
        }
    }
 
    // if the restricted SIDs contain admin or power user return false since we need to strip
    if (fRet)
    {
        for (dwGroupCount = 0; dwGroupCount < dwSidCount; dwGroupCount++)
        {
            for (dwCount = 0; dwCount < dwRestrictedSidCount; dwCount++)
            {
                if (EqualSid(pSids[dwGroupCount].Sid, pRestrictedSids[dwCount].Sid))
                {
                    fRet = FALSE;
                    break;
                }
            }
        }
    }
    return fRet;
}


// This checks to see if any of the  priviliges we have predetermined are part of the current list. If they are we return false else true
BOOL CheckForPrivileges(__in PLUID_AND_ATTRIBUTES pPrivileges,
                        __in DWORD dwPrivilegeCount,
                        __in PLUID_AND_ATTRIBUTES pDisabledPrivileges,
                        __in DWORD dwDisabledPrivilegeCount)
{
    BOOL    fRet = TRUE;
    DWORD   dwPrivCount = 0;;
    DWORD   dwCount = 0;

    for (dwPrivCount=0; dwPrivCount<dwPrivilegeCount; dwPrivCount++)
    {
        for (dwCount=0; dwCount<dwDisabledPrivilegeCount; dwCount++)
        {
            if (0 == memcmp(&pPrivileges[dwPrivCount].Luid, &pDisabledPrivileges[dwCount].Luid, sizeof(LUID)))
            {
                fRet = FALSE;
                break;
            }
        }
    }
    return fRet;
}

// Iterates over the predefined list of disabled SIDs we have and obtains the SID values for them.
// Call FreeSIDArray() when done with the returned data.
DWORD GetDisabledSids(__out PSID_AND_ATTRIBUTES *ppDisabledSids, __out DWORD *pdwDisabledSidCount)
{
    DWORD               dwStatus = ERROR_SUCCESS;
    DWORD               dwCount;
    DWORD               dwSidSize;
    PSID_AND_ATTRIBUTES pDisabledSids;

    pDisabledSids = new SID_AND_ATTRIBUTES[ARRAY_SIZE(g_disableSIDS)];
    if (pDisabledSids)
    {
        memset(pDisabledSids, 0, (ARRAY_SIZE(g_disableSIDS))*sizeof(SID_AND_ATTRIBUTES));
        for (dwCount=0; dwCount<ARRAY_SIZE(g_disableSIDS); dwCount++)
        {
            pDisabledSids[dwCount].Sid = new BYTE[SECURITY_MAX_SID_SIZE];
            if (pDisabledSids[dwCount].Sid)
            {
                dwSidSize = SECURITY_MAX_SID_SIZE;
                if (!CreateWellKnownSid(g_disableSIDS[dwCount], NULL, pDisabledSids[dwCount].Sid, &dwSidSize))
                {
                    dwStatus = GetLastError();
                    break;
                }
            }
            else
            {
                dwStatus = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }

        if (dwStatus == ERROR_SUCCESS)
        {
            *ppDisabledSids = pDisabledSids;
            *pdwDisabledSidCount = ARRAY_SIZE(g_disableSIDS);
        }
        else
        {
            FreeSIDArray(pDisabledSids, ARRAY_SIZE(g_disableSIDS));
        }
    }
    else
    {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
    }
    return dwStatus;
}

//This call gets a list of disabled priviliges
DWORD GetDisabledPrivileges(__out PLUID_AND_ATTRIBUTES *ppDisabledPrivileges, __out DWORD *pdwDisabledPrivlegeCount)
{
    DWORD                   dwStatus = ERROR_SUCCESS;
    DWORD                   dwCount;
    *ppDisabledPrivileges = NULL;
    *pdwDisabledPrivlegeCount = 0;
    PLUID_AND_ATTRIBUTES    pDisabledPrivileges;

    pDisabledPrivileges = new LUID_AND_ATTRIBUTES[ARRAY_SIZE(g_ppwszPrivileges)];
    if (pDisabledPrivileges)
    {
        NTSTATUS ntStatus;
        LSA_HANDLE PolicyHandle;
        LSA_OBJECT_ATTRIBUTES ObjectAttributes;
        InitializeObjectAttributes(&ObjectAttributes, NULL, 0L, NULL, NULL);

        // Instead of calling LookupPrivilegeValue call LsaLookupPrivilegeValue 
        // to avoid LsaOpenPolicy/LsaClose inside the loop.
        ntStatus = LsaOpenPolicy(NULL, 
                        &ObjectAttributes,
                        POLICY_LOOKUP_NAMES,
                        &PolicyHandle);
        if (ntStatus == STATUS_SUCCESS)
        {
            UNICODE_STRING uName;
            for(dwCount=0; dwCount<ARRAY_SIZE(g_ppwszPrivileges); dwCount++)
            {
                // Initialize a counted string for LsaLookupPrivilegeValue.  The buffer of uName points to the WSTR argument
                RtlInitUnicodeString(&uName, g_ppwszPrivileges[dwCount]);
                ntStatus = LsaLookupPrivilegeValue(PolicyHandle, &uName, &pDisabledPrivileges[dwCount].Luid);
                if (ntStatus != STATUS_SUCCESS)
                {
                    dwStatus = LsaNtStatusToWinError(ntStatus);
                    break;
                }
            }

            ntStatus = LsaClose(PolicyHandle);
            if (ntStatus != STATUS_SUCCESS)
            {
                dwStatus = LsaNtStatusToWinError(ntStatus);
            }
        }
        else
        {
            dwStatus = LsaNtStatusToWinError(ntStatus);
        }
            
        if (dwStatus == ERROR_SUCCESS)
        {
            *ppDisabledPrivileges = pDisabledPrivileges;
            *pdwDisabledPrivlegeCount = ARRAY_SIZE(g_ppwszPrivileges);
        }
        else
        {
            delete [] pDisabledPrivileges;
        }
    }
    else
    {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
    }

    return dwStatus;
}

//Extracts the user sid for the process
DWORD GetUserSid(__in HANDLE hProcToken, __out_bcount(SECURITY_MAX_SID_SIZE) PSID pSid)
{
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   cbTokenInfo = 0;
 
    // GetTokenInformation(TokenUser) pupulates a TOKEN_USER structure immediately followed by a
    // SID structure. TOKEN_USER::User.Sid points to the adjoined SID structure.
    BYTE buffer[sizeof TOKEN_USER + SECURITY_MAX_SID_SIZE];
    TOKEN_USER *pTokenUser = reinterpret_cast<TOKEN_USER*>(buffer);
    if (GetTokenInformation(hProcToken, TokenUser, pTokenUser, sizeof buffer, &cbTokenInfo))
    {
        CopySid(SECURITY_MAX_SID_SIZE, pSid, pTokenUser->User.Sid);
    }
    else
    {
        dwError = GetLastError();
    }
    return dwError;
}


// This call adds a list of sids to a token
DWORD AddSidsToToken(__in HANDLE hRestrictedToken, __in PSID *ppSids, __in DWORD dwSids, __in DWORD dwAccess)
{
    PACL                    pOldDacl = NULL;
    PACL                    pNewDacl = NULL;
    BOOL                    fDaclPresent;
    BOOL                    fDaclDefaulted;
    DWORD                   cbSD = 0;
    DWORD                   dwError = ERROR_SUCCESS; 
    PSECURITY_DESCRIPTOR    pSD = NULL;

    // GetKernelObjectSecurity() appends various SIDs to a SECURITY_DESCRIPTOR header.
    // We first query for the needed space.
    if (!GetKernelObjectSecurity(hRestrictedToken, DACL_SECURITY_INFORMATION, NULL, cbSD, &cbSD) &&
        (dwError = GetLastError()) == ERROR_INSUFFICIENT_BUFFER)
    {
        pSD = reinterpret_cast<PSECURITY_DESCRIPTOR>(new BYTE[cbSD]);
        if (pSD)
        {
            dwError = ERROR_SUCCESS;
            if (GetKernelObjectSecurity(hRestrictedToken, DACL_SECURITY_INFORMATION, pSD, cbSD, &cbSD))
            {
                if (GetSecurityDescriptorDacl(pSD, &fDaclPresent, &pOldDacl, &fDaclDefaulted))
                {
                    DWORD   cbDacl = 0;
                    if (ERROR_SUCCESS == (dwError = SetSidsOnAcl(ppSids, dwSids, pOldDacl, &pNewDacl, &cbDacl, dwAccess, 0)))
                    {
                        DWORD   cbTokenInfo;
                        TOKEN_DEFAULT_DACL  tokenDefaultDACL;
                        
                        tokenDefaultDACL.DefaultDacl = pNewDacl;
                        cbTokenInfo = sizeof(tokenDefaultDACL)+cbDacl;

                        if (!SetTokenInformation(hRestrictedToken, TokenDefaultDacl, &tokenDefaultDACL, cbTokenInfo))
                        {
                            dwError = GetLastError();
                        }
                        delete pNewDacl;
                    }
                }
                else
                {
                    dwError = GetLastError();
                }
            }
            else
            {
                dwError = GetLastError();
            }
            delete [] reinterpret_cast<BYTE*>(pSD);
        }
        else
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
   
    return dwError;
}

//gives access to built in users and current users, current value passed in also includes IEGroup but that must die.
BOOL  SetSidsOnAcl(__in PSID *ppSids,
                   __in DWORD dwSids,
                   __in PACL pAclSource,
                   __out PACL *pAclDestination,
                   __out DWORD *pcbDacl,
                   __in DWORD AccessMask,
                   __in BYTE AceFlags)
{
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   dwNewAclSize;
    DWORD   AceCounter;
    __bound DWORD   iSid;
    PACCESS_ALLOWED_ACE  pAce;
    
    ACL_SIZE_INFORMATION AclInfo;

    if (pAclSource != NULL)
    {
        if (GetAclInformation(pAclSource, &AclInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
        {
            // compute size for new Acl, based on addition or subtraction of Ace
            dwNewAclSize = AclInfo.AclBytesInUse;
            HRESULT hr;

            for (iSid = 0; iSid < dwSids; iSid++)
            {
                DWORD addend = GetLengthSid(ppSids[iSid]);
                C_ASSERT(sizeof(ACCESS_ALLOWED_ACE) >= sizeof(DWORD));  
                hr = DWordAdd(addend, sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD), &addend);
                if (FAILED(hr))
                {
	                return ERROR_INVALID_DATA;
                }
                hr = DWordAdd(dwNewAclSize, addend, &dwNewAclSize);
                if (FAILED(hr))
                {
	                return ERROR_INVALID_DATA;
                }
            }

            *pAclDestination = reinterpret_cast<PACL>(new BYTE[dwNewAclSize]);
            if (*pAclDestination)
            {
                *pcbDacl = dwNewAclSize;
                // initialize new Acl
                if (InitializeAcl(*pAclDestination, dwNewAclSize, ACL_REVISION))
                {
                    for (iSid = 0; iSid < dwSids; iSid++)
                    {
                        // if appropriate, add ace representing pSid
                        PACCESS_ALLOWED_ACE pNewAce;
                        if (AddAccessAllowedAce(*pAclDestination, ACL_REVISION, AccessMask, ppSids[iSid]))
                        {
                            // get pointer to ace we just added, so we can change the AceFlags
                            if (GetAce(*pAclDestination, iSid, (void**)&pNewAce))
                                pNewAce->Header.AceFlags = AceFlags;
                            else
                            {
                                dwError = GetLastError();
                                break;
                            }
                        }
                        else
                        {
                            dwError = GetLastError();
                            break;
                        }
                   }
                    if (ERROR_SUCCESS == dwError)
                    {
                        // copy existing aces to new Acl
                        for (AceCounter = 0 ; AceCounter < AclInfo.AceCount ; AceCounter++)
                        {
                            // fetch existing ace
                            if (GetAce(pAclSource, AceCounter, (LPVOID*) & pAce))
                            {
                                if (!AddAce(*pAclDestination, ACL_REVISION, MAXDWORD, pAce, ((PACE_HEADER)pAce)->AceSize))
                                {
                                    dwError = GetLastError();
                                    break;
                                }
                            }
                            else
                            {
                                dwError = GetLastError();
                                break;
                            }
                        }
                    }
                }
                else
                {
                    dwError = GetLastError();
                }
            }
            else
            {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else
        {
            dwError = GetLastError();
        }
    }
    else
    {
        *pAclDestination = NULL;
        *pcbDacl = 0;
    }

    if (dwError != ERROR_SUCCESS && *pAclDestination)
    {
        delete *pAclDestination;
        *pAclDestination = NULL;
    }
   
    return dwError;
}

BOOL IsPresentationHostHighIntegrity()
{
    DWORD pid = GetCurrentProcessId();
    return GetProcessIntegrityLevel(pid) >= (int)SECURITY_MANDATORY_HIGH_RID;
}
