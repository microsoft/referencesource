//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Master include file for the command-line compiler.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#pragma prefast(suppress: 28718 28722, "The interface is actually SAL annotated")
interface IVbCompiler;
interface IVbCompilerHost;
interface IVbCompilerHost2;

// The required callback that, when called, with load the appropriate resource DLL which contains the compile-error strings.
typedef HRESULT __stdcall LoadUICallback(HINSTANCE * phinstance);

bool VBInitCompilerLibrary(
    _In_opt_z_ const char* heapName,
    _In_ DWORD heapFlags = 0); 
void VBDestroyCompilerLibrary();

class VBCompilerLibraryManager
{
public:

    VBCompilerLibraryManager(
        _In_z_ const char* heapName,
        _In_ DWORD heapFlags = 0)
    {
        m_isValid = VBInitCompilerLibrary(heapName, heapFlags);
    }
    ~VBCompilerLibraryManager()
    {
        if ( m_isValid )
        {
            VBDestroyCompilerLibrary();
        }
    }
    bool IsValid() const { return m_isValid; }

private:
    VBCompilerLibraryManager( const VBCompilerLibraryManager& );
    VBCompilerLibraryManager& operator=( const VBCompilerLibraryManager& );
    bool m_isValid;
};

// Defines the main entrypoint.
STDAPI VBCreateBasicCompiler(
    BOOL CompilingTheVBRuntimeLibrary,
    LoadUICallback * pfnLoadUI,
    IVbCompilerHost * pVbCompilerHost,
    IVbCompiler ** ppCompiler);

//==============================================================================
// Logging options for the compiler.  Common to both the command line and the
// IDE
//
// Set through the IVbCompiler2 interface
//==============================================================================
enum VbCompilerLoggingOptions
{
    None = 0,
};

enum PlatformKinds
{
    Platform_Agnostic = 0,
    Platform_X86,
    Platform_IA64,
    Platform_AMD64,
    Platform_ARM,
    Platform_Anycpu32bitpreferred,

    Platform_Max = Platform_Anycpu32bitpreferred,
    Platform_Invalid
};

PlatformKinds MapStringToPlatformKind(LPCWSTR wszPlatformKind);

bool IsValidPlatformKind(unsigned Type);

bool IsValidPlatformKind(LPCWSTR wszType);

BSTR GetNDPSystemPath();

