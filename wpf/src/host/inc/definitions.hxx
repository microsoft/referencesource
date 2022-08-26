//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Contains miscellaneous global definitions
//
//  History
//      2005/10/06-Microsoft
//          Created
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once

#define ERROR_NO_COMMAND_SPECIFIED              1
#define ERROR_COULD_NOT_LAUNCH_RESTRICTED       2
#define ERROR_NAVIGATE_FAILED                   3
#define ERROR_INVOKING_BROWSER                  4
#define ERROR_NO_EVENT                          5
#define ERROR_ACTIVATION_EXCEPTION              6
#define ERROR_HOST_CRASHED                      7
#define ERROR_NO_HOST                           8
#define ERROR_NO_HOST_INIT_FAILED               9
#define ERROR_ACTIVATION_ABORTED                30 // used in managed code

#define FACILITY_NPAPI 0x102 // used by Mozilla plugin
#define FACILITY_WPF_HOST 0x103

inline HRESULT WPFHostError2HResult(int error)
{
    return error == NOERROR ? S_OK : (HRESULT)(error | (FACILITY_WPF_HOST << 16) | 0x80000000);
}


