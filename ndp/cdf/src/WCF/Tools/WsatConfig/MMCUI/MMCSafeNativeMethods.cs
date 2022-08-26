//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.ComTypes;
    using System.Runtime.CompilerServices;
    using System.Runtime.ConstrainedExecution;
    using Microsoft.Win32.SafeHandles;
    using System.Security.AccessControl;

    static partial class SafeNativeMethods
    {
        internal const string ComCtl32 = "comctl32.dll";
        internal const string AclUI = "Aclui.dll";

        [DllImport(AclUI, SetLastError = true)]
        internal static extern int EditSecurity(
            [In] IntPtr hwndOwner,
            [In] ISecurityInformation psi);

        [DllImport(AdvApi32, SetLastError = false)]
        internal static extern void MapGenericMask(
            [Out] out int mask,
            [In] ref GenericMapping mapping);

        [DllImport(AdvApi32, SetLastError = true, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool ConvertStringSecurityDescriptorToSecurityDescriptor(
            [MarshalAs(UnmanagedType.LPWStr)] [In] string stringSecurityDescriptor,
            [In] int stringSDRevision,
            [Out] out IntPtr securityDescriptor,
            [Out] out int size);

        [DllImport(AdvApi32, SetLastError = true, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool ConvertSecurityDescriptorToStringSecurityDescriptorW(
            [In] IntPtr securityDescriptor,
            [In] int stringSDRevision,
            [In] SecurityInfos si,
            [Out] out IntPtr stringSecurityDescriptor,
            [Out] out int size);

        [DllImport(ComCtl32, CharSet = CharSet.Unicode)]
        internal static extern IntPtr CreatePropertySheetPage(
            [In] ref PropSheetPage psp);

        [SuppressUnmanagedCodeSecurity]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
        [DllImport(ComCtl32, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool DestroyPropertySheetPage(
            [In] IntPtr propPage);

        // For Window handle, we dont have to worry about its lifetime?
        [DllImport(User32, SetLastError = true, CharSet = CharSet.Unicode)]
        internal static extern IntPtr SetParent(
            [In] IntPtr hWndChild,
            [In] IntPtr hWndNewParent);

        [DllImport(User32, SetLastError = true, CharSet = CharSet.Unicode)]
        internal static extern IntPtr GetParent(
            [In] IntPtr hWndChild);

        [DllImport(User32, SetLastError = false)]
        internal static extern IntPtr SendMessage(
            [In] IntPtr hWnd,
            [In] int Msg,
            [In] IntPtr wordParameter,
            [In] IntPtr longParameter);

        [DllImport(CryptUI, SetLastError = false, CharSet = CharSet.Unicode)]
        internal static extern SafeCertificateContext CryptUIDlgSelectCertificateFromStore(
            [In] SafeCertificateStore hCertStore,
            [In] IntPtr hWind,
            [MarshalAs(UnmanagedType.LPWStr)] [In] string pwszTitle,
            [MarshalAs(UnmanagedType.LPWStr)] [In] string pwszDisplayString,
            [In] int dwDontUseColumn,
            [In] int dwFlags,
            [In] IntPtr pvReserved);

        [DllImport(Ole32, SetLastError = false)]
        internal static extern void ReleaseStgMedium(
            [In] ref STGMEDIUM pmedium);

        [DllImport(Kernel32, SetLastError = true)]
        internal static extern IntPtr GlobalAlloc(
            [In] int uFlags,
            [In] int uBytes);

        // Do not call this directly - call SetWindowLongWrapper which wraps this instead
        [DllImport(User32, EntryPoint = "SetWindowLong")]
        internal static extern IntPtr SetWindowLongPtr(IntPtr wnd, Int32 index, IntPtr newLong);

        [DllImport(User32, EntryPoint = "SetWindowLong")]
        internal static extern Int32 SetWindowLong(IntPtr wnd, Int32 index, Int32 newLong);
    }
}

