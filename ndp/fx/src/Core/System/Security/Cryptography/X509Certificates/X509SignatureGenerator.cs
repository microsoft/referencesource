// Copyright (c) Microsoft Corporation.  All rights reserved.

namespace System.Security.Cryptography.X509Certificates
{
    public abstract class X509SignatureGenerator
    {
        private PublicKey _publicKey;

        public PublicKey PublicKey
        {
            get
            {
                if (_publicKey == null)
                {
                    _publicKey = BuildPublicKey();
                }

                return _publicKey;
            }
        }

        public abstract byte[] GetSignatureAlgorithmIdentifier(HashAlgorithmName hashAlgorithm);
        public abstract byte[] SignData(byte[] data, HashAlgorithmName hashAlgorithm);
        protected abstract PublicKey BuildPublicKey();

        public static X509SignatureGenerator CreateForECDsa(ECDsa key)
        {
            if (key == null)
                throw new ArgumentNullException(nameof(key));

            return new ECDsaX509SignatureGenerator(key);
        }

        public static X509SignatureGenerator CreateForRSA(RSA key, RSASignaturePadding signaturePadding)
        {
            if (key == null)
                throw new ArgumentNullException(nameof(key));
            if (signaturePadding == null)
                throw new ArgumentNullException(nameof(signaturePadding));

            if (signaturePadding == RSASignaturePadding.Pkcs1)
                return new RSAPkcs1X509SignatureGenerator(key);
            if (signaturePadding.Mode == RSASignaturePaddingMode.Pss)
                return new RSAPssX509SignatureGenerator(key, signaturePadding);

            throw new ArgumentException(SR.GetString(SR.Cryptography_InvalidPaddingMode));
        }
    }
}
