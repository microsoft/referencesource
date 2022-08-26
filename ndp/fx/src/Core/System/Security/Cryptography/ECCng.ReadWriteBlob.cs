// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

using System.Diagnostics;
using Internal.Cryptography;

using BCRYPT_ECC_PARAMETER_HEADER = Interop.BCrypt.BCRYPT_ECC_PARAMETER_HEADER;
using BCRYPT_ECCFULLKEY_BLOB = Interop.BCrypt.BCRYPT_ECCFULLKEY_BLOB;
using BCRYPT_ECCKEY_BLOB = Interop.BCrypt.BCRYPT_ECCKEY_BLOB;
using ErrorCode = Interop.NCrypt.ErrorCode;
using ECC_CURVE_TYPE_ENUM = Interop.BCrypt.ECC_CURVE_TYPE_ENUM;
using KeyBlobMagicNumber = Interop.BCrypt.KeyBlobMagicNumber;

namespace System.Security.Cryptography
{
    internal static partial class ECCng
    {
        [SecuritySafeCritical]
        private static byte[] GetNamedCurveBlob(
            ref ECParameters parameters,
            Func<string, bool, KeyBlobMagicNumber> magicResolver)
        {
            Debug.Assert(parameters.Curve.IsNamed);

            bool includePrivateParameters = (parameters.D != null);
            byte[] blob;
            int offset;
            int blobSize;

            // We need to build a key blob structured as follows:
            //     BCRYPT_ECCKEY_BLOB   header
            //     byte[cbKey]          Q.X
            //     byte[cbKey]          Q.Y
            //     -- Only if "includePrivateParameters" is true --
            //     byte[cbKey]          D

            unsafe
            {
                blobSize = sizeof(BCRYPT_ECCKEY_BLOB) +
                    parameters.Q.X.Length +
                    parameters.Q.Y.Length;

                if (includePrivateParameters)
                {
                    blobSize += parameters.D.Length;
                }

                blob = new byte[blobSize];

                fixed (byte* pBlob = blob)
                {
                    // Build the header
                    BCRYPT_ECCKEY_BLOB* pBcryptBlob = (BCRYPT_ECCKEY_BLOB*)pBlob;
                    pBcryptBlob->Magic = magicResolver(parameters.Curve.Oid.FriendlyName, includePrivateParameters);
                    pBcryptBlob->cbKey = parameters.Q.X.Length;
                }

                offset = sizeof(BCRYPT_ECCKEY_BLOB);
            }

            // Emit the blob
            Interop.BCrypt.Emit(blob, ref offset, parameters.Q.X);
            Interop.BCrypt.Emit(blob, ref offset, parameters.Q.Y);
            if (includePrivateParameters)
            {
                Interop.BCrypt.Emit(blob, ref offset, parameters.D);
            }

            // We better have computed the right allocation size above!
            Debug.Assert(offset == blobSize, "offset == blobSize");
            return blob;
        }

