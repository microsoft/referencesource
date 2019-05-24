//------------------------------------------------------------------------------
// <copyright file="SafeServiceHandle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Security;
using Microsoft.Win32.SafeHandles;
using System.Runtime.InteropServices;
using System.Runtime.ConstrainedExecution;

internal class SafeServiceHandle : SafeHandleZeroOrMinusOneIsInvalid {

    internal SafeServiceHandle(IntPtr handle, bool ownsHandle) : base(ownsHandle) {
        SetHandle(handle);
    }

    override protected bool ReleaseHandle() {
        return System.ServiceProcess.SafeNativeMethods.CloseServiceHandle(handle);
    }

}

