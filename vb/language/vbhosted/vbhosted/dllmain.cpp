//-----------------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements the main entry point of the VB Hosted Compiler.
//
//-----------------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#include <initguid.h>
#include <vsdebugguids.h>
#include "legacyactivationshim.h"

#define TMH_FILENAME "dllmain.tmh"
#include "EnableLogging.def"

extern HINSTANCE g_hinstDll;

#ifdef DEBUG
size_t memMax = 0, memCur = 0;                // VSAlloc mem tracking
#endif

VBComModule _Module;

// The one and only CompilerPackage.
CompilerPackage *g_pCompilerPackage = NULL;

VBComModule::VBComModule()
{
    // Create the heap
    g_vbCommonHeap = VBHeapCreate(HEAP_GENERATE_EXCEPTIONS);
    if ( g_vbCommonHeap != INVALID_HANDLE_VALUE )
    {
        g_pvbNorlsManager = new NorlsAllocatorManager();
    }
}

VBComModule::~VBComModule()
{
    if ( g_pvbNorlsManager )
    {
        delete g_pvbNorlsManager;
    }

    // If the initial heap creation fails, we will immediately be calleod with 
    // DLL_PROCESS_DETATCH and our heap will be invalid
    if ( g_vbCommonHeap )
    {
        VBHeapDestroy(g_vbCommonHeap);
    }
}


// DLL Entry Point
extern "C"
BOOL WINAPI DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
#if DEBUG
        // Make it so we don't have to recompile vbhosted every time we need to debug a suite
        if ( 0 != GetEnvironmentVariable(L"DEBUG_VBHOSTED", NULL, 0) )
        {
            VSFAIL("Attach");
        }
#endif

        if ( g_vbCommonHeap == INVALID_HANDLE_VALUE || !g_pvbNorlsManager) 
        {
            return FALSE;
        }

        g_hinstDll = hInstance;
		// Disabled DLL_THREAD_ATTACH/DETACH notifications
        DisableThreadLibraryCalls(hInstance);   // ignore return error
        
        // Pre-bind the LegacyActivationShim. This is required because the default
        // behaviour of the LegacyActivationShim is to use the binding context of
        // the process, and since this is hosted we can't trust that it will bind
        // to v4. Should only be possible to fail if the DLL finds itself installed
        // on a machine that does not have v4 installed (which shouldn't happen).
        if (FAILED(LegacyActivationShim::Util::BindToV4()))
            return FALSE;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {

        // msvb7.dll is being unloaded -- see if there are any known
        // compiler resource leaks
        DebCheckForResourceLeaks();

        _Module.Term(); //. no return error.

    }

    return TRUE;    // ok
}

// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow()
{
    return (_Module.GetLockCount()==0 && (g_pCompilerPackage == NULL)) ? S_OK : S_FALSE;
}

// Creates an object of the specified CLSID and retrieves an interface pointer to this object
STDAPI DllGetClassObject(
    REFCLSID rclsid,   // [in] The CLSID of the object to be created.
    REFIID riid,       // [in] The IID of the requested interface.
    LPVOID* ppv)        // [out] A pointer to the interface pointer identified by riid.
                        //       If the object does not support this interface, ppv is set to NULL.
{
    HRESULT hr = NOERROR;

    _Module.EnterStaticDataCriticalSection();


    hr = _Module.GetClassObject(rclsid, riid, ppv);

    //Don't currently know if CompilerPackage should be creatable from external. Leaving #IF'd out.
#if IDE
    // The compilerpackage doesn't use ATL.
    if (FAILED(hr) && rclsid == CLSID_VisualBasicCompiler)
    {
        if (!g_pCompilerPackage)
        {
            g_pCompilerPackage = new (zeromemory) CompilerPackage;
        }

        hr = g_pCompilerPackage->QueryInterface(riid, ppv);
    }
#endif

    _Module.LeaveStaticDataCriticalSection();

    return hr;
}

// Adds entries to the system registry.
STDAPI DllRegisterServer()
{
    HRESULT hr = _Module.RegisterServer(TRUE);
    return hr;
}

// Removes entries from the system registry.
STDAPI DllUnregisterServer()
{
    _Module.UnregisterServer();
    return S_OK;
}