        [SecuritySafeCritical]
        private static byte[] GetPrimeCurveBlob(
            ref ECParameters parameters,
            Func<bool, KeyBlobMagicNumber> magicResolver)
        {
            Debug.Assert(parameters.Curve.IsPrime);

            bool includePrivateParameters = (parameters.D != null);
            ECCurve curve = parameters.Curve;
            byte[] blob;
            int offset;
            int blobSize;

            // We need to build a key blob structured as follows:
            //     BCRYPT_ECCFULLKEY_BLOB       header
            //     byte[cbFieldLength]          P
            //     byte[cbFieldLength]          A
            //     byte[cbFieldLength]          B
            //     byte[cbFieldLength]          G.X
            //     byte[cbFieldLength]          G.Y
            //     byte[cbSubgroupOrder]        Order (n)
            //     byte[cbCofactor]             Cofactor (h)
            //     byte[cbSeed]                 Seed
            //     byte[cbFieldLength]          Q.X
            //     byte[cbFieldLength]          Q.Y
            //     -- Only if "includePrivateParameters" is true --
            //     byte[cbSubgroupOrder]        D

            unsafe
            {
                blobSize = sizeof(BCRYPT_ECCFULLKEY_BLOB) +
                    curve.Prime.Length +
                    curve.A.Length +
                    curve.B.Length +
                    curve.G.X.Length +
                    curve.G.Y.Length +
                    curve.Order.Length +
                    curve.Cofactor.Length +
                    (curve.Seed == null ? 0 : curve.Seed.Length) +
                    parameters.Q.X.Length +
                    parameters.Q.Y.Length;

                if (includePrivateParameters)
                {
                    blobSize += parameters.D.Length;
                }

                blob = new byte[blobSize];

                fixed (byte* pBlob = blob)
                {
                    // Build the header
                    BCRYPT_ECCFULLKEY_BLOB* pBcryptBlob = (BCRYPT_ECCFULLKEY_BLOB*)pBlob;
                    pBcryptBlob->Version = 1; // No constant for this found in bcrypt.h
                    pBcryptBlob->Magic = magicResolver(includePrivateParameters);
                    pBcryptBlob->cbCofactor = curve.Cofactor.Length;
                    pBcryptBlob->cbFieldLength = parameters.Q.X.Length;
                    pBcryptBlob->cbSeed = curve.Seed == null ? 0 : curve.Seed.Length;
                    pBcryptBlob->cbSubgroupOrder = curve.Order.Length;
                    pBcryptBlob->CurveGenerationAlgId = GetHashAlgorithmId(curve.Hash);
                    pBcryptBlob->CurveType = ConvertToCurveTypeEnum(curve.CurveType);
                }

                offset = sizeof(BCRYPT_ECCFULLKEY_BLOB);
            }

            // Emit the blob
            Interop.BCrypt.Emit(blob, ref offset, curve.Prime);
            Interop.BCrypt.Emit(blob, ref offset, curve.A);
            Interop.BCrypt.Emit(blob, ref offset, curve.B);
            Interop.BCrypt.Emit(blob, ref offset, curve.G.X);
            Interop.BCrypt.Emit(blob, ref offset, curve.G.Y);
            Interop.BCrypt.Emit(blob, ref offset, curve.Order);
            Interop.BCrypt.Emit(blob, ref offset, curve.Cofactor);
            if (curve.Seed != null)
            {
                Interop.BCrypt.Emit(blob, ref offset, curve.Seed);
            }
            Interop.BCrypt.Emit(blob, ref offset, parameters.Q.X);
            Interop.BCrypt.Emit(blob, ref offset, parameters.Q.Y);
            if (includePrivateParameters)
            {
                Interop.BCrypt.Emit(blob, ref offset, parameters.D);
            }

            // We better have computed the right allocation size above!
            Debug.Assert(offset == blobSize, "offset == blobSize");

            return blob;
        }

        [SecuritySafeCritical]
        private static void ExportNamedCurveParameters(
            ref ECParameters ecParams,
            byte[] ecBlob,
            bool includePrivateParameters)
        {
            // We now have a buffer laid out as follows:
            //     BCRYPT_ECCKEY_BLOB   header
            //     byte[cbKey]          Q.X
            //     byte[cbKey]          Q.Y
            //     -- Private only --
            //     byte[cbKey]          D

            KeyBlobMagicNumber magic = (KeyBlobMagicNumber)BitConverter.ToInt32(ecBlob, 0);

            // Check the magic value in the key blob header. If the blob does not have the required magic,
            // then throw a CryptographicException.
            CheckMagicValueOfKey(magic, includePrivateParameters);

            unsafe
            {
                // Fail-fast if a rogue provider gave us a blob that isn't even the size of the blob header.
                if (ecBlob.Length < sizeof(BCRYPT_ECCKEY_BLOB))
                    throw ErrorCode.E_FAIL.ToCryptographicException();

                fixed (byte* pEcBlob = ecBlob)
                {
                    BCRYPT_ECCKEY_BLOB* pBcryptBlob = (BCRYPT_ECCKEY_BLOB*)pEcBlob;

                    int offset = sizeof(BCRYPT_ECCKEY_BLOB);

                    ecParams.Q = new ECPoint
                    {
                        X = Interop.BCrypt.Consume(ecBlob, ref offset, pBcryptBlob->cbKey),
                        Y = Interop.BCrypt.Consume(ecBlob, ref offset, pBcryptBlob->cbKey)
                    };

                    if (includePrivateParameters)
                    {
                        ecParams.D = Interop.BCrypt.Consume(ecBlob, ref offset, pBcryptBlob->cbKey);
                    }
                }
            }
        }

