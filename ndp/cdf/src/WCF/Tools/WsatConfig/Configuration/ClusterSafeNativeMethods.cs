//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Runtime.CompilerServices;
    using System.Runtime.ConstrainedExecution;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.AccessControl;
    using System.Security.Permissions;
    using System.Text;

    using Microsoft.Win32;

    abstract class SafeClusterHandle : SafeHandle
    {
        [SecurityCritical]
        internal SafeClusterHandle()
            :
            base(IntPtr.Zero, true)
        {
        }

        public override bool IsInvalid
        {
            get { return IsClosed || handle == IntPtr.Zero; }
        }
    }

    class SafeHCluster : SafeClusterHandle
    {
        // MSDN remarks: This function always returns TRUE.
        [DllImport(SafeNativeMethods.ClusApi)]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
        static extern bool CloseCluster([In] IntPtr hCluster);

        protected override bool ReleaseHandle()
        {
            return CloseCluster(handle);
        }
    }

    class SafeHResource : SafeClusterHandle
    {
        [DllImport(SafeNativeMethods.ClusApi)]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        static extern bool CloseClusterResource([In] IntPtr hResource);

        protected override bool ReleaseHandle()
        {
            return CloseClusterResource(handle);
        }
    }

    class SafeHClusEnum : SafeClusterHandle
    {
        [DllImport(SafeNativeMethods.ClusApi)]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        static extern uint ClusterCloseEnum([In] IntPtr hEnum);

        protected override bool ReleaseHandle()
        {
            return ClusterCloseEnum(handle) == SafeNativeMethods.ERROR_SUCCESS;
        }
    }

    class SafeHKey : SafeClusterHandle
    {
        [DllImport(SafeNativeMethods.ClusApi)]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        static extern int ClusterRegCloseKey([In] IntPtr hEnum);

        protected override bool ReleaseHandle()
        {
            return ClusterRegCloseKey(handle) == SafeNativeMethods.ERROR_SUCCESS;
        }
    }

    [Flags]
    enum ClusterEnum : uint
    {
        Node = 0x00000001,
        ResType = 0x00000002,
        Resource = 0x00000004,
        Group = 0x00000008,
        Network = 0x00000010,
        NetInterface = 0x00000020,
        InternalNetwork = 0x80000000
    }

    enum ClusterResourceControlCode : uint
    {
        GetResourceType = 0x0100002d,
        //GetId = 0x01000039
    }

    static partial class SafeNativeMethods
    {
        internal const string ClusApi = "clusapi.dll";

        internal const uint ERROR_SUCCESS = 0;
        internal const uint ERROR_FILE_NOT_FOUND = 2;
        internal const uint ERROR_INSUFFICIENT_BUFFER = 122;
        internal const uint ERROR_MORE_DATA = 234;
        internal const uint ERROR_NO_MORE_ITEMS = 259;
        internal const uint REG_OPTION_NON_VOLATILE = 0;

        [DllImport(ClusApi, SetLastError = true, CharSet = CharSet.Unicode)]
        internal static extern SafeHCluster OpenCluster(
            [MarshalAs(UnmanagedType.LPWStr)] [In] string lpszClusterName);

        [DllImport(ClusApi, SetLastError = false, CharSet = CharSet.Unicode)]
        internal static extern int GetClusterInformation(
            [In] SafeHCluster hCluster,
            [Out] StringBuilder lpszClusterName,
            [In, Out] ref uint lpcchClusterName,
            [In, Out] IntPtr lpClusterInfo
            );

        [DllImport(ClusApi, SetLastError = true, CharSet = CharSet.Unicode)]
        internal static extern SafeHClusEnum ClusterOpenEnum(
            [In] SafeHCluster hCluster,
            [In] ClusterEnum dwType);

        [DllImport(ClusApi, CharSet = CharSet.Unicode)]
        internal static extern uint ClusterEnum(
            [In] SafeHClusEnum hEnum,
            [In] uint dwIndex,
            [Out] out uint lpdwType,
            [Out] StringBuilder lpszName,
            [In, Out] ref uint lpcchName);

        [DllImport(ClusApi, CharSet = CharSet.Unicode)]
        internal static extern uint ClusterResourceControl(
            [In] SafeHResource hResource,
            [In] IntPtr hHostNode,  //HNODE hHostNode, never used
            [In] ClusterResourceControlCode dwControlCode,
            [In] IntPtr lpInBuffer, // LPVOID lpInBuffer, never used
            [In] uint cbInBufferSize,
            [In, Out, MarshalAs(UnmanagedType.LPArray)] byte[] buffer,
            [In] uint cbOutBufferSize,
            [In, Out] ref uint lpcbBytesReturned);

        [DllImport(ClusApi, SetLastError = true, CharSet = CharSet.Unicode)]
        internal static extern SafeHResource OpenClusterResource(
            [In] SafeHCluster hCluster,
            [In, MarshalAs(UnmanagedType.LPWStr)] string lpszResourceName);

        [DllImport(ClusApi, SetLastError = true, CharSet = CharSet.Unicode)]
        internal static extern bool GetClusterResourceNetworkName(
            [In] SafeHResource hResource,
            [Out] StringBuilder lpBuffer,
            [In, Out] ref uint nSize);

        [DllImport(ClusApi, SetLastError = true, CharSet = CharSet.Unicode)]
        internal static extern SafeHKey GetClusterResourceKey(
            [In] SafeHResource hResource,
            [In] RegistryRights samDesired);

        [DllImport(ClusApi, SetLastError = false, CharSet = CharSet.Unicode)]
        internal static extern int ClusterRegCreateKey(
            [In] SafeHKey hKey,
            [In, MarshalAs(UnmanagedType.LPWStr)] string lpszSubKey,
            [In] uint dwOption,
            [In] RegistryRights samDesired,
            [In] IntPtr lpSecurityAttributes,
            [Out] out SafeHKey phkResult,
            [Out] out int lpdwDisposition);

        [DllImport(ClusApi, CharSet = CharSet.Unicode)]
        internal static extern int ClusterRegQueryValue(
            [In] SafeHKey hKey,
            [In, MarshalAs(UnmanagedType.LPWStr)] string lpszValueName,
            [Out] out RegistryValueKind lpdwValueType,
            [Out, MarshalAs(UnmanagedType.LPArray)] byte[] lpbData,
            [In, Out] ref uint lpcbData);

        [DllImport(ClusApi, CharSet = CharSet.Unicode)]
        internal static extern int ClusterRegSetValue(
            [In] SafeHKey hKey,
            [In, MarshalAs(UnmanagedType.LPWStr)] string lpszValueName,
            [In] RegistryValueKind lpdwValueType,
            [In, MarshalAs(UnmanagedType.LPArray)] byte[] lpbData,
            [In] uint lpcbData);

        [DllImport(ClusApi, CharSet = CharSet.Unicode)]
        internal static extern int ClusterRegGetKeySecurity(
            [In] SafeHKey hKey,
            [In] SecurityInfos securityInformation,
            [In, Out] byte[] securityDescriptor,
            [In, Out] ref uint lpcbSecurityDescriptor);

        [DllImport(ClusApi, CharSet = CharSet.Unicode)]
        internal static extern int ClusterRegSetKeySecurity(
            [In] SafeHKey hKey,
            [In] SecurityInfos securityInformation,
            [In] byte[] securityDescriptor);
    }
}
