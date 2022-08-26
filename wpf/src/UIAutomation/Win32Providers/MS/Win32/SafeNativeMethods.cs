//---------------------------------------------------------------------------
//
// <copyright file="SafeNativeMethods.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Safe P/Invokes used by UIAutomation
//
// History:  
//  07/28/2003 : Micw moved to WCP branch.
//
//---------------------------------------------------------------------------

using System.Runtime.InteropServices;
using System;
using System.Security;
using System.Security.Permissions;
using System.Collections;
using System.IO;
using System.Text;

namespace MS.Win32
{
    // This class *MUST* be internal for security purposes
    internal static class SafeNativeMethods
    {
        [DllImport(ExternDll.Kernel32, ExactSpelling = true)]
        public static extern UInt32 GetTickCount();
        [DllImport(ExternDll.User32)]
        internal static extern int GetSysColor(int nIndex);
        [DllImport(ExternDll.User32, SetLastError = true)]
        public static extern bool IntersectRect (ref NativeMethods.Win32Rect rcDest, ref NativeMethods.Win32Rect rc1, ref NativeMethods.Win32Rect rc2);
        [DllImport(ExternDll.User32, ExactSpelling = true)]
        internal static extern bool IsWindowEnabled(IntPtr hWnd);
        [DllImport(ExternDll.User32, ExactSpelling = true)]
        internal static extern bool IsWindowVisible(IntPtr hWnd);
        [DllImport(ExternDll.User32, CharSet = CharSet.Unicode)]
        public static extern int MapVirtualKey(int nVirtKey, int nMapType);
        [DllImport(ExternDll.User32, CharSet = CharSet.Unicode, SetLastError = true)]
        internal static extern int RegisterWindowMessage(string msg);
        [DllImport(ExternDll.User32, SetLastError = true)]
        internal static extern bool UnionRect (out NativeMethods.Win32Rect rcDst, ref NativeMethods.Win32Rect rc1, ref NativeMethods.Win32Rect rc2);
        [DllImport(ExternDll.User32)]
        internal static extern IntPtr GetShellWindow();
    }
}

