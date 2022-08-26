//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using System.Collections;
    using System.Runtime.InteropServices;

    // NET_FIREWALL_IP_VERSION
    enum NetFirewallIPVersion
    {
        V4 = 0,
        V6 = 1,
        Any = 2,
        Max = 3,
    }

    // NET_FIREWALL_SCOPE
    enum NetFirewallScope
    {
        All = 0,
        Subnet = 1,
        Custom = 2,
        Max = 3,
    }

    // NET_FIREWALL_IP_PROTOCOL
    enum NetFirewallIPProtocol
    {
        Tcp = 6,
        Udp = 17,
    }

    // NET_FIREWALL_SERVICE_TYPE
    enum NetFirewallServiceType
    {
        FileAndPrint = 0,
        Upnp = 1,
        RemoteDesktop = 2,
        None = 3,
        Max = 4,
    }

    // NET_FIREWALL_PROFILE_TYPE
    enum NetFirewallProfileType
    {
        Domain = 0,
        Standard = 1,
        Current = 2,
        Max = 3,
    }

    // INetFwRemoteAdminSettings
    [ComImport, TypeLibType(4160), Guid("D4BECDDF-6F73-4A83-B832-9C66874CD20E")]
    interface INetFirewallRemoteAdminSettings
    {
        [DispId(1)]
        NetFirewallIPVersion IPVersion { get; set; }
        [DispId(2)]
        NetFirewallScope Scope { get; set; }
        [DispId(3)]
        string RemoteAddresses { get; set; }
        [DispId(4)]
        bool Enabled { get; set; }
    }

    // INetFwIcmpSettings
    [ComImport, TypeLibType(4160), Guid("A6207B2E-7CDD-426A-951E-5E1CBC5AFEAD")]
    interface INetFirewallIcmpSettings
    {
        [DispId(1)]
        bool AllowOutboundDestinationUnreachable { get; set; }
        [DispId(2)]
        bool AllowRedirect { get; set; }
        [DispId(3)]
        bool AllowInboundEchoRequest { get; set; }
        [DispId(4)]
        bool AllowOutboundTimeExceeded { get; set; }
        [DispId(5)]
        bool AllowOutboundParameterProblem { get; set; }
        [DispId(6)]
        bool AllowOutboundSourceQuench { get; set; }
        [DispId(7)]
        bool AllowInboundRouterRequest { get; set; }
        [DispId(8)]
        bool AllowInboundTimestampRequest { get; set; }
        [DispId(9)]
        bool AllowInboundMaskRequest { get; set; }
        [DispId(10)]
        bool AllowOutboundPacketTooBig { get; set; }
    }

    // INetFwOpenPort
    [ComImport, TypeLibType(4160), Guid("E0483BA0-47FF-4D9C-A6D6-7741D0B195F7")]
    interface INetFirewallOpenPort
    {
        [DispId(1)]
        string Name { get; set; }
        [DispId(2)]
        NetFirewallIPVersion IPVersion { get; set; }
        [DispId(3)]
        NetFirewallIPProtocol Protocol { get; set; }
        [DispId(4)]
        int Port { get; set; }
        [DispId(5)]
        NetFirewallScope Scope { get; set; }
        [DispId(6)]
        string RemoteAddresses { get; set; }
        [DispId(7)]
        bool Enabled { get; set; }
        [DispId(8)]
        bool BuiltIn { get; }
    }

    // INetFwOpenPorts
    [ComImport, TypeLibType(4160), Guid("C0E9D7FA-E07E-430A-B19A-090CE82D92E2")]
    interface INetFirewallOpenPortsCollection : IEnumerable
    {
        [DispId(1)]
        int Count { get; }
        [DispId(2)]
        void Add([MarshalAs(UnmanagedType.Interface)] INetFirewallOpenPort port);
        [DispId(3)]
        void Remove(int portNumber, NetFirewallIPProtocol ipProtocol);
        [DispId(4)]
        INetFirewallOpenPort Item([MarshalAs(UnmanagedType.Interface)] int port, NetFirewallIPProtocol portNumber);
        [DispId(-4)]
        [TypeLibFunc(1)]
        new IEnumerator GetEnumerator();
    }

    // INetFwService    
    [ComImport, TypeLibType(4160), Guid("79FD57C8-908E-4A36-9888-D5B3F0A444CF")]
    interface INetFirewallService
    {
        [DispId(1)]
        string Name { get; }
        [DispId(2)]
        NetFirewallServiceType Type { get; }
        [DispId(3)]
        bool Customized { get; }
        [DispId(4)]
        NetFirewallIPVersion IPVersion { get; set; }
        [DispId(5)]
        NetFirewallScope Scope { get; set; }
        [DispId(6)]
        string RemoteAddresses { get; set; }
        [DispId(7)]
        bool Enabled { get; set; }
        [DispId(8)]
        INetFirewallOpenPortsCollection GloballyOpenPorts { get; }
    }

    // INetFwServices
    [ComImport, TypeLibType(4160), Guid("79649BB4-903E-421B-94C9-79848E79F6EE")]
    interface INetFirewallServicesCollection : IEnumerable
    {
        [DispId(1)]
        int Count { get; }
        [DispId(2)]
        INetFirewallService Item([MarshalAs(UnmanagedType.Interface)] NetFirewallServiceType serviceType);
        [TypeLibFunc(1)]
        [DispId(-4)]
        new IEnumerator GetEnumerator();
    }

    // INetFwAuthorizedApplication
    [ComImport, TypeLibType(4160), Guid("B5E64FFA-C2C5-444E-A301-FB5E00018050")]
    interface INetFirewallAuthorizedApplication
    {
        [DispId(1)]
        string Name { get; set; }
        [DispId(2)]
        string ProcessImageFileName { get; set; }
        [DispId(3)]
        NetFirewallIPVersion IPVersion { get; set; }
        [DispId(4)]
        NetFirewallScope Scope { get; set; }
        [DispId(5)]
        string RemoteAddresses { get; set; }
        [DispId(6)]
        bool Enabled { get; set; }
    }

    // INetFwAuthorizedApplications
    [ComImport, TypeLibType(4160), Guid("644EFD52-CCF9-486C-97A2-39F352570B30")]
    interface INetFirewallAuthorizedApplicationsCollection : IEnumerable
    {
        [DispId(1)]
        int Count { get; }
        [DispId(2)]
        void Add([MarshalAs(UnmanagedType.Interface)] INetFirewallAuthorizedApplication app);
        [DispId(3)]
        void Remove([MarshalAs(UnmanagedType.BStr)] string imageFileName);
        [DispId(4)]
        INetFirewallAuthorizedApplication Item([MarshalAs(UnmanagedType.Interface)] string name);
        [DispId(-4)]
        [TypeLibFunc(1)]
        new IEnumerator GetEnumerator();
    }

    // INetFwProfile
    [ComImport, TypeLibType(4160), Guid("174A0DDA-E9F9-449D-993B-21AB667CA456")]
    interface INetFirewallProfile
    {
        [DispId(1)]
        NetFirewallProfileType Type { get; }
        [DispId(2)]
        bool FirewallEnabled { get; set; }
        [DispId(3)]
        bool ExceptionsNotAllowed { get; set; }
        [DispId(4)]
        bool NotificationsDisabled { get; set; }
        [DispId(5)]
        bool UnicastResponsesToMulticastBroadcastDisabled { get; set; }
        [DispId(6)]
        INetFirewallRemoteAdminSettings RemoteAdminSettings { get; }
        [DispId(7)]
        INetFirewallIcmpSettings IcmpSettings { get; }
        [DispId(8)]
        INetFirewallOpenPortsCollection GloballyOpenPorts { get; }
        [DispId(9)]
        INetFirewallServicesCollection Services { get; }
        [DispId(10)]
        INetFirewallAuthorizedApplicationsCollection AuthorizedApplications { get; }
    }

    // INetFwPolicy
    [ComImport, TypeLibType(4160), Guid("D46D2478-9AC9-4008-9DC7-5563CE5536CC")]
    interface INetFirewallPolicy
    {
        [DispId(1)]
        INetFirewallProfile CurrentProfile { get; }
        [DispId(2)]
        INetFirewallProfile GetProfileByType([MarshalAs(UnmanagedType.Interface)] NetFirewallProfileType profileType);
    }

    // INetFwMgr
    [ComImport, TypeLibType(4160), Guid("F7898AF5-CAC4-4632-A2EC-DA06E5111AF2")]
    interface INetFirewallMgr
    {
        [DispId(1)]
        INetFirewallPolicy LocalPolicy { get; }
        [DispId(2)]
        NetFirewallProfileType CurrentProfileType { get; }
        [DispId(3)]
        void RestoreDefaults();
        [DispId(4)]
        void IsPortAllowed([MarshalAs(UnmanagedType.BStr)] string imageFileName, NetFirewallIPVersion ipVersion, int portNumber, [MarshalAs(UnmanagedType.BStr)] string localAddress, NetFirewallIPProtocol ipProtocol, [MarshalAs(UnmanagedType.Struct)] ref object allowed, [MarshalAs(UnmanagedType.Struct)] ref object restricted);
        [DispId(5)]
        void IsIcmpTypeAllowed(NetFirewallIPVersion ipVersion, [MarshalAs(UnmanagedType.BStr)] string localAddress, byte type, [MarshalAs(UnmanagedType.Struct)] ref INetFirewallIcmpSettings allowed, [MarshalAs(UnmanagedType.Struct)] ref object restricted);
    }
}
