//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     The precompiled header for PresentationHost.
//
//  Note:
//      To maximize the PCH benefit, include here all external (SDK) headers and the commonly used but 
//      infrequently changed shared ones. Avoid including headers from this project. Each .cxx file should
//      directly include the headers from this project it needs. This prevents the need for PCH recompilation
//      when headers are changed and makes dependencies between classes more apparent (which makes you think
//      twice when introducing a new one).
//
//  History:
//     2002/06/12-murrayw
//          Created
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once

#ifndef _UNICODE
#pragma error
#endif

// Include initguid.h first so that guiddef.h makes GUID/IID definitions available in header files 
// (with __declspec(selectany)).
#include <wpfsdl.h>
#include <initguid.h>
#include <windows.h> 
#include <cguid.h>
#include <intsafe.h>
#include <tchar.h>
#include <strsafe.h>
#include <wininet.h>
#include <urlmon.h>
#include <hlink.h>
#include <mshtml.h>
#include <shlobj.h>
#include <shellapi.h>
#include <msxml6.h>
#include <comdef.h>
#include <muiload.h>
#include <windowsx.h>

// We don't want to bring in the prototypes for the deprecated shlwapi string 
// functions since they will cause warnings which are treated as errors. We 
// don't want to use them and should not use them anyway, so excluding them 
// is no loss.
#define NO_SHLWAPI_STRFCNS
#include <shlwapi.h>
#include <perhist.h>
#include <webevnts.h>
#include <shlguid.h>
#include <cor.h>
// Explicitly request wrapper for CoEEShutDownCOM in LegacyActivationShim.h
#define LEGACY_ACTIVATION_SHIM_DEFINE_CoEEShutDownCOM
#include <LegacyActivationShim.h>
#include <mshtmdid.h>
#include <exdispid.h>
#include <Tlhelp32.h>
#include <process.h>
#include <lmcons.h>
#include <Msdrmdefs.h>

#include <Crtdbg.h>
#define ASSERT _ASSERTE
#define Assert ASSERT 

#include "Resource.hxx"
#include "HostServices.h" // generated form .idl
#include "..\inc\ATL.hxx"
#include "..\inc\guids.hxx"
#include "..\inc\definitions.hxx"
#include "..\Shared\String.hxx"
#include "..\Shared\SharedUtilities.hxx"
#include "..\Shared\WatsonReportingShared.hxx"

// from WPF's shared inc folder
#include "BuildInfo.hxx"
#include "MiscMacros.hxx"
#include "WPFEventTrace.h"

// Forward declaration
class CHostShim;
class CVersion;
class COleDocument;

#define ASSERT_UNUSED Assert("This method was marked as unused. Please contact actapp with the call stack.")
#define STD_ASSERT_UNUSED ASSERT_UNUSED; return E_NOTIMPL;

