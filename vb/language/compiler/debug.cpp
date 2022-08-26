//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  This module contains functions that report debugging errors (typcially
//  assertion failure) to the user in whatever way is best.
//
//  THIS FILE SHOULD ONLY BE IN DEBUGGING VERSIONS!
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#undef DebPrintf
#undef DebDoPrint

//
// ADDING A TEST SWITCH
//
// A test switch is a switch that you can change in the IDE to turn on and off some
// debug behavior. Test switches automatically appear in the debug dialog of the IDE
// under the "VB Compiler" heading. Switches might also appear under the "VB Compiler (Lib)"
// category if the compiler lib was linked into another IDE package.
//
// There are two kinds of switches: regular switches that only live in the debug
// version of the compiler, and switches that live in BOTH debug AND retail. By
// default, you should only create debug-only switches. Only create one that appears
// in both if you absolutely need to debug something in retail.
//
// TO CREATE A REGULAR SWITCH:
//
// 1) Add a VBDEFINE_SWITCH below with the switch name and a description.
// 2) Add a VSEXTERN_SWITCH to debug.h that uses the switch name.
// 3) Read the switch using the VSFSWITCH macro.
//
// TO CREATE A DEBUG AND RETAIL SWITCH:
//
// 1) Add a VBDEFINE_TESTSWITCH below with the switch name and a description.
// 2) Add a VBEXTERN_TESTSWITCH to debug.h that uses the switch name.
// 3) Read/set the switch using the VBFTESTSWITCH macro.
// 4) Add a test shell command to turn the switch on/off in retail. (See existing
//    switches for how this is done.)
//

// VB Wrapper around VSDEFINE_SWITCH, use for ***DEBUG SPECIFIC***
#if IDE
#define VBDEFINE_SWITCH(NAME, DESC)         VSDEFINE_SWITCH(NAME, "VB Compiler", DESC)
#else
#define VBDEFINE_SWITCH(NAME, DESC)         VSDEFINE_SWITCH(NAME, "VB Compiler(Lib)", DESC)
#endif IDE

// Wrapper around VSDEFINE_SWITCH that works in both ***DEBUG AND RETAIL***
// Wrapper for VSFSWITCH and VSEXTERN_SWITCH that works in Debug and Retail
#if ID_TEST
#if DEBUG
#if IDE
#define VBDEFINE_TESTSWITCH(NAME, DESC)     VSDEFINE_SWITCH(NAME, "VB Compiler", DESC)
#else
#define VBDEFINE_TESTSWITCH(NAME, DESC)     VSDEFINE_SWITCH(NAME, "VB Compiler(Lib)", DESC)
#endif // IDE
#else  // DEBUG
#define VBDEFINE_TESTSWITCH(NAME, DESC)     bool g_Switch_ ## NAME
#endif  // DEBUG
#else  // ID_TEST
#define VBDEFINE_TESTSWITCH(NAME, DESC)
#endif // ID_TEST

