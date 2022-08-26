// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

using System.IO;

using KeyBlobMagicNumber = System.Security.Cryptography.BCryptNative.KeyBlobMagicNumber;

namespace System.Security.Cryptography
{
    public sealed partial class DSACng : DSA
    {
        public override void ImportParameters(DSAParameters parameters)
        {
            if (parameters.P == null || parameters.Q == null || parameters.G == null || parameters.Y == null)
                throw new ArgumentException(SR.Cryptography_InvalidDsaParameters_MissingFields);

            // J is not required and is not even used on CNG blobs. It should however be less than P (J == (P-1) / Q). This validation check
            // is just to maintain parity with DSACryptoServiceProvider, which also performs this check.
            if (parameters.J != null && parameters.J.Length >= parameters.P.Length)
                throw new ArgumentException(SR.Cryptography_InvalidDsaParameters_MismatchedPJ);

            bool hasPrivateKey = parameters.X != null;

            int keySizeInBytes = parameters.P.Length;
            int keySizeInBits = keySizeInBytes * 8;

            if (parameters.G.Length != keySizeInBytes || parameters.Y.Length != keySizeInBytes)
                throw new ArgumentException(SR.Cryptography_InvalidDsaParameters_MismatchedPGY);
            if (hasPrivateKey && parameters.X.Length != parameters.Q.Length)
                throw new ArgumentException(SR.Cryptography_InvalidDsaParameters_MismatchedQX);

            using (MemoryStream ms = new MemoryStream())
            {
                using (BinaryWriter bw = new BinaryWriter(ms))
                {
                    if (keySizeInBits <= MaxV1KeySize)
                    {
                        GenerateV1DsaBlob(bw, parameters, keySizeInBytes, hasPrivateKey);
                    }
                    else
                    {
                        GenerateV2DsaBlob(bw, parameters, keySizeInBytes, hasPrivateKey);
                    }
                }

                ms.Flush();
                byte[] blob = ms.ToArray();
                CngKey cngKey = CngKey.Import(blob, hasPrivateKey ? CngKeyBlobFormat.GenericPrivateBlob : CngKeyBlobFormat.GenericPublicBlob);
                CngExportPolicies exportPolicy = cngKey.ExportPolicy | CngExportPolicies.AllowPlaintextExport;
                cngKey.SetProperty(new CngProperty(NCryptNative.KeyPropertyName.ExportPolicy, BitConverter.GetBytes((int)exportPolicy), CngPropertyOptions.None));
                Key = cngKey;
            }
        }

        private static void GenerateV1DsaBlob(BinaryWriter bw, DSAParameters parameters, int keySizeInBytes, bool hasPrivateKey)
        {
            // Write out a (V1) BCRYPT_DSA_KEY_BLOB
            bw.Write((int)(hasPrivateKey ? KeyBlobMagicNumber.DsaPrivate : KeyBlobMagicNumber.DsaPublic));
            bw.Write((int)keySizeInBytes);

            if (parameters.Seed != null)
            {
                // The Seed length is hardcoded into BCRYPT_DSA_KEY_BLOB, so check it now we can give a nicer error message.
                if (parameters.Seed.Length != Sha1HashOutputSize)
                    throw new ArgumentException(SR.Cryptography_InvalidDsaParameters_SeedRestriction_ShortKey);

                bw.Write((byte[])(ToBigEndian(parameters.Counter)));
                bw.Write(parameters.Seed);
            }
            else
            {
                // If Seed is not present, back fill both counter and seed with 0xff. Do not use parameters.Counter as CNG is more strict than CAPI and will reject
                // anything other than 0xffffffff. That could complicate efforts to switch usage of DSACryptoServiceProvider to DSACng.
                bw.Write((uint)0xffffffff);
                for (int i = 0; i < Sha1HashOutputSize; i++)
                {
                    bw.Write((byte)0xff);
                }
            }

            // The Q length is hardcoded into BCRYPT_DSA_KEY_BLOB, so check it now we can give a nicer error message.
            if (parameters.Q.Length != Sha1HashOutputSize)
                throw new ArgumentException(SR.Cryptography_InvalidDsaParameters_QRestriction_ShortKey);

            bw.Write(parameters.Q);
            bw.Write(parameters.P);
            bw.Write(parameters.G);
            bw.Write(parameters.Y);
            if (hasPrivateKey)
            {
                bw.Write(parameters.X);
            }
        }

