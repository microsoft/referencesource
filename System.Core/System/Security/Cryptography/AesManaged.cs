// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

using System;
#if !SILVERLIGHT
using System.Diagnostics.Contracts;
#endif // !SILVERLIGHT

namespace System.Security.Cryptography {
    /// <summary>
    ///     Managed implementation of the AES algorithm. AES is esentially Rijndael with a fixed block size
    ///     and iteration count, so we just wrap the RijndaelManaged class and allow only 128 bit blocks to
    ///     be used.
    /// </summary>
    public sealed class AesManaged : Aes {
        private SymmetricAlgorithm m_impl;

        public AesManaged() {
#if !SILVERLIGHT
            Contract.Ensures(m_impl != null);

            if (CryptoConfig.AllowOnlyFipsAlgorithms) {
                if (LocalAppContextSwitches.UseLegacyFipsThrow) {
                    throw new InvalidOperationException(SR.GetString(SR.Cryptography_NonCompliantFIPSAlgorithm));
                }

                m_impl = new AesCryptoServiceProvider();
            } else {
                m_impl = new RijndaelManaged();
            }
#endif // !SILVERLIGHT

            m_impl.BlockSize = BlockSize;
            m_impl.KeySize = KeySize;
        }

#if !SILVERLIGHT
        public override int FeedbackSize {
            get { return m_impl.FeedbackSize; }
            set { m_impl.FeedbackSize = value; }
        }
#endif // !SILVERLIGHT

        public override byte[] IV {
            get { return m_impl.IV; }
            set { m_impl.IV = value; }
        }

        public override byte[] Key {
            get { return m_impl.Key; }
            set { m_impl.Key = value; }
        }

        public override int KeySize {
            get { return m_impl.KeySize; }
            set { m_impl.KeySize = value; }
        }

#if !SILVERLIGHT
        public override CipherMode Mode {
            get { return m_impl.Mode; }

            set {
                Contract.Ensures(m_impl.Mode != CipherMode.CFB && m_impl.Mode != CipherMode.OFB);

                // RijndaelManaged will implicitly change the block size of an algorithm to match the number
                // of feedback bits being used. Since AES requires a block size of 128 bits, we cannot allow
                // the user to use the feedback modes, as this will end up breaking that invarient.
                if (value == CipherMode.CFB || value == CipherMode.OFB) {
                    throw new CryptographicException(SR.GetString(SR.Cryptography_InvalidCipherMode));
                }

                m_impl.Mode = value;
            }
        }

        public override PaddingMode Padding {
            get { return m_impl.Padding; }
            set { m_impl.Padding = value; }
        }
#endif // !SILVERLIGHT

        public override ICryptoTransform CreateDecryptor() {
            return m_impl.CreateDecryptor();
        }

        public override ICryptoTransform CreateDecryptor(byte[] key, byte[] iv) {
            if (key == null) {
                throw new ArgumentNullException("key");
            }
#if !SILVERLIGHT
            if (!ValidKeySize(key.Length * 8)) {
                throw new ArgumentException(SR.GetString(SR.Cryptography_InvalidKeySize), "key");
            }
            if (iv != null && iv.Length * 8 != BlockSizeValue) {
                throw new ArgumentException(SR.GetString(SR.Cryptography_InvalidIVSize), "iv");
            }
#endif

            return m_impl.CreateDecryptor(key, iv);
        }


        public override ICryptoTransform CreateEncryptor() {
            return m_impl.CreateEncryptor();
        }

        public override ICryptoTransform CreateEncryptor(byte[] key, byte[] iv) {
            if (key == null) {
                throw new ArgumentNullException("key");
            }
#if !SILVERLIGHT
            if (!ValidKeySize(key.Length * 8)) {
                throw new ArgumentException(SR.GetString(SR.Cryptography_InvalidKeySize), "key");
            }
            if (iv != null && iv.Length * 8 != BlockSizeValue) {
                throw new ArgumentException(SR.GetString(SR.Cryptography_InvalidIVSize), "iv");
            }
#endif // SILVERLIGHT

            return m_impl.CreateEncryptor(key, iv);
        }

        protected override void Dispose(bool disposing) {
            try {
                if (disposing) {
                    (m_impl as IDisposable).Dispose();
                }
            }
            finally {
                base.Dispose(disposing);
            }
        }

        public override void GenerateIV() {
            m_impl.GenerateIV();
        }

        public override void GenerateKey() {
            m_impl.GenerateKey();
        }
    }
}
