//------------------------------------------------------------------------------
// <copyright file="MessageLookupAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1027:MarkEnumsWithFlags")]
    public enum MessageLookupAction
    {

        Current = 0x0,

        Next = 0x1,

        Previous = 0x2,

        First = 0x4,

        Last = 0x8,

        //ReceiveAllowPeek - TBD
    }
}