        [SecuritySafeCritical]
        internal static void ExportPrimeCurveParameters(
            ref ECParameters ecParams,
            byte[] ecBlob,
            bool includePrivateParameters)
        {
            // We now have a buffer laid out as follows:
            //     BCRYPT_ECCFULLKEY_BLOB       header
            //     byte[cbFieldLength]          P
            //     byte[cbFieldLength]          A
            //     byte[cbFieldLength]          B
            //     byte[cbFieldLength]          G.X
            //     byte[cbFieldLength]          G.Y
            //     byte[cbSubgroupOrder]        Order (n)
            //     byte[cbCofactor]             Cofactor (h)
            //     byte[cbSeed]                 Seed
            //     byte[cbFieldLength]          Q.X
            //     byte[cbFieldLength]          Q.Y
            //     -- Private only --
            //     byte[cbSubgroupOrder]        D

            KeyBlobMagicNumber magic = (KeyBlobMagicNumber)BitConverter.ToInt32(ecBlob, 0);

            // Check the magic value in the key blob header. If the blob does not have the required magic,
            // then throw a CryptographicException.
            CheckMagicValueOfKey(magic, includePrivateParameters);

            unsafe
            {
                // Fail-fast if a rogue provider gave us a blob that isn't even the size of the blob header.
                if (ecBlob.Length < sizeof(BCRYPT_ECCFULLKEY_BLOB))
                    throw ErrorCode.E_FAIL.ToCryptographicException();

                fixed (byte* pEcBlob = ecBlob)
                {
                    BCRYPT_ECCFULLKEY_BLOB* pBcryptBlob = (BCRYPT_ECCFULLKEY_BLOB*)pEcBlob;

                    var primeCurve = new ECCurve();
                    primeCurve.CurveType = ConvertToCurveTypeEnum(pBcryptBlob->CurveType);
                    primeCurve.Hash = GetHashAlgorithmName(pBcryptBlob->CurveGenerationAlgId);

                    int offset = sizeof(BCRYPT_ECCFULLKEY_BLOB);

                    primeCurve.Prime = Interop.BCrypt.Consume(ecBlob, ref offset, pBcryptBlob->cbFieldLength);
                    primeCurve.A = Interop.BCrypt.Consume(ecBlob, ref offset, pBcryptBlob->cbFieldLength);
                    primeCurve.B = Interop.BCrypt.Consume(ecBlob, ref offset, pBcryptBlob->cbFieldLength);
                    primeCurve.G = new ECPoint()
                    {
                        X = Interop.BCrypt.Consume(ecBlob, ref offset, pBcryptBlob->cbFieldLength),
                        Y = Interop.BCrypt.Consume(ecBlob, ref offset, pBcryptBlob->cbFieldLength),
                    };
                    primeCurve.Order = Interop.BCrypt.Consume(ecBlob, ref offset, pBcryptBlob->cbSubgroupOrder);
                    primeCurve.Cofactor = Interop.BCrypt.Consume(ecBlob, ref offset, pBcryptBlob->cbCofactor);

                    // Optional parameters
                    primeCurve.Seed = pBcryptBlob->cbSeed == 0 ?
                        null :
                        Interop.BCrypt.Consume(ecBlob, ref offset, pBcryptBlob->cbSeed);

                    ecParams.Q = new ECPoint
                    {
                        X = Interop.BCrypt.Consume(ecBlob, ref offset, pBcryptBlob->cbFieldLength),
                        Y = Interop.BCrypt.Consume(ecBlob, ref offset, pBcryptBlob->cbFieldLength)
                    };

                    if (includePrivateParameters)
                    {
                        ecParams.D = Interop.BCrypt.Consume(ecBlob, ref offset, pBcryptBlob->cbSubgroupOrder);
                    }

                    ecParams.Curve = primeCurve;
                }
            }
        }

