// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

using AlgorithmName = System.Security.Cryptography.BCryptNative.AlgorithmName;
using KeyBlobMagicNumber = Interop.BCrypt.KeyBlobMagicNumber;

namespace System.Security.Cryptography
{
    internal static partial class ECCng
    {
        private static readonly Func<string, bool, KeyBlobMagicNumber> s_ecdhNamedMagicResolver =
            (curveName, includePrivate) => EcdhCurveNameToMagicNumber(curveName, includePrivate);

        private static readonly Func<bool, KeyBlobMagicNumber> s_ecdhExplicitMagicResolver =
            includePrivate => includePrivate ?
                KeyBlobMagicNumber.BCRYPT_ECDH_PRIVATE_GENERIC_MAGIC :
                KeyBlobMagicNumber.BCRYPT_ECDH_PUBLIC_GENERIC_MAGIC;

        private static readonly Func<string, bool, KeyBlobMagicNumber> s_ecdsaNamedMagicResolver =
            (curveName, includePrivate) => ECDsaCurveNameToMagicNumber(curveName, includePrivate);

        private static readonly Func<bool, KeyBlobMagicNumber> s_ecdsaExplicitMagicResolver =
            includePrivate => includePrivate ?
                KeyBlobMagicNumber.BCRYPT_ECDSA_PRIVATE_GENERIC_MAGIC :
                KeyBlobMagicNumber.BCRYPT_ECDSA_PUBLIC_GENERIC_MAGIC;

        /// <summary>
        /// Map a curve name to magic number. Maps the names of the curves that worked pre-Win10
        /// to the pre-Win10 magic numbers to support import on pre-Win10 environments 
        /// that don't have the named curve functionality.
        /// </summary>
        private static KeyBlobMagicNumber ECDsaCurveNameToMagicNumber(string name, bool includePrivateParameters)
        {
            switch (CngKey.EcdsaCurveNameToAlgorithm(name).Algorithm)
            {
                case AlgorithmName.ECDsaP256:
                    return includePrivateParameters ?
                        KeyBlobMagicNumber.BCRYPT_ECDSA_PRIVATE_P256_MAGIC :
                        KeyBlobMagicNumber.BCRYPT_ECDSA_PUBLIC_P256_MAGIC;

                case AlgorithmName.ECDsaP384:
                    return includePrivateParameters ?
                    KeyBlobMagicNumber.BCRYPT_ECDSA_PRIVATE_P384_MAGIC :
                    KeyBlobMagicNumber.BCRYPT_ECDSA_PUBLIC_P384_MAGIC;

                case AlgorithmName.ECDsaP521:
                    return includePrivateParameters ?
                    KeyBlobMagicNumber.BCRYPT_ECDSA_PRIVATE_P521_MAGIC :
                    KeyBlobMagicNumber.BCRYPT_ECDSA_PUBLIC_P521_MAGIC;

                default:
                    // all other curves are new in Win10 so use named curves
                    return includePrivateParameters ?
                        KeyBlobMagicNumber.BCRYPT_ECDSA_PRIVATE_GENERIC_MAGIC :
                        KeyBlobMagicNumber.BCRYPT_ECDSA_PUBLIC_GENERIC_MAGIC;
            }
        }

        private static KeyBlobMagicNumber EcdhCurveNameToMagicNumber(string name, bool includePrivateParameters)
        {
            switch (CngKey.EcdhCurveNameToAlgorithm(name).Algorithm)
            {
                case AlgorithmName.ECDHP256:
                    return includePrivateParameters ?
                        KeyBlobMagicNumber.BCRYPT_ECDH_PRIVATE_P256_MAGIC :
                        KeyBlobMagicNumber.BCRYPT_ECDH_PUBLIC_P256_MAGIC;

                case AlgorithmName.ECDHP384:
                    return includePrivateParameters ?
                    KeyBlobMagicNumber.BCRYPT_ECDH_PRIVATE_P384_MAGIC :
                    KeyBlobMagicNumber.BCRYPT_ECDH_PUBLIC_P384_MAGIC;

                case AlgorithmName.ECDHP521:
                    return includePrivateParameters ?
                    KeyBlobMagicNumber.BCRYPT_ECDH_PRIVATE_P521_MAGIC :
                    KeyBlobMagicNumber.BCRYPT_ECDH_PUBLIC_P521_MAGIC;

                default:
                    // all other curves are new in Win10 so use named curves
                    return includePrivateParameters ?
                        KeyBlobMagicNumber.BCRYPT_ECDH_PRIVATE_GENERIC_MAGIC :
                        KeyBlobMagicNumber.BCRYPT_ECDH_PUBLIC_GENERIC_MAGIC;
            }
        }
    }
}
