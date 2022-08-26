/// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Purpose: Pipeline Attributes for the AddIn model
** 
===========================================================*/

using System;

namespace System.AddIn.Pipeline
{
    [AttributeUsage(AttributeTargets.Interface)]
    public sealed class AddInContractAttribute : Attribute
    {
        public AddInContractAttribute() { }
    }
}

