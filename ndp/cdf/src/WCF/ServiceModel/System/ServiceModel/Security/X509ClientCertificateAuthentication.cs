//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Security
{
    using System.IdentityModel.Selectors;
    using System.ServiceModel;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using Text;
    using IdentityModel.Tokens;

    public class X509ClientCertificateAuthentication
    {
        internal const X509CertificateValidationMode DefaultCertificateValidationMode = X509CertificateValidationMode.ChainTrust;
        internal const X509RevocationMode DefaultRevocationMode = X509RevocationMode.Online;
        internal const StoreLocation DefaultTrustedStoreLocation = StoreLocation.LocalMachine;
        internal const bool DefaultMapCertificateToWindowsAccount = false;
        static X509CertificateValidator defaultCertificateValidator;

        X509CertificateValidationMode certificateValidationMode = DefaultCertificateValidationMode;
        X509RevocationMode revocationMode = DefaultRevocationMode;
        StoreLocation trustedStoreLocation = DefaultTrustedStoreLocation;
        X509CertificateValidator customCertificateValidator = null;
        bool mapClientCertificateToWindowsAccount = DefaultMapCertificateToWindowsAccount;
        bool includeWindowsGroups = SspiSecurityTokenProvider.DefaultExtractWindowsGroupClaims;
        bool isReadOnly;

        internal X509ClientCertificateAuthentication()
        {
        }

        internal X509ClientCertificateAuthentication(X509ClientCertificateAuthentication other)
        {
            this.certificateValidationMode = other.certificateValidationMode;
            this.customCertificateValidator = other.customCertificateValidator;
            this.includeWindowsGroups = other.includeWindowsGroups;
            this.mapClientCertificateToWindowsAccount = other.mapClientCertificateToWindowsAccount;
            this.trustedStoreLocation = other.trustedStoreLocation;
            this.revocationMode = other.revocationMode;
            this.isReadOnly = other.isReadOnly;
        }

        internal static X509CertificateValidator DefaultCertificateValidator
        {
            get
            {
                if (defaultCertificateValidator == null)
                {
                    bool useMachineContext = DefaultTrustedStoreLocation == StoreLocation.LocalMachine;
                    X509ChainPolicy chainPolicy = new X509ChainPolicy();
                    chainPolicy.RevocationMode = DefaultRevocationMode;

                    if (!ServiceModelAppSettings.UseLegacyCertificateUsagePolicy)
                    {
                        defaultCertificateValidator = new ClientChainTrustValidator(useMachineContext, chainPolicy);
                    }
                    else
                    {
                        defaultCertificateValidator = X509CertificateValidator.CreateChainTrustValidator(useMachineContext, chainPolicy);
                    }
                }

                return defaultCertificateValidator;
            }
        }

        public X509CertificateValidationMode CertificateValidationMode 
        { 
            get 
            { 
                return this.certificateValidationMode; 
            }
            set 
            {
                X509CertificateValidationModeHelper.Validate(value);
                ThrowIfImmutable();
                this.certificateValidationMode = value; 
            }
        }

        public X509RevocationMode RevocationMode 
        {
            get 
            { 
                return this.revocationMode; 
            }
            set 
            {
                ThrowIfImmutable();
                this.revocationMode = value; 
            }
        }

        public StoreLocation TrustedStoreLocation
        {
            get 
            { 
                return this.trustedStoreLocation; 
            }
            set 
            {
                ThrowIfImmutable();
                this.trustedStoreLocation = value; 
            }
        }

        public X509CertificateValidator CustomCertificateValidator
        {
            get
            {
                return this.customCertificateValidator;
            }
            set
            {
                ThrowIfImmutable();
                this.customCertificateValidator = value;
            }
        }

        public bool MapClientCertificateToWindowsAccount
        {
            get
            {
                return this.mapClientCertificateToWindowsAccount;
            }
            set
            {
                ThrowIfImmutable();
                this.mapClientCertificateToWindowsAccount = value;
            }
        }

        public bool IncludeWindowsGroups
        {
            get
            {
                return this.includeWindowsGroups;
            }
            set
            {
                ThrowIfImmutable();
                this.includeWindowsGroups = value;
            }
        }

        internal X509CertificateValidator GetCertificateValidator()
        {
            if (this.certificateValidationMode == X509CertificateValidationMode.None)
            {
                return X509CertificateValidator.None;
            }
            else if (this.certificateValidationMode == X509CertificateValidationMode.PeerTrust)
            {
                return X509CertificateValidator.PeerTrust;
            }
            else if (this.certificateValidationMode == X509CertificateValidationMode.Custom)
            {
                if (this.customCertificateValidator == null)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new InvalidOperationException(SR.GetString(SR.MissingCustomCertificateValidator)));
                }
                return this.customCertificateValidator;
            }
            else
            {
                bool useMachineContext = this.trustedStoreLocation == StoreLocation.LocalMachine;
                X509ChainPolicy chainPolicy = new X509ChainPolicy();
                chainPolicy.RevocationMode = this.revocationMode;

                if (!ServiceModelAppSettings.UseLegacyCertificateUsagePolicy)
                {
                    if (this.certificateValidationMode == X509CertificateValidationMode.ChainTrust)
                    {
                        return new ClientChainTrustValidator(useMachineContext, chainPolicy);
                    }

                    return new ClientPeerOrChainTrustValidator(useMachineContext, chainPolicy);
                }
                else
                {
                    if (this.certificateValidationMode == X509CertificateValidationMode.ChainTrust)
                    {
                        return X509CertificateValidator.CreateChainTrustValidator(useMachineContext, chainPolicy);
                    }

                    return X509CertificateValidator.CreatePeerOrChainTrustValidator(useMachineContext, chainPolicy);
                }
            }
        }

        internal void MakeReadOnly()
        {
            this.isReadOnly = true;
        }

        void ThrowIfImmutable()
        {
            if (this.isReadOnly)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new InvalidOperationException(SR.GetString(SR.ObjectIsReadOnly)));
            }
        }

        // Code based off from IdentityModel's X509CertificateValidator.CreateChainTrustValidator
        private class ClientChainTrustValidator : X509CertificateValidator
        {
            bool useMachineContext;
            X509ChainPolicy chainPolicy;
            static readonly X509ChainPolicy OidChainPolicy;

            static ClientChainTrustValidator()
            {
                // ASN.1 description: {iso(1) identified-organization(3) dod(6) internet(1) security(5) mechanisms(5) pkix(7) kp(3) clientAuth(2)}
                Oid clientAuthOid = new Oid("1.3.6.1.5.5.7.3.2", "1.3.6.1.5.5.7.3.2");

                X509ChainPolicy policy = new X509ChainPolicy();
                policy.ApplicationPolicy.Add(clientAuthOid);
                policy.RevocationMode = X509RevocationMode.NoCheck;
                OidChainPolicy = policy;
            }

            public ClientChainTrustValidator(bool useMachineContext, X509ChainPolicy chainPolicy)
            {
                if (chainPolicy == null)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperArgumentNull("chainPolicy");
                }

                this.useMachineContext = useMachineContext;
                this.chainPolicy = chainPolicy;
            }

            public override void Validate(X509Certificate2 certificate)
            {
                if (certificate == null)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperArgumentNull("certificate");
                }

                Exception exception;

                if (!TryValidate(certificate, out exception))
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(exception);
                }
            }

            internal bool TryValidate(X509Certificate2 certificate, out Exception exception)
            {
                using (X509Chain chain = new X509Chain(this.useMachineContext))
                {                    
                    chain.ChainPolicy = this.chainPolicy;
                    chain.ChainPolicy.VerificationTime = DateTime.Now;

                    if (!chain.Build(certificate))
                    {
                        exception = new SecurityTokenValidationException(SR.GetString(SR.X509ChainBuildFail,
                            SecurityUtils.GetCertificateId(certificate), GetChainStatusInformation(chain.ChainStatus)));
                        return false;
                    }

                    if (chain.ChainElements.Count > 1)  //is not self-signed
                    {
                        chain.ChainPolicy = OidChainPolicy;
                        chain.ChainPolicy.VerificationTime = DateTime.Now;

                        X509Certificate2 cert = chain.ChainElements[1].Certificate;

                        if (!chain.Build(cert))
                        {
                            exception = new SecurityTokenValidationException(SR.GetString(SR.X509ChainBuildFail,
                                SecurityUtils.GetCertificateId(certificate), GetChainStatusInformation(chain.ChainStatus)));
                            return false;
                        }
                    }

                    exception = null;
                    return true;
                }
            }

            static string GetChainStatusInformation(X509ChainStatus[] chainStatus)
            {
                if (chainStatus != null)
                {
                    StringBuilder error = new StringBuilder(128);
                    for (int i = 0; i < chainStatus.Length; ++i)
                    {
                        error.Append(chainStatus[i].StatusInformation);
                        error.Append(" ");
                    }
                    return error.ToString();
                }
                return String.Empty;
            }
        }

        // Code based off from IdentityModel's X509CertificateValidator.CreatePeerOrChainTrustValidator
        private class ClientPeerOrChainTrustValidator : X509CertificateValidator
        {
            ClientChainTrustValidator chain;
            X509CertificateValidator peer;

            public ClientPeerOrChainTrustValidator(bool useMachineContext, X509ChainPolicy chainPolicy)
            {
                this.chain = new ClientChainTrustValidator(useMachineContext, chainPolicy);
                this.peer = X509CertificateValidator.PeerTrust;
            }

            public override void Validate(X509Certificate2 certificate)
            {
                if (certificate == null)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperArgumentNull("certificate");
                }

                Exception exception;
                if (this.chain.TryValidate(certificate, out exception))
                {
                    return;
                }

                try
                {
                    this.peer.Validate(certificate);
                }
                catch (SecurityTokenValidationException ex)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new SecurityTokenValidationException(ex.Message + " " + exception.Message));
                }
            }
        }
    }
}
