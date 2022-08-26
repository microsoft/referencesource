// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

//
// mansign2.cs
//

using Microsoft.Win32;
using System.Collections;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Security.Cryptography.Pkcs;
using System.Security.Cryptography.X509Certificates;
using System.Security.Cryptography.Xml;
using System.Text;
using System.Windows.Forms;
using System.Xml;

namespace System.Deployment.Internal.CodeSigning
{
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    internal struct BLOBHEADER
    {
        internal byte bType;
        internal byte bVersion;
        internal short reserved;
        internal uint aiKeyAlg;
    };

    internal class ManifestSignedXml2 : SignedXml
    {
        private bool m_verify = false;
        private const string Sha256SignatureMethodUri = @"http://www.w3.org/2000/09/xmldsig#rsa-sha256";
        private const string Sha256DigestMethod = @"http://www.w3.org/2000/09/xmldsig#sha256";

        internal ManifestSignedXml2()
            : base()
        {
            init();
        }
        internal ManifestSignedXml2(XmlElement elem)
            : base(elem)
        {
            init();
        }
        internal ManifestSignedXml2(XmlDocument document)
            : base(document)
        {
            init();
        }

        internal ManifestSignedXml2(XmlDocument document, bool verify)
            : base(document)
        {
            m_verify = verify;
            init();
        }

        private void init()
        {
            CryptoConfig.AddAlgorithm(typeof(RSAPKCS1SHA256SignatureDescription),
                               Sha256SignatureMethodUri);

            CryptoConfig.AddAlgorithm(typeof(System.Security.Cryptography.SHA256Cng),
                               Sha256DigestMethod);
        }

        public override XmlElement GetIdElement(XmlDocument document, string idValue)
        {
            // We only care about Id references inside of the KeyInfo section
            if (m_verify)
                return base.GetIdElement(document, idValue);

            KeyInfo keyInfo = this.KeyInfo;
            if (keyInfo.Id != idValue)
                return null;
            return keyInfo.GetXml();
        }
    }

    internal class SignedCmiManifest2
    {
        private XmlDocument m_manifestDom = null;
        private CmiStrongNameSignerInfo m_strongNameSignerInfo = null;
        private CmiAuthenticodeSignerInfo m_authenticodeSignerInfo = null;
        private bool m_useSha256;

        private const string Sha256SignatureMethodUri = @"http://www.w3.org/2000/09/xmldsig#rsa-sha256";
        private const string Sha256DigestMethod = @"http://www.w3.org/2000/09/xmldsig#sha256";

        private const string wintrustPolicyFlagsRegPath = "Software\\Microsoft\\Windows\\CurrentVersion\\WinTrust\\Trust Providers\\Software Publishing";
        private const string wintrustPolicyFlagsRegName = "State";

        private SignedCmiManifest2() { }

        internal SignedCmiManifest2(XmlDocument manifestDom, bool useSha256)
        {
            if (manifestDom == null)
                throw new ArgumentNullException("manifestDom");
            m_manifestDom = manifestDom;
            m_useSha256 = useSha256;
        }

        internal void Sign(CmiManifestSigner2 signer)
        {
            Sign(signer, null);
        }

        internal void Sign(CmiManifestSigner2 signer, string timeStampUrl)
        {
            // Reset signer infos.
            m_strongNameSignerInfo = null;
            m_authenticodeSignerInfo = null;

            // Signer cannot be null.
            if (signer == null || signer.StrongNameKey == null)
            {
                throw new ArgumentNullException("signer");
            }

            // Remove existing SN signature.
            RemoveExistingSignature(m_manifestDom);

            // Replace public key token in assemblyIdentity if requested.
            if ((signer.Flag & CmiManifestSignerFlag.DontReplacePublicKeyToken) == 0)
            {
                ReplacePublicKeyToken(m_manifestDom, signer.StrongNameKey, m_useSha256);
            }

            // No cert means don't Authenticode sign and timestamp.
            XmlDocument licenseDom = null;
            if (signer.Certificate != null)
            {
                // Yes. We will Authenticode sign, so first insert <publisherIdentity />
                // element, if necessary.
                InsertPublisherIdentity(m_manifestDom, signer.Certificate);

                // Now create the license DOM, and then sign it.
                licenseDom = CreateLicenseDom(signer, ExtractPrincipalFromManifest(), ComputeHashFromManifest(m_manifestDom, m_useSha256));
                AuthenticodeSignLicenseDom(licenseDom, signer, timeStampUrl, m_useSha256);
            }
            StrongNameSignManifestDom(m_manifestDom, licenseDom, signer, m_useSha256);
        }

        // throw cryptographic exception for any verification errors.
        internal void Verify(CmiManifestVerifyFlags verifyFlags)
        {
            // Reset signer infos.
            m_strongNameSignerInfo = null;
            m_authenticodeSignerInfo = null;

            XmlNamespaceManager nsm = new XmlNamespaceManager(m_manifestDom.NameTable);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);

            XmlElement signatureNode = GetSingleNode(m_manifestDom, "//ds:Signature[@Id=\"StrongNameSignature\"]", nsm) as XmlElement;
            if (signatureNode == null)
            {
                throw new CryptographicException(Win32.TRUST_E_NOSIGNATURE);
            }

            // Make sure it is indeed SN signature, and it is an enveloped signature.
            bool oldFormat = VerifySignatureForm(signatureNode, "StrongNameSignature", nsm);

            // It is the DSig we want, now make sure the public key matches the token.
            string publicKeyToken = VerifyPublicKeyToken();

            // OK. We found the SN signature with matching public key token, so
            // instantiate the SN signer info property.
            m_strongNameSignerInfo = new CmiStrongNameSignerInfo(Win32.TRUST_E_FAIL, publicKeyToken);

            // Now verify the SN signature, and Authenticode license if available.
            ManifestSignedXml2 signedXml = new ManifestSignedXml2(m_manifestDom, true);
            signedXml.LoadXml(signatureNode);
            if (m_useSha256)
            {
                signedXml.SignedInfo.SignatureMethod = Sha256SignatureMethodUri;
            }

            AsymmetricAlgorithm key = null;
            bool dsigValid = signedXml.CheckSignatureReturningKey(out key);
            m_strongNameSignerInfo.PublicKey = key;
            if (!dsigValid)
            {
                m_strongNameSignerInfo.ErrorCode = Win32.TRUST_E_BAD_DIGEST;
                throw new CryptographicException(Win32.TRUST_E_BAD_DIGEST);
            }

            // Verify license as well if requested.
            if ((verifyFlags & CmiManifestVerifyFlags.StrongNameOnly) != CmiManifestVerifyFlags.StrongNameOnly)
            {
                if (m_useSha256)
                {
                    VerifyLicenseNew(verifyFlags, oldFormat);
                }
                else
                {
                    VerifyLicense(verifyFlags, oldFormat);
                }
            }
        }

        internal CmiStrongNameSignerInfo StrongNameSignerInfo
        {
            get
            {
                return m_strongNameSignerInfo;
            }
        }

        internal CmiAuthenticodeSignerInfo AuthenticodeSignerInfo
        {
            get
            {
                return m_authenticodeSignerInfo;
            }
        }

        //
        // Privates.
        //
        private void VerifyLicense(CmiManifestVerifyFlags verifyFlags, bool oldFormat)
        {
            XmlNamespaceManager nsm = new XmlNamespaceManager(m_manifestDom.NameTable);
            nsm.AddNamespace("asm", AssemblyNamespaceUri);
            nsm.AddNamespace("asm2", AssemblyV2NamespaceUri);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);
            nsm.AddNamespace("msrel", MSRelNamespaceUri);
            nsm.AddNamespace("r", LicenseNamespaceUri);
            nsm.AddNamespace("as", AuthenticodeNamespaceUri);

            // We are done if no license.
            XmlElement licenseNode = GetSingleNode(m_manifestDom, "asm:assembly/ds:Signature/ds:KeyInfo/msrel:RelData/r:license", nsm) as XmlElement;
            if (licenseNode == null)
            {
                return;
            }

            if (!OverrideTimestampImprovements(nsm))
            {
                XmlNodeList nodes = licenseNode.SelectNodes("r:issuer/ds:Signature", nsm);
                if ((nodes == null) || (nodes.Count != 1))
                {
                    m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_NOSIGNATURE;
                    throw new CryptographicException(Win32.TRUST_E_NOSIGNATURE);
                }

                // We are not worried about return values, CertVerifyAuthenticodeLicense would validate signature+timestamp
                // VerifySignatureTimestamp would throw if timestamp is invalid
                DateTime verificationTime;
                VerifySignatureTimestampNew(nodes[0] as XmlElement, nsm, out verificationTime);
            }

            // Make sure this license is for this manifest.
            VerifyAssemblyIdentity(nsm);

            // Found a license, so instantiate signer info property.
            m_authenticodeSignerInfo = new CmiAuthenticodeSignerInfo(Win32.TRUST_E_FAIL);

