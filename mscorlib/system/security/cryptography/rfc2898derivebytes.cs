// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// Rfc2898DeriveBytes.cs
//

// This implementation follows RFC 2898 recommendations. See http://www.ietf.org/rfc/Rfc2898.txt
// It uses HMACSHA1 as the underlying pseudorandom function.

namespace System.Security.Cryptography {
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Diagnostics.Contracts;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using System.Runtime.Versioning;
    using System.Security.Cryptography.X509Certificates;

    [System.Runtime.InteropServices.ComVisible(true)]
    public class Rfc2898DeriveBytes : DeriveBytes
    {
        private byte[] m_buffer;
        private byte[] m_salt;
        private HMAC m_hmac;  // The pseudo-random generator function used in PBKDF2
        private byte[] m_password;
        private CspParameters m_cspParams = new CspParameters();

        private uint m_iterations;
        private uint m_block;
        private int m_startIndex;
        private int m_endIndex;

        private int m_blockSize;

        //
        // public constructors
        //

        public Rfc2898DeriveBytes(string password, int saltSize) : this(password, saltSize, 1000) {}

        public Rfc2898DeriveBytes(string password, int saltSize, int iterations)
            : this(password, saltSize, iterations, HashAlgorithmName.SHA1)
        {
        }

        // This method needs to be safe critical, because in debug builds the C# compiler will include null
        // initialization of the _safeProvHandle field in the method.  Since SafeProvHandle is critical, a
        // transparent reference triggers an error using PasswordDeriveBytes.
        [SecuritySafeCritical]
        public Rfc2898DeriveBytes(string password, int saltSize, int iterations, HashAlgorithmName hashAlgorithm) {
            if (saltSize < 0)
                throw new ArgumentOutOfRangeException("saltSize", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (string.IsNullOrEmpty(hashAlgorithm.Name))
                throw new ArgumentException(Environment.GetResourceString("Cryptography_HashAlgorithmNameNullOrEmpty"), nameof(hashAlgorithm));

            HMAC hmac = HMAC.Create("HMAC" + hashAlgorithm.Name);

            if (hmac == null)
                throw new CryptographicException(Environment.GetResourceString("Cryptography_UnknownHashAlgorithm", hashAlgorithm.Name));

            Contract.EndContractBlock();

            byte[] salt = new byte[saltSize];
            Utils.StaticRandomNumberGenerator.GetBytes(salt);

            Salt = salt;
            IterationCount = iterations;
            m_password = new UTF8Encoding(false).GetBytes(password);
            hmac.Key = m_password;
            m_hmac = hmac;
            // m_blockSize is in bytes, HashSize is in bits. 
            m_blockSize = hmac.HashSize >> 3;

            Initialize();
        }

        public Rfc2898DeriveBytes(string password, byte[] salt) : this(password, salt, 1000) {}

        public Rfc2898DeriveBytes(string password, byte[] salt, int iterations) : this (password, salt, iterations, HashAlgorithmName.SHA1) {}

        public Rfc2898DeriveBytes(string password, byte[] salt, int iterations, HashAlgorithmName hashAlgorithm)
            : this(new UTF8Encoding(false).GetBytes(password), salt, iterations, hashAlgorithm) {
        }

        public Rfc2898DeriveBytes(byte[] password, byte[] salt, int iterations) : this(password, salt, iterations, HashAlgorithmName.SHA1) { }

        // This method needs to be safe critical, because in debug builds the C# compiler will include null
        // initialization of the _safeProvHandle field in the method.  Since SafeProvHandle is critical, a
        // transparent reference triggers an error using PasswordDeriveBytes.
        [SecuritySafeCritical]
        public Rfc2898DeriveBytes(byte[] password, byte[] salt, int iterations, HashAlgorithmName hashAlgorithm) {
            if (string.IsNullOrEmpty(hashAlgorithm.Name))
                throw new ArgumentException(Environment.GetResourceString("Cryptography_HashAlgorithmNameNullOrEmpty"), nameof(hashAlgorithm));

            HMAC hmac = HMAC.Create("HMAC" + hashAlgorithm.Name);

            if (hmac == null)
                throw new CryptographicException(Environment.GetResourceString("Cryptography_UnknownHashAlgorithm", hashAlgorithm.Name));

            Contract.EndContractBlock();

            Salt = salt;
            IterationCount = iterations;
            m_password = password;
            hmac.Key = password;
            m_hmac = hmac;
            // m_blockSize is in bytes, HashSize is in bits. 
            m_blockSize = hmac.HashSize >> 3;

            Initialize();
        }

        //
        // public properties
        //

        public int IterationCount {
            get { return (int) m_iterations; }
            set {
                if (value <= 0)
                    throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("ArgumentOutOfRange_NeedPosNum"));
                Contract.EndContractBlock();
                m_iterations = (uint) value;
                Initialize();
            }
        }

