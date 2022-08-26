//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Text;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.ComTypes;
    using System.Runtime.CompilerServices;
    using System.Runtime.ConstrainedExecution;
    using Microsoft.Win32.SafeHandles;

    static partial class SafeNativeMethods
    {
        internal const int X509_ASN_ENCODING = 0x00000001;
        internal const int CERT_FIND_ANY = 0;
        internal const int CERT_REGISTRY_STORE_REMOTE_FLAG = 0x10000;
        internal const int CERT_STORE_OPEN_EXISTING_FLAG = 0x00004000;
        internal const int CERT_STORE_READONLY_FLAG = 0x00008000;
        internal const int CERT_STORE_PROV_REG = 4;
        internal const int CRYPTUI_SELECT_LOCATION_COLUMN = 0x000000010;
        internal const int GMEM_SHARE = 0x2000;
        internal const int LMEM_ZEROINIT = 0x0040;
        internal const int WM_USER = 1024;      //0x0400
        internal const int TCM_FIRST = 4864;    //0x1300 
        internal const int PSM_GETCURRENTPAGEHWND = WM_USER + 118;
        internal const int PSM_INDEXTOHWND = WM_USER + 130;
        internal const int PSM_GETTABCONTROL = WM_USER + 116;
        internal const int TCM_GETITEMCOUNT = TCM_FIRST + 4;
        internal const int ErrorAlreadyExists = 183;
        internal const int NoError = 0;
        internal const int FileNotFound = 2;
        internal const int ErrorInvalidParameter = 87;
        internal const int HTTP_INITIALIZE_CONFIG = 0x00000002;
        internal const int HTTP_SERVICE_CONFIG_SSL_FLAG_NEGOTIATE_CLIENT_CERT = 0x00000002;

        internal const int OLE_TM_FLAG_NONE = 0x00000000;

        internal const int DWL_MSGRESULT = 0;

        internal const int NERR_Success = 0;

        //Registry Key Security and Access Rights
        internal const int KEY_WOW64_64KEY = 0x100;
        internal const int KEY_QUERY_VALUE = 0x01;
        internal const int KEY_READ = 0x20019;

        internal const string XOleHlp = "xolehlp.dll";
        internal const string HttpApi = "httpapi.dll";
        internal const string CryptUI = "Cryptui.dll";
        internal const string Crypt32 = "Crypt32.dll";
        internal const string User32 = "User32.dll";
        internal const string Kernel32 = "Kernel32.dll";
        internal const string AdvApi32 = "Advapi32.DLL";
        internal const string NetApi32 = "netapi32.dll";

        internal const uint OLE_TM_CONFIG_VERSION_2 = 2;

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        internal struct OLE_TM_CONFIG_PARAMS_V2
        {
            internal uint dwVersion;
            internal uint dwcConcurrencyHint;

            [MarshalAs(UnmanagedType.LPWStr)]
            internal string pwszClusterResourceName;
        };

        [DllImport(XOleHlp, CharSet = CharSet.Unicode)]
        internal static extern int DtcGetTransactionManagerEx(
            [In, MarshalAs(UnmanagedType.LPWStr)] string hostName,
            [In, MarshalAs(UnmanagedType.LPWStr)] string tmName,
            [In] ref Guid riid,
            [In] UInt32 grfOptions,
            [In] IntPtr pvConfigParams, //must be IntPtr.Zero
            [Out, MarshalAs(UnmanagedType.Interface)]out IDtcNetworkAccessConfig idtcnac);

        [DllImport(XOleHlp, CharSet = CharSet.Unicode, EntryPoint = "DtcGetTransactionManagerEx")]
        internal static extern int DtcGetTransactionManagerEx_WithConfigParams(
            [In, MarshalAs(UnmanagedType.LPWStr)] string hostName,
            [In, MarshalAs(UnmanagedType.LPWStr)] string tmName,
            [In] ref Guid riid,
            [In] UInt32 grfOptions,
            [In] ref OLE_TM_CONFIG_PARAMS_V2 pvConfigParams,
            [Out, MarshalAs(UnmanagedType.Interface)]out IDtcNetworkAccessConfig idtcnac);
        
        [DllImport(HttpApi, SetLastError = false, EntryPoint = "HttpDeleteServiceConfiguration")]
        internal static extern int HttpDeleteServiceConfiguration_UrlAcl(
            [In] IntPtr serviceIntPtr,
            [In] HttpServiceConfigId configId,
            [In] ref HttpServiceConfigUrlAclSet configInformation,
            [In] int configInformationLength,
            // this must be IntPtr.Zero
            [In] IntPtr pOverlapped);

        [DllImport(HttpApi, SetLastError = false, EntryPoint = "HttpDeleteServiceConfiguration")]
        internal static extern int HttpDeleteServiceConfiguration_Ssl(
            [In] IntPtr serviceIntPtr,
            [In] HttpServiceConfigId configId,
            [In] ref HttpServiceConfigSslSet configInformation,
            [In] int configInformationLength,
            // this must be IntPtr.Zero
            [In] IntPtr pOverlapped);

        [DllImport(HttpApi, SetLastError = false)]
        internal static extern int HttpInitialize(
            [In] HttpApiVersion version,
            [In] int flags,
            // this must be IntPtr.Zero 
            [In] IntPtr pReserved);

        [DllImport(HttpApi, SetLastError = false, EntryPoint = "HttpSetServiceConfiguration")]
        internal static extern int HttpSetServiceConfiguration_UrlAcl(
            [In] IntPtr serviceIntPtr,
            [In] HttpServiceConfigId configId,
            [In] ref HttpServiceConfigUrlAclSet configInformation,
            [In] int configInformationLength,
            // this must be IntPtr.Zero 
            [In] IntPtr pOverlapped);

        [DllImport(HttpApi, SetLastError = false, EntryPoint = "HttpSetServiceConfiguration")]
        internal static extern int HttpSetServiceConfiguration_Ssl(
            [In] IntPtr serviceIntPtr,
            [In] HttpServiceConfigId configId,
            [In] ref HttpServiceConfigSslSet configInformation,
            [In] int configInformationLength,
            // this must be IntPtr.Zero
            [In] IntPtr pOverlapped);

        [DllImport(HttpApi, SetLastError = false)]
        internal static extern uint HttpTerminate(
            [In] int Flags,
            // this must be IntPtr.Zero
            [In] IntPtr pReserved);

        [DllImport(Crypt32, SetLastError = true, EntryPoint = "CertOpenStore")]
        internal static extern SafeCertificateStore CertOpenStore_ptr(
            [In] int lpszStoreProvider,
            [In] int dwMsgAndCertEncodingType,
            [In] int hCryptProvider,
            [In] int dwFlags,
            [In] SafeRegistryKey pvPara);

        [SuppressUnmanagedCodeSecurity]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
        [DllImport(Crypt32, SetLastError = true, CallingConvention = CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool CertCloseStore(
            [In] IntPtr hCertStore,
            [In] int dwFlags);

        [DllImport(Crypt32, SetLastError = true)]
        internal static extern SafeCertificateContext CertFindCertificateInStore(
            [In] SafeCertificateStore hCertStore,
            [In] int dwCertEncodingType,
            [In] int dwFindFlags,
            [In] int dwFindType,
            [In] IntPtr pvFindPara,
            [In] SafeCertificateContext pPrevCertContext);

        [SuppressUnmanagedCodeSecurity]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
        [DllImport(Crypt32, SetLastError = false, CallingConvention = CallingConvention.StdCall)]
        internal static extern int CertFreeCertificateContext(
            [In] IntPtr pCertContext);

        [DllImport(AdvApi32, SetLastError = false, CharSet = CharSet.Unicode)]
        internal static extern int RegOpenKeyEx(
            [In] SafeRegistryKey hKey,
            [MarshalAs(UnmanagedType.LPWStr)] [In] string lpSubKey,
            [In] uint options,
            [In] int sam,
            [Out] out SafeRegistryKey phkResult);

#if WSAT_UI
        [DllImport(AdvApi32, SetLastError = false, CharSet = CharSet.Unicode)]
        internal static extern int RegConnectRegistry(
            [MarshalAs(UnmanagedType.LPWStr)] [In] string machineName,
            [In] SafeRegistryKey key,
            [Out] out SafeRegistryKey phkResult);
#endif

        [SuppressUnmanagedCodeSecurity]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
        [DllImport(AdvApi32, SetLastError = false, CallingConvention = CallingConvention.StdCall)]
        internal static extern int RegCloseKey(
            [In] IntPtr hKey);

        [SuppressUnmanagedCodeSecurity]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
        [DllImport(Kernel32, SetLastError = true)]
        internal static extern IntPtr LocalFree(
            [In] IntPtr handle);

        [DllImport(Kernel32, SetLastError = true)]
        internal static extern IntPtr LocalAlloc(
            [In] int uFlags,
            [In] int uBytes);

        [DllImport(Kernel32, SetLastError = true, CharSet = CharSet.Unicode)]
        internal static extern IntPtr LoadLibrary([In] string lpFileName);

        [DllImport(Kernel32, SetLastError = true)]
        internal static extern bool FreeLibrary(IntPtr hModule);

        [DllImport(User32, SetLastError = true, CharSet = CharSet.Unicode)]
        internal static extern int LoadString(IntPtr hInstance, uint uID, StringBuilder lpBuffer, int nBufferMax);

        [StructLayout(LayoutKind.Sequential)]
        internal struct WKSTA_INFO_100
        {
            public int id;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string computername;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string langroup;
            public int ver_major;
            public int ver_minor;
        }

        [DllImport(NetApi32, CharSet = CharSet.Unicode)]
        internal static extern int NetWkstaGetInfo(string servername, int level, out IntPtr bufptr);

        [DllImport(NetApi32, SetLastError = true)]
        internal static extern int NetApiBufferFree(IntPtr bufptr);
    }
}
