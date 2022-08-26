//------------------------------------------------------------------------------
// <copyright file="IPersist.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging.Interop
{
    using System;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.InteropServices;

    [ComImport(),
    Guid("0000010C-0000-0000-C000-000000000046"),
    InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IPersist
    {
        [SuppressUnmanagedCodeSecurity()]
        void GetClassID([Out] out Guid pClassID);
    }
}
