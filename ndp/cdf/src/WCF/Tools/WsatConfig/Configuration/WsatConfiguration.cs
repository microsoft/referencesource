//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Text;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Principal;
    using System.Globalization;
    using System.Diagnostics;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Cryptography;
    using System.Security.AccessControl;
    using Microsoft.Win32;

    // In theory we could abstract this class out and provide concrete implementation for config from different
    // sources, in practice we didn't do that for historical code reasons
    class WsatConfiguration
    {
        const string TransactionBridgeRegistryValue = "{BFFECCA7-4069-49F9-B5AB-7CCBB078ED91}";
        const string TransactionBridge30RegistryValue = "{EEC5DCCA-05DC-4B46-8AF7-2881C1635AEA}";

        internal const bool DefaultNetworkSupport = false;
        internal const uint DefaultHttpsPort = 443;
        internal const SourceLevels DefaultTraceLevel = SourceLevels.Warning;
        internal const bool DefaultActivityPropagation = false;
        internal const bool DefaultActivityTracing = false;
        internal const uint DefaultDefaultTimeout = 60;
        internal const uint DefaultMaxTimeout = 3600;
        internal const bool DefaultTracingPii = false;
        internal string[] DefaultX509GlobalAcl = { string.Empty };
        internal string[] DefaultKerberosGlobalAcl = { @"NT AUTHORITY\Authenticated Users" };
        const X509Certificate2 DefaultX509CertificateIdentity = null;

        WsatConfiguration previousConfig;
        FirewallWrapper firewallWrapper;
        string machineName;
        string virtualServer;
        ConfigurationProvider wsatConfigProvider;
        ConfigurationProvider msdtcConfigProvider;
        
        bool isClusterRemoteNode = false;
        uint httpsPort = DefaultHttpsPort;
        uint defaultTimeout = DefaultDefaultTimeout;
        uint maxTimeout = DefaultMaxTimeout;
        SourceLevels diagnosticTraceLevel = DefaultTraceLevel;
        bool activityPropagation = DefaultActivityPropagation;
        bool activityTracing = DefaultActivityTracing;
        bool tracePii = DefaultTracingPii;
        bool transactionBridgeEnabled = false;
        bool transactionBridge30Enabled = false;
        X509Certificate2 certificate = DefaultX509CertificateIdentity;
        string[] allowedCertificates = null;
        string[] kerberosGlobalAcl = null;
        bool minimalWrite = false;
        string[] clusterNodes = null;

        SafeHResource hClusterDtcResource;

        [SecurityCritical]
        internal WsatConfiguration(string machineName, string virtualServer, WsatConfiguration previousConfig, bool minimalWrite)
        {
            this.MachineName = machineName;
            this.minimalWrite = minimalWrite;
            this.firewallWrapper = new FirewallWrapper();
            this.previousConfig = previousConfig;
            this.virtualServer = virtualServer;
            if (previousConfig == null)
            {
                this.allowedCertificates = DefaultX509GlobalAcl;
                this.kerberosGlobalAcl = DefaultKerberosGlobalAcl;
            }
            else
            {
                CopyConfigurationData(previousConfig, this);
            }

            if (MsdtcClusterUtils.IsClusterServer(MachineName))
            {
                this.hClusterDtcResource = MsdtcClusterUtils.GetTransactionManagerClusterResource(VirtualServer, out clusterNodes);
                if (hClusterDtcResource == null || hClusterDtcResource.IsInvalid)
                {
                    if (!string.IsNullOrEmpty(VirtualServer))
                    {
                        throw new WsatAdminException(WsatAdminErrorCode.CANNOT_FIND_CLUSTER_VIRTUAL_SERVER, SR.GetString(SR.ErrorCanNotFindVirtualServer));
                    }
                }
            }
            InitializeConfigurationProvider();
        }

        void CopyConfigurationData(WsatConfiguration src, WsatConfiguration dest)
        {
            dest.TransactionBridgeEnabled = src.TransactionBridgeEnabled;
            dest.TransactionBridge30Enabled = src.TransactionBridge30Enabled;
            dest.HttpsPort = src.HttpsPort;
            dest.X509Certificate = src.X509Certificate;
            dest.KerberosGlobalAcl = src.KerberosGlobalAcl;
            dest.X509GlobalAcl = src.X509GlobalAcl;
            dest.DefaultTimeout = src.DefaultTimeout;
            dest.MaxTimeout = src.MaxTimeout;
            dest.TraceLevel = src.TraceLevel;
            dest.ActivityPropagation = src.ActivityPropagation;
            dest.ActivityTracing = src.ActivityTracing;
            dest.TracePii = src.TracePii;
            dest.MachineName = src.MachineName;
            dest.IsClusterRemoteNode = src.IsClusterRemoteNode;
            dest.VirtualServer = src.VirtualServer;
        }       

        internal MsdtcWrapper GetMsdtcWrapper()
        {
            return MsdtcWrapper.GetWrapper(MachineName, VirtualServer, this.msdtcConfigProvider);
        }

        // A cluster node can spawn remote processes to setup other nodes.
        // Is this a node of cluster, but not the originating one?
        internal bool IsClusterRemoteNode
        {
            get { return this.isClusterRemoteNode; }
            set { this.isClusterRemoteNode = value; }
        }

        internal bool IsClustered
        {
            get
            {
                return (hClusterDtcResource != null && !hClusterDtcResource.IsInvalid);
            }
        }

        internal string MachineName
        {
            get { return string.IsNullOrEmpty(this.machineName) ? string.Empty : machineName; }
            set { this.machineName = value; }
        }

        internal string VirtualServer
        {
            get { return this.virtualServer; }
            set { this.virtualServer = value; }
        }

        internal bool TransactionBridgeEnabled
        {
            get { return this.transactionBridgeEnabled; }
            set { this.transactionBridgeEnabled = value; }
        }

        internal bool TransactionBridge30Enabled
        {
            get { return this.transactionBridge30Enabled; }
            set { this.transactionBridge30Enabled = value; }
        }

        internal uint HttpsPort
        {
            get { return this.httpsPort; }
            set { this.httpsPort = value; }
        }

        internal SourceLevels TraceLevel
        {
            get { return this.diagnosticTraceLevel; }
            set { this.diagnosticTraceLevel = value; }
        }

        internal uint DefaultTimeout
        {
            get { return this.defaultTimeout; }
            set { this.defaultTimeout = value; }
        }

        internal uint MaxTimeout
        {
            get { return this.maxTimeout; }
            set { this.maxTimeout = value; }
        }

        internal bool ActivityTracing
        {
            get { return this.activityTracing; }
            set { this.activityTracing = value; }
        }

        internal bool ActivityPropagation
        {
            get { return this.activityPropagation; }
            set { this.activityPropagation = value; }
        }

        internal bool TracePii
        {
            get { return this.tracePii; }
            set { this.tracePii = value; }
        }

        internal string[] X509GlobalAcl
        {
            get { return this.allowedCertificates; }
            set
            {
                if (value == null)
                {
                    this.allowedCertificates = new string[] { };
                }
                else
                {
                    this.allowedCertificates = value;
                }
            }
        }

        internal string[] KerberosGlobalAcl
        {
            get { return kerberosGlobalAcl; }
            set
            {
                if (value == null)
                {
                    this.kerberosGlobalAcl = new string[] { };
                }
                else
                {
                    this.kerberosGlobalAcl = value;
                }
            }
        }

        internal X509Certificate2 X509Certificate
        {
            get { return this.certificate; }
            set { this.certificate = value; }
        }

        internal void ValidateThrow()
        {
            if (this.TransactionBridgeEnabled)
            {
                // rule: WS-AT network support requires MSDTC network transaction to be enabled
                
                // GetNetworkTransactionAccess fails if current remote cluster node does not take ownership
                if (!IsClusterRemoteNode)
                {
                    MsdtcWrapper wrapper = this.GetMsdtcWrapper();
                    if (!wrapper.GetNetworkTransactionAccess())
                    {
                        throw new WsatAdminException(WsatAdminErrorCode.MSDTC_NETWORK_ACCESS_DISABLED,
                                                        SR.GetString(SR.ErrorMsdtcNetworkAccessDisabled));
                    }
                }

                // rule: HTTPS port must be in range 1-65535
                if (this.HttpsPort < 1)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.INVALID_HTTPS_PORT,
                                                    SR.GetString(SR.ErrorHttpsPortRange));
                }
                
                // rule: local endpoint certificate must be specified and valid
                ValidateIdentityCertificateThrow(this.X509Certificate, !Utilities.IsLocalMachineName(MachineName));

                // rule: default timeout should be in range 1-3600
                if (this.DefaultTimeout < 1 || this.DefaultTimeout > 3600)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.INVALID_DEFTIMEOUT_ARGUMENT,
                                                    SR.GetString(SR.ErrorDefaultTimeoutRange));
                }

                // rule: max timeout be in range 0-3600
                if (this.MaxTimeout > 3600)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.INVALID_MAXTIMEOUT_ARGUMENT,
                                                    SR.GetString(SR.ErrorMaximumTimeoutRange));
                }
            }
        }

        void InitializeConfigurationProvider()
        {
            if (IsClustered)
            {
                this.msdtcConfigProvider = new ClusterRegistryConfigurationProvider(this.hClusterDtcResource, GetClusterMstdcRegistryKey());
                this.wsatConfigProvider = new ClusterRegistryConfigurationProvider(this.hClusterDtcResource, WsatKeys.WsatClusterRegKey);
            }
            else
            {
                this.msdtcConfigProvider = new RegistryConfigurationProvider(RegistryHive.LocalMachine, WsatKeys.MsdtcRegKey, MachineName);
                this.wsatConfigProvider = new RegistryConfigurationProvider(RegistryHive.LocalMachine, WsatKeys.WsatRegKey, MachineName);
            }
        }

        //
        // LH: Cluster\Resources\GUID_OF_DTC\MSDTCPrivate\MSDTC
        // W2k3: Cluster\Resources\GUID_OF_DTC\SOME_GUID\, here SOME_GUID is the default value of GUID_OF_DTC\DataPointer\
        //       
        string GetClusterMstdcRegistryKey()
        {
            Debug.Assert(IsClustered);

            if (Utilities.OSMajor > 5)
            {
                return WsatKeys.MsdtcClusterRegKey_OS6;
            }
            ClusterRegistryConfigurationProvider clusterReg = new ClusterRegistryConfigurationProvider(this.hClusterDtcResource, WsatKeys.MsdtcClusterDataPointerRegKey_OS5);
            using (clusterReg)
            {
                //the default value 
                string subKey = clusterReg.ReadString(string.Empty, string.Empty);
                if (!string.IsNullOrEmpty(subKey))
                {
                    return subKey;
                }
            }
            RegistryExceptionHelper registryExceptionHelper = new RegistryExceptionHelper(WsatKeys.MsdtcClusterDataPointerRegKey_OS5);
            throw registryExceptionHelper.CreateRegistryAccessException(null);
        }

        [SecurityCritical]
        internal void LoadFromRegistry()
        {
            string value = msdtcConfigProvider.ReadString(WsatKeys.TransactionBridgeRegKey, null);
            TransactionBridgeEnabled = Utilities.SafeCompare(value, TransactionBridgeRegistryValue);

            TransactionBridge30Enabled = Utilities.SafeCompare(value, TransactionBridge30RegistryValue);

            HttpsPort = wsatConfigProvider.ReadUInt32(WsatKeys.RegistryEntryHttpsPort, DefaultHttpsPort);
            X509Certificate = CertificateManager.GetCertificateFromThumbprint(
                wsatConfigProvider.ReadString(WsatKeys.RegistryEntryX509CertificateIdentity, string.Empty),
                MachineName);
            KerberosGlobalAcl = wsatConfigProvider.ReadMultiString(WsatKeys.RegistryEntryKerberosGlobalAcl, DefaultKerberosGlobalAcl);
            X509GlobalAcl = wsatConfigProvider.ReadMultiString(WsatKeys.RegistryEntryX509GlobalAcl, DefaultX509GlobalAcl);
            TraceLevel = (SourceLevels)wsatConfigProvider.ReadUInt32(WsatKeys.RegistryEntryTraceLevel, (uint)DefaultTraceLevel);
#pragma warning disable 429            
            ActivityTracing = wsatConfigProvider.ReadUInt32(WsatKeys.RegistryEntryActivityTracing, (DefaultActivityTracing ? 1 : 0)) != 0;
            ActivityPropagation = wsatConfigProvider.ReadUInt32(WsatKeys.RegistryEntryPropagateActivity, (DefaultActivityPropagation ? 1 : 0)) != 0;
            TracePii = wsatConfigProvider.ReadUInt32(WsatKeys.RegistryEntryTracingPii, (DefaultTracingPii ? 1 : 0)) != 0;
#pragma warning restore 429
            DefaultTimeout = wsatConfigProvider.ReadUInt32(WsatKeys.RegistryEntryDefTimeout, DefaultDefaultTimeout);
            MaxTimeout = wsatConfigProvider.ReadUInt32(WsatKeys.RegistryEntryMaxTimeout, DefaultMaxTimeout);
        }

        internal bool IsLocalMachine
        {
            get { return !IsClustered && Utilities.IsLocalMachineName(this.MachineName); }
        }

        // The code should align with the ValidateIdentityCertificate implementation in
        // src\TransactionBridge\Microsoft\Transactions\Wsat\protocol\Configuration.cs
        internal static void ValidateIdentityCertificateThrow(X509Certificate2 cert, bool remoteCert)
        {            
            // I wish we had system-defined constants for these. We don't.
            const string KeyUsage = "2.5.29.15";
            const string EnhancedKeyUsage = "2.5.29.37";
            const string ClientAuthentication = "1.3.6.1.5.5.7.3.2";
            const string ServerAuthentication = "1.3.6.1.5.5.7.3.1";

            X509Certificate2 identity = cert;

            // 0) The certificate should be present
            if (identity == null)
            {
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_OR_MISSING_SSL_CERTIFICATE,
                                                              SR.GetString(SR.ErrorMissingSSLCert));
            }

            if (remoteCert)
            {
                return; // the following info is not accurate for remote cert, so we do not bother to check them
            }

            // 1) A certificate identity must have a private key
            if (!identity.HasPrivateKey)
            {
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_OR_MISSING_SSL_CERTIFICATE,
                                                                SR.GetString(SR.ErrorSSLCertHasNoPrivateKey));
            }

            // 2) A certificate identity must have an accessible private key
            try
            {
                // Yes, this property throws on error...
                AsymmetricAlgorithm privateKey = identity.PrivateKey;
            }
            catch (CryptographicException e)
            {
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_OR_MISSING_SSL_CERTIFICATE,
                                                                SR.GetString(SR.ErrorSSLCertCanNotAccessPrivateKey), e);
            }

            // 3) If a "Key Usage" extension is present, it must allow "Key Encipherment"
            // 4) If a "Key Usage" extension is present, it must allow "Digital Signature"
            X509KeyUsageExtension keyUsage = (X509KeyUsageExtension)identity.Extensions[KeyUsage];
            if (keyUsage != null)
            {
                const X509KeyUsageFlags required = X509KeyUsageFlags.KeyEncipherment |
                                                   X509KeyUsageFlags.DigitalSignature;

                if ((keyUsage.KeyUsages & required) != required)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.INVALID_OR_MISSING_SSL_CERTIFICATE,
                                                                    SR.GetString(SR.ErrorSSLCertDoesNotSupportKeyEnciphermentOrDsig));
                }
            }
            
            X509EnhancedKeyUsageExtension enhancedKeyUsage = (X509EnhancedKeyUsageExtension)identity.Extensions[EnhancedKeyUsage];
            if (enhancedKeyUsage != null)
            {
                // 5) If an "Enhanced Key Usage" extension is present, it must allow "Client Authentication"
                if (enhancedKeyUsage.EnhancedKeyUsages[ClientAuthentication] == null)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.INVALID_OR_MISSING_SSL_CERTIFICATE,
                                                                    SR.GetString(SR.ErrorSSLCertDoesNotSupportClientAuthentication));
                }

                // 6) If an "Enhanced Key Usage" extension is present, it must allow "Server Authentication"
                if (enhancedKeyUsage.EnhancedKeyUsages[ServerAuthentication] == null)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.INVALID_OR_MISSING_SSL_CERTIFICATE,
                                                                    SR.GetString(SR.ErrorSSLCertDoesNotSupportServerAuthentication));                }
            }
        }

        void UpdateClusterNodesPorts(bool restart)
        {
            if (clusterNodes != null)
            {
                foreach (string node in clusterNodes)
                {
                    if (Utilities.SafeCompare(node, Utilities.LocalHostName))
                    {
                        UpdatePorts();
                        UpdateCertificatePrivateKeyAccess();
                    }
                    else
                    {
                        //Explicitly not to restart DTC on remote cluster node, actually restart will be ignored anyway.
                        SaveRemote(node, false, true);
                    }
                }
            }
        }

        void RestartHelper(bool restart)
        {
            if (restart)
            {
                try
                {
                    MsdtcWrapper msdtc = this.GetMsdtcWrapper();
                    msdtc.RestartDtcService();
                }
                catch (WsatAdminException)
                {
                    throw;
                }
#pragma warning suppress 56500
                catch (Exception e)
                {
                    if (Utilities.IsCriticalException(e))
                    {
                        throw;
                    }
                    throw new WsatAdminException(WsatAdminErrorCode.DTC_RESTART_ERROR, SR.GetString(SR.ErrorRestartMSDTC), e);
                }
            }
        }

        internal void Save(bool restart)
        {
            if (IsLocalMachine)
            {
                Utilities.Log("Save - LocalMachine");
                // Single local machine:
                //   1. Update local SSL binding
                //   2. Update URL ACL
                //   3. Update firewall port status
                //   4. Update the endpoint cert's private key permission
                //   5. Save to local registry
                UpdatePorts();
                UpdateCertificatePrivateKeyAccess();
                SaveToRegistry();
                RestartHelper(restart);
            }
            else if (IsClusterRemoteNode)
            {
                Utilities.Log("Save - Cluster Remote Node");
                // Cluster remote node machine:
                //   DO NOT save to cluster registry, DO NOT restart DTC
                //   1. Update SSL binding on the node
                //   2. Update URL ACL on the node
                //   3. Update firewall port status on the node
                //   4. Update the endpoint cert's private key permission
                UpdatePorts();
                UpdateCertificatePrivateKeyAccess();
            }
            else if (IsClustered) // the orignating cluster node
            {
                Utilities.Log("Save - Cluster");
                // Cluster originating node machine:
                //   1. Update SSL binding on each node
                //   2. Update URL ACL on each node
                //   3. Update firewall port status on each remote node
                //   4. Update the endpoint cert's private key permission on each remote node
                //   5. Save to cluster registry
                UpdateClusterNodesPorts(restart);
                SaveToRegistry();
                RestartHelper(restart);
            }
            else
            {
                Utilities.Log("Save - Remote");
                // Remote machine:
                //   1. Save to remote registry
                //   2. Update SSL binding on remote machine
                //   3. Update URL ACL on remote machine
                //   4. Update firewall port status on remote machine
                //   5. Update the endpoint cert's private key permission on remote machine
                SaveRemote(restart, false);
            }
            CopyConfigurationData(this, previousConfig);
        }

        void SaveRemote(bool restart, bool clusterRemoteNode)
        {
            System.Diagnostics.Debug.Assert(!(IsLocalMachine || IsClustered));
            SaveRemote(MachineName, restart, clusterRemoteNode);
        }

        void SaveRemote(string machineName, bool restart, bool clusterRemoteNode)
        {
            string portString = null;
            string endpointCertString = null;
            string accountsString = null;
            string accountsCertsString = null;
            string defaultTimeoutString = null;
            string maxTimeoutString = null;
            string traceLevelString = null;
            string traceActivityString = null;
            string tracePropString = null;
            string tracePiiString = null;

            // Performance is not a concern here so we just do plain string concatenation
            string networkEnabledString = " -" + CommandLineOption.Network + ":" +
                            (this.TransactionBridgeEnabled ? CommandLineOption.Enable : CommandLineOption.Disable);
            string virtualServerString = null;
            if (!string.IsNullOrEmpty(VirtualServer))
            {
                virtualServerString = " -" + CommandLineOption.ClusterVirtualServer + ":" + "\"" + VirtualServer + "\"";
            }
            
            if (this.TransactionBridgeEnabled)
            {
                portString = " -" + CommandLineOption.Port + ":" + this.HttpsPort;
                endpointCertString = this.X509Certificate == null ? "" : " -" + CommandLineOption.EndpointCert + ":" + this.X509Certificate.Thumbprint;
                accountsString = " -" + CommandLineOption.Accounts + ":" + BuildAccountsArgument();
                accountsCertsString = " -" + CommandLineOption.AccountsCerts + ":" + BuildAccountsCertsArgument();
                defaultTimeoutString = " -" + CommandLineOption.DefaultTimeout + ":" + this.DefaultTimeout.ToString(CultureInfo.InvariantCulture);
                traceLevelString = " -" + CommandLineOption.TraceLevel + ":" + ((uint)this.TraceLevel).ToString(CultureInfo.InvariantCulture);
                traceActivityString = " -" + CommandLineOption.TraceActivity + ":" + (this.ActivityTracing ? CommandLineOption.Enable : CommandLineOption.Disable);
                tracePropString = " -" + CommandLineOption.TraceProp + ":" + (this.ActivityPropagation ? CommandLineOption.Enable : CommandLineOption.Disable);
                tracePiiString = " -" + CommandLineOption.TracePii + ":" + (this.TracePii ? CommandLineOption.Enable : CommandLineOption.Disable);
                maxTimeoutString = " -" + CommandLineOption.MaxTimeout + ":" + this.MaxTimeout.ToString(CultureInfo.InvariantCulture);
            }

            string arguments = networkEnabledString + virtualServerString + portString + endpointCertString + accountsString +
                                accountsCertsString + defaultTimeoutString + maxTimeoutString +
                                traceLevelString + traceActivityString + tracePropString + tracePiiString;

            if (clusterRemoteNode)
            {
                arguments += " -" + CommandLineOption.ClusterRemoteNode + ":" + CommandLineOption.Enable;
            }

            if (restart)
            {
                arguments += " -" + CommandLineOption.Restart;
            }

            Utilities.Log("Remote command arguments: " + arguments);
            
            RemoteHelper remote = new RemoteHelper(machineName);
            remote.ExecuteWsatProcess(arguments);
        }

        string BuildAccountsCertsArgument()
        {
            string result = string.Empty;
            if (this.X509GlobalAcl != null && this.X509GlobalAcl.Length > 0)
            {
                result += "\"" + this.X509GlobalAcl[0] + "\"";
                for (int i = 1; i < this.X509GlobalAcl.Length; ++i)
                {
                    result += ",\"" + this.X509GlobalAcl[i] + "\"";
                }
            }
            return result;
        }

        string BuildAccountsArgument()
        {
            string result = string.Empty;
            if (this.KerberosGlobalAcl != null && this.KerberosGlobalAcl.Length > 0)
            {
                result += "\"" + this.KerberosGlobalAcl[0] + "\"";
                for (int i = 1; i < this.KerberosGlobalAcl.Length; ++i)
                {
                    result += ",\"" + this.KerberosGlobalAcl[i] + "\"";
                }
            }
            return result;
        }

        void UpdateCertificatePrivateKeyAccess()
        {
            if (previousConfig == null)
            {
                AddCertificatePrivateKeyAccess(X509Certificate);
            }
            else if (X509Certificate != previousConfig.X509Certificate)
            {
                RemoveCertificatePrivateKeyAccess(previousConfig.X509Certificate);
                AddCertificatePrivateKeyAccess(X509Certificate);
            }
        }

        // This method could throw any exception, because RSACryptoServiceProvider ctor could do so
        // We will escalate the exceptions to the callers who will be more sensible on how to deal with them
        void CommitCryptoKeySecurity(CspKeyContainerInfo info, CryptoKeySecurity keySec)
        {
            CspParameters cspParams = new CspParameters(
                info.ProviderType, info.ProviderName,
                info.KeyContainerName);
            cspParams.CryptoKeySecurity = keySec;
            // Important flag, or the security setting will silently fail
            cspParams.Flags = CspProviderFlags.UseMachineKeyStore;

            // The RSACryptoServiceProvider ctor will automatically apply DACLs set in CSP's securtiy info 
            new RSACryptoServiceProvider(cspParams);
        }

        void RemoveCertificatePrivateKeyAccess(X509Certificate2 cert)
        {
            if (cert != null && cert.HasPrivateKey)
            {
                try
                {
                    AsymmetricAlgorithm key = cert.PrivateKey;

                    // Only RSA provider is supported here
                    if (key is RSACryptoServiceProvider)
                    {
                        RSACryptoServiceProvider prov = key as RSACryptoServiceProvider;
                        CspKeyContainerInfo info = prov.CspKeyContainerInfo;
                        CryptoKeySecurity keySec = info.CryptoKeySecurity;

                        SecurityIdentifier ns = new SecurityIdentifier(WellKnownSidType.NetworkServiceSid, null);
                        AuthorizationRuleCollection rules = keySec.GetAccessRules(true, false, typeof(SecurityIdentifier));
                        foreach (AuthorizationRule rule in rules)
                        {
                            CryptoKeyAccessRule keyAccessRule = (CryptoKeyAccessRule)rule;

                            if (keyAccessRule.AccessControlType == AccessControlType.Allow &&
                                (int)(keyAccessRule.CryptoKeyRights & CryptoKeyRights.GenericRead) != 0)
                            {
                                SecurityIdentifier sid = keyAccessRule.IdentityReference as SecurityIdentifier;
                                if (ns.Equals(sid))
                                {
                                    CryptoKeyAccessRule nsReadRule = new CryptoKeyAccessRule(ns,
                                            CryptoKeyRights.GenericRead,
                                            AccessControlType.Allow);
                                    keySec.RemoveAccessRule(nsReadRule);

                                    CommitCryptoKeySecurity(info, keySec);
                                    break;
                                }
                            }
                        }
                    }
                }
#pragma warning suppress 56500
                catch (Exception e)
                {
                    // CommitCryptoKeySecurity can actually throw any exception,
                    // so the safest way here is to catch a generic exception while throw on critical ones
                    if (Utilities.IsCriticalException(e))
                    {
                        throw;
                    }
                    throw new WsatAdminException(WsatAdminErrorCode.CANNOT_UPDATE_PRIVATE_KEY_PERM,
                                           SR.GetString(SR.ErrorUpdateCertPrivateKeyPerm), e);
                }
            }
        }

        void AddCertificatePrivateKeyAccess(X509Certificate2 cert)
        {
            if (cert != null && cert.HasPrivateKey)
            {
                try
                {
                    AsymmetricAlgorithm key = cert.PrivateKey;

                    // Only RSA provider is supported here
                    if (key is RSACryptoServiceProvider)
                    {
                        RSACryptoServiceProvider prov = key as RSACryptoServiceProvider;
                        CspKeyContainerInfo info = prov.CspKeyContainerInfo;
                        CryptoKeySecurity keySec = info.CryptoKeySecurity;

                        SecurityIdentifier ns = new SecurityIdentifier(WellKnownSidType.NetworkServiceSid, null);
                        // Just add a rule, exisitng settings will be merged
                        CryptoKeyAccessRule rule = new CryptoKeyAccessRule(ns,
                                    CryptoKeyRights.GenericRead,
                                    AccessControlType.Allow);
                        keySec.AddAccessRule(rule);

                        CommitCryptoKeySecurity(info, keySec);
                    }
                }
#pragma warning suppress 56500
                catch (Exception e)
                {
                    // CommitCryptoKeySecurity can actually throw any exception,
                    // so the safest way here is to catch a generic exception while throw on critical ones
                    if (Utilities.IsCriticalException(e))
                    {
                        throw;
                    }
                    throw new WsatAdminException(WsatAdminErrorCode.CANNOT_UPDATE_PRIVATE_KEY_PERM,
                                           SR.GetString(SR.ErrorUpdateCertPrivateKeyPerm), e);
                }

            }
        }

        void SaveToRegistry()
        {
            if (!this.minimalWrite || this.previousConfig == null || this.TransactionBridgeEnabled != this.previousConfig.TransactionBridgeEnabled || (this.previousConfig.TransactionBridge30Enabled && !this.TransactionBridgeEnabled) )
            {
                msdtcConfigProvider.WriteString(
                    WsatKeys.TransactionBridgeRegKey,
                    this.TransactionBridgeEnabled ? TransactionBridgeRegistryValue : string.Empty);
            }

            if (!this.minimalWrite || this.previousConfig == null || this.TraceLevel != this.previousConfig.TraceLevel)
            {
                wsatConfigProvider.WriteUInt32(WsatKeys.RegistryEntryTraceLevel, (uint)this.TraceLevel);
            }

            if (!this.minimalWrite || this.previousConfig == null || this.ActivityTracing != this.previousConfig.ActivityTracing)
            {
                wsatConfigProvider.WriteUInt32(WsatKeys.RegistryEntryActivityTracing, (this.ActivityTracing ? 1u : 0));
            }

            if (!this.minimalWrite || this.previousConfig == null || this.ActivityPropagation != this.previousConfig.ActivityPropagation)
            {
                wsatConfigProvider.WriteUInt32(WsatKeys.RegistryEntryPropagateActivity, (this.ActivityPropagation ? 1u : 0));
            }

            if (!this.minimalWrite || this.previousConfig == null || this.TracePii != this.previousConfig.TracePii)
            {
                wsatConfigProvider.WriteUInt32(WsatKeys.RegistryEntryTracingPii, (this.TracePii ? 1u : 0));
            }

            if (!this.minimalWrite || this.previousConfig == null || this.DefaultTimeout != this.previousConfig.DefaultTimeout)
            {
                wsatConfigProvider.WriteUInt32(WsatKeys.RegistryEntryDefTimeout, (uint)this.DefaultTimeout);
            }

            if (!this.minimalWrite || this.previousConfig == null || this.MaxTimeout != this.previousConfig.MaxTimeout)
            {
                wsatConfigProvider.WriteUInt32(WsatKeys.RegistryEntryMaxTimeout, (uint)this.MaxTimeout);
            }

            if (!this.minimalWrite || this.previousConfig == null || this.X509GlobalAcl != this.previousConfig.X509GlobalAcl)
            {
                wsatConfigProvider.WriteMultiString(WsatKeys.RegistryEntryX509GlobalAcl, this.X509GlobalAcl);
            }

            if (!this.minimalWrite || this.previousConfig == null || this.X509Certificate != this.previousConfig.X509Certificate)
            {
                wsatConfigProvider.WriteString(WsatKeys.RegistryEntryX509CertificateIdentity, (this.X509Certificate == null ? string.Empty : this.X509Certificate.Thumbprint));
            }

            if (!this.minimalWrite || this.previousConfig == null || this.HttpsPort != this.previousConfig.HttpsPort)
            {
                wsatConfigProvider.WriteUInt32(WsatKeys.RegistryEntryHttpsPort, this.HttpsPort);
            }

            if (!this.minimalWrite || this.previousConfig == null || this.KerberosGlobalAcl != this.previousConfig.KerberosGlobalAcl)
            {
                wsatConfigProvider.WriteMultiString(WsatKeys.RegistryEntryKerberosGlobalAcl, this.KerberosGlobalAcl);
            }

            if (IsClustered || IsLocalMachine)
            {
                wsatConfigProvider.AdjustRegKeyPermission();
            }
        }

        void UpdateUrlAclReservation()
        {
            WsatServiceAddress wsatServiceAddress;

            if (this.previousConfig != null)
            {
                wsatServiceAddress = new WsatServiceAddress(this.previousConfig.HttpsPort);
                wsatServiceAddress.FreeWsatServiceAddress();
            }

            if (this.TransactionBridgeEnabled)
            {
                wsatServiceAddress = new WsatServiceAddress(this.HttpsPort);
                wsatServiceAddress.ReserveWsatServiceAddress();
            }
        }

        void UpdateSSLBinding()
        {
            WsatServiceCertificate wsatServiceCertificate;

            if (this.previousConfig != null && this.previousConfig.X509Certificate != null)
            {
                wsatServiceCertificate = new WsatServiceCertificate(this.previousConfig.X509Certificate, previousConfig.HttpsPort);
                wsatServiceCertificate.UnbindSSLCertificate();
            }
            if (this.TransactionBridgeEnabled && this.X509Certificate != null)
            {
                wsatServiceCertificate = new WsatServiceCertificate(this.X509Certificate, HttpsPort);
                wsatServiceCertificate.BindSSLCertificate();
            }
        }

        void UpdateFirewallPort()
        {
            FirewallWrapper firewallWrapper = new FirewallWrapper();
            firewallWrapper.RemoveHttpsPort((int)this.previousConfig.HttpsPort);

            if (this.TransactionBridgeEnabled)
            {
                firewallWrapper.AddHttpsPort((int)this.HttpsPort);
            }
        }

        // update the ports for the SSL Binding, URL ACL Reservation and Firewall
        void UpdatePorts()
        {
            UpdateFirewallPort();
            UpdateUrlAclReservation();
            UpdateSSLBinding();
        }