VBDEFINE_SWITCH(fDebugEE,                   "Debug EE Package");
VBDEFINE_SWITCH(fDebugEEFields,             "Debug EE IDebugFields");
VBDEFINE_SWITCH(fDumpBoundEETrees,          "Dump bound EE Trees");
VBDEFINE_SWITCH(fDumpCallGraph,             "Dump Call Graph");
VBDEFINE_SWITCH(fDumpDeclTrees,             "Dump declaration trees");
VBDEFINE_SWITCH(fDumpDocCommentString,      "Dump Doc Comment Signature");
VBDEFINE_SWITCH(fDumpLineTable,             "Dump Line Table");
VBDEFINE_SWITCH(fDumpSyntheticCode,         "Dump Synthetic code" );
VBDEFINE_SWITCH(fTraceCodeElementsExt,      "External CodeModel:  CodeElements - Trace interface calls");
VBDEFINE_SWITCH(fTraceCodeElementsInt,      "CodeModel:  CodeElements - Trace interface calls");
VBDEFINE_SWITCH(fTraceCodeElementsSpit,     "CodeModel:  Code Spit- Trace interface calls");
VBDEFINE_SWITCH(fTraceCodeModelEvents,      "CodeModelEvents - Trace element changes");
VBDEFINE_SWITCH(fTraceCodeTypeRef,          "CodeModel:  CodeTypeRef - Trace interface calls");
VBDEFINE_SWITCH(fTraceCodeModelEventsExt,   "External CodeModelEvents - Trace element changes");
VBDEFINE_SWITCH(fTraceCodeModelInt,         "CodeModel:  CodeModel - Trace interface calls");
VBDEFINE_SWITCH(fTraceCollectionsInt,       "CodeModel:  Collections - Trace interface calls");
VBDEFINE_SWITCH(fTraceFileCodeModelInt,     "CodeModel:  FileCodeModel - Trace interface calls");
VBDEFINE_SWITCH(fDumpBoundMethodTrees,      "Dump bound method body trees");
VBDEFINE_SWITCH(fDumpUnboundMethodTrees,    "Dump unbound method body trees");
VBDEFINE_SWITCH(fDumpClosures,              "Dump Closure Information");
VBDEFINE_SWITCH(fDumpInference,             "Dump Type Parameter Inference Information");
VBDEFINE_SWITCH(fDumpOverload,              "Dump overload resolution information");
VBDEFINE_SWITCH(fDumpRelaxedDel,            "Dump Relaxed Delegate Information");
VBDEFINE_SWITCH(fDumpText,                  "Dump the text of the modules of the project");
VBDEFINE_SWITCH(fDumpStateChanges,          "Dump all state transitions");
VBDEFINE_SWITCH(fDebugIntellidocCache,      "Turn on Intellidoc cache debugging");
VBDEFINE_SWITCH(fDumpSymbols,               "Dump generated symbols");
VBDEFINE_SWITCH(fDumpNonVBSymbols,          "Dump symbols for non-vb files");
VBDEFINE_SWITCH(fDumpBasicRep,              "Dump Basic representation");
VBDEFINE_SWITCH(fTraceReturn,               "Trace FAILED(hr) returns");
VBDEFINE_SWITCH(fDebugEmptyEdit,            "Turn on empty edit debugging (EditClassify)");
VBDEFINE_SWITCH(fDebugEditClassify,         "Debug EditClassify");
VBDEFINE_SWITCH(fDebugEditFilter,           "Debug EditFilter");
VBDEFINE_SWITCH(fDebugEnC,                  "Debug EditAndContinue");
VBDEFINE_SWITCH(fEnableIntellisenseBuilders, "Enable Intellisense builders");
VBDEFINE_SWITCH(fDebugNewSpanTracking,      "Turn on new span tracking debugging (EditFilterCache)");
VBDEFINE_SWITCH(fDebugOldSpanTracking,      "Turn on old span tracking debugging (EditFilterCache)");
VBDEFINE_SWITCH(fDumpPrettyListMinDistanceStats, "Turn on DumpPrettyListMinDistance Statistics");
VBDEFINE_SWITCH(fDumpPrettyListMinDistanceArray, "Turn on DumpPrettyListMinDistance Array");
VBDEFINE_SWITCH(fDebugSourceFileView,       "Turn on SourceFileView debugging");
VBDEFINE_SWITCH(fDebugTaskList,             "Turn on TaskList Update debugging");
VBDEFINE_SWITCH(fDebugSmartTagHover,        "Turn on smart tag hover debugging");
VBDEFINE_SWITCH(fDebugLocks,                "Turn on Lock Debugging");
VBDEFINE_SWITCH(fTraceHiddenText,           "Trace the VB hidden text manager");
VBDEFINE_SWITCH(fTraceHiddenTextEvents,     "Trace the VB hidden text manager received events");
VBDEFINE_SWITCH(fDebugXMLDocParser,         "Dump XML Doc parse trees");
VBDEFINE_SWITCH(fDumpReferenceUsage,        "Dump reference usage information during compilation.");
VBDEFINE_SWITCH(fDumpImportUsage,           "Dump import usage information during compilation.");
VBDEFINE_SWITCH(fDumpReferencesForMetaData, "Dump metadata->metadata project references.");
VBDEFINE_SWITCH(fDumpXML,                   "Dumps the XML form of all files.");
VBDEFINE_SWITCH(fDumpFlow,                  "Dump flow semantics.");
VBDEFINE_SWITCH(fKeepECWindowVisibleOnFocusLoss, "Keep the error correction window visible on focus loss");
VBDEFINE_SWITCH(fDebugFindSymbolReference, "Turn on find symbol reference debugging");
VBDEFINE_SWITCH(fTestCrash, "Test Watson handling");
VBDEFINE_SWITCH(fDebugNoMy, "Compiling without My extensions");
VBDEFINE_SWITCH(fENCDumpMethodLocations, "Dump ENC Method Locations");
VBDEFINE_SWITCH(fENCShowHiddenMarkers, "Reveal ENC Hidden Markers");
VBDEFINE_SWITCH(fENCDumpLineMap, "Dump ENC Line Map");
VBDEFINE_SWITCH(fENCDumpStatements, "Dump ENC Statements");
VBDEFINE_SWITCH(fENCDumpDeltas, "Dump ENC Deltas");
VBDEFINE_SWITCH(fENCDumpMetaTokens, "Dump ENC Builder Meta Tokens");
VBDEFINE_SWITCH(fEEShowHiddenLocals, "Show hidden locals in the EE");
VBDEFINE_SWITCH(fExceptionNoRethrowNoWatson, "Bypass Exception Rethrow and Watson when Debugger is Present");
VBDEFINE_SWITCH(fDebugIDEExtensionMethodCacheTestHook, "Debug the test hook for the IDE extension method cache");
VBDEFINE_SWITCH(fENCDebugTransientCaching, "Turn on ENC transient cache debugging");
VBDEFINE_TESTSWITCH(fDumpDecompiles,                "Dump the names of files as they're decompiled.");
VBDEFINE_TESTSWITCH(fDumpDecompilesForSuites,   "Dump the names of files as they're decompiled (intended for suite use only)");
VBDEFINE_TESTSWITCH(fDumpFileChangeNotifications, "Dump file change notifications.");
VBDEFINE_TESTSWITCH(fIDEDumpXML,                    "Dumps CodeModel XML to file");
VBDEFINE_TESTSWITCH(fDebugLineMark,                 "Turn on LineMark Debugging");
VBDEFINE_TESTSWITCH(fDebugEditCacheInOnReplace,     "Turn on edit cache debugging in SourceFileView::OnReplace");
VBDEFINE_TESTSWITCH(fMyGroupAndDefaultInst,         "Turn on MyGroup Collections and Default Instace debug and tracing");
VBDEFINE_TESTSWITCH(fDumpSnippetMarkerInfo,         "Dump Snippet Marker information as it changes");
VBDEFINE_SWITCH(fDumpIntellisenseDropdown, "Dump intellisense dropdown contents");
VBDEFINE_SWITCH(fDumpIntellisenseEvents, "Dump Intellisense events");
VBDEFINE_SWITCH(fShowSnippetBounds, "Show snippet bounds for newly created snippets");
VBDEFINE_SWITCH(fThreading, "Show Fore & backgd thread timing/events");
VBDEFINE_SWITCH(fCompCaches, "Dump compilation caches stats");

