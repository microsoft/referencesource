// ==++==
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
    [AttributeUsage(AttributeTargets.Class)]
    public sealed class HostAdapterAttribute : Attribute
    {
        public HostAdapterAttribute() { }
    }

    [AttributeUsage(AttributeTargets.Class)]
    public sealed class AddInAdapterAttribute : Attribute
    {
        public AddInAdapterAttribute() { }
    }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Interface)]
    public sealed class AddInBaseAttribute : Attribute
    {
        private Type[] _activatableAs;

        // note that in the reflection-only context code here won't execute
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays", Justification="Array size will be small")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Activatable")]
        public Type[] ActivatableAs
        {
            get { return _activatableAs; }
            set { _activatableAs = value; }
        }
    }

}