#if WSAT_CMDLINE
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(SR.GetString(SR.ConfigNetworkSupport, Utilities.GetEnabledStatusString(this.TransactionBridgeEnabled || this.TransactionBridge30Enabled)));

            if (this.TransactionBridgeEnabled || this.TransactionBridge30Enabled)
            {
                sb.Append(SR.GetString(SR.ConfigHTTPSPort, this.HttpsPort));
                sb.Append(SR.GetString(SR.ConfigIdentityCertificate,
                                       this.X509Certificate == null ? SR.GetString(SR.ConfigNone) : this.X509Certificate.Thumbprint));
                sb.Append(SR.GetString(SR.ConfigKerberosGACL));
                if (this.KerberosGlobalAcl == null || this.KerberosGlobalAcl.Length < 1)
                {
                    sb.AppendLine(SR.GetString(SR.ConfigNone));
                }
                else
                {
                    int i = 0;
                    foreach (string ace in this.KerberosGlobalAcl)
                    {
                        if (i++ > 0)
                        {
                            sb.Append(SR.GetString(SR.ConfigACEPrefix));
                        }
                        sb.AppendLine(ace);
                    }
                }

                sb.Append(SR.GetString(SR.ConfigAcceptedCertificates));
                if (this.X509GlobalAcl == null || this.X509GlobalAcl.Length < 1)
                {
                    sb.AppendLine(SR.GetString(SR.ConfigNone));
                }
                else
                {
                    int i = 0;
                    foreach (string cert in this.X509GlobalAcl)
                    {
                        if (i++ > 0)
                        {
                            sb.Append(SR.GetString(SR.ConfigAcceptedCertPrefix));
                        }
                        sb.AppendLine(cert);
                    }
                }

                sb.Append(SR.GetString(SR.ConfigDefaultTimeout, this.DefaultTimeout));
                sb.Append(SR.GetString(SR.ConfigMaximumTimeout, this.MaxTimeout));

                SourceLevels level = this.TraceLevel;
                if (level != SourceLevels.All)
                {
                    level = level & ~SourceLevels.ActivityTracing;
                }
                sb.Append(SR.GetString(SR.ConfigTraceLevel, level));
                sb.Append(SR.GetString(SR.ConfigActivityTracing, Utilities.GetEnabledStatusString(this.ActivityTracing)));
                sb.Append(SR.GetString(SR.ConfigActivityProp, Utilities.GetEnabledStatusString(this.ActivityPropagation)));
                sb.Append(SR.GetString(SR.ConfigPiiTracing, Utilities.GetEnabledStatusString(this.TracePii)));

                MsdtcWrapper msdtc = this.GetMsdtcWrapper();
                if (!msdtc.GetNetworkTransactionAccess())
                {
                    sb.Append(Environment.NewLine);
                    sb.Append(SR.GetString(SR.ConfigWarningNetworkDTCAccessIsDisabled));
                }
            }
            else
            {
                // When network support is disabled, the only setting that still matters is the MaxTimeout
                sb.Append(SR.GetString(SR.ConfigMaximumTimeoutWhenNetworkDisabled, this.MaxTimeout));
            }

            return sb.ToString();
        }
#endif
    }
}
