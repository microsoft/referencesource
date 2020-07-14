//------------------------------------------------------------------------------
// <copyright file="IPersistStream.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging.Interop {
    using System;   
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.InteropServices;    
    
    [ComImport(), 
    Guid("00000109-0000-0000-C000-000000000046"), 
    InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IPersistStream {        
        [SuppressUnmanagedCodeSecurity()]
        void GetClassID([Out] out Guid pClassID);

        [SuppressUnmanagedCodeSecurity()]
        int IsDirty();

        [SuppressUnmanagedCodeSecurity()]
        void Load([In, MarshalAs(UnmanagedType.Interface)] IStream pstm);

        [SuppressUnmanagedCodeSecurity()]
        void Save([In, MarshalAs(UnmanagedType.Interface)] IStream pstm,
                  [In, MarshalAs(UnmanagedType.Bool)] bool fClearDirty);

        [SuppressUnmanagedCodeSecurity()] 
        long GetSizeMax();
    }
}