        public byte[] Salt {
            get { return (byte[]) m_salt.Clone(); }
            set { 
                if (value == null)
                    throw new ArgumentNullException("value");
                if (value.Length < 8) 
                    throw new ArgumentException(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_FewBytesSalt"));
                Contract.EndContractBlock();
                m_salt = (byte[]) value.Clone(); 
                Initialize();
            }
        }

        //
        // public methods
        //

        public override byte[] GetBytes(int cb) {
            if (cb <= 0)
                throw new ArgumentOutOfRangeException("cb", Environment.GetResourceString("ArgumentOutOfRange_NeedPosNum"));
            Contract.EndContractBlock();

            Contract.Assert(m_blockSize > 0);

            byte[] password = new byte[cb];

            int offset = 0;
            int size = m_endIndex - m_startIndex;
            if (size > 0) {
                if (cb >= size) {
                    Buffer.InternalBlockCopy(m_buffer, m_startIndex, password, 0, size);
                    m_startIndex = m_endIndex = 0;
                    offset += size;
                } else {
                    Buffer.InternalBlockCopy(m_buffer, m_startIndex, password, 0, cb);
                    m_startIndex += cb;
                    return password;
                }
            }

            Contract.Assert(m_startIndex == 0 && m_endIndex == 0, "Invalid start or end index in the internal buffer." );

            while(offset < cb) {
                byte[] T_block = Func();
                int remainder = cb - offset;
                if(remainder > m_blockSize) {
                    Buffer.InternalBlockCopy(T_block, 0, password, offset, m_blockSize);
                    offset += m_blockSize;
                } else {
                    Buffer.InternalBlockCopy(T_block, 0, password, offset, remainder);
                    offset += remainder;
                    Buffer.InternalBlockCopy(T_block, remainder, m_buffer, m_startIndex, m_blockSize - remainder);
                    m_endIndex += (m_blockSize - remainder);
                    return password;
                }
            }
            return password;
        }

        public override void Reset() {
            Initialize();
        }

        protected override void Dispose(bool disposing) {
            base.Dispose(disposing);

            if (disposing) {
                if (m_hmac != null) {
                    ((IDisposable)m_hmac).Dispose();
                }

                if (m_buffer != null) {
                    Array.Clear(m_buffer, 0, m_buffer.Length);
                }
                if (m_salt != null) {
                    Array.Clear(m_salt, 0, m_salt.Length);
                }
            }
        }

        private void Initialize() {
            if (m_buffer != null)
                Array.Clear(m_buffer, 0, m_buffer.Length);
            m_buffer = new byte[m_blockSize];
            m_block = 1;
            m_startIndex = m_endIndex = 0;
        }

        // This function is defined as follow :
        // Func (S, i) = HMAC(S || i) | HMAC2(S || i) | ... | HMAC(iterations) (S || i) 
        // where i is the block number.
        private byte[] Func () {
            byte[] INT_block = Utils.Int(m_block);

            m_hmac.TransformBlock(m_salt, 0, m_salt.Length, null, 0);
            m_hmac.TransformBlock(INT_block, 0, INT_block.Length, null, 0);
            m_hmac.TransformFinalBlock(EmptyArray<Byte>.Value, 0, 0);
            byte[] temp = m_hmac.HashValue;
            m_hmac.Initialize();

            byte[] ret = temp;
            for (int i = 2; i <= m_iterations; i++) {
                m_hmac.TransformBlock(temp, 0, temp.Length, null, 0);
                m_hmac.TransformFinalBlock(EmptyArray<Byte>.Value, 0, 0);
                temp = m_hmac.HashValue;
                for (int j = 0; j < m_blockSize; j++) {
                    ret[j] ^= temp[j];
                }
                m_hmac.Initialize();
            }

            // increment the block count.
            m_block++;
            return ret;
        }

        [System.Security.SecuritySafeCritical]  // auto-generated
        public byte[] CryptDeriveKey(string algname, string alghashname, int keySize, byte[] rgbIV)
        {
            if (keySize < 0)
                throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeySize"));

            int algidhash = X509Utils.NameOrOidToAlgId(alghashname, OidGroup.HashAlgorithm);
            if (algidhash == 0)
                throw new CryptographicException(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_InvalidAlgorithm"));

            int algid = X509Utils.NameOrOidToAlgId(algname, OidGroup.AllGroups);
            if (algid == 0)
                throw new CryptographicException(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_InvalidAlgorithm"));

            // Validate the rgbIV array
            if (rgbIV == null)
                throw new CryptographicException(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_InvalidIV"));

            byte[] key = null;
            DeriveKey(ProvHandle, algid, algidhash,
                      m_password, m_password.Length, keySize << 16, rgbIV, rgbIV.Length,
                      JitHelpers.GetObjectHandleOnStack(ref key));
            return key;
        }

        [System.Security.SecurityCritical] // auto-generated
        private SafeProvHandle _safeProvHandle = null;
        private SafeProvHandle ProvHandle
        {
            [System.Security.SecurityCritical]  // auto-generated
            get
            {
                if (_safeProvHandle == null)
                {
                    lock (this)
                    {
                        if (_safeProvHandle == null)
                        {
                            SafeProvHandle safeProvHandle = Utils.AcquireProvHandle(m_cspParams);
                            System.Threading.Thread.MemoryBarrier();
                            _safeProvHandle = safeProvHandle;
                        }
                    }
                }
                return _safeProvHandle;
            }
        }

        [System.Security.SecurityCritical]  // auto-generated
        [ResourceExposure(ResourceScope.None)]
        [DllImport(JitHelpers.QCall, CharSet = CharSet.Unicode), SuppressUnmanagedCodeSecurity]
        private static extern void DeriveKey(SafeProvHandle hProv, int algid, int algidHash,
                                      byte[] password, int cbPassword, int dwFlags, byte[] IV, int cbIV,
                                      ObjectHandleOnStack retKey);

    }
}