            unsafe
            {
                byte[] licenseXml = Encoding.UTF8.GetBytes(licenseNode.OuterXml);
                fixed (byte* pbLicense = licenseXml)
                {
                    Win32.AXL_SIGNER_INFO signerInfo = new Win32.AXL_SIGNER_INFO();
                    signerInfo.cbSize = (uint)Marshal.SizeOf(typeof(Win32.AXL_SIGNER_INFO));
                    Win32.AXL_TIMESTAMPER_INFO timestamperInfo = new Win32.AXL_TIMESTAMPER_INFO();
                    timestamperInfo.cbSize = (uint)Marshal.SizeOf(typeof(Win32.AXL_TIMESTAMPER_INFO));
                    Win32.CRYPT_DATA_BLOB licenseBlob = new Win32.CRYPT_DATA_BLOB();
                    IntPtr pvLicense = new IntPtr(pbLicense);
                    licenseBlob.cbData = (uint)licenseXml.Length;
                    licenseBlob.pbData = pvLicense;

                    int hr = Win32.CertVerifyAuthenticodeLicense(ref licenseBlob, (uint)verifyFlags, ref signerInfo, ref timestamperInfo);
                    if (Win32.TRUST_E_NOSIGNATURE != (int)signerInfo.dwError)
                    {
                        m_authenticodeSignerInfo = new CmiAuthenticodeSignerInfo(signerInfo, timestamperInfo);
                    }

                    Win32.CertFreeAuthenticodeSignerInfo(ref signerInfo);
                    Win32.CertFreeAuthenticodeTimestamperInfo(ref timestamperInfo);

                    if (hr != Win32.S_OK)
                    {
                        throw new CryptographicException(hr);
                    }
                }
            }


#if (true) //
            if (!oldFormat)
#endif
                // Make sure we have the intended Authenticode signer.
                VerifyPublisherIdentity(nsm);
        }

        // can be used with sha1 or sha2 
        // logic is copied from the "isolation library" in NDP\iso_whid\ds\security\cryptoapi\pkisign\msaxlapi\mansign.cpp
        private void VerifyLicenseNew(CmiManifestVerifyFlags verifyFlags, bool oldFormat)
        {
            XmlNamespaceManager nsm = new XmlNamespaceManager(m_manifestDom.NameTable);
            nsm.AddNamespace("asm", AssemblyNamespaceUri);
            nsm.AddNamespace("asm2", AssemblyV2NamespaceUri);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);
            nsm.AddNamespace("msrel", MSRelNamespaceUri);
            nsm.AddNamespace("r", LicenseNamespaceUri);
            nsm.AddNamespace("as", AuthenticodeNamespaceUri);

            // We are done if no license.
            XmlElement licenseNode = GetSingleNode(m_manifestDom, "asm:assembly/ds:Signature/ds:KeyInfo/msrel:RelData/r:license", nsm) as XmlElement;
            if (licenseNode == null)
            {
                return;
            }

            // Make sure this license is for this manifest.
            VerifyAssemblyIdentity(nsm);

            // Found a license, so instantiate signer info property.
            m_authenticodeSignerInfo = new CmiAuthenticodeSignerInfo(Win32.TRUST_E_FAIL);

            // Find the license's signature
            XmlElement signatureNode = GetSingleNode(licenseNode, "//r:issuer/ds:Signature", nsm) as XmlElement;
            if (signatureNode == null)
            {
                throw new CryptographicException(Win32.TRUST_E_NOSIGNATURE);
            }

            // Make sure it is indeed an Authenticode signature, and it is an enveloped signature.
            // Then make sure the transforms are valid.
            VerifySignatureForm(signatureNode, "AuthenticodeSignature", nsm);

            // Now read the enveloped license signature.
            XmlDocument licenseDom = new XmlDocument();
            licenseDom.LoadXml(licenseNode.OuterXml);
            signatureNode = GetSingleNode(licenseDom, "//r:issuer/ds:Signature", nsm) as XmlElement;

            ManifestSignedXml2 signedXml = new ManifestSignedXml2(licenseDom);
            signedXml.LoadXml(signatureNode);
            if (m_useSha256)
            {
                signedXml.SignedInfo.SignatureMethod = Sha256SignatureMethodUri;
            }

            // Check the signature
            if (!signedXml.CheckSignature())
            {
                m_authenticodeSignerInfo = null;
                throw new CryptographicException(Win32.TRUST_E_CERT_SIGNATURE);
            }

            X509Certificate2 signingCertificate = GetSigningCertificate(signedXml, nsm);

            // First make sure certificate is not explicitly disallowed.
            X509Store store = new X509Store(StoreName.Disallowed, StoreLocation.CurrentUser);
            store.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly);
            X509Certificate2Collection storedCertificates = null;
            try
            {
                storedCertificates = (X509Certificate2Collection)store.Certificates;
                if (storedCertificates == null)
                {
                    m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_FAIL;
                    throw new CryptographicException(Win32.TRUST_E_FAIL);
                }
                if (storedCertificates.Contains(signingCertificate))
                {
                    m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_EXPLICIT_DISTRUST;
                    throw new CryptographicException(Win32.TRUST_E_EXPLICIT_DISTRUST);
                }
            }
            finally
            {
                store.Close();
            }

            // prepare information for the TrustManager to display
            string hash;
            string description;
            string url;
            if (!GetManifestInformation(licenseNode, nsm, out hash, out description, out url))
            {
                m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_SUBJECT_FORM_UNKNOWN;
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }
            m_authenticodeSignerInfo.Hash = hash;
            m_authenticodeSignerInfo.Description = description;
            m_authenticodeSignerInfo.DescriptionUrl = url;

            // read the timestamp from the manifest
            DateTime verificationTime;
            bool isTimestamped = OverrideTimestampImprovements(nsm) ? VerifySignatureTimestamp(signatureNode, nsm, out verificationTime) : VerifySignatureTimestampNew(signatureNode, nsm, out verificationTime);
            bool isLifetimeSigning = false;
            if (isTimestamped)
            {
                isLifetimeSigning = ((verifyFlags & CmiManifestVerifyFlags.LifetimeSigning) == CmiManifestVerifyFlags.LifetimeSigning);
                if (!isLifetimeSigning)
                {
                    isLifetimeSigning = GetLifetimeSigning(signingCertificate);
                }
            }

            // Retrieve the Authenticode policy settings from registry.
            uint policies = GetAuthenticodePolicies();

            X509Chain chain = new X509Chain(); // use the current user profile
            chain.ChainPolicy.RevocationFlag = X509RevocationFlag.ExcludeRoot;
            chain.ChainPolicy.RevocationMode = X509RevocationMode.Online;
            if ((CmiManifestVerifyFlags.RevocationCheckEndCertOnly & verifyFlags) == CmiManifestVerifyFlags.RevocationCheckEndCertOnly)
            {
                chain.ChainPolicy.RevocationFlag = X509RevocationFlag.EndCertificateOnly;
            }
            else if ((CmiManifestVerifyFlags.RevocationCheckEntireChain & verifyFlags) == CmiManifestVerifyFlags.RevocationCheckEntireChain)
            {
                chain.ChainPolicy.RevocationFlag = X509RevocationFlag.EntireChain;
            }
            else if (((CmiManifestVerifyFlags.RevocationNoCheck & verifyFlags) == CmiManifestVerifyFlags.RevocationNoCheck) ||
                ((Win32.WTPF_IGNOREREVOKATION & policies) == Win32.WTPF_IGNOREREVOKATION))
            {
                chain.ChainPolicy.RevocationMode = X509RevocationMode.NoCheck;
            }

            chain.ChainPolicy.VerificationTime = verificationTime; // local time
            if (isTimestamped && isLifetimeSigning)
            {
                chain.ChainPolicy.ApplicationPolicy.Add(new Oid(Win32.szOID_KP_LIFETIME_SIGNING));
            }

            chain.ChainPolicy.UrlRetrievalTimeout = new TimeSpan(0, 1, 0);
            chain.ChainPolicy.VerificationFlags = X509VerificationFlags.NoFlag; // don't ignore anything

            bool chainIsValid = chain.Build(signingCertificate);

            if (!chainIsValid)
            {
#if DEBUG
                X509ChainStatus[] statuses = chain.ChainStatus;
                foreach (X509ChainStatus status in statuses)
                {
                    System.Diagnostics.Debug.WriteLine("flag = " + status.Status + " " + status.StatusInformation);
                }
#endif
                AuthenticodeSignerInfo.ErrorCode = Win32.TRUST_E_SUBJECT_NOT_TRUSTED;
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_NOT_TRUSTED);
            }

            // package information for the trust manager
            m_authenticodeSignerInfo.SignerChain = chain;

            store = new X509Store(StoreName.TrustedPublisher, StoreLocation.CurrentUser);
            store.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly);
            try
            {
                storedCertificates = (X509Certificate2Collection)store.Certificates;
                if (storedCertificates == null)
                {
                    m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_FAIL;
                    throw new CryptographicException(Win32.TRUST_E_FAIL);
                }
                if (!storedCertificates.Contains(signingCertificate))
                {
                    AuthenticodeSignerInfo.ErrorCode = Win32.TRUST_E_SUBJECT_NOT_TRUSTED;
                    throw new CryptographicException(Win32.TRUST_E_SUBJECT_NOT_TRUSTED);
                }
            }
            finally
            {
                store.Close();
            }

            // Verify Certificate publisher name
            XmlElement subjectNode = GetSingleNode(licenseNode, "r:grant/as:AuthenticodePublisher/as:X509SubjectName", nsm) as XmlElement;
            if (subjectNode == null || String.Compare(signingCertificate.Subject, subjectNode.InnerText, StringComparison.Ordinal) != 0)
            {
                AuthenticodeSignerInfo.ErrorCode = Win32.TRUST_E_CERT_SIGNATURE;
                throw new CryptographicException(Win32.TRUST_E_CERT_SIGNATURE);
            }

#if (true) //
            if (!oldFormat)
