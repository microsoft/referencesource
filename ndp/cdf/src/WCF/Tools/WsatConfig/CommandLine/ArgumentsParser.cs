//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using System.Reflection;
    using System.Diagnostics;
    using System.Security.Principal;
    using System.Security.Cryptography.X509Certificates;

    class ArgumentsParser
    {
        Dictionary<string, ParserBase> parsersTable = new Dictionary<string, ParserBase>();
        Dictionary<string, string> optionsLookupTable = new Dictionary<string, string>();

        Dictionary<string, int> optionsHitCount = new Dictionary<string, int>();
        WsatConfiguration config;

        public ArgumentsParser(WsatConfiguration config)
        {
            this.config = config;
            InitializeParsers();
        }

        public bool ParseOptionAndArgument(string optionAndArgument)
        {
            string value;
            string lowerCaseOption = ExtractOption(optionAndArgument, out value);
            if (!string.IsNullOrEmpty(lowerCaseOption) && optionsLookupTable.ContainsKey(lowerCaseOption))
            {
                string normalizedOption = optionsLookupTable[lowerCaseOption];
                ParserBase parser = parsersTable[normalizedOption];
                parser.Parse(value, config);
                UpdateOptionHitCount(normalizedOption);
                return true;
            }
            return false;
        }

        public void ValidateArgumentsThrow()
        {
            if (optionsHitCount.Count >= 1)
            {
                foreach (string option in optionsHitCount.Keys)
                {
                    if (optionsHitCount[option] > 1)
                    {
                        throw new WsatAdminException(WsatAdminErrorCode.DUPLICATE_OPTION, SR.GetString(SR.ErrorDuplicateOption, option));
                    }
                }

                if (!config.TransactionBridgeEnabled)
                {
                    int optionsAllowed = 0;
                    if (optionsHitCount.ContainsKey(CommandLineOption.Network))
                    {
                        ++optionsAllowed;
                    }

                    if (optionsHitCount.ContainsKey(CommandLineOption.ClusterRemoteNode))
                    {
                        ++optionsAllowed;
                    }

                    if (optionsHitCount.ContainsKey(CommandLineOption.Restart))
                    {
                        ++optionsAllowed;
                    }

                    if (optionsHitCount.Count > optionsAllowed)
                    {
                        throw new WsatAdminException(WsatAdminErrorCode.CANNOT_UPDATE_SETTINGS_WHEN_NETWORK_IS_DISABLED, SR.GetString(SR.ErrorUpdateSettingsWhenNetworkIsDisabled));
                    }
                }
            }
        }

        void UpdateOptionHitCount(string option)
        {
            int hitCount = 0;
            if (optionsHitCount.TryGetValue(option, out hitCount))
            {
                optionsHitCount[option] = hitCount + 1;
            }
            else
            {
                optionsHitCount.Add(option, 1);
            }
        }

        static public string ExtractOption(string arg, out string value)
        {
            value = null;
            if (!string.IsNullOrEmpty(arg))
            {
                int separator = arg.IndexOf(':');
                if (separator != -1)
                {
                    value = arg.Substring(separator + 1);
                    return arg.Substring(0, separator).Trim().ToLowerInvariant();
                }
            }
            return string.Empty;
        }

        void InitializeParsers()
        {
            AddOptionParserPair(CommandLineOption.Network, new NetworkParser());
            AddOptionParserPair(CommandLineOption.Port, new PortParser());
            AddOptionParserPair(CommandLineOption.MaxTimeout, new MaxTimeoutParser());
            AddOptionParserPair(CommandLineOption.DefaultTimeout, new DefaultTimeoutParser());
            AddOptionParserPair(CommandLineOption.TraceLevel, new TraceLevelParser());
            AddOptionParserPair(CommandLineOption.TraceActivity, new TraceActivityParser());
            AddOptionParserPair(CommandLineOption.TraceProp, new TracePropagationParser());
            AddOptionParserPair(CommandLineOption.TracePii, new TracePiiParser());
            AddOptionParserPair(CommandLineOption.EndpointCert, new EndpointCertificateParser());
            AddOptionParserPair(CommandLineOption.Accounts, new AccountsParser());
            AddOptionParserPair(CommandLineOption.ClusterRemoteNode, new ClusterRemoteNodeParser());
            AddOptionParserPair(CommandLineOption.AccountsCerts, new AccountsCertificatesParser());
        }

        void AddOptionParserPair(string standardOption, ParserBase parser)
        {
            optionsLookupTable.Add(standardOption.ToLowerInvariant(), standardOption);
            parsersTable.Add(standardOption, parser);
        }

        abstract class ParserBase
        {
            abstract protected void DoParse(string value, WsatConfiguration config);

            public void Parse(string value, WsatConfiguration config)
            {
                Debug.Assert(config != null, "config is null");

                DoParse(value, config);
            }
        }

        class NetworkParser : ParserBase
        {
            // -network:{enable|disable}
            protected override void DoParse(string value, WsatConfiguration config)
            {
                if (!String.IsNullOrEmpty(value))
                {
                    if (Utilities.SafeCompare(value, CommandLineOption.Enable))
                    {
                        QfeChecker.CheckQfe();
                        config.TransactionBridgeEnabled = true;
                        return;
                    }
                    else if (Utilities.SafeCompare(value, CommandLineOption.Disable))
                    {
                        config.TransactionBridgeEnabled = false;
                        return;
                    }
                }
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_NETWORK_ARGUMENT, SR.GetString(SR.ErrorNetworkArgument));
            }
        }

        class PortParser : ParserBase
        {
            // -port:<portNum>
            protected override void DoParse(string value, WsatConfiguration config)
            {
                if (!String.IsNullOrEmpty(value))
                {
                    UInt16 parsedValue;
                    if (UInt16.TryParse(value, out parsedValue))
                    {
                        config.HttpsPort = parsedValue;
                        return;
                    }
                }
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_HTTPS_PORT, SR.GetString(SR.ErrorHttpsPortRange));
            }
        }

        class MaxTimeoutParser : ParserBase
        {
            // -maxTimeout:<sec>
            override protected void DoParse(string value, WsatConfiguration config)
            {
                if (!String.IsNullOrEmpty(value))
                {
                    UInt16 parsedValue;
                    if (UInt16.TryParse(value, out parsedValue))
                    {
                        config.MaxTimeout = parsedValue;
                        return;
                    }
                }
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_MAXTIMEOUT_ARGUMENT, SR.GetString(SR.ErrorMaximumTimeoutRange));
            }
        }

        class DefaultTimeoutParser : ParserBase
        {
            // -timeout:<sec>
            override protected void DoParse(string value, WsatConfiguration config)
            {
                if (!String.IsNullOrEmpty(value))
                {
                    UInt16 parsedValue;
                    if (UInt16.TryParse(value, out parsedValue))
                    {
                        config.DefaultTimeout = parsedValue;
                        return;
                    }
                }
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_DEFTIMEOUT_ARGUMENT, SR.GetString(SR.ErrorDefaultTimeoutRange));
            }
        }

        // -traceLevel:{Off|Error|Critical|Warning|Information|Verbose|All}
        class TraceLevelParser : ParserBase
        {
            override protected void DoParse(string value, WsatConfiguration config)
            {
                if (!String.IsNullOrEmpty(value))
                {
                    SourceLevels level;
                    bool parsedCorrectly = Utilities.ParseSourceLevel(value, out level);
                    if (parsedCorrectly)
                    {
                        config.TraceLevel = level;
                        return;
                    }
                    else
                    {
                        // check to see if it's a number
                        uint temp;
                        if (UInt32.TryParse(value, out temp))
                        {
                            config.TraceLevel = (SourceLevels)temp;
                            return;
                        }
                    }
                }
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_TRACELEVEL_ARGUMENT,
                                    SR.GetString(SR.ErrorTraceLevelArgument));
            }
        }

        class TraceActivityParser : ParserBase
        {
            // -traceActivity:{enable|disable}
            protected override void DoParse(string value, WsatConfiguration config)
            {
                if (!String.IsNullOrEmpty(value))
                {
                    if (Utilities.SafeCompare(value, CommandLineOption.Enable))
                    {
                        config.ActivityTracing = true;
                        return;
                    }
                    else if (Utilities.SafeCompare(value, CommandLineOption.Disable))
                    {
                        config.ActivityTracing = false;
                        return;
                    }
                }
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_TRACE_ACTIVITY_ARGUMENT,
                                                    SR.GetString(SR.ErrorTraceActivityArgument));
            }
        }

        class TracePropagationParser : ParserBase
        {
            // -traceProp:{enable|disable}
            protected override void DoParse(string value, WsatConfiguration config)
            {
                if (!String.IsNullOrEmpty(value))
                {
                    if (Utilities.SafeCompare(value, CommandLineOption.Enable))
                    {
                        config.ActivityPropagation = true;
                        return;
                    }
                    else if (Utilities.SafeCompare(value, CommandLineOption.Disable))
                    {
                        config.ActivityPropagation = false;
                        return;
                    }
                }
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_TRACE_PROPAGATION_ARGUMENT,
                                        SR.GetString(SR.ErrorTracePropArgument));
            }
        }

        class TracePiiParser : ParserBase
        {
            // -tracePII:{enable|disable}
            protected override void DoParse(string value, WsatConfiguration config)
            {
                if (!String.IsNullOrEmpty(value))
                {
                    if (Utilities.SafeCompare(value, CommandLineOption.Enable))
                    {
                        config.TracePii = true;
                        return;
                    }
                    else if (Utilities.SafeCompare(value, CommandLineOption.Disable))
                    {
                        config.TracePii = false;
                        return;
                    }
                }
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_TRACE_PII_ARGUMENT,
                                    SR.GetString(SR.ErrorTracePiiArgument));
            }
        }

        // -endpointCert:{machine|<thumb>|"Issuer\SubjectName"}
        class EndpointCertificateParser : ParserBase
        {
            protected override void DoParse(string value, WsatConfiguration config)
            {
                if (!String.IsNullOrEmpty(value))
                {
                    if (Utilities.SafeCompare(value, CommandLineOption.CertMachine))
                    {
                        config.X509Certificate = CertificateManager.GetMachineIdentityCertificate();
                        if (config.X509Certificate == null)
                        {
                            throw new WsatAdminException(WsatAdminErrorCode.CANNOT_FIND_MACHINE_CERTIFICATE,
                                                            SR.GetString(SR.ErrorCannotFindMachineCertificate));
                        }
                    }
                    else if (value.IndexOf('\\') >= 0) // "Issuer\SubjectName" e.g. "*\This is a distinguished subject"
                    {
                        config.X509Certificate = CertificateManager.GetCertificateFromIssuerAndSubjectName(value);
                    }
                    else // thumbprint
                    {
                        config.X509Certificate = CertificateManager.GetCertificateFromThumbprint(value, string.Empty);
                    }

                    if (config.X509Certificate != null)
                    {
                        return;
                    }
                }
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_OR_MISSING_SSL_CERTIFICATE,
                                    SR.GetString(SR.ErrorMissingSSLCert));
            }
        }

        // -accounts:<account,>
        class AccountsParser : ParserBase
        {
            protected override void DoParse(string value, WsatConfiguration config)
            {
                // Empty value is allowed and means then users want to remove all accounts
                config.KerberosGlobalAcl = value.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
                List<string> acceptedAccounts = new List<string>(config.KerberosGlobalAcl);

                List<string> validAccounts = new List<string>();
                foreach (string account in acceptedAccounts)
                {
                    string normalizedAccount = account.Trim();
                    if (!string.IsNullOrEmpty(normalizedAccount))
                    {
                        if (IsValidAccount(normalizedAccount))
                        {
                            validAccounts.Add(normalizedAccount);
                        }
                        else
                        {
                            throw new WsatAdminException(WsatAdminErrorCode.INVALID_ACCOUNT, SR.GetString(SR.ErrorAccountArgument, normalizedAccount));
                        }
                    }
                }
                config.KerberosGlobalAcl = validAccounts.ToArray();
            }

            bool IsValidAccount(string account)
            {
                bool isValid = false;
                try
                {
                    NTAccount ntAccount = new NTAccount(account);
                    IdentityReference identityRef = ntAccount.Translate(typeof(SecurityIdentifier));
                    isValid = identityRef != null;
                }
                catch (IdentityNotMappedException)
                {
                }
                catch (Exception e)
                {
                    if (Utilities.IsCriticalException(e))
                    {
                        throw;
                    }
                }

                return isValid;
            }
        }

        class ClusterRemoteNodeParser : ParserBase
        {
            // private switch: -clusterRemoteNode
            protected override void DoParse(string value, WsatConfiguration config)
            {
                if (!String.IsNullOrEmpty(value))
                {
                    if (Utilities.SafeCompare(value, CommandLineOption.Enable))
                    {
                        config.IsClusterRemoteNode = true;
                        return;
                    }
                    else if (Utilities.SafeCompare(value, CommandLineOption.Disable))
                    {
                        config.IsClusterRemoteNode = false;
                        return;
                    }
                }
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_CLUSTER_REMOTE_NODE_ARGUMENT, SR.GetString(SR.ErrorClusterRemoteNodeArgument));
            }
        }

        class AccountsCertificatesParser : ParserBase
        {
            // -accountsCerts:<base64hash|"Issuer\SubjectName",>
            protected override void DoParse(string value, WsatConfiguration config)
            {
                // Empty value is allowed and means then users want to remove all accounts certs
                config.X509GlobalAcl = value.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
                // newConfig.X509GlobalAcl can be empty, but never null

                int i = 0;
                foreach (string certString in config.X509GlobalAcl)
                {
                    if (!string.IsNullOrEmpty(certString))
                    {
                        X509Certificate2 cert = null;
                        if (certString.IndexOf('\\') >= 0)
                        {
                            // issuer\subject
                            cert = CertificateManager.GetCertificateFromIssuerAndSubjectName(certString);
                        }
                        else
                        {
                            // thumbprint
                            cert = CertificateManager.GetCertificateFromThumbprint(certString, string.Empty);
                        }

                        if (cert != null)
                        {
                            config.X509GlobalAcl[i] = cert.Thumbprint;
                        }
                        else
                        {
                            throw new WsatAdminException(WsatAdminErrorCode.INVALID_OR_MISSING_CLIENT_CERTIFICATE,
                                                            SR.GetString(SR.ErrorInvalidOrMissingClientCert));
                        }
                    }
                    i++;
                }
            }
        }
    }
}
