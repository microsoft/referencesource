// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

using System.Diagnostics;
using Microsoft.Win32.SafeHandles;
using Internal.Cryptography;

namespace System.Security.Cryptography
{
    public sealed partial class CngKey : IDisposable
    {
        /// <summary>
        /// Does the key represent a named curve (Win10+)
        /// </summary>
        /// <returns></returns>
        internal bool IsECNamedCurve()
        {
            return IsECNamedCurve(Algorithm.Algorithm);
        }

        internal static bool IsECNamedCurve(string algorithm)
        {
            return (algorithm == CngAlgorithm.ECDiffieHellman.Algorithm ||
                algorithm == CngAlgorithm.ECDsa.Algorithm);
        }

        [SecuritySafeCritical]
        internal string GetCurveName()
        {
            if (IsECNamedCurve())
            {
                return NCryptNative.GetPropertyAsString(
                    m_keyHandle,
                    KeyPropertyName.ECCCurveName,
                    CngPropertyOptions.None);
            }

            // Use hard-coded values (for use with pre-Win10 APIs)
            return GetECSpecificCurveName();
        }

        private string GetECSpecificCurveName()
        {
            string algorithm = Algorithm.Algorithm;

            if (algorithm == CngAlgorithm.ECDiffieHellmanP256.Algorithm ||
                algorithm == CngAlgorithm.ECDsaP256.Algorithm)
            {
                return BCryptNative.BCRYPT_ECC_CURVE_NISTP256;
            }

            if (algorithm == CngAlgorithm.ECDiffieHellmanP384.Algorithm ||
                algorithm == CngAlgorithm.ECDsaP384.Algorithm)
            {
                return BCryptNative.BCRYPT_ECC_CURVE_NISTP384;
            }

            if (algorithm == CngAlgorithm.ECDiffieHellmanP521.Algorithm ||
                algorithm == CngAlgorithm.ECDsaP521.Algorithm)
            {
                return BCryptNative.BCRYPT_ECC_CURVE_NISTP521;
            }

            Debug.Fail(string.Format("Unknown curve {0}", algorithm));
            throw new PlatformNotSupportedException(SR.GetString(SR.Cryptography_CurveNotSupported, algorithm));
        }

        /// <summary>
        ///     Return a CngProperty representing a named curve.
        /// </summary>
        internal static CngProperty GetPropertyFromNamedCurve(ECCurve curve)
        {
            string curveName = curve.Oid.FriendlyName ?? "";
            unsafe
            {
                byte[] curveNameBytes = new byte[(curveName.Length + 1) * sizeof(char)]; // +1 to add trailing null
                System.Text.Encoding.Unicode.GetBytes(curveName, 0, curveName.Length, curveNameBytes, 0);
                return new CngProperty(KeyPropertyName.ECCCurveName, curveNameBytes, CngPropertyOptions.None);
            }
        }

        /// <summary>
        /// Map a curve name to algorithm. This enables curves that worked pre-Win10
        /// to work with newer APIs for import and export.
        /// </summary>
        internal static CngAlgorithm EcdsaCurveNameToAlgorithm(string name)
        {
            switch (name)
            {
                case BCryptNative.BCRYPT_ECC_CURVE_NISTP256:
                case "ECDSA_P256":
                    return CngAlgorithm.ECDsaP256;

                case BCryptNative.BCRYPT_ECC_CURVE_NISTP384:
                case "ECDSA_P384":
                    return CngAlgorithm.ECDsaP384;

                case BCryptNative.BCRYPT_ECC_CURVE_NISTP521:
                case "ECDSA_P521":
                    return CngAlgorithm.ECDsaP521;
            }

            // All other curves are new in Win10 so use generic algorithm
            return CngAlgorithm.ECDsa;
        }

        /// <summary>
        /// Map a curve name to algorithm. This enables curves that worked pre-Win10
        /// to work with newer APIs for import and export.
        /// </summary>
        internal static CngAlgorithm EcdhCurveNameToAlgorithm(string name)
        {
            switch (name)
            {
                case BCryptNative.BCRYPT_ECC_CURVE_NISTP256:
                case "ECDH_P256":
                    return CngAlgorithm.ECDiffieHellmanP256;

                case BCryptNative.BCRYPT_ECC_CURVE_NISTP384:
                case "ECDH_P384":
                    return CngAlgorithm.ECDiffieHellmanP384;

                case BCryptNative.BCRYPT_ECC_CURVE_NISTP521:
                case "ECDH_P521":
                    return CngAlgorithm.ECDiffieHellmanP521;
            }

            // All other curves are new in Win10 so use generic algorithm
            return CngAlgorithm.ECDiffieHellman;
        }

        internal static CngKey Create(ECCurve curve, Func<string, CngAlgorithm> algorithmResolver)
        {
            System.Diagnostics.Debug.Assert(algorithmResolver != null);

            curve.Validate();

            CngKeyCreationParameters creationParameters = new CngKeyCreationParameters
            {
                ExportPolicy = CngExportPolicies.AllowPlaintextExport,
            };

            CngAlgorithm alg;

            if (curve.IsNamed)
            {
                // Map curve name to algorithm to support pre-Win10 curves
                alg = algorithmResolver(curve.Oid.FriendlyName);

                if (CngKey.IsECNamedCurve(alg.Algorithm))
                {
                    creationParameters.Parameters.Add(GetPropertyFromNamedCurve(curve));
                }
                else
                {
                    if (alg == CngAlgorithm.ECDsaP256 || alg == CngAlgorithm.ECDiffieHellmanP256 ||
                        alg == CngAlgorithm.ECDsaP384 || alg == CngAlgorithm.ECDiffieHellmanP384 ||
                        alg == CngAlgorithm.ECDsaP521 || alg == CngAlgorithm.ECDiffieHellmanP521)
                    {
                        // No parameters required, the algorithm ID has everything built-in.
                    }
                    else
                    {
                        Debug.Fail(string.Format("Unknown algorithm {0}", alg.ToString()));
                        throw new ArgumentException(SR.GetString(SR.Cryptography_InvalidKeySize));
                    }
                }
            }
            else if (curve.IsPrime)
            {
                byte[] parametersBlob = ECCng.GetPrimeCurveParameterBlob(ref curve);

                CngProperty prop = new CngProperty(
                    KeyPropertyName.ECCParameters,
                    parametersBlob,
                    CngPropertyOptions.None);

                creationParameters.Parameters.Add(prop);
                alg = algorithmResolver(null);
            }
            else
            {
                throw new PlatformNotSupportedException(
                    SR.GetString(SR.Cryptography_CurveNotSupported, curve.CurveType.ToString()));
            }

            try
            {
                return Create(alg, null, creationParameters);
            }
            catch (CryptographicException e)
            {
                Interop.NCrypt.ErrorCode errorCode = (Interop.NCrypt.ErrorCode)e.HResult;

                if (errorCode == Interop.NCrypt.ErrorCode.NTE_INVALID_PARAMETER ||
                    errorCode == Interop.NCrypt.ErrorCode.NTE_NOT_SUPPORTED)
                {
                    string target = curve.IsNamed ? curve.Oid.FriendlyName : curve.CurveType.ToString();
                    throw new PlatformNotSupportedException(SR.GetString(SR.Cryptography_CurveNotSupported, target), e);
                }

                throw;
            }
        }
    }
}
