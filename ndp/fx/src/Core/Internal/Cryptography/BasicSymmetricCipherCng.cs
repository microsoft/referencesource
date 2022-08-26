// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Security;
using System.Security.Cryptography;
using System.Text;
using Microsoft.Win32.SafeHandles;

using ErrorCode = Interop.NCrypt.ErrorCode;
using AsymmetricPaddingMode = Interop.NCrypt.AsymmetricPaddingMode;

namespace Internal.Cryptography
{
    internal sealed class BasicSymmetricCipherNCrypt : BasicSymmetricCipher
    {
        //
        // The first parameter is a delegate that instantiates a CngKey rather than a CngKey itself. That's because CngKeys are stateful objects
        // and concurrent encryptions on the same CngKey will corrupt each other.
        //
        // The delegate must instantiate a new CngKey, based on a new underlying NCryptKeyHandle, each time is called.
        //
        public BasicSymmetricCipherNCrypt(Func<CngKey> cngKeyFactory, CipherMode cipherMode, int blockSizeInBytes, byte[] iv, bool encrypting)
            : base(iv, blockSizeInBytes)
        {
            _encrypting = encrypting;
            _cngKey = cngKeyFactory();

            CngProperty chainingModeProperty;
            switch (cipherMode)
            {
                case CipherMode.ECB:
                    chainingModeProperty = s_ECBMode;
                    break;
                case CipherMode.CBC:
                    chainingModeProperty = s_CBCMode;
                    break;
                default:
                    throw new CryptographicException(SR.GetString(SR.Cryptography_InvalidCipherMode));
            }
            _cngKey.SetProperty(chainingModeProperty);

            Reset();
        }

        [SecuritySafeCritical]
        public sealed override int Transform(byte[] input, int inputOffset, int count, byte[] output, int outputOffset)
        {
            Debug.Assert(input != null);
            Debug.Assert(inputOffset >= 0);
            Debug.Assert(count > 0);
            Debug.Assert((count % BlockSizeInBytes) == 0);
            Debug.Assert(input.Length - inputOffset >= count);
            Debug.Assert(output != null);
            Debug.Assert(outputOffset >= 0);
            Debug.Assert(output.Length - outputOffset >= count);

            unsafe
            {
                fixed (byte *pInput = input, pOutput = output)
                {
                    int numBytesWritten;
                    ErrorCode errorCode;
                    if (_encrypting)
                    {
                        errorCode = Interop.NCrypt.NCryptEncrypt(_cngKey.Handle, pInput + inputOffset, count, null, pOutput + outputOffset, count, out numBytesWritten, AsymmetricPaddingMode.None);
                    }
                    else
                    {
                        errorCode = Interop.NCrypt.NCryptDecrypt(_cngKey.Handle, pInput + inputOffset, count, null, pOutput + outputOffset, count, out numBytesWritten, AsymmetricPaddingMode.None);
                    }
                    if (errorCode != ErrorCode.ERROR_SUCCESS)
                        throw errorCode.ToCryptographicException();

                    if (numBytesWritten != count)
                    {
                        // CNG gives us no way to tell NCryptDecrypt() that we're decrypting the final block, nor is it performing any padding/depadding for us.
                        // So there's no excuse for a provider to hold back output for "future calls." Though this isn't technically our problem to detect, we might as well
                        // detect it now for easier diagnosis.
                        throw new CryptographicException(SR.GetString(SR.Cryptography_UnexpectedTransformTruncation));
                    }

                    return numBytesWritten;
                }
            }
        }

        public sealed override byte[] TransformFinal(byte[] input, int inputOffset, int count)
        {
            Debug.Assert(input != null);
            Debug.Assert(inputOffset >= 0);
            Debug.Assert(count >= 0);
            Debug.Assert((count % BlockSizeInBytes) == 0);
            Debug.Assert(input.Length - inputOffset >= count);

            byte[] output = new byte[count];
            if (count != 0)
            {
                int numBytesWritten = Transform(input, inputOffset, count, output, 0);
                Debug.Assert(numBytesWritten == count);  // Our implementation of Transform() guarantees this. See comment above.
            }

            Reset();
            return output;
        }

        protected sealed override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (_cngKey != null)
                {
                    _cngKey.Dispose();
                    _cngKey = null;
                }
            }

