// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Enum:  AddInSecurityLevel
**
** Purpose: Describes the trust level granted to the add-in's
**        AppDomain.
**
===========================================================*/
using System;

namespace System.AddIn.Hosting
{
    [Serializable]
    // Not (currently) designed to be bitwise OR'ed together.
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="InSecurity")]
    public enum AddInSecurityLevel
    {
        Internet = 0,
        Intranet = 1,
        FullTrust = 2,
        Host = 3
    }
}