#endif
                // Make sure we have the intended Authenticode signer.
                VerifyPublisherIdentity(nsm);
        }

        private X509Certificate2 GetSigningCertificate(ManifestSignedXml2 signedXml, XmlNamespaceManager nsm)
        {
            X509Certificate2 signingCertificate = null;

            KeyInfo keyInfo = signedXml.KeyInfo;
            KeyInfoX509Data kiX509 = null;
            RSAKeyValue keyValue = null;
            foreach (KeyInfoClause kic in keyInfo)
            {
                if (keyValue == null)
                {
                    keyValue = kic as RSAKeyValue;
                    if (keyValue == null)
                    {
                        break;
                    }
                }

                if (kiX509 == null)
                {
                    kiX509 = kic as KeyInfoX509Data;
                }

                if (keyValue != null && kiX509 != null)
                {
                    break;
                }
            }

            if (keyValue == null || kiX509 == null)
            {
                // no X509Certificate KeyInfoClause
                m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_SUBJECT_FORM_UNKNOWN;
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            // get public key from signing keyInfo
            RSAParameters signingPublicKey;
            RSA rsaProvider = keyValue.Key;
            if (rsaProvider != null)
            {
                signingPublicKey = rsaProvider.ExportParameters(false);
            }
            else
            {
                m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_CERT_SIGNATURE;
                throw new CryptographicException(Win32.TRUST_E_CERT_SIGNATURE);
            }

            // enumerate all certificates in x509Data searching for the one whose public key is used in <RSAKeyValue>
            foreach (X509Certificate2 certificate in kiX509.Certificates)
            {
                if (certificate == null)
                {
                    continue;
                }

                bool certificateAuthority = false;
                foreach (X509Extension extention in certificate.Extensions)
                {
                    X509BasicConstraintsExtension basicExtention = extention as X509BasicConstraintsExtension;
                    if (basicExtention != null)
                    {
                        certificateAuthority = basicExtention.CertificateAuthority;
                        if (certificateAuthority)
                        {
                            break;
                        }
                    }
                }

                if (certificateAuthority)
                {
                    // Ignore certs that have "Subject Type=CA" in basic contraints
                    continue;
                }

                RSA publicKey = CngLightup.GetRSAPublicKey(certificate);
                RSAParameters certificatePublicKey = publicKey.ExportParameters(false);
                if ((StructuralComparisons.StructuralEqualityComparer.Equals(signingPublicKey.Exponent, certificatePublicKey.Exponent))
                   && (StructuralComparisons.StructuralEqualityComparer.Equals(signingPublicKey.Modulus, certificatePublicKey.Modulus)))
                {
                    signingCertificate = certificate;
                    break;
                }
            }

            if (signingCertificate == null)
            {
                m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_CERT_SIGNATURE;
                throw new CryptographicException(Win32.TRUST_E_CERT_SIGNATURE);
            }
            return signingCertificate;
        }

        private bool VerifySignatureForm(XmlElement signatureNode, string signatureKind, XmlNamespaceManager nsm)
        {
            bool oldFormat = false;
            string snIdName = "Id";
            if (!signatureNode.HasAttribute(snIdName))
            {
                snIdName = "id";
                if (!signatureNode.HasAttribute(snIdName))
                {
                    snIdName = "ID";
                    if (!signatureNode.HasAttribute(snIdName))
                    {
                        throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
                    }
                }
            }

            string snIdValue = signatureNode.GetAttribute(snIdName);
            if (snIdValue == null ||
                String.Compare(snIdValue, signatureKind, StringComparison.Ordinal) != 0)
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            // Make sure it is indeed an enveloped signature.
            bool validFormat = false;
            XmlNodeList referenceNodes = signatureNode.SelectNodes("ds:SignedInfo/ds:Reference", nsm);
            foreach (XmlNode referenceNode in referenceNodes)
            {
                XmlElement reference = referenceNode as XmlElement;
                if (reference != null && reference.HasAttribute("URI"))
                {
                    string uriValue = reference.GetAttribute("URI");
                    if (uriValue != null)
                    {
                        // We expect URI="" (empty URI value which means to hash the entire document).
                        if (uriValue.Length == 0)
                        {
                            XmlNode transformsNode = GetSingleNode(reference, "ds:Transforms", nsm);
                            if (transformsNode == null)
                            {
                                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
                            }

                            // Make sure the transforms are what we expected.
                            XmlNodeList transforms = transformsNode.SelectNodes("ds:Transform", nsm);
                            if (transforms.Count < 2)
                            {
                                // We expect at least:
                                //  <Transform Algorithm="http://www.w3.org/2001/10/xml-exc-c14n#" />
                                //  <Transform Algorithm="http://www.w3.org/2000/09/xmldsig#enveloped-signature" /> 
                                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
                            }

                            bool c14 = false;
                            bool enveloped = false;
                            for (int i = 0; i < transforms.Count; i++)
                            {
                                XmlElement transform = transforms[i] as XmlElement;
                                string algorithm = transform.GetAttribute("Algorithm");
                                if (algorithm == null)
                                {
                                    break;
                                }
                                else if (String.Compare(algorithm, SignedXml.XmlDsigExcC14NTransformUrl, StringComparison.Ordinal) != 0)
                                {
                                    c14 = true;
                                    if (enveloped)
                                    {
                                        validFormat = true;
                                        break;
                                    }
                                }
                                else if (String.Compare(algorithm, SignedXml.XmlDsigEnvelopedSignatureTransformUrl, StringComparison.Ordinal) != 0)
                                {
                                    enveloped = true;
                                    if (c14)
                                    {
                                        validFormat = true;
                                        break;
                                    }
                                }
                            }
                        }
#if (true) // 
                        else if (String.Compare(uriValue, "#StrongNameKeyInfo", StringComparison.Ordinal) == 0)
                        {
                            oldFormat = true;

                            XmlNode transformsNode = GetSingleNode(referenceNode, "ds:Transforms", nsm);
                            if (transformsNode == null)
                            {
                                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
                            }

                            // Make sure the transforms are what we expected.
                            XmlNodeList transforms = transformsNode.SelectNodes("ds:Transform", nsm);
                            if (transforms.Count < 1)
                            {
                                // We expect at least:
                                //  <Transform Algorithm="http://www.w3.org/2001/10/xml-exc-c14n#" />
                                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
                            }

                            for (int i = 0; i < transforms.Count; i++)
                            {
                                XmlElement transform = transforms[i] as XmlElement;
                                string algorithm = transform.GetAttribute("Algorithm");
                                if (algorithm == null)
                                {
                                    break;
                                }
                                else if (String.Compare(algorithm, SignedXml.XmlDsigExcC14NTransformUrl, StringComparison.Ordinal) != 0)
                                {
                                    validFormat = true;
                                    break;
                                }
                            }
                        }
#endif // 
                    }
                }
            }

            if (!validFormat)
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            return oldFormat;
        }

        private bool GetManifestInformation(XmlElement licenseNode, XmlNamespaceManager nsm, out string hash, out string description, out string url)
        {
            hash = "";
            description = "";
            url = "";

            XmlElement manifestInformation = GetSingleNode(licenseNode, "r:grant/as:ManifestInformation", nsm) as XmlElement;
            if (manifestInformation == null)
            {
                return false;
            }
            if (!manifestInformation.HasAttribute("Hash"))
            {
                return false;
            }

            hash = manifestInformation.GetAttribute("Hash");
            if (string.IsNullOrEmpty(hash))
            {
                return false;
            }

            foreach (char c in hash)
            {
                if (0xFF == HexToByte(c))
                {
                    return false;
                }
            }

            if (manifestInformation.HasAttribute("Description"))
            {
                description = manifestInformation.GetAttribute("Description");
            }

            if (manifestInformation.HasAttribute("Url"))
            {
                url = manifestInformation.GetAttribute("Url");
            }

            return true;
        }

        private bool VerifySignatureTimestamp(XmlElement signatureNode, XmlNamespaceManager nsm, out DateTime verificationTime)
        {
            verificationTime = DateTime.Now;

            XmlElement node = GetSingleNode(signatureNode, "ds:Object/as:Timestamp", nsm) as XmlElement;
            if (node != null)
            {
                string encodedMessage = node.InnerText;

                if (!string.IsNullOrEmpty(encodedMessage))
                {
                    byte[] base64DecodedMessage = null;
                    try
                    {
                        base64DecodedMessage = Convert.FromBase64String(encodedMessage);
                    }
                    catch (FormatException)
                    {
                        m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_TIME_STAMP;
                        throw new CryptographicException(Win32.TRUST_E_TIME_STAMP);
                    }
                    if (base64DecodedMessage != null)
                    {
                        // Create a new, nondetached SignedCms message.
                        SignedCms signedCms = new SignedCms();
                        signedCms.Decode(base64DecodedMessage);

                        // Verify the signature without validating the 
                        // certificate.
                        signedCms.CheckSignature(true);

                        byte[] signingTime = null;
                        CryptographicAttributeObjectCollection caos = signedCms.SignerInfos[0].SignedAttributes;
                        foreach (CryptographicAttributeObject cao in caos)
                        {
                            if (0 == string.Compare(cao.Oid.Value, Win32.szOID_RSA_signingTime, StringComparison.Ordinal))
                            {
                                foreach (AsnEncodedData d in cao.Values)
                                {
                                    if (0 == string.Compare(d.Oid.Value, Win32.szOID_RSA_signingTime, StringComparison.Ordinal))
                                    {
                                        signingTime = d.RawData;
                                        Pkcs9SigningTime time = new Pkcs9SigningTime(signingTime);
                                        verificationTime = time.SigningTime;
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            return false;
        }

        private bool VerifySignatureTimestampNew(XmlElement signatureNode, XmlNamespaceManager nsm, out DateTime verificationTime)
        {
            verificationTime = DateTime.Now;
            Pkcs9SigningTime time = null;
            string algOid = null;
            byte[] messageDigest = null;

            // get XML elements
            XmlNodeList timestamps = signatureNode.SelectNodes("ds:Object/as:Timestamp", nsm);
            XmlNodeList signatureValues = signatureNode.SelectNodes("ds:SignatureValue", nsm);
            if ((timestamps == null) || (timestamps.Count == 0) || (signatureValues == null) || (signatureValues.Count == 0))
            {
                // no timestamp
                return false;
            }

            if ((timestamps.Count > 1) || (signatureValues.Count > 1) ||
                string.IsNullOrEmpty(timestamps[0].InnerText) || string.IsNullOrEmpty(signatureValues[0].InnerText))
            {
                m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_TIME_STAMP;
                throw new CryptographicException(Win32.TRUST_E_TIME_STAMP);
            }

            // decode
            byte[] base64DecodedMessage = null;
            byte[] base64DecodedSignatureValue = null;
            try
            {
                base64DecodedMessage = Convert.FromBase64String(timestamps[0].InnerText);
                base64DecodedSignatureValue = Convert.FromBase64String(signatureValues[0].InnerText);
            }
            catch (FormatException)
            {
                m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_TIME_STAMP;
                throw new CryptographicException(Win32.TRUST_E_TIME_STAMP);
            }
            if ((base64DecodedMessage == null) || (base64DecodedSignatureValue == null))
            {
                return false;
            }

            // Create a new, nondetached SignedCms message.
            SignedCms signedCms = new SignedCms();
            signedCms.Decode(base64DecodedMessage);

            // Verify the signature without validating the 
            // certificate.
            signedCms.CheckSignature(true);

            if (signedCms.SignerInfos.Count != 1)
            {
                m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_NO_SIGNER_CERT;
                throw new CryptographicException(Win32.TRUST_E_NO_SIGNER_CERT);
            }

            algOid = signedCms.SignerInfos[0].DigestAlgorithm.Value;

            byte[] signingTime = null;
            CryptographicAttributeObjectCollection caos = signedCms.SignerInfos[0].SignedAttributes;
            foreach (CryptographicAttributeObject cao in caos)
            {
                if ((time == null) && (0 == string.Compare(cao.Oid.Value, Win32.szOID_RSA_signingTime, StringComparison.Ordinal)))
                {
                    foreach (AsnEncodedData d in cao.Values)
                    {
                        if (0 == string.Compare(d.Oid.Value, Win32.szOID_RSA_signingTime, StringComparison.Ordinal))
                        {
                            signingTime = d.RawData;
                            time = new Pkcs9SigningTime(signingTime);
                            break;
                        }
                    }
                }
                else if ((messageDigest == null) && (0 == string.Compare(cao.Oid.Value, Win32.szOID_RSA_messageDigest, StringComparison.Ordinal)))
                {
                    foreach (AsnEncodedData d in cao.Values)
                    {
                        if (0 == string.Compare(d.Oid.Value, Win32.szOID_RSA_messageDigest, StringComparison.Ordinal))
                        {
                            byte[] rawMessageDigest = d.RawData;
                            Pkcs9MessageDigest digest = new Pkcs9MessageDigest();
                            digest.RawData = rawMessageDigest;
                            messageDigest = digest.MessageDigest;
                            break;
                        }
                    }
                }
            }

            bool hasCorrectDigest = false;
            bool rfc3161 = false;

            // check digest hash
            try
            {
                // Verification of RFC3161-compliant timestamps requires usage of
                // CryptVerifyTimeStampSignature API which is invoked from VerifyRFC3161Timestamp
                VerifyRFC3161Timestamp(base64DecodedMessage, base64DecodedSignatureValue);
                hasCorrectDigest = true;
                rfc3161 = true;
            }
            catch (Exception e)
            {
                if (!(e is CryptographicException) ||
                    (e.HResult != Win32.NTE_BAD_HASH))
                {
                    if (messageDigest != null)
                    {
                        byte[] sigHash = null;

                        HashAlgorithm hasher = null;
                        if (algOid == Win32.szOID_NIST_sha256)
                        {
                            hasher = SHA256.Create();
                        }
                        else if (algOid == Win32.szOID_OIWSEC_sha1)
                        {
                            hasher = SHA1.Create();
                        }

                        if (hasher != null)
                        {
                            sigHash = hasher.ComputeHash(base64DecodedSignatureValue);

                            // compare hashes
                            if ((sigHash != null) && (sigHash.Length == messageDigest.Length))
                            {
                                hasCorrectDigest = sigHash.SequenceEqual(messageDigest);
                            }
                        }
                    }
                }
            }

            if (!hasCorrectDigest)
            {
                m_authenticodeSignerInfo.ErrorCode = Win32.NTE_BAD_HASH;
                throw new CryptographicException(Win32.NTE_BAD_HASH);
            }

            // check timestamp certificate validity at the time of timestamping
            if (time != null)
            {
                if ((signedCms.SignerInfos[0].Certificate.NotAfter < time.SigningTime) ||
                (signedCms.SignerInfos[0].Certificate.NotBefore > time.SigningTime))
                {
                    m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_TIME_STAMP;
                    throw new CryptographicException(Win32.TRUST_E_TIME_STAMP);
                }
            }
            else
            {
                if (!rfc3161)
                {
                    return false;
                }
            }

            bool validCertificate = false;

            try
            {
                // check certificate chain
                using (X509Chain chain = new X509Chain())
                {
                    chain.ChainPolicy.ExtraStore.AddRange(signedCms.Certificates);
                    chain.ChainPolicy.VerificationTime = time.SigningTime;
                    chain.ChainPolicy.ApplicationPolicy.Add(new Oid(Win32.szOID_PKIX_KP_TIMESTAMP_SIGNING));

                    validCertificate = chain.Build(signedCms.SignerInfos[0].Certificate);
                }
            }
            catch (Exception e)
            {
                if ((e is ArgumentException) || (e is CryptographicException))
                {
                    m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_TIME_STAMP;
                    throw new CryptographicException(Win32.TRUST_E_TIME_STAMP);
                }
            }

            if (!validCertificate)
            {
                m_authenticodeSignerInfo.ErrorCode = Win32.TRUST_E_SUBJECT_NOT_TRUSTED;
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_NOT_TRUSTED);
            }

            verificationTime = time.SigningTime;
            return true;
        }

        private void VerifyRFC3161Timestamp(byte[] base64DecodedMessage, byte[] base64DecodedSignatureValue)
        {
            unsafe
            {
                IntPtr ppTsContext = IntPtr.Zero;
                IntPtr ppTsSigner = IntPtr.Zero;
                IntPtr phStore = IntPtr.Zero;

                try
                {
                    if (!Win32.CryptVerifyTimeStampSignature(
                        base64DecodedMessage,
                        base64DecodedMessage.Length,
                        base64DecodedSignatureValue,
                        base64DecodedSignatureValue.Length,
                        IntPtr.Zero,
                        ref ppTsContext,
                        ref ppTsSigner,
                        ref phStore))
                    {
                        throw new CryptographicException(Marshal.GetLastWin32Error());
                    }
                }
                finally
                {
                    if (ppTsContext != IntPtr.Zero)
                        Win32.CryptMemFree(ppTsContext);

                    if (ppTsSigner != IntPtr.Zero)
                        Win32.CertFreeCertificateContext(ppTsSigner);

                    if (phStore != IntPtr.Zero)
                        Win32.CertCloseStore(phStore, 0);
                }
            }
        }

        private bool OverrideTimestampImprovements(XmlNamespaceManager nsm)
        {
            // There are two cases where asm:assembly/asm2:publisherIdentity or asm:assembly/asm2:publisherIdentity@issuerKeyHash are missing:
            // 1) Unsigned manifest - where publisher is not present and this code path would not be hit
            // 2) Malformed manifest - publisher validation code exists later in this code path
            // Returning 'true' to preserve compat, without introducing any security issues.

            // Find the publisherIdentity element.
            XmlElement publisherIdentity = GetSingleNode(m_manifestDom, "asm:assembly/asm2:publisherIdentity", nsm) as XmlElement;
            if (publisherIdentity == null || !publisherIdentity.HasAttributes)
            {
                return true;
            }

            // Get name and issuerKeyHash attribute values.
            if (!publisherIdentity.HasAttribute("issuerKeyHash"))
            {
                return true;
            }

            string publisherIssuerKeyHash = publisherIdentity.GetAttribute("issuerKeyHash");

            try
            {
                // read ClickOnceTimeStampImprovementsOverride value from config file, not case sensitive
                //
                //    <appSettings>
                //      <add key="ClickOnceTimeStampImprovementsOverride " value="753aed0977d8bb3aa8ce400685ec8d9d5cb83650;c5ed935f2b38477e58d357c7ff45c54441e15fbf " />
                //    </appSettings>
                //

                string value = System.Configuration.ConfigurationManager.AppSettings.Get("ClickOnceTimeStampImprovementsOverride");

                if (!string.IsNullOrEmpty(value))
                {
                    string[] hashes = value.Split(';');

                    foreach (string hash in hashes)
                    {
                        if (hash.Trim().Equals(publisherIssuerKeyHash, StringComparison.OrdinalIgnoreCase))
                        {
                            return true;
                        }
                    }
                }
            }
            catch { } // ignore all exceptions

            return false;
        }

        private bool GetLifetimeSigning(X509Certificate2 signingCertificate)
        {
            foreach (X509Extension extension in signingCertificate.Extensions)
            {
                X509EnhancedKeyUsageExtension ekuExtention = extension as X509EnhancedKeyUsageExtension;
                if (ekuExtention != null)
                {
                    OidCollection oids = ekuExtention.EnhancedKeyUsages;
                    foreach (Oid oid in oids)
                    {
                        if (0 == string.Compare(Win32.szOID_KP_LIFETIME_SIGNING, oid.Value, StringComparison.Ordinal))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        // Retrieve the Authenticode policy settings from registry. 
        // Isolation library was ignoring missing or inaccessible key/value errors
        private uint GetAuthenticodePolicies()
        {
            uint policies = 0;

            try
            {
                RegistryKey key = Registry.CurrentUser.OpenSubKey(wintrustPolicyFlagsRegPath);
                if (key != null)
                {
                    RegistryValueKind kind = key.GetValueKind(wintrustPolicyFlagsRegName);
                    if (kind == RegistryValueKind.DWord || kind == RegistryValueKind.Binary)
                    {
                        object value = key.GetValue(wintrustPolicyFlagsRegName);
                        if (value != null)
                        {
                            policies = Convert.ToUInt32(value);
                        }
                    }
                    key.Close();
                }
            }
            catch (System.Security.SecurityException)
            {
            }
            catch (ObjectDisposedException)
            {
            }
            catch (UnauthorizedAccessException)
            {
            }
            catch (IOException)
            {
            }
            return policies;
        }

        private XmlElement ExtractPrincipalFromManifest()
        {
            XmlNamespaceManager nsm = new XmlNamespaceManager(m_manifestDom.NameTable);
            nsm.AddNamespace("asm", AssemblyNamespaceUri);
            XmlNode assemblyIdentityNode = GetSingleNode(m_manifestDom, "asm:assembly/asm:assemblyIdentity", nsm);
            if (assemblyIdentityNode == null)
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            return assemblyIdentityNode as XmlElement;
        }

        private void VerifyAssemblyIdentity(XmlNamespaceManager nsm)
        {
            XmlElement assemblyIdentity = GetSingleNode(m_manifestDom, "asm:assembly/asm:assemblyIdentity", nsm) as XmlElement;
            XmlElement principal = GetSingleNode(m_manifestDom, "asm:assembly/ds:Signature/ds:KeyInfo/msrel:RelData/r:license/r:grant/as:ManifestInformation/as:assemblyIdentity", nsm) as XmlElement;

            if (assemblyIdentity == null || principal == null ||
                !assemblyIdentity.HasAttributes || !principal.HasAttributes)
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            XmlAttributeCollection asmIdAttrs = assemblyIdentity.Attributes;

            if (asmIdAttrs.Count == 0 || asmIdAttrs.Count != principal.Attributes.Count)
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            foreach (XmlAttribute asmIdAttr in asmIdAttrs)
            {
                if (!principal.HasAttribute(asmIdAttr.LocalName) ||
                    asmIdAttr.Value != principal.GetAttribute(asmIdAttr.LocalName))
                {
                    throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
                }
            }

            VerifyHash(nsm);
        }

        private void VerifyPublisherIdentity(XmlNamespaceManager nsm)
        {
            // Nothing to do if no signature.
            if (m_authenticodeSignerInfo.ErrorCode == Win32.TRUST_E_NOSIGNATURE)
            {
                return;
            }

            X509Certificate2 signerCert = m_authenticodeSignerInfo.SignerChain.ChainElements[0].Certificate;

            // Find the publisherIdentity element.
            XmlElement publisherIdentity = GetSingleNode(m_manifestDom, "asm:assembly/asm2:publisherIdentity", nsm) as XmlElement;
            if (publisherIdentity == null || !publisherIdentity.HasAttributes)
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            // Get name and issuerKeyHash attribute values.
            if (!publisherIdentity.HasAttribute("name") || !publisherIdentity.HasAttribute("issuerKeyHash"))
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            string publisherName = publisherIdentity.GetAttribute("name");
            string publisherIssuerKeyHash = publisherIdentity.GetAttribute("issuerKeyHash");

            // Calculate the issuer key hash.
            IntPtr pIssuerKeyHash = new IntPtr();
            int hr = Win32._AxlGetIssuerPublicKeyHash(signerCert.Handle, ref pIssuerKeyHash);
            if (hr != Win32.S_OK)
            {
                throw new CryptographicException(hr);
            }

            string issuerKeyHash = Marshal.PtrToStringUni(pIssuerKeyHash);
            Win32.HeapFree(Win32.GetProcessHeap(), 0, pIssuerKeyHash);

            // Make sure name and issuerKeyHash match.
            if (String.Compare(publisherName, signerCert.SubjectName.Name, StringComparison.Ordinal) != 0 ||
                String.Compare(publisherIssuerKeyHash, issuerKeyHash, StringComparison.Ordinal) != 0)
            {
                throw new CryptographicException(Win32.TRUST_E_FAIL);
            }
        }

        private void VerifyHash(XmlNamespaceManager nsm)
        {
            XmlDocument manifestDom = new XmlDocument();
            // We always preserve white space as Fusion XML engine always preserve white space.
            manifestDom.PreserveWhitespace = true;
            manifestDom = (XmlDocument)m_manifestDom.Clone();

            XmlElement manifestInformation = GetSingleNode(manifestDom, "asm:assembly/ds:Signature/ds:KeyInfo/msrel:RelData/r:license/r:grant/as:ManifestInformation", nsm) as XmlElement;
            if (manifestInformation == null)
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            if (!manifestInformation.HasAttribute("Hash"))
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            string hash = manifestInformation.GetAttribute("Hash");
            if (hash == null || hash.Length == 0)
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            // Now compute the hash for the manifest without the entire SN
            // signature element.

            // First remove the Signture element from the DOM.
            XmlElement dsElement = GetSingleNode(manifestDom, "asm:assembly/ds:Signature", nsm) as XmlElement;
            if (dsElement == null)
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            dsElement.ParentNode.RemoveChild(dsElement);

            // Now compute the hash from the manifest, without the Signature element.
            byte[] hashBytes = HexStringToBytes(manifestInformation.GetAttribute("Hash"));
            byte[] computedHashBytes = ComputeHashFromManifest(manifestDom, m_useSha256);

            // Do they match?
            if (hashBytes.Length == 0 || hashBytes.Length != computedHashBytes.Length)
            {
#if (true) // 
                byte[] computedOldHashBytes = ComputeHashFromManifest(manifestDom, true, m_useSha256);

                // Do they match?
                if (hashBytes.Length == 0 || hashBytes.Length != computedOldHashBytes.Length)
                {
                    throw new CryptographicException(Win32.TRUST_E_BAD_DIGEST);
                }

                for (int i = 0; i < hashBytes.Length; i++)
                {
                    if (hashBytes[i] != computedOldHashBytes[i])
                    {
                        throw new CryptographicException(Win32.TRUST_E_BAD_DIGEST);
                    }
                }
#else
                throw new CryptographicException(Win32.TRUST_E_BAD_DIGEST);
#endif
            }

            for (int i = 0; i < hashBytes.Length; i++)
            {
                if (hashBytes[i] != computedHashBytes[i])
                {
#if (true) // 
                    byte[] computedOldHashBytes = ComputeHashFromManifest(manifestDom, true, m_useSha256);

                    // Do they match?
                    if (hashBytes.Length == 0 || hashBytes.Length != computedOldHashBytes.Length)
                    {
                        throw new CryptographicException(Win32.TRUST_E_BAD_DIGEST);
                    }

                    for (i = 0; i < hashBytes.Length; i++)
                    {
                        if (hashBytes[i] != computedOldHashBytes[i])
                        {
                            throw new CryptographicException(Win32.TRUST_E_BAD_DIGEST);
                        }
                    }
#else
                throw new CryptographicException(Win32.TRUST_E_BAD_DIGEST);
#endif
                }
            }
        }

        private string VerifyPublicKeyToken()
        {
            XmlNamespaceManager nsm = new XmlNamespaceManager(m_manifestDom.NameTable);
            nsm.AddNamespace("asm", AssemblyNamespaceUri);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);

            XmlElement snModulus = GetSingleNode(m_manifestDom, "asm:assembly/ds:Signature/ds:KeyInfo/ds:KeyValue/ds:RSAKeyValue/ds:Modulus", nsm) as XmlElement;
            XmlElement snExponent = GetSingleNode(m_manifestDom, "asm:assembly/ds:Signature/ds:KeyInfo/ds:KeyValue/ds:RSAKeyValue/ds:Exponent", nsm) as XmlElement;

            if (snModulus == null || snExponent == null)
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            byte[] modulus = Encoding.UTF8.GetBytes(snModulus.InnerXml);
            byte[] exponent = Encoding.UTF8.GetBytes(snExponent.InnerXml);

            string tokenString = GetPublicKeyToken(m_manifestDom);
            byte[] publicKeyToken = HexStringToBytes(tokenString);
            byte[] computedPublicKeyToken;

            unsafe
            {
                fixed (byte* pbModulus = modulus)
                {
                    fixed (byte* pbExponent = exponent)
                    {
                        Win32.CRYPT_DATA_BLOB modulusBlob = new Win32.CRYPT_DATA_BLOB();
                        Win32.CRYPT_DATA_BLOB exponentBlob = new Win32.CRYPT_DATA_BLOB();
                        IntPtr pComputedToken = new IntPtr();

                        modulusBlob.cbData = (uint)modulus.Length;
                        modulusBlob.pbData = new IntPtr(pbModulus);
                        exponentBlob.cbData = (uint)exponent.Length;
                        exponentBlob.pbData = new IntPtr(pbExponent);

                        // Now compute the public key token.
                        int hr = Win32._AxlRSAKeyValueToPublicKeyToken(ref modulusBlob, ref exponentBlob, ref pComputedToken);
                        if (hr != Win32.S_OK)
                        {
                            throw new CryptographicException(hr);
                        }

                        computedPublicKeyToken = HexStringToBytes(Marshal.PtrToStringUni(pComputedToken));
                        Win32.HeapFree(Win32.GetProcessHeap(), 0, pComputedToken);
                    }
                }
            }

            // Do they match?
            if (publicKeyToken.Length == 0 || publicKeyToken.Length != computedPublicKeyToken.Length)
            {
                throw new CryptographicException(Win32.TRUST_E_FAIL);
            }

            for (int i = 0; i < publicKeyToken.Length; i++)
            {
                if (publicKeyToken[i] != computedPublicKeyToken[i])
                {
                    throw new CryptographicException(Win32.TRUST_E_FAIL);
                }
            }

            return tokenString;
        }

        //
        // Statics.
        //
        private static void InsertPublisherIdentity(XmlDocument manifestDom, X509Certificate2 signerCert)
        {

            XmlNamespaceManager nsm = new XmlNamespaceManager(manifestDom.NameTable);
            nsm.AddNamespace("asm", AssemblyNamespaceUri);
            nsm.AddNamespace("asm2", AssemblyV2NamespaceUri);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);

            XmlElement assembly = GetSingleNode(manifestDom, "asm:assembly", nsm) as XmlElement;
            XmlElement assemblyIdentity = GetSingleNode(manifestDom, "asm:assembly/asm:assemblyIdentity", nsm) as XmlElement;
            if (assemblyIdentity == null)
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            // Reuse existing node if exists
            XmlElement publisherIdentity = GetSingleNode(manifestDom, "asm:assembly/asm2:publisherIdentity", nsm) as XmlElement;
            if (publisherIdentity == null)
            {
                // create new if not exist
                publisherIdentity = manifestDom.CreateElement("publisherIdentity", AssemblyV2NamespaceUri);
            }
            // Get the issuer's public key blob hash.
            IntPtr pIssuerKeyHash = new IntPtr();
            int hr = Win32._AxlGetIssuerPublicKeyHash(signerCert.Handle, ref pIssuerKeyHash);
            if (hr != Win32.S_OK)
            {
                throw new CryptographicException(hr);
            }

            string issuerKeyHash = Marshal.PtrToStringUni(pIssuerKeyHash);
            Win32.HeapFree(Win32.GetProcessHeap(), 0, pIssuerKeyHash);

            publisherIdentity.SetAttribute("name", signerCert.SubjectName.Name);
            publisherIdentity.SetAttribute("issuerKeyHash", issuerKeyHash);

            XmlElement signature = GetSingleNode(manifestDom, "asm:assembly/ds:Signature", nsm) as XmlElement;
            if (signature != null)
            {
                assembly.InsertBefore(publisherIdentity, signature);
            }
            else
            {
                assembly.AppendChild(publisherIdentity);
            }
        }

        private static void RemoveExistingSignature(XmlDocument manifestDom)
        {
            XmlNamespaceManager nsm = new XmlNamespaceManager(manifestDom.NameTable);
            nsm.AddNamespace("asm", AssemblyNamespaceUri);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);
            XmlNode signatureNode = GetSingleNode(manifestDom, "asm:assembly/ds:Signature", nsm);
            if (signatureNode != null)
                signatureNode.ParentNode.RemoveChild(signatureNode);
        }

        /// <summary>
        /// The reason you need provider type 24, is because that’s the only RSA provider type that supports SHA-2 operations.   (For instance, PROV_RSA_FULL does not support SHA-2).
        /// As for official guidance – I’m not sure of any.    For workarounds though, if you’re using the Microsoft software CSPs, they share the underlying key store.  You can get the key container name from your RSA object, then open up a new RSA object with the same key container name but with PROV_RSA_AES.   At that point, you should be able to use SHA-2 algorithms.
        /// </summary>
        /// <param name="oldCsp"></param>
        /// <returns></returns>
        internal static RSACryptoServiceProvider GetFixedRSACryptoServiceProvider(RSACryptoServiceProvider oldCsp, bool useSha256)
        {
            if (!useSha256)
            {
                return oldCsp;
            }

            // 3rd party crypto providers in general don't need to be forcefully upgraded.
            // This is not an ideal way to check for that but is the best we have available.
            if (!oldCsp.CspKeyContainerInfo.ProviderName.StartsWith("Microsoft", StringComparison.Ordinal))
            {
                return oldCsp;
            }

            const int PROV_RSA_AES = 24;    // CryptoApi provider type for an RSA provider supporting sha-256 digital signatures
            CspParameters csp = new CspParameters();
            csp.ProviderType = PROV_RSA_AES;
            csp.KeyContainerName = oldCsp.CspKeyContainerInfo.KeyContainerName;
            csp.KeyNumber = (int)oldCsp.CspKeyContainerInfo.KeyNumber;
            if (oldCsp.CspKeyContainerInfo.MachineKeyStore)
            {
                csp.Flags = CspProviderFlags.UseMachineKeyStore;
            }
            RSACryptoServiceProvider fixedRsa = new RSACryptoServiceProvider(csp);

            return fixedRsa;

        }

        private static void ReplacePublicKeyToken(XmlDocument manifestDom, AsymmetricAlgorithm snKey, bool useSha256)
        {
            // Make sure we can find the publicKeyToken attribute.
            XmlNamespaceManager nsm = new XmlNamespaceManager(manifestDom.NameTable);
            nsm.AddNamespace("asm", AssemblyNamespaceUri);
            XmlElement assemblyIdentity = GetSingleNode(manifestDom, "asm:assembly/asm:assemblyIdentity", nsm) as XmlElement;
            if (assemblyIdentity == null)
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            if (!assemblyIdentity.HasAttribute("publicKeyToken"))
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            byte[] cspPublicKeyBlob;

            if (snKey is RSACryptoServiceProvider)
            {
                cspPublicKeyBlob = (GetFixedRSACryptoServiceProvider((RSACryptoServiceProvider)snKey, useSha256)).ExportCspBlob(false);
                if (cspPublicKeyBlob == null || cspPublicKeyBlob.Length == 0)
                {
                    throw new CryptographicException(Win32.NTE_BAD_KEY);
                }
            }
            else
            {
                using (RSACryptoServiceProvider rsaCsp = new RSACryptoServiceProvider())
                {
                    rsaCsp.ImportParameters(((RSA)snKey).ExportParameters(false));
                    cspPublicKeyBlob = rsaCsp.ExportCspBlob(false);
                }
            }

            // Now compute the public key token.
            unsafe
            {
                fixed (byte* pbPublicKeyBlob = cspPublicKeyBlob)
                {
                    Win32.CRYPT_DATA_BLOB publicKeyBlob = new Win32.CRYPT_DATA_BLOB();
                    publicKeyBlob.cbData = (uint)cspPublicKeyBlob.Length;
                    publicKeyBlob.pbData = new IntPtr(pbPublicKeyBlob);
                    IntPtr pPublicKeyToken = new IntPtr();

                    int hr = Win32._AxlPublicKeyBlobToPublicKeyToken(ref publicKeyBlob, ref pPublicKeyToken);
                    if (hr != Win32.S_OK)
                    {
                        throw new CryptographicException(hr);
                    }

                    string publicKeyToken = Marshal.PtrToStringUni(pPublicKeyToken);
                    Win32.HeapFree(Win32.GetProcessHeap(), 0, pPublicKeyToken);

                    assemblyIdentity.SetAttribute("publicKeyToken", publicKeyToken);
                }
            }
        }

        private static string GetPublicKeyToken(XmlDocument manifestDom)
        {
            XmlNamespaceManager nsm = new XmlNamespaceManager(manifestDom.NameTable);
            nsm.AddNamespace("asm", AssemblyNamespaceUri);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);

            XmlElement assemblyIdentity = GetSingleNode(manifestDom, "asm:assembly/asm:assemblyIdentity", nsm) as XmlElement;

            if (assemblyIdentity == null || !assemblyIdentity.HasAttribute("publicKeyToken"))
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            return assemblyIdentity.GetAttribute("publicKeyToken");
        }

        private static byte[] ComputeHashFromManifest(XmlDocument manifestDom, bool useSha256)
        {
#if (true) // 
            return ComputeHashFromManifest(manifestDom, false, useSha256);
        }

        private static byte[] ComputeHashFromManifest(XmlDocument manifestDom, bool oldFormat, bool useSha256)
        {
            if (oldFormat)
            {
                XmlDsigExcC14NTransform exc = new XmlDsigExcC14NTransform();
                exc.LoadInput(manifestDom);

                if (useSha256)
                {
                    using (SHA256CryptoServiceProvider sha2 = new SHA256CryptoServiceProvider())
                    {
                        byte[] hash = sha2.ComputeHash(exc.GetOutput() as MemoryStream);
                        if (hash == null)
                        {
                            throw new CryptographicException(Win32.TRUST_E_BAD_DIGEST);
                        }

                        return hash;
                    }
                }
                else
                {
                    using (SHA1CryptoServiceProvider sha1 = new SHA1CryptoServiceProvider())
                    {
                        byte[] hash = sha1.ComputeHash(exc.GetOutput() as MemoryStream);
                        if (hash == null)
                        {
                            throw new CryptographicException(Win32.TRUST_E_BAD_DIGEST);
                        }

                        return hash;
                    }
                }
            }
            else
            {
#endif
                // Since the DOM given to us is not guaranteed to be normalized,
                // we need to normalize it ourselves. Also, we always preserve
                // white space as Fusion XML engine always preserve white space.
                XmlDocument normalizedDom = new XmlDocument();
                normalizedDom.PreserveWhitespace = true;

                // Normalize the document
                using (TextReader stringReader = new StringReader(manifestDom.OuterXml))
                {
                    XmlReaderSettings settings = new XmlReaderSettings();
                    settings.DtdProcessing = DtdProcessing.Parse;
                    XmlReader reader = XmlReader.Create(stringReader, settings, manifestDom.BaseURI);
                    normalizedDom.Load(reader);
                }

                XmlDsigExcC14NTransform exc = new XmlDsigExcC14NTransform();
                exc.LoadInput(normalizedDom);

                if (useSha256)
                {
                    using (SHA256CryptoServiceProvider sha2 = new SHA256CryptoServiceProvider())
                    {
                        byte[] hash = sha2.ComputeHash(exc.GetOutput() as MemoryStream);
                        if (hash == null)
                        {
                            throw new CryptographicException(Win32.TRUST_E_BAD_DIGEST);
                        }

                        return hash;
                    }
                }
                else
                {
                    using (SHA1CryptoServiceProvider sha1 = new SHA1CryptoServiceProvider())
                    {
                        byte[] hash = sha1.ComputeHash(exc.GetOutput() as MemoryStream);
                        if (hash == null)
                        {
                            throw new CryptographicException(Win32.TRUST_E_BAD_DIGEST);
                        }

                        return hash;
                    }
                }

#if (true) // 
            }
#endif
        }

        private const string AssemblyNamespaceUri = "urn:schemas-microsoft-com:asm.v1";
        private const string AssemblyV2NamespaceUri = "urn:schemas-microsoft-com:asm.v2";
        private const string MSRelNamespaceUri = "http://schemas.microsoft.com/windows/rel/2005/reldata";
        private const string LicenseNamespaceUri = "urn:mpeg:mpeg21:2003:01-REL-R-NS";
        private const string AuthenticodeNamespaceUri = "http://schemas.microsoft.com/windows/pki/2005/Authenticode";
        private const string licenseTemplate = "<r:license xmlns:r=\"" + LicenseNamespaceUri + "\" xmlns:as=\"" + AuthenticodeNamespaceUri + "\">" +
                                                    @"<r:grant>" +
                                                    @"<as:ManifestInformation>" +
                                                    @"<as:assemblyIdentity />" +
                                                    @"</as:ManifestInformation>" +
                                                    @"<as:SignedBy/>" +
                                                    @"<as:AuthenticodePublisher>" +
                                                    @"<as:X509SubjectName>CN=dummy</as:X509SubjectName>" +
                                                    @"</as:AuthenticodePublisher>" +
                                                    @"</r:grant><r:issuer></r:issuer></r:license>";

        private static XmlDocument CreateLicenseDom(CmiManifestSigner2 signer, XmlElement principal, byte[] hash)
        {
            XmlDocument licenseDom = new XmlDocument();
            licenseDom.PreserveWhitespace = true;
            licenseDom.LoadXml(licenseTemplate);
            XmlNamespaceManager nsm = new XmlNamespaceManager(licenseDom.NameTable);
            nsm.AddNamespace("r", LicenseNamespaceUri);
            nsm.AddNamespace("as", AuthenticodeNamespaceUri);
            XmlElement assemblyIdentityNode = GetSingleNode(licenseDom, "r:license/r:grant/as:ManifestInformation/as:assemblyIdentity", nsm) as XmlElement;
            assemblyIdentityNode.RemoveAllAttributes();
            foreach (XmlAttribute attribute in principal.Attributes)
            {
                assemblyIdentityNode.SetAttribute(attribute.Name, attribute.Value);
            }

            XmlElement manifestInformationNode = GetSingleNode(licenseDom, "r:license/r:grant/as:ManifestInformation", nsm) as XmlElement;

            manifestInformationNode.SetAttribute("Hash", hash.Length == 0 ? "" : BytesToHexString(hash, 0, hash.Length));
            manifestInformationNode.SetAttribute("Description", signer.Description == null ? "" : signer.Description);
            manifestInformationNode.SetAttribute("Url", signer.DescriptionUrl == null ? "" : signer.DescriptionUrl);

            XmlElement authenticodePublisherNode = GetSingleNode(licenseDom, "r:license/r:grant/as:AuthenticodePublisher/as:X509SubjectName", nsm) as XmlElement;
            authenticodePublisherNode.InnerText = signer.Certificate.SubjectName.Name;

            return licenseDom;
        }

        private static void AuthenticodeSignLicenseDom(XmlDocument licenseDom, CmiManifestSigner2 signer, string timeStampUrl, bool useSha256)
        {
            // Make sure it is RSA, as this is the only one Fusion will support.
            using (RSA rsaPrivateKey = CngLightup.GetRSAPrivateKey(signer.Certificate))
            {
                if (rsaPrivateKey == null)
                {
                    throw new NotSupportedException();
                }

                // Setup up XMLDSIG engine.
                ManifestSignedXml2 signedXml = new ManifestSignedXml2(licenseDom);
                signedXml.SigningKey = rsaPrivateKey;
                signedXml.SignedInfo.CanonicalizationMethod = SignedXml.XmlDsigExcC14NTransformUrl;
                if (signer.UseSha256)
                    signedXml.SignedInfo.SignatureMethod = Sha256SignatureMethodUri;

                // Add the key information.
                signedXml.KeyInfo.AddClause(new RSAKeyValue(rsaPrivateKey));
                signedXml.KeyInfo.AddClause(new KeyInfoX509Data(signer.Certificate, signer.IncludeOption));

                // Add the enveloped reference.
                Reference reference = new Reference();
                reference.Uri = "";
                if (signer.UseSha256)
                    reference.DigestMethod = Sha256DigestMethod;

                // Add an enveloped and an Exc-C14N transform.
                reference.AddTransform(new XmlDsigEnvelopedSignatureTransform());
#if (false) // 
            reference.AddTransform(new XmlLicenseTransform()); 
#endif
                reference.AddTransform(new XmlDsigExcC14NTransform());

                // Add the reference.
                signedXml.AddReference(reference);

                // Compute the signature.
                signedXml.ComputeSignature();

                // Get the XML representation
                XmlElement xmlDigitalSignature = signedXml.GetXml();
                xmlDigitalSignature.SetAttribute("Id", "AuthenticodeSignature");

                // Insert the signature node under the issuer element.
                XmlNamespaceManager nsm = new XmlNamespaceManager(licenseDom.NameTable);
                nsm.AddNamespace("r", LicenseNamespaceUri);
                XmlElement issuerNode = GetSingleNode(licenseDom, "r:license/r:issuer", nsm) as XmlElement;
                issuerNode.AppendChild(licenseDom.ImportNode(xmlDigitalSignature, true));

                // Time stamp it if requested.
                if (timeStampUrl != null && timeStampUrl.Length != 0)
                {
                    TimestampSignedLicenseDom(licenseDom, timeStampUrl, useSha256);
                }

                // Wrap it inside a RelData element.
                licenseDom.DocumentElement.ParentNode.InnerXml = "<msrel:RelData xmlns:msrel=\"" +
                                                                 MSRelNamespaceUri + "\">" +
                                                                 licenseDom.OuterXml + "</msrel:RelData>";
            }
        }

        private static string ObtainRFC3161Timestamp(string timeStampUrl, string signatureValue, bool useSha256)
        {
            byte[] sigValueBytes = Convert.FromBase64String(signatureValue);
            string timestamp = String.Empty;

            string algId = useSha256 ? Win32.szOID_NIST_sha256 : Win32.szOID_OIWSEC_sha1;

            unsafe
            {
                IntPtr ppTsContext = IntPtr.Zero;
                IntPtr ppTsSigner = IntPtr.Zero;
                IntPtr phStore = IntPtr.Zero;

                try
                {
                    byte[] nonce = new byte[24];

                    using (RandomNumberGenerator rng = RandomNumberGenerator.Create())
                    {
                        rng.GetBytes(nonce);
                    }

                    Win32.CRYPT_TIMESTAMP_PARA para = new Win32.CRYPT_TIMESTAMP_PARA()
                    {
                        fRequestCerts = true,
                        pszTSAPolicyId = IntPtr.Zero,
                    };

                    fixed (byte* pbNonce = nonce)
                    {
                        para.Nonce.cbData = (uint)nonce.Length;
                        para.Nonce.pbData = (IntPtr)pbNonce;

                        if (!Win32.CryptRetrieveTimeStamp(
                            timeStampUrl,
                            0,
                            60 * 1000,  // 1 minute timeout
                            algId,
                            ref para,
                            sigValueBytes,
                            sigValueBytes.Length,
                            ref ppTsContext,
                            ref ppTsSigner,
                            ref phStore))
                        {
                            throw new CryptographicException(Marshal.GetLastWin32Error());
                        }
                    }

                    var timestampContext = (Win32.CRYPT_TIMESTAMP_CONTEXT)Marshal.PtrToStructure(ppTsContext, typeof(Win32.CRYPT_TIMESTAMP_CONTEXT));
                    byte[] encodedBytes = new byte[(int)timestampContext.cbEncoded];
                    Marshal.Copy(timestampContext.pbEncoded, encodedBytes, 0, (int)timestampContext.cbEncoded);
                    timestamp = Convert.ToBase64String(encodedBytes);
                }
                finally
                {
                    if (ppTsContext != IntPtr.Zero)
                        Win32.CryptMemFree(ppTsContext);

                    if (ppTsSigner != IntPtr.Zero)
                        Win32.CertFreeCertificateContext(ppTsSigner);

                    if (phStore != IntPtr.Zero)
                        Win32.CertCloseStore(phStore, 0);
                }
            }

            return timestamp;
        }

        private static void TimestampSignedLicenseDom(XmlDocument licenseDom, string timeStampUrl, bool useSha256)
        {
            XmlNamespaceManager nsm = new XmlNamespaceManager(licenseDom.NameTable);
            nsm.AddNamespace("r", LicenseNamespaceUri);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);
            nsm.AddNamespace("as", AuthenticodeNamespaceUri);

            string timestamp = String.Empty;

            try
            {
                // Try RFC3161 first
                XmlElement signatureValueNode = GetSingleNode(licenseDom, "r:license/r:issuer/ds:Signature/ds:SignatureValue", nsm) as XmlElement;
                string signatureValue = signatureValueNode.InnerText;
                timestamp = ObtainRFC3161Timestamp(timeStampUrl, signatureValue, useSha256);
            }
            // Catch all exceptions to ensure fallback to old code (non-RFC3161)
            catch
            {
                Win32.CRYPT_DATA_BLOB timestampBlob = new Win32.CRYPT_DATA_BLOB();

                byte[] licenseXml = Encoding.UTF8.GetBytes(licenseDom.OuterXml);

                unsafe
                {
                    fixed (byte* pbLicense = licenseXml)
                    {
                        Win32.CRYPT_DATA_BLOB licenseBlob = new Win32.CRYPT_DATA_BLOB();
                        IntPtr pvLicense = new IntPtr(pbLicense);
                        licenseBlob.cbData = (uint)licenseXml.Length;
                        licenseBlob.pbData = pvLicense;

                        int hr = Win32.CertTimestampAuthenticodeLicense(ref licenseBlob, timeStampUrl, ref timestampBlob);
                        if (hr != Win32.S_OK)
                        {
                            throw new CryptographicException(hr);
                        }
                    }
                }

                byte[] timestampSignature = new byte[timestampBlob.cbData];
                Marshal.Copy(timestampBlob.pbData, timestampSignature, 0, timestampSignature.Length);
                Win32.HeapFree(Win32.GetProcessHeap(), 0, timestampBlob.pbData);
                timestamp = Encoding.UTF8.GetString(timestampSignature);
            }

            XmlElement asTimestamp = licenseDom.CreateElement("as", "Timestamp", AuthenticodeNamespaceUri);
            asTimestamp.InnerText = timestamp;

            XmlElement dsObject = licenseDom.CreateElement("Object", SignedXml.XmlDsigNamespaceUrl);
            dsObject.AppendChild(asTimestamp);

            XmlElement signatureNode = GetSingleNode(licenseDom, "r:license/r:issuer/ds:Signature", nsm) as XmlElement;
            signatureNode.AppendChild(dsObject);
        }

        private static void StrongNameSignManifestDom(XmlDocument manifestDom, XmlDocument licenseDom, CmiManifestSigner2 signer, bool useSha256)
        {
            RSA snKey = signer.StrongNameKey as RSA;

            // Make sure it is RSA, as this is the only one Fusion will support.
            if (snKey == null)
            {
                throw new NotSupportedException();
            }

            // Setup namespace manager.
            XmlNamespaceManager nsm = new XmlNamespaceManager(manifestDom.NameTable);
            nsm.AddNamespace("asm", AssemblyNamespaceUri);

            // Get to root element.
            XmlElement signatureParent = GetSingleNode(manifestDom, "asm:assembly", nsm) as XmlElement;
            if (signatureParent == null)
            {
                throw new CryptographicException(Win32.TRUST_E_SUBJECT_FORM_UNKNOWN);
            }

            if (!(signer.StrongNameKey is RSA))
            {
                throw new NotSupportedException();
            }

            // Setup up XMLDSIG engine.
            ManifestSignedXml2 signedXml = new ManifestSignedXml2(signatureParent);
            if (signer.StrongNameKey is RSACryptoServiceProvider)
            {
                signedXml.SigningKey = GetFixedRSACryptoServiceProvider(signer.StrongNameKey as RSACryptoServiceProvider, useSha256);
            }
            else
            {
                signedXml.SigningKey = signer.StrongNameKey;
            }
            signedXml.SignedInfo.CanonicalizationMethod = SignedXml.XmlDsigExcC14NTransformUrl;
            if (signer.UseSha256)
                signedXml.SignedInfo.SignatureMethod = Sha256SignatureMethodUri;

            // Add the key information.
            signedXml.KeyInfo.AddClause(new RSAKeyValue(snKey));
            if (licenseDom != null)
            {
                signedXml.KeyInfo.AddClause(new KeyInfoNode(licenseDom.DocumentElement));
            }
            signedXml.KeyInfo.Id = "StrongNameKeyInfo";

            // Add the enveloped reference.
            Reference enveloped = new Reference();
            enveloped.Uri = "";
            if (signer.UseSha256)
                enveloped.DigestMethod = Sha256DigestMethod;

            // Add an enveloped then Exc-C14N transform.
            enveloped.AddTransform(new XmlDsigEnvelopedSignatureTransform());
            enveloped.AddTransform(new XmlDsigExcC14NTransform());
            signedXml.AddReference(enveloped);

#if (false) // DSIE: New format does not sign KeyInfo.
            // Add the key info reference.
            Reference strongNameKeyInfo = new Reference();
            strongNameKeyInfo.Uri = "#StrongNameKeyInfo";
            strongNameKeyInfo.AddTransform(new XmlDsigExcC14NTransform());
            signedXml.AddReference(strongNameKeyInfo);
#endif
            // Compute the signature.
            signedXml.ComputeSignature();

            // Get the XML representation
            XmlElement xmlDigitalSignature = signedXml.GetXml();
            xmlDigitalSignature.SetAttribute("Id", "StrongNameSignature");

            // Insert the signature now.
            signatureParent.AppendChild(xmlDigitalSignature);
        }
        private static readonly char[] hexValues = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

        private static string BytesToHexString(byte[] array, int start, int end)
        {
            string result = null;
            if (array != null)
            {
                char[] hexOrder = new char[(end - start) * 2];
                int i = end;
                int digit, j = 0;
                while (i-- > start)
                {
                    digit = (array[i] & 0xf0) >> 4;
                    hexOrder[j++] = hexValues[digit];
                    digit = (array[i] & 0x0f);
                    hexOrder[j++] = hexValues[digit];
                }
                result = new String(hexOrder);
            }
            return result;
        }

        private static byte[] HexStringToBytes(string hexString)
        {
            uint cbHex = (uint)hexString.Length / 2;
            byte[] hex = new byte[cbHex];
            int i = hexString.Length - 2;
            for (int index = 0; index < cbHex; index++)
            {
                hex[index] = (byte)((HexToByte(hexString[i]) << 4) | HexToByte(hexString[i + 1]));
                i -= 2;
            }
            return hex;
        }

        private static byte HexToByte(char val)
        {
            if (val <= '9' && val >= '0')
                return (byte)(val - '0');
            else if (val >= 'a' && val <= 'f')
                return (byte)((val - 'a') + 10);
            else if (val >= 'A' && val <= 'F')
                return (byte)((val - 'A') + 10);
            else
                return 0xFF;
        }

        private static XmlNode GetSingleNode(XmlNode parentNode,
                                             string xPath,
                                             XmlNamespaceManager namespaceManager = null)
        {
            XmlNodeList nodes = (namespaceManager != null) ? parentNode.SelectNodes(xPath, namespaceManager) : parentNode.SelectNodes(xPath);

            if (nodes == null)
            {
                return null;
            }

            // throw if multiple nodes are found
            if (nodes.Count > 1)
            {
                throw new CryptographicException(Win32.TRUST_E_SYSTEM_ERROR); //Throws a System Trust error
            }

            return nodes[0];
        }
    }

    internal class CmiManifestSigner2
    {
        private AsymmetricAlgorithm m_strongNameKey;
        private X509Certificate2 m_certificate;
        private string m_description;
        private string m_url;
        private X509Certificate2Collection m_certificates;
        private X509IncludeOption m_includeOption;
        private CmiManifestSignerFlag m_signerFlag;
        private bool m_useSha256;

        private CmiManifestSigner2() { }

        internal CmiManifestSigner2(AsymmetricAlgorithm strongNameKey) :
            this(strongNameKey, null, false)
        { }

        internal CmiManifestSigner2(AsymmetricAlgorithm strongNameKey, X509Certificate2 certificate, bool useSha256)
        {
            if (strongNameKey == null)
                throw new ArgumentNullException("strongNameKey");

#if (true) // 
            RSA rsa = strongNameKey as RSA;
            if (rsa == null)
                throw new ArgumentNullException("strongNameKey");
#endif
            m_strongNameKey = strongNameKey;
            m_certificate = certificate;
            m_certificates = new X509Certificate2Collection();
            m_includeOption = X509IncludeOption.ExcludeRoot;
            m_signerFlag = CmiManifestSignerFlag.None;
            m_useSha256 = useSha256;
        }

        internal bool UseSha256
        {
            get
            {
                return m_useSha256;
            }
        }

        internal AsymmetricAlgorithm StrongNameKey
        {
            get
            {
                return m_strongNameKey;
            }
        }

        internal X509Certificate2 Certificate
        {
            get
            {
                return m_certificate;
            }
        }

        internal string Description
        {
            get
            {
                return m_description;
            }
            set
            {
                m_description = value;
            }
        }

        internal string DescriptionUrl
        {
            get
            {
                return m_url;
            }
            set
            {
                m_url = value;
            }
        }

        internal X509Certificate2Collection ExtraStore
        {
            get
            {
                return m_certificates;
            }
        }

        internal X509IncludeOption IncludeOption
        {
            get
            {
                return m_includeOption;
            }
            set
            {
                if (value < X509IncludeOption.None || value > X509IncludeOption.WholeChain)
                    throw new ArgumentException("value");
                if (m_includeOption == X509IncludeOption.None)
                    throw new NotSupportedException();
                m_includeOption = value;
            }
        }

        internal CmiManifestSignerFlag Flag
        {
            get
            {
                return m_signerFlag;
            }
            set
            {
                unchecked
                {
                    if ((value & ((CmiManifestSignerFlag)~CimManifestSignerFlagMask)) != 0)
                        throw new ArgumentException("value");
                }
                m_signerFlag = value;
            }
        }

        internal const uint CimManifestSignerFlagMask = (uint)0x00000001;
    }
}

