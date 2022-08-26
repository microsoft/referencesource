//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Net;
    using System.Net.Sockets;

    struct HttpWrapper
    {
        internal static HttpApiVersion HttpApiVersion1 = new HttpApiVersion(1, 0);
        internal static HttpApiVersion HttpApiVersion2 = new HttpApiVersion(2, 0);
    }

    // HTTP_SERVICE_CONFIG_ID
    enum HttpServiceConfigId
    {
        HttpServiceConfigIPListenList = 0,
        HttpServiceConfigSSLCertInfo,
        HttpServiceConfigUrlAclInfo,
        HttpServiceConfigSSLCertInfoSafe,
        HttpServiceConfigTimeout,
        HttpServiceConfigMax
    }

    // HTTP_API_VERSION
    [StructLayout(LayoutKind.Sequential, Pack = 2)]
    struct HttpApiVersion
    {
        internal ushort HttpApiMajorVersion;
        internal ushort HttpApiMinorVersion;

        internal HttpApiVersion(ushort majorVersion, ushort minorVersion)
        {
            HttpApiMajorVersion = majorVersion;
            HttpApiMinorVersion = minorVersion;
        }
    }

    // HTTP_SERVICE_CONFIG_SSL_SET
    [StructLayout(LayoutKind.Sequential)]
    struct HttpServiceConfigSslSet
    {
        internal HttpServiceConfigSslKey KeyDesc;
        internal HttpServiceConfigSslParam ParamDesc;
    }

    // HTTP_SERVICE_CONFIG_SSL_KEY
    [StructLayout(LayoutKind.Sequential)]
    struct HttpServiceConfigSslKey
    {
        internal IntPtr pIpPort;
    }

    // HTTP_SERVICE_CONFIG_SSL_PARAM
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    struct HttpServiceConfigSslParam
    {
        internal int SslHashLength;
        internal SafeLocalAllocation pSslHash;
        internal Guid AppId;
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string pSslCertStoreName;
        internal int DefaultCertCheckMode;
        internal int DefaultRevocationFreshnessTime;
        internal int DefaultRevocationUrlRetrievalTimeout;
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string pDefaultSslCtlIdentifier;
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string pDefaultSslCtlStoreName;
        internal int DefaultFlags;
    }

    // HTTP_SERVICE_CONFIG_APP_REFERENCE
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    struct HttpServiceConfigAppReference
    {
        internal Guid AppGuid;
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string AppFriendlyName;
        [MarshalAs(UnmanagedType.Bool)]
        internal bool IsLegacyreference;
    }

    // HTTP_SERVICE_CONFIG_URLACL_SET
    [StructLayout(LayoutKind.Sequential)]
    struct HttpServiceConfigUrlAclSet
    {
        internal HttpServiceConfigUrlAclKey KeyDesc;
        internal HttpServiceConfigUrlAclParam ParamDesc;
    }

    // HTTP_SERVICE_CONFIG_URLACL_KEY
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    struct HttpServiceConfigUrlAclKey
    {
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string pUrlPrefix;

        internal HttpServiceConfigUrlAclKey(string urlPrefix)
        {
            pUrlPrefix = urlPrefix;
        }
    }

    // HTTP_SERVICE_CONFIG_URLACL_PARAM
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    struct HttpServiceConfigUrlAclParam
    {
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string pStringSecurityDescriptor;

        internal HttpServiceConfigUrlAclParam(string securityDescriptor)
        {
            pStringSecurityDescriptor = securityDescriptor;
        }
    }

    // SOCKADDR
    class WinsockSockAddr : IDisposable
    {
        const Int16 AF_INET = 2;
        const Int16 AF_INET6 = 23;

        object address;
        GCHandle pinnedAddress;
        IntPtr pAddress;

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        internal struct SOCKADDR_IN
        {
            internal Int16 family;
            internal Int16 port;
            internal Byte addr00; internal Byte addr01;
            internal Byte addr02; internal Byte addr03;
            internal Int32 nothing;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        internal struct SOCKADDR_IN6
        {
            internal Int16 family;
            internal Int16 port;
            internal Int32 flowInfo;
            internal Byte addr00; internal Byte addr01;
            internal Byte addr02; internal Byte addr03;
            internal Byte addr04; internal Byte addr05;
            internal Byte addr06; internal Byte addr07;
            internal Byte addr08; internal Byte addr09;
            internal Byte addr10; internal Byte addr11;
            internal Byte addr12; internal Byte addr13;
            internal Byte addr14; internal Byte addr15;
            public Int32 scopeID;
        }

        internal WinsockSockAddr(IPAddress source, short port)
        {
            pAddress = IntPtr.Zero;

            if (source.AddressFamily == AddressFamily.InterNetwork)
            {
                SOCKADDR_IN a;
                Byte[] addr = source.GetAddressBytes();

                a.family = AF_INET;
                a.port = IPAddress.HostToNetworkOrder(port);
                a.addr00 = addr[0]; a.addr01 = addr[1];
                a.addr02 = addr[2]; a.addr03 = addr[3];
                a.nothing = 0;

                address = a;
            }
            else if (source.AddressFamily == AddressFamily.InterNetworkV6)
            {
                SOCKADDR_IN6 a;
                Byte[] addr = source.GetAddressBytes();

                a.family = AF_INET6;
                a.port = IPAddress.HostToNetworkOrder(port);
                a.flowInfo = 0;
                a.addr00 = addr[0]; a.addr01 = addr[1];
                a.addr02 = addr[2]; a.addr03 = addr[3];
                a.addr04 = addr[4]; a.addr05 = addr[5];
                a.addr06 = addr[6]; a.addr07 = addr[7];
                a.addr08 = addr[8]; a.addr09 = addr[9];
                a.addr10 = addr[10]; a.addr11 = addr[11];
                a.addr12 = addr[12]; a.addr13 = addr[13];
                a.addr14 = addr[14]; a.addr15 = addr[15];
                a.scopeID = (Int32)source.ScopeId;

                address = a;
            }

            pinnedAddress = GCHandle.Alloc(address, GCHandleType.Pinned);
            pAddress = pinnedAddress.AddrOfPinnedObject();
        }

        public void Dispose()
        {
            Close();
            GC.SuppressFinalize(this);
        }

        void Close()
        {
            if (pinnedAddress.IsAllocated)
            {
                pinnedAddress.Free();
            }

            address = null;
            pAddress = IntPtr.Zero;
        }

        ~WinsockSockAddr()
        {
            Close();
        }

        internal IntPtr PinnedSockAddr
        {
            get { return pAddress; }
        }
    }
}
