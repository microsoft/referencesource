//-----------------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Shared Library initialization functions 
//
//-----------------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//
// Gets the path to the NDP SDK files
//
BSTR GetNDPSystemPath()
{
    BSTR bstrName = NULL;
#ifndef FEATURE_CORESYSTEM
    DWORD cchName;
    LPWSTR wszTemp = NULL;

    LegacyActivationShim::GetCORSystemDirectory(NULL, 0, &cchName);

    if (cchName + 1 > cchName &&
        VBMath::TryMultiply((cchName + 1), sizeof(WCHAR)))
    {
        wszTemp = (LPWSTR)VSAlloc((cchName + 1) * sizeof(WCHAR));

        if (wszTemp)
        {
            if (LegacyActivationShim::GetCORSystemDirectory(wszTemp, cchName + 1, &cchName) == S_OK)
            {
                bstrName = ::SysAllocString(wszTemp);
            }
            VSFree(wszTemp);
        }
    }
#else
    bstrName = ::SysAllocString(CORESYS_SYSTEM_DIR);
#endif
    return bstrName;
}

bool VBInitCompilerLibrary(
    _In_opt_z_ const char* heapName,
    _In_ DWORD heapFlags )
{
    if ( !heapName )
    {
        heapName = "VB Compiler Library";
    }
#if DEBUG || BUILDING_VBC
    g_vbCommonHeap = VBHeapCreateNamed(heapFlags, heapName);
#else
    g_vbCommonHeap = GetProcessHeap();// for retail, our heap should use ProcessHeap (#910120)
#endif DEBUG
    if ( g_vbCommonHeap )
    {
        g_pvbNorlsManager = new (zeromemory) NorlsAllocatorManager();
    }

    return g_vbCommonHeap && g_pvbNorlsManager;
}

void VBDestroyCompilerLibrary()
{
    if ( g_pvbNorlsManager )
    {
        delete g_pvbNorlsManager;
    }

    // If the initial heap creation fails, we will immediately be calleod with 
    // DLL_PROCESS_DETATCH and our heap will be invalid
    if ( g_vbCommonHeap && g_vbCommonHeap != GetProcessHeap())
    {
        VBHeapDestroy(g_vbCommonHeap);
    }

    g_pvbNorlsManager = NULL;
    g_vbCommonHeap = NULL;
}

