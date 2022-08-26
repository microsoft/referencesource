// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  AddInSegmentType
**
===========================================================*/
using System;
using System.Diagnostics.Contracts;

namespace System.AddIn.Hosting
{
    public enum AddInSegmentType
    {
        HostViewOfAddIn,
        HostSideAdapter,
        Contract,
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId = "InSide")]
        AddInSideAdapter,
        AddInView,
        AddIn
    }
}