            base.Dispose(disposing);
        }

        private void Reset()
        {
            if (IV != null)
            {
                CngProperty prop = new CngProperty(Interop.NCrypt.NCRYPT_INITIALIZATION_VECTOR, IV, CngPropertyOptions.None);
                _cngKey.SetProperty(prop);
            }
        }

        private static CngProperty CreateCngPropertyForCipherMode(string cipherMode)
        {
            byte[] cipherModeBytes = Encoding.Unicode.GetBytes((cipherMode + "\0").ToCharArray());
            return new CngProperty(Interop.NCrypt.NCRYPT_CHAINING_MODE_PROPERTY, cipherModeBytes, CngPropertyOptions.None);
        }

        private CngKey _cngKey;
        private readonly bool _encrypting;

        private readonly static CngProperty s_ECBMode = CreateCngPropertyForCipherMode(Interop.BCrypt.BCRYPT_CHAIN_MODE_ECB);
        private readonly static CngProperty s_CBCMode = CreateCngPropertyForCipherMode(Interop.BCrypt.BCRYPT_CHAIN_MODE_CBC);
    }

    internal sealed class BasicSymmetricCipherBCrypt : BasicSymmetricCipher
    {
        private readonly bool _encrypting;
        private SafeBCryptKeyHandle _hKey;
        private byte[] _currentIv;  // CNG mutates this with the updated IV for the next stage on each Encrypt/Decrypt call.
                                    // The base IV holds a copy of the original IV for Reset(), until it is cleared by Dispose().

        [SecuritySafeCritical]
        public BasicSymmetricCipherBCrypt(SafeBCryptAlgorithmHandle algorithm, CipherMode cipherMode, int blockSizeInBytes, byte[] key, byte[] iv, bool encrypting)
            : base(cipherMode.GetCipherIv(iv), blockSizeInBytes)
        {
            Debug.Assert(algorithm != null);

            _encrypting = encrypting;

            if (IV != null)
            {
                _currentIv = new byte[IV.Length];
            }

            _hKey = BCryptNative.BCryptImportKey(algorithm, key);

            Reset();
        }

        [SecuritySafeCritical]
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                SafeBCryptKeyHandle hKey = _hKey;
                _hKey = null;
                if (hKey != null)
                {
                    hKey.Dispose();
                }

                byte[] currentIv = _currentIv;
                _currentIv = null;
                if (currentIv != null)
                {
                    Array.Clear(currentIv, 0, currentIv.Length);
                }
            }

            base.Dispose(disposing);
        }

        [SecuritySafeCritical]
        public override int Transform(byte[] input, int inputOffset, int count, byte[] output, int outputOffset)
        {
            Debug.Assert(input != null);
            Debug.Assert(inputOffset >= 0);
            Debug.Assert(count > 0);
            Debug.Assert((count % BlockSizeInBytes) == 0);
            Debug.Assert(input.Length - inputOffset >= count);
            Debug.Assert(output != null);
            Debug.Assert(outputOffset >= 0);
            Debug.Assert(output.Length - outputOffset >= count);

            int numBytesWritten;
            if (_encrypting)
            {
                numBytesWritten = BCryptNative.BCryptEncrypt(_hKey, input, inputOffset, count, _currentIv, output, outputOffset, output.Length - outputOffset);
            }
            else
            {
                numBytesWritten = BCryptNative.BCryptDecrypt(_hKey, input, inputOffset, count, _currentIv, output, outputOffset, output.Length - outputOffset);
            }

            if (numBytesWritten != count)
            {
                // CNG gives us no way to tell BCryptDecrypt() that we're decrypting the final block, nor is it performing any
                // padding /depadding for us. So there's no excuse for a provider to hold back output for "future calls." Though
                // this isn't technically our problem to detect, we might as well detect it now for easier diagnosis.
                throw new CryptographicException(SR.Cryptography_UnexpectedTransformTruncation);
            }

            return numBytesWritten;
        }

        public override byte[] TransformFinal(byte[] input, int inputOffset, int count)
        {
            Debug.Assert(input != null);
            Debug.Assert(inputOffset >= 0);
            Debug.Assert(count >= 0);
            Debug.Assert((count % BlockSizeInBytes) == 0);
            Debug.Assert(input.Length - inputOffset >= count);

            byte[] output = new byte[count];
            if (count != 0)
            {
                int numBytesWritten = Transform(input, inputOffset, count, output, 0);
                Debug.Assert(numBytesWritten == count);  // Our implementation of Transform() guarantees this. See comment above.
            }

            Reset();
            return output;
        }

        private void Reset()
        {
            if (IV != null)
            {
                Buffer.BlockCopy(IV, 0, _currentIv, 0, IV.Length);
            }
        }
    }
}
