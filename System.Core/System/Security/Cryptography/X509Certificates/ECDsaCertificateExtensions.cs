// Copyright (c) Microsoft Corporation.  All rights reserved.
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace System.Security.Cryptography.X509Certificates
{
    /// <summary>
    /// Provides extension methods for retrieving <see cref="ECDsa" /> implementations for the
    /// public and private keys of a <see cref="X509Certificate2" />.
    /// </summary>
    public static class ECDsaCertificateExtensions
    {

        /// <summary>
        /// Gets the <see cref="ECDsa" /> private key from the certificate or null if 
        /// the certificate does not have an ECDsa private key.
        /// </summary>
        [SecuritySafeCritical]
        public static ECDsa GetECDsaPrivateKey(this X509Certificate2 certificate)
        {
            if (certificate == null) { throw new ArgumentNullException("certificate"); }
            //Check cert for private key and confirm it is ECDSA cert
            if (!certificate.HasPrivateKey || !IsECDsa(certificate)) { return null; }

            CngKeyHandleOpenOptions openOptions;

            using (SafeCertContextHandle certificateContext = X509Native.GetCertificateContext(certificate))
            using (SafeNCryptKeyHandle privateKeyHandle = X509Native.TryAcquireCngPrivateKey(certificateContext, out openOptions))
            {           
                CngKey key = CngKey.Open(privateKeyHandle, openOptions);
                return new ECDsaCng(key);
            }
        }

        /// <summary>
        /// Gets the <see cref="ECDsa" /> public key from the certificate or null if the certificate does not have an ECDsa public key.
        /// </summary>
        [SecuritySafeCritical]
        public static ECDsa GetECDsaPublicKey(this X509Certificate2 certificate)
        {
            if (certificate == null) { throw new ArgumentNullException("certificate"); }
            if (!IsECDsa(certificate)) { return null; }

            SafeCertContextHandle safeCertContext = X509Native.GetCertificateContext(certificate);
            IntPtr certHandle = safeCertContext.DangerousGetHandle();
            //Read the public key blob from the certificate 
            X509Native.CERT_CONTEXT pCertContext = (X509Native.CERT_CONTEXT)Marshal.PtrToStructure(certHandle, typeof(X509Native.CERT_CONTEXT));

            IntPtr pSubjectPublicKeyInfo = new IntPtr((long)pCertContext.pCertInfo +
                                           (long)Marshal.OffsetOf(typeof(X509Native.CERT_INFO), "SubjectPublicKeyInfo"));

            X509Native.CERT_PUBLIC_KEY_INFO certPublicKeyInfo = (X509Native.CERT_PUBLIC_KEY_INFO)Marshal.PtrToStructure(pSubjectPublicKeyInfo,
                                                            typeof(X509Native.CERT_PUBLIC_KEY_INFO));
            CngKey key;
            //Import the public key blob to BCRYPT_KEY_HANDLE
            using (SafeBCryptKeyHandle bcryptKeyHandle = BCryptNative.ImportAsymmetricPublicKey(certPublicKeyInfo, 0))
            {
                if (bcryptKeyHandle.IsInvalid)
                {
                    throw new CryptographicException("SR.GetString(SR.Cryptography_OpenInvalidHandle)");
                }
                key = BCryptHandleToNCryptHandle(bcryptKeyHandle);
            }
            GC.KeepAlive(safeCertContext);
            return new ECDsaCng(key);
        }

        [SecuritySafeCritical]
        public static X509Certificate2 CopyWithPrivateKey(this X509Certificate2 certificate, ECDsa privateKey)
        {
            if (certificate == null)
                throw new ArgumentNullException(nameof(certificate));
            if (privateKey == null)
                throw new ArgumentNullException(nameof(privateKey));

            if (certificate.HasPrivateKey)
                throw new InvalidOperationException(SR.GetString(SR.Cryptography_Cert_AlreadyHasPrivateKey));

            using (ECDsa publicKey = GetECDsaPublicKey(certificate))
            {
                if (publicKey == null)
                    throw new ArgumentException(SR.GetString(SR.Cryptography_PrivateKey_WrongAlgorithm));

                if (!IsSameKey(publicKey, privateKey))
                {
                    throw new ArgumentException(SR.GetString(SR.Cryptography_PrivateKey_DoesNotMatch), nameof(privateKey));
                }
            }

            ECDsaCng ecdsaCng = privateKey as ECDsaCng;
            X509Certificate2 newCert = null;

            if (ecdsaCng != null)
            {
                newCert = CertificateExtensionsCommon.CopyWithPersistedCngKey(certificate, ecdsaCng.Key);
            }

            // No CAPI option for ECDSA

            if (newCert == null)
            {
                ECParameters parameters = privateKey.ExportParameters(true);

                using (PinAndClear.Track(parameters.D))
                using (ecdsaCng = new ECDsaCng())
                {
                    ecdsaCng.ImportParameters(parameters);

                    newCert = CertificateExtensionsCommon.CopyWithEphemeralCngKey(certificate, ecdsaCng.Key);
                }
            }

            Debug.Assert(newCert != null);
            Debug.Assert(!ReferenceEquals(certificate, newCert));
            Debug.Assert(!certificate.HasPrivateKey);
            Debug.Assert(newCert.HasPrivateKey);
            return newCert;
        }

        private static bool IsSameKey(ECDsa a, ECDsa b)
        {
            ECParameters aParameters = a.ExportParameters(false);
            ECParameters bParameters = b.ExportParameters(false);

            if (aParameters.Curve.CurveType != bParameters.Curve.CurveType)
                return false;

            if (!aParameters.Q.X.SequenceEqual(bParameters.Q.X) ||
                !aParameters.Q.Y.SequenceEqual(bParameters.Q.Y))
            {
                return false;
            }

            ECCurve aCurve = aParameters.Curve;
            ECCurve bCurve = bParameters.Curve;

            if (aCurve.IsNamed)
            {
                // On Windows we care about FriendlyName, on Unix we care about Value
                return (aCurve.Oid.Value == bCurve.Oid.Value && aCurve.Oid.FriendlyName == bCurve.Oid.FriendlyName);
            }

            if (!aCurve.IsExplicit)
            {
                // Implicit curve, always fail.
                return false;
            }

            // Ignore Cofactor (which is derivable from the prime or polynomial and Order)
            // Ignore Seed and Hash (which are entirely optional, and about how A and B were built)
            if (!aCurve.G.X.SequenceEqual(bCurve.G.X) ||
                !aCurve.G.Y.SequenceEqual(bCurve.G.Y) ||
                !aCurve.Order.SequenceEqual(bCurve.Order) ||
                !aCurve.A.SequenceEqual(bCurve.A) ||
                !aCurve.B.SequenceEqual(bCurve.B))
            {
                return false;
            }

            if (aCurve.IsPrime)
            {
                return aCurve.Prime.SequenceEqual(bCurve.Prime);
            }

            if (aCurve.IsCharacteristic2)
            {
                return aCurve.Polynomial.SequenceEqual(bCurve.Polynomial);
            }

            Debug.Fail($"Missing match criteria for curve type {aCurve.CurveType}");
            return false;
        }

        /// <summary>
        /// Method take BCrypt handle as input and returns the CNGKey
        /// </summary>
        /// <param name="bcryptKeyHandle">Accepts BCrypt Handle</param>
        /// <returns>Returns CNG key with NCrypt Handle</returns>
        private static CngKey BCryptHandleToNCryptHandle(SafeBCryptKeyHandle bcryptKeyHandle)
        {            
            byte[] keyBlob = BCryptNative.ExportBCryptKey(bcryptKeyHandle, BCryptNative.BCRYPT_ECCPUBLIC_BLOB);
            //Now Import the key blob as NCRYPT_KEY_HANDLE            
            CngKey Key = CngKey.Import(keyBlob, CngKeyBlobFormat.EccPublicBlob);
            return Key;
        }

        /// <summary>
        /// Check if the certificate contains ECDsa key or ECDH / ECMQV key.
        /// </summary>
        /// <param name="certificate">Certificate object</param>
        /// <returns>true if ECDsa key. False otherwise</returns>
        private static bool IsECDsa(X509Certificate2 certificate)
        {
            string algName = certificate.PublicKey.Oid.FriendlyName;
            string value = certificate.PublicKey.Oid.Value;
            //At this point check OID. If it matches for ECC certs
            //then go to extensions and find out difference between ECDSA and ECDH certs
            if (value != X509Native.szOID_ECC_PUBLIC_KEY) { return false; }
            else
            {
                //Following section is built based on RFC 
                //http://www.ietf.org/rfc/rfc5280.txt and
                //http://www.rfc-archive.org/getrfc.php?rfc=5480. This RFC, section 3 describes when 
                // key can be ECDSA or ECDH or ECMQV.
                foreach (X509Extension extension in certificate.Extensions)
                {
                    //Check Key Usage OID value
                    if (extension.Oid.Value == "2.5.29.15")
                    {
                        X509KeyUsageExtension ext = (X509KeyUsageExtension)extension;

                        if (!(ext.KeyUsages.HasFlag(X509KeyUsageFlags.KeyAgreement)))
                        {
                            //If this does not have KeyAgreement flag present, it cannot be ECDH or ECMQV key as KeyAgreement 
                            // is mandatory flag for ECDH or ECMQV. In that case, at this point, it is safe to assume it is ECDSA
                            return true;
                        }
                        //If key has any of the following flag then it cannot be ECDH or ECMQV. Assume 
                        //it is ECDSA.
                        if (ext.KeyUsages.HasFlag(X509KeyUsageFlags.DigitalSignature) ||
                            ext.KeyUsages.HasFlag(X509KeyUsageFlags.NonRepudiation) ||
                            ext.KeyUsages.HasFlag(X509KeyUsageFlags.KeyCertSign) ||
                            ext.KeyUsages.HasFlag(X509KeyUsageFlags.CrlSign))
                        {
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    }
                }
            }
            //If key usage extension is not present in the certificate assume ECDSA 
            return true;
        }
    }
}