        [SecuritySafeCritical]
        internal static byte[] GetPrimeCurveParameterBlob(ref ECCurve curve)
        {
            byte[] blob;
            int offset;
            int blobSize;

            unsafe
            {
                // We need to build a key blob structured as follows:
                //     BCRYPT_ECC_PARAMETER_HEADER  header
                //     byte[cbFieldLength]          P
                //     byte[cbFieldLength]          A
                //     byte[cbFieldLength]          B
                //     byte[cbFieldLength]          G.X
                //     byte[cbFieldLength]          G.Y
                //     byte[cbSubgroupOrder]        Order (n)
                //     byte[cbCofactor]             Cofactor (h)
                //     byte[cbSeed]                 Seed

                blobSize = sizeof(BCRYPT_ECC_PARAMETER_HEADER) +
                    curve.Prime.Length +
                    curve.A.Length +
                    curve.B.Length +
                    curve.G.X.Length +
                    curve.G.Y.Length +
                    curve.Order.Length +
                    curve.Cofactor.Length +
                    (curve.Seed == null ? 0 : curve.Seed.Length);

                blob = new byte[blobSize];
                fixed (byte* pBlob = blob)
                {
                    // Build the header
                    BCRYPT_ECC_PARAMETER_HEADER* pBcryptBlob = (BCRYPT_ECC_PARAMETER_HEADER*)pBlob;
                    pBcryptBlob->Version = Interop.BCrypt.BCRYPT_ECC_PARAMETER_HEADER_V1;
                    pBcryptBlob->cbCofactor = curve.Cofactor.Length;
                    pBcryptBlob->cbFieldLength = curve.A.Length; // P, A, B, X, Y have the same length
                    pBcryptBlob->cbSeed = curve.Seed == null ? 0 : curve.Seed.Length;
                    pBcryptBlob->cbSubgroupOrder = curve.Order.Length;
                    pBcryptBlob->CurveGenerationAlgId = ECCng.GetHashAlgorithmId(curve.Hash);
                    pBcryptBlob->CurveType = ECCng.ConvertToCurveTypeEnum(curve.CurveType);
                }

                offset = sizeof(BCRYPT_ECC_PARAMETER_HEADER);
            }

            // Emit the blob
            Interop.BCrypt.Emit(blob, ref offset, curve.Prime);
            Interop.BCrypt.Emit(blob, ref offset, curve.A);
            Interop.BCrypt.Emit(blob, ref offset, curve.B);
            Interop.BCrypt.Emit(blob, ref offset, curve.G.X);
            Interop.BCrypt.Emit(blob, ref offset, curve.G.Y);
            Interop.BCrypt.Emit(blob, ref offset, curve.Order);
            Interop.BCrypt.Emit(blob, ref offset, curve.Cofactor);

            if (curve.Seed != null)
            {
                Interop.BCrypt.Emit(blob, ref offset, curve.Seed);
            }

            // We better have computed the right allocation size above!
            Debug.Assert(offset == blobSize, "offset == blobSize");

            return blob;
        }

        /// <summary>
        ///     This function checks the magic value in the key blob header
        /// </summary>
        /// <param name="includePrivateParameters">Private blob if true else public key blob</param>
        private static void CheckMagicValueOfKey(KeyBlobMagicNumber magic, bool includePrivateParameters)
        {
            if (includePrivateParameters)
            {
                if (!IsMagicValueOfKeyPrivate(magic))
                {
                    throw new CryptographicException(SR.GetString(SR.Cryptography_NotValidPrivateKey));
                }
            }
            else
            {
                if (!IsMagicValueOfKeyPublic(magic))
                {
                    throw new CryptographicException(SR.GetString(SR.Cryptography_NotValidPublicOrPrivateKey));
                }
            }
        }

        private static bool IsMagicValueOfKeyPrivate(KeyBlobMagicNumber magic)
        {
            switch (magic)
            {
                case KeyBlobMagicNumber.BCRYPT_ECDSA_PRIVATE_GENERIC_MAGIC:
                case KeyBlobMagicNumber.BCRYPT_ECDH_PRIVATE_GENERIC_MAGIC:
                case KeyBlobMagicNumber.BCRYPT_ECDSA_PRIVATE_P256_MAGIC:
                case KeyBlobMagicNumber.BCRYPT_ECDH_PRIVATE_P256_MAGIC:
                case KeyBlobMagicNumber.BCRYPT_ECDSA_PRIVATE_P384_MAGIC:
                case KeyBlobMagicNumber.BCRYPT_ECDH_PRIVATE_P384_MAGIC:
                case KeyBlobMagicNumber.BCRYPT_ECDSA_PRIVATE_P521_MAGIC:
                case KeyBlobMagicNumber.BCRYPT_ECDH_PRIVATE_P521_MAGIC:
                    return true;
            }

            return false;
        }

        private static bool IsMagicValueOfKeyPublic(KeyBlobMagicNumber magic)
        {
            switch (magic)
            {
                case KeyBlobMagicNumber.BCRYPT_ECDSA_PUBLIC_GENERIC_MAGIC:
                case KeyBlobMagicNumber.BCRYPT_ECDH_PUBLIC_GENERIC_MAGIC:
                case KeyBlobMagicNumber.BCRYPT_ECDSA_PUBLIC_P256_MAGIC:
                case KeyBlobMagicNumber.BCRYPT_ECDH_PUBLIC_P256_MAGIC:
                case KeyBlobMagicNumber.BCRYPT_ECDSA_PUBLIC_P384_MAGIC:
                case KeyBlobMagicNumber.BCRYPT_ECDH_PUBLIC_P384_MAGIC:
                case KeyBlobMagicNumber.BCRYPT_ECDSA_PUBLIC_P521_MAGIC:
                case KeyBlobMagicNumber.BCRYPT_ECDH_PUBLIC_P521_MAGIC:
                    return true;
            }

            // Private key magic is permissible too since the public key can be derived from the private key blob.
            return IsMagicValueOfKeyPrivate(magic);
        }

