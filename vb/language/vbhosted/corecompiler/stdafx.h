//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Precompiled header for the Visual Basic Hosted Compiler
//
//-------------------------------------------------------------------------------------------------

#pragma once

class CompilerPackage;
extern CompilerPackage *g_pCompilerPackage;

#include "StdAfx.Shared.h"

#include <exception>

//-------------------------------------------------------------------------------------------------
//
// Additional external includes.
// public\vc\inc
//

// Windows and ATL
#include <limits.h>
#include <float.h>
#include <ddeml.h>
#include <math.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <time.h>

//-------------------------------------------------------------------------------------------------
//
// public\vc\inc
// public\sdk\inc
//
#include <mlang.h>
#include <msxml2.h>
#include <richedit.h>
#include <fcntl.h>
#include <psapi.h>
#include <servprov.h>
#include <shellapi.h>
#include <io.h>
#include <shlobj.h>
#include <tlhelp32.h>
#include <time.h>
#include <wincrypt.h>


//-------------------------------------------------------------------------------------------------
//
// public\devdiv\inc
//
#include <cor.h>
#include <corsym.h>
#if DEBUG
#include <dbghelp.h>
#endif // DEBUG
#include <fusion.h>
#include <iceefilegen.h>
#include <mscoree.h>
#include <strongname.h>
#include <LegacyActivationShim.h>
//-------------------------------------------------------------------------------------------------
//
// public\internal\VSCommon\inc
//
#include <alink.h>
#include <CodeMarkers.h>
#include <completion.h>
#include <compsvcspkg80.h>
#include <containedlanguage.h>
#include <containedlanguageprivate.h>
#include <crypthash.h>
#if DEBUG
#include <dbghlper.h>
#endif //DEBUG
#include <encoding.h>
#include <fontsandcolors.h>
#include <helpsys.h>
#include <multi.h>
#include <msodw.h>
#include <objext.h>
#include <olecm.h>
#include <safeint.h>
#include <servexp.h>
#include <singlefileeditor.h>
#include <singlefileeditor90.h>
#include <stdidcmd.h>
#include <textfind.h>
#include <TextManagerInterfaces.h>
#include <textspan.h>
#include <TxFmt.h>
#include <txfmt.h>
#include <uilocale.h>
#include <unilib.h>
#include <vscocreate.h>
#include <vshelp.h>
#include <vslog.h>
#include <vsmanaged.h>
#include <vsmsxml.h>
#include <vspublishframework.h>
#include <VSRegKeynames.h>
#include <VSShellInterfaces.h>

// Munge Expression, Property and Debugger types from DTE because they conflit with our types.
#define Expression _DTEExpression
#define Property _DTEProperty
#define Debugger _DTEDebugger
#include <dte.h>
#include <dte80.h>
#include <vslangproj.h>
#include <vslangproj2.h>
#include <vslangproj80.h>
#undef Expression
#undef Property
#undef Debugger

//-------------------------------------------------------------------------------------------------
//
// public\internal\VCTools\include
// public\internal\env\inc
//
#include <omglyphs.h>

//-------------------------------------------------------------------------------------------------
//
// public\internal\vsproject\inc
//
//#include <langproject.h>
//#include <util_watson.h>
#include <subtypes.h>

//-------------------------------------------------------------------------------------------------
//
// public\internal\vb\inc
//
#include <vbidl.h>
#include <VBCodeModelExtensibility.h>

//-------------------------------------------------------------------------------------------------
//
// Compiler dependencies.
// public\onternal\Venus\inc
//
#include <Webapplicationctx.h>
#include <WebApplicationProject.h>
#include <WebProperties.h>

class StringBuffer;
//-------------------------------------------------------------------------------------------------
//
// public\internal\Debugger\inc
//
#include <ee.h>
#include <ee90.h>
#include <msdbg.h>
#include <msdbg90.h>
#include "CompilerFacade.h"
#include <sh.h>
#include "..\Compiler\Logging.h"
#include "..\Compiler\Resources.h"
#include <VsDbgCmd.h>
#include <vsdebugguids.h>

//-------------------------------------------------------------------------------------------------
//
// Command-line compiler includes.
// public\internal\CSharp\inc
//


//-------------------------------------------------------------------------------------------------
//
// Other declarations.
// Visual Basic VS IDE includes.
//
// Note that currently some part of #include-s is in SharedLib.h because compiler sources
// depend on those. They should eventually move here and compiler dependency removed.
//

typedef const BYTE *LPCBYTE;
#include "CompilerFacade.h"

HRESULT __stdcall DelayLoadUICallback(HINSTANCE *phinstance);

int LoadUIString(
    UINT uID,
    _Out_bytecap_(nBufferMax)LPWSTR lpBuffer,
    int nBufferMax);
// This is the bad one - includes whole compiler in here...
#include "SharedLib.h"

int __cdecl PrintMessage(
    _In_z_ const WCHAR * format, 
    ...);
#include "VsPackageFacade.h"

int _cdecl PrintMessage(
    unsigned id, 
    ...);
struct MFUMemberInfo;
class  ENCProcessor;
class  CProcLocalSymbolLookup;
struct TextEx;
class  ParseTreeContext;
class  ParseTreeService;
struct EditFilterCache;
class  CVBViewFilterBase;

#include "..\VsPackage\IdeNorlsAllocatorManager.h"

#include "..\VsPackage\VBComModule.h"
extern VBComModule _Module;

#undef VSAlloc
#undef VSAllocZero
#undef VSRealloc
#undef VSFree
#undef VSSize

#ifndef DEBUG 

#define VSAlloc(cb)       LocalAlloc(LMEM_FIXED,cb)
#define VSAllocZero(cb)   LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, cb)
#define VSRealloc(pv, cb) LocalReAlloc(pv, cb, 0)
#define VSFree(pv)        LocalFree(pv)
#define VSSize(pv)        LocalSize(pv)

#else // DEBUG

#pragma prefast(push)
#pragma prefast(suppress: 22023, "Debug only")

extern size_t memCur, memMax;

inline void * VSAlloc(size_t cb)
{
    memCur += cb;
    if (memCur > memMax)
    {
        memMax = memCur;
    }
    return VsDebAlloc(0, cb);
}

inline void * VSAllocZero(size_t cb)
{
    memCur += cb;
    if (memCur > memMax)
    {
        memMax = memCur;
    }
    return VsDebAlloc(HEAP_ZERO_MEMORY, cb);
}

inline void * VSRealloc(void *pv, size_t cb)
{
    memCur += cb - VsDebSize(pv);
    if (memCur > memMax)
    {
        memMax = memCur;
    }
    return VsDebRealloc(pv, 0, cb);
}
#pragma prefast(pop)

inline void VSFree(void *pv)
{
    memCur -= VsDebSize(pv);
    VsDebFree(pv);
}

#define VSSize(pv)        VsDebSize(pv)

#endif //DEBUG
