//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    public enum LogExtentState
    {
        Unknown = 0x0,
        Initializing = 0x1,
        Inactive = 0x2,
        Active = 0x4,
        ActivePendingDelete = 0x8,
        PendingArchive = 0x10,
        PendingArchiveAndDelete = 0x20
    }
}
