// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  AddInProcess
**
** Purpose:  
**
===========================================================*/
using System;
using System.Runtime.InteropServices;
using Microsoft.Win32;
using System.Diagnostics.Contracts;
using System.Security;

namespace System.AddIn
{
    [SuppressUnmanagedCodeSecurity]
    [SecurityCritical]
    internal static class NativeMethods
    {
        [DllImport("KERNEL32.DLL", EntryPoint="IsWow64Process", PreserveSig=true, ExactSpelling=true, SetLastError=true)]
        internal static extern bool IsWow64Process(IntPtr hProcess, [MarshalAs(UnmanagedType.Bool)] ref bool bIsWow);
    }
}
