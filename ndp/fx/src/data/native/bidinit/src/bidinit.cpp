//****************************************************************************
//              Copyright (c) 1988-2003 Microsoft Corporation.
//
// @File:  bidinit.cpp
// @Owner: Microsoft
// @test:
//
// <owner current="true" primary="true">Microsoft</owner>
//
// Purpose:
//   BID initialization code for SNIX
//
// Notes:
//
// @EndHeader@
//****************************************************************************

// keep this include - bidinit.lib is built as UNICODE, while SNIX as ASCII
#include "bidinitpch.hpp"

#ifndef  _BID_DECLARED
  #error "BidApi.h" must be included
#endif

#define _BID_IDENTITY_A     "System.Data.SNI.1"
#define _BID_SYNC_LOAD

#include "BidApi_ldr.h"

//
//  Explicit initialization
//
static volatile LONG _bidIniCount = 0;

extern "C" _bidAPI_EXPORT void WINAPI DllBidInitialize()
{
    if( InterlockedIncrement( &_bidIniCount ) == 1 )
    {
        BidSelectAndLoad();
    }
}

extern "C" _bidAPI_EXPORT void WINAPI DllBidFinalize()
{
    if( InterlockedDecrement( &_bidIniCount ) <= 0 )
    {
        _bidIniCount = 0;

        //
        //  Disabled to reflect the fact that native part of the assembly stays loaded
        //  for the lifetime of Win32 process, no matter how many times managed appdomains
        //  get loaded / unloaded.
        //
       #if 0
        BidUnload();
       #endif
    }
}

