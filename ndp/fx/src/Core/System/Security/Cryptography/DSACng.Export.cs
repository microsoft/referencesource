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
        public override DSAParameters ExportParameters(bool includePrivateParameters)
        {
            byte[] blob = Key.Export(includePrivateParameters ? CngKeyBlobFormat.GenericPrivateBlob : CngKeyBlobFormat.GenericPublicBlob);
            using (BinaryReader br = new BinaryReader(new MemoryStream(blob)))
            {
                try
                {
                    DSAParameters dp = new DSAParameters();

                    KeyBlobMagicNumber magic = (KeyBlobMagicNumber)(br.ReadInt32());
                    if (magic == KeyBlobMagicNumber.DsaPublic || magic == KeyBlobMagicNumber.DsaPrivate)
                    {
                        if (includePrivateParameters && magic != KeyBlobMagicNumber.DsaPrivate)
                            throw new CryptographicException(SR.Cryptography_NotValidPublicOrPrivateKey);

                        // Read out a (V1) BCRYPT_DSA_KEY_BLOB structure.
                        int keySizeInBytes = br.ReadInt32();
                        dp.Counter = FromBigEndian(br.ReadBytes(4));
                        dp.Seed = br.ReadBytes(Sha1HashOutputSize);
                        dp.Q = br.ReadBytes(Sha1HashOutputSize);
                        dp.P = br.ReadBytes(keySizeInBytes);
                        dp.G = br.ReadBytes(keySizeInBytes);
                        dp.Y = br.ReadBytes(keySizeInBytes);
                        if (includePrivateParameters)
                        {
                            dp.X = br.ReadBytes(Sha1HashOutputSize);
                        }
                    }
                    else if (magic == KeyBlobMagicNumber.DsaPublicV2 || magic == KeyBlobMagicNumber.DsaPrivateV2)
                    {
                        if (includePrivateParameters && magic != KeyBlobMagicNumber.DsaPrivateV2)
                            throw new CryptographicException(SR.Cryptography_NotValidPublicOrPrivateKey);

                        // Read out a BCRYPT_DSA_KEY_BLOB_V2 structure.
                        int keySizeInBytes = br.ReadInt32();
                        HASHALGORITHM_ENUM hashAlgorithm = (HASHALGORITHM_ENUM)(br.ReadInt32());
                        DSAFIPSVERSION_ENUM standardVersion = (DSAFIPSVERSION_ENUM)(br.ReadInt32());
                        int seedLengthInBytes = br.ReadInt32();
                        int qLengthInBytes = br.ReadInt32();
                        dp.Counter = FromBigEndian(br.ReadBytes(4));
                        dp.Seed = br.ReadBytes(seedLengthInBytes);
                        dp.Q = br.ReadBytes(qLengthInBytes);
                        dp.P = br.ReadBytes(keySizeInBytes);
                        dp.G = br.ReadBytes(keySizeInBytes);
                        dp.Y = br.ReadBytes(keySizeInBytes);
                        if (includePrivateParameters)
                        {
                            dp.X = br.ReadBytes(qLengthInBytes);
                        }
                    }
                    else
                    {
                        throw new CryptographicException(SR.Cryptography_NotValidPublicOrPrivateKey);
                    }

                    // If no counter/seed information is present, normalize Counter and Seed to 0/null to maintain parity with the CAPI version of DSA.
                    if (dp.Counter == -1)
                    {
                        dp.Counter = 0;
                        dp.Seed = null;
                    }
                    return dp;
                }
                catch (EndOfStreamException)
                {
                    throw new CryptographicException(SR.Cryptography_NotValidPublicOrPrivateKey);
                }
            }
        }

        private static int FromBigEndian(byte[] b)
        {
            return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
        }
    }
}
