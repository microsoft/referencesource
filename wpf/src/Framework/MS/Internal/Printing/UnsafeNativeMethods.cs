using System;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;

using MS.Internal.PresentationFramework;

namespace MS.Internal.Printing
{
    internal static class UnsafeNativeMethods
    {
        /// <SecurityNote>
        ///     Critical: Because of suppression
        /// </SecurityNote>
        [SecurityCritical]
        [SuppressUnmanagedCodeSecurity]
        [DllImport("comdlg32.dll", CharSet = CharSet.Auto)]
        internal
        static
        extern
        Int32
        PrintDlgEx(
            IntPtr pdex
            );

        /// <SecurityNote>
        ///     Critical: Because of suppression
        /// </SecurityNote>
        [SecurityCritical]
        [SuppressUnmanagedCodeSecurity]
        [DllImport("kernel32.dll")]
        internal
        static
        extern
        IntPtr
        GlobalFree(
            IntPtr hMem
            );

        /// <SecurityNote>
        ///     Critical: Because of suppression
        /// </SecurityNote>
        [SecurityCritical]
        [SuppressUnmanagedCodeSecurity]
        [DllImport("kernel32.dll")]
        internal
        static
        extern
        IntPtr
        GlobalLock(
            IntPtr hMem
            );

        /// <SecurityNote>
        ///     Critical: Because of suppression
        /// </SecurityNote>
        [SecurityCritical]
        [SuppressUnmanagedCodeSecurity]
        [DllImport("kernel32.dll")]
        internal
        static
        extern
        bool
        GlobalUnlock(
            IntPtr hMem
            );
    }
}
