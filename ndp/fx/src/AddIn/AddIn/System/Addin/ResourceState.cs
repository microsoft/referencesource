// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ResourceState
**
** Purpose: Stores resource values looked up for precisely 
**     one culture.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Reflection;
using System.Resources;
using System.Text;
using System.Threading;
using System.Diagnostics.Contracts;

namespace System.AddIn
{
    [Serializable]
    internal class ResourceState
    {
#if LOCALIZABLE_ADDIN_ATTRIBUTE  // Disabled in Orcas for scheduling, security & a required fix in red bits
        internal String CultureName;
#endif
        internal String Name;
        internal String Publisher;
        internal String Description;
    }
}
