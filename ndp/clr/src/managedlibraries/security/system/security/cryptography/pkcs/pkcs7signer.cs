// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

//
// Pkcs7Signer.cs
//

namespace System.Security.Cryptography.Pkcs {
    using System.Globalization;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class CmsSigner {
        private SubjectIdentifierType            m_signerIdentifierType;
        private X509Certificate2                m_certificate;
        private Oid                              m_digestAlgorithm;
        private CryptographicAttributeObjectCollection m_signedAttributes;
        private CryptographicAttributeObjectCollection m_unsignedAttributes;
        private X509Certificate2Collection      m_certificates;
        private X509IncludeOption                m_includeOption;
        private bool                             m_dummyCert;
        private const string Sha256Oid = "2.16.840.1.101.3.4.2.1";

        //
        // Constructors.
        //

        public CmsSigner () : this(SubjectIdentifierType.IssuerAndSerialNumber, null) {}

        public CmsSigner (SubjectIdentifierType signerIdentifierType) : this (signerIdentifierType, null) {}

        public CmsSigner (X509Certificate2 certificate) : this(SubjectIdentifierType.IssuerAndSerialNumber, certificate) {}

        [SecuritySafeCritical]
        public CmsSigner (CspParameters parameters) : this(SubjectIdentifierType.SubjectKeyIdentifier, 
                                                           PkcsUtils.CreateDummyCertificate(parameters)) {
            m_dummyCert = true;
            this.IncludeOption = X509IncludeOption.None;
        }

        public CmsSigner (SubjectIdentifierType signerIdentifierType, X509Certificate2 certificate) {
            switch (signerIdentifierType) {
            case SubjectIdentifierType.Unknown:
                this.SignerIdentifierType = SubjectIdentifierType.IssuerAndSerialNumber;
                this.IncludeOption = X509IncludeOption.ExcludeRoot;
                break;
            case SubjectIdentifierType.IssuerAndSerialNumber:
                this.SignerIdentifierType = signerIdentifierType;
                this.IncludeOption = X509IncludeOption.ExcludeRoot;
                break;
            case SubjectIdentifierType.SubjectKeyIdentifier:
                this.SignerIdentifierType = signerIdentifierType;
                this.IncludeOption = X509IncludeOption.ExcludeRoot;
                break;
            case SubjectIdentifierType.NoSignature:
                this.SignerIdentifierType = signerIdentifierType;
                this.IncludeOption        = X509IncludeOption.None;
                break;
            default:
                this.SignerIdentifierType = SubjectIdentifierType.IssuerAndSerialNumber;
                this.IncludeOption = X509IncludeOption.ExcludeRoot;
                break;
            }
            this.Certificate = certificate;
            string oidValue = LocalAppContextSwitches.CmsUseInsecureHashAlgorithms ? CAPI.szOID_OIWSEC_sha1 : Sha256Oid;
            this.DigestAlgorithm = Oid.FromOidValue(oidValue, OidGroup.HashAlgorithm);

            m_signedAttributes = new CryptographicAttributeObjectCollection();
            m_unsignedAttributes = new CryptographicAttributeObjectCollection();
            m_certificates = new X509Certificate2Collection();
        }

        //
        // Public APIs.
        //

        public SubjectIdentifierType SignerIdentifierType {
            get {
                return m_signerIdentifierType;
            }
            set {
                if (value != SubjectIdentifierType.IssuerAndSerialNumber && 
                    value != SubjectIdentifierType.SubjectKeyIdentifier  &&
                    value != SubjectIdentifierType.NoSignature)
                    throw new ArgumentException(String.Format(CultureInfo.CurrentCulture, SecurityResources.GetResourceString("Arg_EnumIllegalVal"), "value"));

                if (m_dummyCert && value != SubjectIdentifierType.SubjectKeyIdentifier)
                    throw new ArgumentException(String.Format(CultureInfo.CurrentCulture, SecurityResources.GetResourceString("Arg_EnumIllegalVal"), "value"));
                m_signerIdentifierType = value;
            }
        }

        public X509Certificate2 Certificate {
            get {
                return m_certificate;
            }
            set {
                m_certificate = value;
            }
        }

        public Oid DigestAlgorithm {
            get {
                return m_digestAlgorithm;
            }
            set {
                m_digestAlgorithm = value;
            }
        }

        public CryptographicAttributeObjectCollection SignedAttributes {
            get {
                return m_signedAttributes;
            }
        }

        public CryptographicAttributeObjectCollection UnsignedAttributes {
            get {
                return m_unsignedAttributes;
            }
        }

        public X509Certificate2Collection Certificates {
            get {
                return m_certificates;
            }
        }

        public X509IncludeOption IncludeOption {
            get {
                return m_includeOption;
            }
            set {
                if (value < X509IncludeOption.None || value > X509IncludeOption.WholeChain)
                    throw new ArgumentException(String.Format(CultureInfo.CurrentCulture, SecurityResources.GetResourceString("Arg_EnumIllegalVal"), "value"));
                m_includeOption = value;
            }
        }
    }
}