        private static void GenerateV2DsaBlob(BinaryWriter bw, DSAParameters parameters, int keySizeInBytes, bool hasPrivateKey)
        {
            // Write out a BCRYPT_DSA_KEY_BLOB_V2
            bw.Write((int)(hasPrivateKey ? KeyBlobMagicNumber.DsaPrivateV2 : KeyBlobMagicNumber.DsaPublicV2));
            bw.Write((int)keySizeInBytes);

            //
            // For some reason, Windows bakes the hash algorithm into the key itself. Furthermore, it demands that the Q length match the 
            // length of the named hash algorithm's output - otherwise, the Import fails. So we have to give it the hash algorithm that matches 
            // the Q length - and if there is no matching hash algorithm, we throw up our hands and throw a PlatformNotSupported.
            //
            // Note that this has no bearing on the hash algorithm you pass to SignData(). The class library (not Windows) hashes that according 
            // to the hash algorithm passed to SignData() and presents the hash result to NCryptSignHash(), truncating the hash to the Q length 
            // if necessary (and as demanded by the NIST spec.) Windows will be no wiser and we'll get the result we want. 
            //  
            HASHALGORITHM_ENUM hashAlgorithm;
            switch (parameters.Q.Length)
            {
                case Sha1HashOutputSize:
                    hashAlgorithm = HASHALGORITHM_ENUM.DSA_HASH_ALGORITHM_SHA1;
                    break;

                case Sha256HashOutputSize:
                    hashAlgorithm = HASHALGORITHM_ENUM.DSA_HASH_ALGORITHM_SHA256;
                    break;

                case Sha512HashOutputSize:
                    hashAlgorithm = HASHALGORITHM_ENUM.DSA_HASH_ALGORITHM_SHA512;
                    break;

                default:
                    throw new PlatformNotSupportedException(SR.Cryptography_InvalidDsaParameters_QRestriction_LargeKey);
            }
            bw.Write((int)hashAlgorithm);

            bw.Write((int)(DSAFIPSVERSION_ENUM.DSA_FIPS186_3));

            if (parameters.Seed != null)
            {
                bw.Write((int)(parameters.Seed.Length)); //cbSeedLength
                bw.Write((int)(parameters.Q.Length)); //cbGroupLength (the Q length)

                bw.Write((byte[])ToBigEndian(parameters.Counter));
                bw.Write(parameters.Seed);
            }
            else
            {
                // If Seed is not present, back fill both counter and seed with 0xff. Do not use parameters.Counter as CNG is more strict than CAPI and will reject
                // anything other than 0xffffffff. That could complicate efforts to switch usage of DSACryptoServiceProvider to DSACng.

                int defaultSeedLength = parameters.Q.Length;

                bw.Write((int)(defaultSeedLength)); //cbSeedLength
                bw.Write((int)(parameters.Q.Length)); //cbGroupLength (the Q length)

                bw.Write((uint)0xffffffff);
                for (int i = 0; i < defaultSeedLength; i++)
                {
                    bw.Write((byte)0xff);
                }
            }

            bw.Write(parameters.Q);
            bw.Write(parameters.P);
            bw.Write(parameters.G);
            bw.Write(parameters.Y);
            if (hasPrivateKey)
            {
                bw.Write(parameters.X);
            }
        }

        private static byte[] ToBigEndian(int i)
        {
            byte[] b = new byte[4];
            b[0] = ((byte)(i >> 24));
            b[1] = ((byte)(i >> 16));
            b[2] = ((byte)(i >> 8));
            b[3] = ((byte)(i));
            return b;
        }
    }
}
