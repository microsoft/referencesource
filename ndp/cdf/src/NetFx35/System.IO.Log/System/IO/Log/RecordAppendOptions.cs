//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;

    [Flags]
    public enum RecordAppendOptions
    {
        None        = 0x00,
        ForceAppend = 0x01,
        ForceFlush  = 0x02,
    }
}
