// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

using System.Diagnostics;
using System.IO;
using System.Security.Permissions;

using KeyBlobMagicNumber = System.Security.Cryptography.BCryptNative.KeyBlobMagicNumber;

namespace System.Security.Cryptography
{
    public sealed partial class DSACng : DSA
    {
        /// <summary>
        ///     Create a DSACng algorithm with a random 2048 bit key pair.
        /// </summary>
        public DSACng() 
            : this(keySize: 2048)
        {
        }

        /// <summary>
        ///     Creates a new DSACng object that will use a randomly generated key of the specified size.
        ///     Valid key sizes range from 512 to 3072 bits, in increments of 64. It's suggested that a
        ///     minimum size of 2048 bits be used for all keys.
        /// </summary>
        /// <param name="keySize">Size of the key to generate, in bits.</param>
        /// <exception cref="CryptographicException">if <paramref name="keySize" /> is not valid</exception>
        public DSACng(int keySize)
        {
            LegalKeySizesValue = s_legalKeySizes;
            KeySize = keySize;
        }

        /// <summary>
        ///     Creates a new DSACng object that will use the specified key. The key's
        ///     <see cref="CngKey.AlgorithmGroup" /> must be Dsa.
        ///     This constructor creates a copy of the key. Even if someone disposes the key passed
        ///     copy of this key object in DSA stays alive. 
        /// </summary>
        /// <param name="key">Key to use for DSA operations</param>
        /// <exception cref="ArgumentException">if <paramref name="key" /> is not a DSA key</exception>
        /// <exception cref="ArgumentNullException">if <paramref name="key" /> is null.</exception>
        [SecuritySafeCritical]
        [SecurityPermission(SecurityAction.Assert, UnmanagedCode = true)]
        public DSACng(CngKey key)
        {
            if (key == null)
                throw new ArgumentNullException("key");

            if (key.AlgorithmGroup != CngAlgorithmGroup.Dsa)
                throw new ArgumentException(SR.GetString(SR.Cryptography_ArgDSARequiresDSAKey), "key");

            LegalKeySizesValue = s_legalKeySizes;
            CngKey cngKey = CngKey.Open(key.Handle, key.IsEphemeral ? CngKeyHandleOpenOptions.EphemeralKey : CngKeyHandleOpenOptions.None);
            Key = cngKey;
        }

        /// <summary>
        ///     Gets the key that will be used by the DSA object for any cryptographic operation that it uses.
        ///     This key object will be disposed if the key is reset, for instance by changing the KeySize
        ///     property, using ImportParamers to create a new key, or by Disposing of the parent DSA object.
        ///     Therefore, you should make sure that the key object is no longer used in these scenarios. This
        ///     object will not be the same object as the CngKey passed to the DSACng constructor if that
        ///     constructor was used, however it will point at the same CNG key.
        /// </summary>
        /// <permission cref="SecurityPermission">
        ///     SecurityPermission/UnmanagedCode is required to read this property.
        /// </permission>
        public CngKey Key
        {
            [SecuritySafeCritical]
            [SecurityPermission(SecurityAction.Assert, UnmanagedCode = true)]
            get
            {
                // If our key size was changed from the key we're using, we need to generate a new key
                if (_key != null && _key.KeySize != KeySize)
                {
                    _key.Dispose();
                    _key = null;
                }

                // If we don't have a key yet, we need to generate a random one now
                if (_key == null)
                {
                    CngKeyCreationParameters creationParameters = new CngKeyCreationParameters()
                    {
                        ExportPolicy = CngExportPolicies.AllowPlaintextExport,
                    };

                    CngProperty keySizeProperty = new CngProperty(NCryptNative.KeyPropertyName.Length, BitConverter.GetBytes(KeySize), CngPropertyOptions.None);
                    creationParameters.Parameters.Add(keySizeProperty);
                    _key = CngKey.Create(s_cngAlgorithmDsa, null, creationParameters);
                }

                return _key;
            }

            private set
            {
                Debug.Assert(value != null, "value != null");
                if (value.AlgorithmGroup != CngAlgorithmGroup.Dsa)
                    throw new ArgumentException(SR.GetString(SR.Cryptography_ArgDSARequiresDSAKey), "value");

                // If we already have a key, clear it out
                if (_key != null)
                {
                    _key.Dispose();
                }

                _key = value;

                // Our LegalKeySizes value stores the values that we encoded as being the correct
                // legal key size limitations for this algorithm, as documented on MSDN.
                //
                // But on a new OS version we might not question if our limit is accurate, or MSDN
                // could have been innacurate to start with.
                //
                // Since the key is already loaded, we know that Windows thought it to be valid;
                // therefore we should set KeySizeValue directly to bypass the LegalKeySizes conformance
                // check.
                //
                // For RSA there are known cases where this change matters. RSACryptoServiceProvider can
                // create a 384-bit RSA key, which we consider too small to be legal. It can also create
                // a 1032-bit RSA key, which we consider illegal because it doesn't match our 64-bit
                // alignment requirement. (In both cases Windows loads it just fine)
                KeySizeValue = value.KeySize;
            }
        }