        /// <summary>
        /// Helper method to map between ECC_CURVE_TYPE_ENUM and ECCurve.ECCurveType
        /// </summary>
        private static ECC_CURVE_TYPE_ENUM ConvertToCurveTypeEnum(ECCurve.ECCurveType value)
        {
            // Currently values 1-3 are interchangeable
            Debug.Assert(value == ECCurve.ECCurveType.Characteristic2 ||
                value == ECCurve.ECCurveType.PrimeShortWeierstrass ||
                value == ECCurve.ECCurveType.PrimeTwistedEdwards);
            return (ECC_CURVE_TYPE_ENUM)value;
        }

        /// <summary>
        /// Helper method to map between ECC_CURVE_TYPE_ENUM and ECCurve.ECCurveType
        /// </summary>
        private static ECCurve.ECCurveType ConvertToCurveTypeEnum(ECC_CURVE_TYPE_ENUM value)
        {
            // Currently values 1-3 are interchangeable
            ECCurve.ECCurveType curveType = (ECCurve.ECCurveType)value;
            Debug.Assert(curveType == ECCurve.ECCurveType.Characteristic2 ||
                curveType == ECCurve.ECCurveType.PrimeShortWeierstrass ||
                curveType == ECCurve.ECCurveType.PrimeTwistedEdwards);
            return curveType;
        }

        /// <summary>
        /// Get the ALG_ID from the given HashAlgorithmName
        /// </summary>
        private static Interop.BCrypt.ECC_CURVE_ALG_ID_ENUM GetHashAlgorithmId(HashAlgorithmName? name)
        {
            if (name.HasValue == false || string.IsNullOrEmpty(name.Value.Name))
            {
                return Interop.BCrypt.ECC_CURVE_ALG_ID_ENUM.BCRYPT_NO_CURVE_GENERATION_ALG_ID;
            }

            Interop.Crypt32.CRYPT_OID_INFO oid = Interop.Crypt32.FindOidInfo(
                Interop.Crypt32.CryptOidInfoKeyType.CRYPT_OID_INFO_NAME_KEY,
                name.Value.Name,
                OidGroup.HashAlgorithm,
                false);

            if (oid.AlgId == -1)
            {
                throw new CryptographicException(SR.GetString(SR.Cryptography_UnknownHashAlgorithm, name.Value.Name));
            }

            return (Interop.BCrypt.ECC_CURVE_ALG_ID_ENUM)oid.AlgId;
        }

        /// <summary>
        /// Get the HashAlgorithmName from the given ALG_ID
        /// </summary>
        private static HashAlgorithmName? GetHashAlgorithmName(Interop.BCrypt.ECC_CURVE_ALG_ID_ENUM hashId)
        {
            Interop.Crypt32.CRYPT_OID_INFO oid = Interop.Crypt32.FindAlgIdOidInfo((int)hashId);
            if (oid.AlgId == -1)
            {
                // The original hash algorithm may not be found and is optional
                return null;
            }

            return new HashAlgorithmName(oid.Name);
        }