#if ID_TEST

//////////////////////////////////////////////////////////////////////////////
//
// Threadsafe DebPrintf.
//
//////////////////////////////////////////////////////////////////////////////

#if !IDE

void DebDoPrint(_In_z_ char * sz)
{
    printf("%s", sz);
}

void __cdecl DebPrintf(
    const char * szFmt,
    ...)
{
    va_list vaArgPtr;
    va_start(vaArgPtr, szFmt);

    char szBuffer[1024];

    // NULL Terminate the string if it overflows.
    if (_vsnprintf_s(szBuffer,
                   (sizeof(szBuffer)-1),
                   _TRUNCATE,
                   szFmt,
                   vaArgPtr) == (-1)) {

      szBuffer[sizeof(szBuffer)-1]='\0';
    }

    // Print it.
    DebDoPrint(szBuffer);

    va_end(vaArgPtr);
}

void __cdecl DebPrintf(
    const WCHAR * format,
    ...)
{
    va_list argList;
    va_start(argList, format);
    WCHAR buffer[1024];

    StringCchVPrintf(buffer, _countof(buffer), format, argList);
    wprintf(L"%s",buffer);
    va_end(argList);
}

void __cdecl DebVPrintf(
    const WCHAR * format,
    va_list argList)
{
    ThrowIfNull(format);
    WCHAR buffer[1024];
    StringCchVPrintf(buffer, _countof(buffer), format, argList);
    wprintf(L"%s", buffer);
}

void __cdecl DebTShellPrintf(
    const char * szFmt,
    ...)
{
    va_list vaArgPtr;
    va_start(vaArgPtr, szFmt);

    char szBuffer[1024];

    // NULL Terminate the string if it overflows.
    if (_vsnprintf_s(szBuffer,
                   (sizeof(szBuffer)-1),
                   _TRUNCATE,
                   szFmt,
                   vaArgPtr) == (-1)) {

      szBuffer[sizeof(szBuffer)-1]='\0';
    }

    // Print it.
    DebDoPrint(szBuffer);

    va_end(vaArgPtr);
}

#else IDE

static IVsTshell *s_ptshell;

HRESULT DebInit(IVsTshell *ptshell)
{
    s_ptshell = ptshell;

    if (ptshell)
    {
        ptshell->AddRef();
    }

    return NOERROR;
}

void DebTerm()
{
    RELEASE(s_ptshell);
}