        /// <summary>
        ///     Helper property to get the NCrypt key handle
        /// </summary>
        private Microsoft.Win32.SafeHandles.SafeNCryptKeyHandle KeyHandle
        {
            [SecuritySafeCritical]
            [SecurityPermission(SecurityAction.Assert, UnmanagedCode = true)]
            get { return Key.Handle; }
        }

        public override KeySizes[] LegalKeySizes
        {
            get
            {
                return base.LegalKeySizes;
            }
        }

        public override string SignatureAlgorithm { get { return "DSA"; } }
        public override string KeyExchangeAlgorithm { get { return null; } }

        [SecuritySafeCritical]
        public override byte[] CreateSignature(byte[] rgbHash)
        {
            if (rgbHash == null)
                throw new ArgumentNullException("rgbHash");

            rgbHash = AdjustHashSizeIfNecessary(rgbHash);
            return NCryptNative.SignHash(KeyHandle, rgbHash, rgbHash.Length * 2);
        }

        [SecuritySafeCritical]
        public override bool VerifySignature(byte[] rgbHash, byte[] rgbSignature)
        {
            if (rgbHash == null)
                throw new ArgumentNullException("rgbHash");

            if (rgbSignature == null)
                throw new ArgumentNullException("rgbSignature");

            rgbHash = AdjustHashSizeIfNecessary(rgbHash);

            return NCryptNative.VerifySignature(KeyHandle, rgbHash, rgbSignature);
        }

        // Need to override since base methods throw a "override me" exception: makes SignData/VerifyData function.
        protected override byte[] HashData(byte[] data, int offset, int count, HashAlgorithmName hashAlgorithm)
        {
            // we're sealed and the base should have checked this already
            Debug.Assert(data != null);
            Debug.Assert(offset >= 0 && offset <= data.Length);
            Debug.Assert(count >= 0 && count <= data.Length);
            Debug.Assert(!string.IsNullOrEmpty(hashAlgorithm.Name));

            using (BCryptHashAlgorithm hasher = new BCryptHashAlgorithm(new CngAlgorithm(hashAlgorithm.Name), BCryptNative.ProviderName.MicrosoftPrimitiveProvider))
            {
                hasher.HashCore(data, offset, count);
                return hasher.HashFinal();
            }
        }

        protected override byte[] HashData(Stream data, HashAlgorithmName hashAlgorithm)
        {
            // We're sealed and the base should have checked these alread.
            Debug.Assert(data != null);
            Debug.Assert(!string.IsNullOrEmpty(hashAlgorithm.Name));

            using (BCryptHashAlgorithm hasher = new BCryptHashAlgorithm(new CngAlgorithm(hashAlgorithm.Name), BCryptNative.ProviderName.MicrosoftPrimitiveProvider))
            {
                hasher.HashStream(data);
                return hasher.HashFinal();
            }
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (_key != null)
                {
                    _key.Dispose();
                    _key = null;
                }
            }
        }

        //
        // This routine is needed because Windows refuses to verify or sign unless the input hash block is the same size as the Q value. However, the NIST
        // spec simply specifies that the hash be truncated to the length of the Q value before signing or verifying. So we will make the adjustment on behalf of Windows.
        // 
        private byte[] AdjustHashSizeIfNecessary(byte[] hash)
        {
            Debug.Assert(hash != null);

            int qLength = ComputeQLength();
            if (qLength > hash.Length)
                throw new PlatformNotSupportedException(SR.Cryptography_DSA_HashTooShort);
            Array.Resize(ref hash, qLength);
            return hash;
        }

        [SecuritySafeCritical]
        private int ComputeQLength()
        {
            CngKey key = Key;
            byte[] blob = key.Export(CngKeyBlobFormat.GenericPublicBlob);

            unsafe
            {
                if (blob.Length < sizeof(BCRYPT_DSA_KEY_BLOB_V2))
                    return Sha1HashOutputSize;

                fixed (byte* pBlobBytes = blob)
                {
                    BCRYPT_DSA_KEY_BLOB_V2* pBlob = (BCRYPT_DSA_KEY_BLOB_V2*)pBlobBytes;
                    if (pBlob->dwMagic != KeyBlobMagicNumber.DsaPublicV2 && pBlob->dwMagic != KeyBlobMagicNumber.DsaPrivateV2)
                    {
                        // This is a V1 BCRYPT_DSA_KEY_BLOB, which hardcodes the Q length to 20 bytes.
                        return Sha1HashOutputSize;
                    }

                    return pBlob->cbGroupSize;
                }
            }
        }

        private CngKey _key;

        private static KeySizes[] s_legalKeySizes = new KeySizes[] { new KeySizes(minSize: 512, maxSize: 3072, skipSize: 64) };
        private static CngAlgorithm s_cngAlgorithmDsa = new CngAlgorithm("DSA");

        // For keysizes up to and including 1024 bits, CNG's blob format is BCRYPT_DSA_KEY_BLOB.
        // For keysizes exceeding 1024 bits, CNG's blob format is BCRYPT_DSA_KEY_BLOB_V2.
        private const int MaxV1KeySize = 1024;

        private const int Sha1HashOutputSize = 20;
        private const int Sha256HashOutputSize = 32;
        private const int Sha512HashOutputSize = 64; 
    }
}
