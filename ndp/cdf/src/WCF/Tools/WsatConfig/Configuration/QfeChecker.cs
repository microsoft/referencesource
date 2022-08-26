//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Management;
    using System.Threading;
    using System.IO;
    using System.Runtime.InteropServices;

    static class QfeChecker
    {
        const string CLSID_CGatewayProtocol = "{37de7045-5056-456f-8409-c871e0f8b0e0}";
        const string IID_IClassFactory = "{00000001-0000-0000-C000-000000000046}";
        const string RegKeyClassIdRoot = @"SOFTWARE\Classes\CLSID\";

        //
        //The hotfix that is checked here depends on the system. WinXP:912817,  Win2k3:912818 
        //
        public static void CheckQfe()
        {
            bool qfeInstalled = false;
            int hr = SafeNativeMethods.CoInitializeEx(IntPtr.Zero, SafeNativeMethods.COINIT_APARTMENTTHREADED);
            if (hr >= 0)
            {
                try
                {
                    SafeIUnknown pClassFactory = null;

                    hr = SafeNativeMethods.CoGetClassObject(new Guid(CLSID_CGatewayProtocol),
                        SafeNativeMethods.CLSCTX.CLSCTX_INPROC_SERVER,
                        IntPtr.Zero,
                        new Guid(IID_IClassFactory),
                        out pClassFactory);

                    if (hr >= 0 && pClassFactory != null)
                    {
                        qfeInstalled = true;
                    }
                    else
                    {
                        qfeInstalled = RegistryEntryExists();
                    }
                }
                finally
                {
                    SafeNativeMethods.CoUninitialize();
                }
            }

            if (!qfeInstalled)
            {
                throw new WsatAdminException(WsatAdminErrorCode.CANNOT_ENABLE_NETWORK_SUPPORT_WHEN_QFE_IS_NOT_INSTALLED, SR.GetString(SR.ErrorNoQFE));
            }
        }

        static bool RegistryEntryExists()
        {
            bool regKeyExists = false;
            using (SafeRegistryKey hive = new SafeRegistryKey(new IntPtr((int)Microsoft.Win32.RegistryHive.LocalMachine), false))
            {
                SafeRegistryKey regKey = null;
                int ret = SafeNativeMethods.RegOpenKeyEx(
                            hive,
                            RegKeyClassIdRoot + CLSID_CGatewayProtocol,
                            0,
                            SafeNativeMethods.KEY_WOW64_64KEY | SafeNativeMethods.KEY_QUERY_VALUE,
                            out regKey);

                if (ret == 0 && regKey != null && !regKey.IsInvalid)
                {
                    regKeyExists = true;
                    regKey.Close();
                }
            }
            return regKeyExists;
        }
    }
 
    static partial class SafeNativeMethods
    {
        internal const uint COINIT_APARTMENTTHREADED = 0x2;

        internal const string Ole32 = "ole32.dll";

        [DllImport(Ole32)]
        internal static extern int CoInitializeEx(
            [In] IntPtr pvReserved, 
            [In] uint dwCoInit);

        [DllImport(Ole32, CharSet = CharSet.Unicode)]
        internal static extern int CoGetClassObject(
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid rclsid,
            [In] CLSCTX dwClsContext,
            [In] IntPtr pServerInfo,
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid riid,
            [Out] out SafeIUnknown ppv);

        [DllImport(Ole32)]
        internal static extern void CoUninitialize();

        [Flags]
        internal enum CLSCTX : uint
        {
            CLSCTX_INPROC_SERVER = 0x1,
            CLSCTX_INPROC_HANDLER = 0x2,
            CLSCTX_LOCAL_SERVER = 0x4,
            CLSCTX_INPROC_SERVER16 = 0x8,
            CLSCTX_REMOTE_SERVER = 0x10,
            CLSCTX_INPROC_HANDLER16 = 0x20,
            CLSCTX_RESERVED1 = 0x40,
            CLSCTX_RESERVED2 = 0x80,
            CLSCTX_RESERVED3 = 0x100,
            CLSCTX_RESERVED4 = 0x200,
            CLSCTX_NO_CODE_DOWNLOAD = 0x400,
            CLSCTX_RESERVED5 = 0x800,
            CLSCTX_NO_CUSTOM_MARSHAL = 0x1000,
            CLSCTX_ENABLE_CODE_DOWNLOAD = 0x2000,
            CLSCTX_NO_FAILURE_LOG = 0x4000,
            CLSCTX_DISABLE_AAA = 0x8000,
            CLSCTX_ENABLE_AAA = 0x10000,
            CLSCTX_FROM_DEFAULT_CONTEXT = 0x20000,
            CLSCTX_INPROC = CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
            CLSCTX_SERVER = CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER,
            CLSCTX_ALL = CLSCTX_SERVER | CLSCTX_INPROC_HANDLER
        }
    }
}