void DebDoPrint(_In_z_ char *sz)
{
    if (GetCompilerSharedState()->IsInMainThread())
    {
        if (s_ptshell)
        {
            s_ptshell->DebOutputStringA(sz);
        }
        else
        {
            printf("%s", sz);
        }

    }
    else
    {
        VSFAIL(L"Background thread should not be outputting to TShell");
    }
}

// DebTShellPrintf will output to the Test log for TShell AND the Output Debug window
void __cdecl DebTShellPrintf(
    const char * szFmt,
    ...)
{
    va_list vaArgPtr;
    va_start(vaArgPtr, szFmt);

    char szBuffer[1024];

    // NULL Terminate the string if it overflows.
    if (FAILED(StringCchVPrintfA(
                        szBuffer,
                        DIM(szBuffer),
                        szFmt,
                        vaArgPtr)))
    {
        VSFAIL("String operation overflow");
        szBuffer[0]='\0';
    }

    // Print it.
    DebDoPrint(szBuffer);

    va_end(vaArgPtr);
}

// DebPrintf only supports our Test Switches
// This will not output to the test log when run under TShell.
void __cdecl DebPrintf(
    const char * szFmt,
    ...)
{
    va_list vaArgPtr;
    va_start(vaArgPtr, szFmt);

    char szBuffer[1024];

    // NULL Terminate the string if it overflows.
    if (FAILED(StringCchVPrintfA(
                        szBuffer,
                        DIM(szBuffer),
                        szFmt,
                        vaArgPtr)))
    {
        VSFAIL("String operation overflow");
        szBuffer[0]='\0';
    }

    // Print it.
    OutputDebugStringA(szBuffer);

    va_end(vaArgPtr);
}

void __cdecl DebPrintf(
    const WCHAR * format,
    ...)
{
    va_list argList;
    va_start(argList, format);
    DebVPrintf(format, argList);
    va_end(argList);
}

void __cdecl DebVPrintf(
    const WCHAR * format,
    va_list argList)
{
    WCHAR buffer[1024];
    StringCchVPrintf(buffer, _countof(buffer), format, argList);
    OutputDebugStringW(buffer);
}

#endif IDE

#endif // ID_TEST

//////////////////////////////////////////////////////////////////////////////

#if DEBUG

// Ignore everything below if DEBUG not defined

// This is used by the IfFailRet and other macros - do not use directly.
HRESULT DebTraceReturn(
    HRESULT hr,
    _In_z_ const WCHAR* szFile,
    int iLine)
{

  if (FAILED(hr)) {

    if(VSFSWITCH(fTraceReturn))
    {
      WCHAR szMessageError[128];
      szMessageError[0] = L'\0';
      BOOL fMessage;

      // Get the message from the system
      fMessage = FormatMessage(FORMAT_MESSAGE_MAX_WIDTH_MASK
                                | FORMAT_MESSAGE_FROM_SYSTEM,
                               NULL, hr,
                               MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),
                               szMessageError, sizeof(szMessageError), NULL);

      // Erps didn't get a message.
      if(!fMessage)
      {
        if (FAILED(StringCchCopy(szMessageError, DIM(szMessageError), L"Unknown Hresult")))
        {
            VSFAIL("String copy operation failed");
            szMessageError[0] = L'\0';
        }
      }

      // Output the information that we want.
      DebPrintf(L"FAILED RETURN: %s(%d) : 0x%08lx, %s\n",
                  szFile, iLine, hr, szMessageError);
    }
  }

  return hr;
}

void DebCheckForResourceLeaks()
{
#if IDE
    // By the time we unload, all users of IVbCompiler and IVbCompilerProject
    // must have released all of their references.  If they haven't, then
    // we'll still have a CompilerPackage and g_pCompilerPackage will be non-NULL.
    // 
    VSASSERT(GetCompilerPackage() == NULL,
        "msvbide.dll is being unloaded but there are still references "
        "to the VB compiler (probably via IVbCompiler or IVbCompilerProject)");
#endif
}


bool DebVPAssert()
{
    static int ShowVPAssert = 2; // 0=no, 1=yes, 2=unknown. It only ever transitions, once, from 2->0 or 1.
    if (ShowVPAssert == 2)
    {
        GetEnvironmentVariableW(L"VBC_COMPILER_DISABLE_VPASSERT", NULL, 0);
        ShowVPAssert = (::GetLastError() == ERROR_ENVVAR_NOT_FOUND) ? 1 : 0;
        // Note: concurrency isn't a problem here. That's because reads and writes of an int
        // are themselves atomic. And even if two threads end up racing to read or write
        // the value of ShowVPAssert, well, it doesn't actually matter if both of them retrieve
        // the environment variable simultaneously.
    }
    return (ShowVPAssert!=0);
}

#endif   // DEBUG