        [SecuritySafeCritical]
        private static unsafe void FixupGenericBlob(byte[] blob)
        {
            if (blob.Length > sizeof(BCRYPT_ECCKEY_BLOB))
            {
                fixed (byte* pBlob = blob)
                {
                    BCRYPT_ECCKEY_BLOB* pBcryptBlob = (BCRYPT_ECCKEY_BLOB*)pBlob;

                    switch ((KeyBlobMagicNumber)pBcryptBlob->Magic)
                    {
                        case KeyBlobMagicNumber.BCRYPT_ECDH_PUBLIC_P256_MAGIC:
                        case KeyBlobMagicNumber.BCRYPT_ECDH_PUBLIC_P384_MAGIC:
                        case KeyBlobMagicNumber.BCRYPT_ECDH_PUBLIC_P521_MAGIC:
                            pBcryptBlob->Magic = KeyBlobMagicNumber.BCRYPT_ECDH_PUBLIC_GENERIC_MAGIC;
                            break;
                        case KeyBlobMagicNumber.BCRYPT_ECDH_PRIVATE_P256_MAGIC:
                        case KeyBlobMagicNumber.BCRYPT_ECDH_PRIVATE_P384_MAGIC:
                        case KeyBlobMagicNumber.BCRYPT_ECDH_PRIVATE_P521_MAGIC:
                            pBcryptBlob->Magic = KeyBlobMagicNumber.BCRYPT_ECDH_PRIVATE_GENERIC_MAGIC;
                            break;
                        case KeyBlobMagicNumber.BCRYPT_ECDSA_PUBLIC_P256_MAGIC:
                        case KeyBlobMagicNumber.BCRYPT_ECDSA_PUBLIC_P384_MAGIC:
                        case KeyBlobMagicNumber.BCRYPT_ECDSA_PUBLIC_P521_MAGIC:
                            pBcryptBlob->Magic = KeyBlobMagicNumber.BCRYPT_ECDSA_PUBLIC_GENERIC_MAGIC;
                            break;
                        case KeyBlobMagicNumber.BCRYPT_ECDSA_PRIVATE_P256_MAGIC:
                        case KeyBlobMagicNumber.BCRYPT_ECDSA_PRIVATE_P384_MAGIC:
                        case KeyBlobMagicNumber.BCRYPT_ECDSA_PRIVATE_P521_MAGIC:
                            pBcryptBlob->Magic = KeyBlobMagicNumber.BCRYPT_ECDSA_PRIVATE_GENERIC_MAGIC;
                            break;
                    }
                }
            }
        }

        private static CngKey ImportKeyBlob(
            byte[] blob,
            string curveName,
            CngKeyBlobFormat format,
            ECCurve.ECCurveType curveType)
        {
            try
            {
                CngKey newKey = CngKey.Import(blob, curveName, format);
                newKey.ExportPolicy |= CngExportPolicies.AllowPlaintextExport;

                return newKey;
            }
            catch (CryptographicException e)
            {
                if (curveType != ECCurve.ECCurveType.Named &&
                    e.HResult == (int)ErrorCode.NTE_NOT_SUPPORTED)
                {
                    throw new PlatformNotSupportedException(
                        SR.GetString(SR.Cryptography_CurveNotSupported, curveType),
                        e);
                }

                throw;
            }
        }

        private static byte[] ExportKeyBlob(CngKey key, bool includePrivateParameters)
        {
            CngKeyBlobFormat blobFormat = includePrivateParameters ?
                CngKeyBlobFormat.EccPrivateBlob :
                CngKeyBlobFormat.EccPublicBlob;

            return key.Export(blobFormat);
        }

        private static byte[] ExportFullKeyBlob(CngKey key, bool includePrivateParameters)
        {
            CngKeyBlobFormat blobFormat = includePrivateParameters ?
                CngKeyBlobFormat.EccFullPrivateBlob :
                CngKeyBlobFormat.EccFullPublicBlob;

            return key.Export(blobFormat);
        }

        private static byte[] ParametersToBlob(
            ref ECParameters parameters,
            Func<string, bool, KeyBlobMagicNumber> namedCurveResolver,
            Func<bool, KeyBlobMagicNumber> explicitCurveResolver,
            out CngKeyBlobFormat format,
            out string curveName)
        {
            parameters.Validate();
            ECCurve curve = parameters.Curve;
            bool includePrivateParameters = (parameters.D != null);

            if (curve.IsPrime)
            {
                curveName = null;

                format = includePrivateParameters ?
                    CngKeyBlobFormat.EccFullPrivateBlob :
                    CngKeyBlobFormat.EccFullPublicBlob;

                return GetPrimeCurveBlob(ref parameters, explicitCurveResolver);
            }
            else if (curve.IsNamed)
            {
                // FriendlyName is required; an attempt was already made to default it in ECCurve
                curveName = curve.Oid.FriendlyName;

                if (string.IsNullOrEmpty(curveName))
                {
                    throw new PlatformNotSupportedException(SR.GetString(SR.Cryptography_InvalidCurveOid, curve.Oid.Value.ToString()));
                }

                format = includePrivateParameters ?
                    CngKeyBlobFormat.EccPrivateBlob :
                    CngKeyBlobFormat.EccPublicBlob;

                return GetNamedCurveBlob(ref parameters, namedCurveResolver);
            }
            else
            {
                throw new PlatformNotSupportedException(SR.GetString(SR.Cryptography_CurveNotSupported, curve.CurveType.ToString()));
            }
        }
    }
}
