//------------------------------------------------------------------------------
// <copyright file="SqlException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <owner current="true" primary="true">balnee</owner>
// <owner current="true" primary="false">krishnib</owner>
//------------------------------------------------------------------------------
namespace System.Data.SqlClient
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Reflection;
    using System.Security;
    using System.Security.Cryptography;
    using System.Text;

    internal static class SqlSecurityUtility {

        static readonly ColumnMasterKeyMetadataSignatureVerificationCache ColumnMasterKeyMetadataSignatureVerificationCache = ColumnMasterKeyMetadataSignatureVerificationCache.Instance;

        /// <summary>
        /// Computes a keyed hash of a given text and returns. It fills the buffer "hash" with computed hash value.
        /// </summary>
        /// <param name="plainText">Plain text bytes whose hash has to be computed.</param>
        /// <param name="key">key used for the HMAC</param>
        /// <param name="hash">Output buffer where the computed hash value is stored. If its less that 64 bytes, the hash is truncated</param>
        /// <returns>HMAC value</returns>
        internal static void GetHMACWithSHA256(byte[] plainText, byte[] key, byte[] hash) {
            const int MaxSHA256HashBytes = 32;

            Debug.Assert(key != null && plainText != null);
            Debug.Assert(hash.Length != 0 && hash.Length <= MaxSHA256HashBytes);

            using (HMACSHA256 hmac = new HMACSHA256(key)) {
                byte[] computedHash = hmac.ComputeHash(plainText);

                // Truncate the hash if needed
                Buffer.BlockCopy (computedHash, 0, hash, 0, hash.Length);
            }
        }

        /// <summary>
        /// Computes SHA256 hash of a given input
        /// </summary>
        /// <param name="input">input byte array which needs to be hashed</param>
        /// <returns>Returns SHA256 hash in a string form</returns>
        internal static string GetSHA256Hash(byte[] input) {
            Debug.Assert(input != null);

            using (SHA256 sha256 = SHA256Cng.Create()) {
                byte[] hashValue = sha256.ComputeHash(input);
                return GetHexString(hashValue);
            }
        }

        /// <summary>
        /// Generates cryptographicall random bytes
        /// </summary>
        /// <param name="length">No of cryptographically random bytes to be generated</param>
        /// <returns>A byte array containing cryptographically generated random bytes</returns>
        internal static void GenerateRandomBytes(byte[] randomBytes) {
            // Generate random bytes cryptographically.
            RNGCryptoServiceProvider rngCsp = new RNGCryptoServiceProvider();
            rngCsp.GetBytes(randomBytes);
        }

        /// <summary>
        /// Compares two byte arrays and returns true if all bytes are equal
        /// </summary>
        /// <param name="buffer1">input buffer</param>
        /// <param name="buffer2">another buffer to be compared against</param>
        /// <returns>returns true if both the arrays have the same byte values else returns false</returns>
        internal static bool CompareBytes(byte[] buffer1, byte[] buffer2, int buffer2Index, int lengthToCompare) {
            if (null == buffer1 || null == buffer2) {
                return false;
            }
            
            Debug.Assert (buffer2Index > -1 && buffer2Index < buffer2.Length, "invalid index");// bounds on buffer2Index
            if ((buffer2.Length -buffer2Index) < lengthToCompare) {
                return false;
            }

            for (int index = 0; index < buffer1.Length && index < lengthToCompare; ++index) {
                if (buffer1[index] != buffer2[buffer2Index + index]) {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// Gets hex representation of byte array.
        /// <param name="input">input byte array</param>
        /// </summary>
        internal static string GetHexString(byte[] input) {
            Debug.Assert(input != null);

            StringBuilder str = new StringBuilder();
            foreach (byte b in input) {
                str.AppendFormat(b.ToString(@"X2"));
            }

            return str.ToString();
        }

        /// <summary>
        /// Returns the caller's function name in the format of [ClassName].[FunctionName]
        /// </summary>
        internal static string GetCurrentFunctionName() {
            StackTrace stackTrace = new StackTrace();
            StackFrame stackFrame = stackTrace.GetFrame(1);
            MethodBase methodBase = stackFrame.GetMethod();
            return string.Format(@"{0}.{1}", methodBase.DeclaringType.Name, methodBase.Name);
        }

        /// <summary>
        /// Return the algorithm name mapped to an Id.
        /// </summary>
        /// <param name="cipherAlgorithmId"></param>
        /// <returns></returns>
        private static string ValidateAndGetEncryptionAlgorithmName (byte cipherAlgorithmId, string cipherAlgorithmName) {
            if (TdsEnums.CustomCipherAlgorithmId == cipherAlgorithmId) {
                if (null == cipherAlgorithmName) {
                    throw SQL.NullColumnEncryptionAlgorithm(SqlClientEncryptionAlgorithmFactoryList.GetInstance().GetRegisteredCipherAlgorithmNames());
                }

                return cipherAlgorithmName;
            }
            else if (TdsEnums.AEAD_AES_256_CBC_HMAC_SHA256 == cipherAlgorithmId) {
                return SqlAeadAes256CbcHmac256Algorithm.AlgorithmName;
            }
            else if (TdsEnums.AES_256_CBC == cipherAlgorithmId) {
                return SqlAes256CbcAlgorithm.AlgorithmName;
            }
            else {
                throw SQL.UnknownColumnEncryptionAlgorithmId(cipherAlgorithmId, GetRegisteredCipherAlgorithmIds());
            }
        }

        /// <summary>
        /// Retrieves a string with comma separated list of registered algorithm Ids (enclosed in quotes).
        /// </summary>
        private static string GetRegisteredCipherAlgorithmIds () {
            return @"'1', '2'";
        }

        /// <summary>
        /// Encrypts the plaintext.
        /// </summary>
        internal static byte[] EncryptWithKey (byte[] plainText, SqlCipherMetadata md, string serverName) {
            Debug.Assert(serverName != null, @"serverName should not be null in EncryptWithKey.");

            // Initialize cipherAlgo if not already done.
            if (!md.IsAlgorithmInitialized()) {
                SqlSecurityUtility.DecryptSymmetricKey(md, serverName);
            }

            Debug.Assert(md.IsAlgorithmInitialized(), "Encryption Algorithm is not initialized");
            byte[] cipherText = md.CipherAlgorithm.EncryptData(plainText); // this call succeeds or throws.
            if (null == cipherText || 0 == cipherText.Length) {
                SQL.NullCipherText();
            }

            return cipherText;
        }

        /// <summary>
        /// Gets a string with first/last 10 bytes in the buff (useful for exception handling).
        /// </summary>
        internal static string GetBytesAsString(byte[] buff, bool fLast, int countOfBytes) {
            int count = (buff.Length > countOfBytes) ? countOfBytes : buff.Length;
            int startIndex = 0;
            if (fLast) {
                startIndex = buff.Length - count;
                Debug.Assert(startIndex >= 0);
            }

            return BitConverter.ToString(buff, startIndex, count);
        }

        /// <summary>
        /// Decrypts the ciphertext.
        /// </summary>
        internal static byte[] DecryptWithKey(byte[] cipherText, SqlCipherMetadata md, string serverName) {
            Debug.Assert(serverName != null, @"serverName should not be null in DecryptWithKey.");

            // Initialize cipherAlgo if not already done.
            if (!md.IsAlgorithmInitialized()) { 
                SqlSecurityUtility.DecryptSymmetricKey(md, serverName);
            }

            Debug.Assert(md.IsAlgorithmInitialized(), "Decryption Algorithm is not initialized");
            try {
                byte[] plainText = md.CipherAlgorithm.DecryptData(cipherText); // this call succeeds or throws.
                if (null == plainText) {
                    throw SQL.NullPlainText ();
                }

                return plainText;
            }
            catch (Exception e) {
                // compute the strings to pass
                string keyStr = GetBytesAsString(md.EncryptionKeyInfo.Value.encryptedKey, fLast:true, countOfBytes:10);
                string valStr = GetBytesAsString(cipherText, fLast:false, countOfBytes:10);
                throw SQL.ThrowDecryptionFailed(keyStr, valStr, e);
            }
        }

        /// <summary>
        /// <para> Decrypts the symmetric key and saves it in metadata. In addition, intializes 
        /// the SqlClientEncryptionAlgorithm for rapid decryption.</para>
        /// </summary>
        internal static void DecryptSymmetricKey(SqlCipherMetadata md, string serverName) {
            Debug.Assert(md != null, "md should not be null in DecryptSymmetricKey.");

            SqlClientSymmetricKey symKey = null;
            SqlEncryptionKeyInfo? encryptionkeyInfoChosen = null;

            DecryptSymmetricKey(md.EncryptionInfo, serverName, out symKey, out encryptionkeyInfoChosen);

            // Given the symmetric key instantiate a SqlClientEncryptionAlgorithm object and cache it in metadata 
            md.CipherAlgorithm = null;
            SqlClientEncryptionAlgorithm cipherAlgorithm = null;
            string algorithmName = ValidateAndGetEncryptionAlgorithmName(md.CipherAlgorithmId, md.CipherAlgorithmName); // may throw
            SqlClientEncryptionAlgorithmFactoryList.GetInstance().GetAlgorithm(symKey, md.EncryptionType, algorithmName, out cipherAlgorithm); // will validate algorithm name and type
            Debug.Assert(cipherAlgorithm != null);
            md.CipherAlgorithm = cipherAlgorithm;
            md.EncryptionKeyInfo = encryptionkeyInfoChosen;
            return;
        }

        /// <summary>
        /// <para> Decrypts the symmetric key and saves it in metadata.
        /// </summary>
        internal static void DecryptSymmetricKey(SqlTceCipherInfoEntry? sqlTceCipherInfoEntry, string serverName, out SqlClientSymmetricKey sqlClientSymmetricKey, out SqlEncryptionKeyInfo? encryptionkeyInfoChosen) {
            Debug.Assert(serverName != null, @"serverName should not be null in DecryptSymmetricKey.");
            Debug.Assert(sqlTceCipherInfoEntry.HasValue, "sqlTceCipherInfoEntry should not be null in DecryptSymmetricKey.");
            Debug.Assert(sqlTceCipherInfoEntry.Value.ColumnEncryptionKeyValues != null,
                "sqlTceCipherInfoEntry.ColumnEncryptionKeyValues should not be null in DecryptSymmetricKey.");

            sqlClientSymmetricKey = null;
            encryptionkeyInfoChosen = null;
            Exception lastException = null;
            SqlSymmetricKeyCache cache = SqlSymmetricKeyCache.GetInstance();

            foreach (SqlEncryptionKeyInfo keyInfo in sqlTceCipherInfoEntry.Value.ColumnEncryptionKeyValues) {
                try {
                    if (cache.GetKey(keyInfo, serverName, out sqlClientSymmetricKey)) {
                        encryptionkeyInfoChosen = keyInfo;
                        break;
                    }
                } catch (Exception e) {
                    lastException = e;
                }
            }

            if (null == sqlClientSymmetricKey) {
                Debug.Assert(null != lastException, "CEK decryption failed without raising exceptions");
                throw lastException;
            }

            Debug.Assert(encryptionkeyInfoChosen.HasValue, "encryptionkeyInfoChosen must have a value.");
        }

        /// <summary>
        /// Calculates the length of the Base64 string used to represent a byte[] with the specified length.
        /// </summary>
        /// <param name="byteLength"></param>
        /// <returns></returns>
        internal static int GetBase64LengthFromByteLength(int byteLength) {
            Debug.Assert(byteLength <= UInt16.MaxValue, @"Encrypted column encryption key cannot be larger than 65536 bytes");

            // Base64 encoding uses 1 character to encode 6 bits which means 4 characters for 3 bytes and pads to 4 byte multiples.
            return (int)((double)byteLength * 4 / 3) + 4;
        }

        /// <summary>
        /// <para> Verifies Column Master Key Signature.
        /// </summary>
        internal static void VerifyColumnMasterKeySignature(string keyStoreName, string keyPath, string serverName, bool isEnclaveEnabled, byte[] CMKSignature) {
            bool isValidSignature = false;
            
            try {
                Debug.Assert(SqlConnection.ColumnEncryptionTrustedMasterKeyPaths != null,
                    @"SqlConnection.ColumnEncryptionTrustedMasterKeyPaths should not be null");

                if (CMKSignature == null || CMKSignature.Length == 0) {
                    throw SQL.ColumnMasterKeySignatureNotFound(keyPath);
                }

                // Check against the trusted key paths
                //
                // Get the List corresponding to the connected server
                IList<string> trustedKeyPaths;
                if (SqlConnection.ColumnEncryptionTrustedMasterKeyPaths.TryGetValue(serverName, out trustedKeyPaths)) {
                    // If the list is null or is empty or if the keyPath doesn't exist in the trusted key paths, then throw an exception.
                    if ((trustedKeyPaths == null) || (trustedKeyPaths.Count() == 0) ||
                        // (trustedKeyPaths.Where(s => s.Equals(keyInfo.keyPath, StringComparison.InvariantCultureIgnoreCase)).Count() == 0)) {
                        (trustedKeyPaths.Any(
                            s => s.Equals(keyPath, StringComparison.InvariantCultureIgnoreCase)) == false)) {
                        // throw an exception since the key path is not in the trusted key paths list for this server
                        throw SQL.UntrustedKeyPath(keyPath, serverName);
                    }
                }

                // Key Not found, attempt to look up the provider and verify CMK Signature
                SqlColumnEncryptionKeyStoreProvider provider;
                if (!SqlConnection.TryGetColumnEncryptionKeyStoreProvider(keyStoreName, out provider)) {
                    throw SQL.InvalidKeyStoreProviderName(keyStoreName,
                        SqlConnection.GetColumnEncryptionSystemKeyStoreProviders(),
                        SqlConnection.GetColumnEncryptionCustomKeyStoreProviders());
                }

                bool? signatureVerificationResult = ColumnMasterKeyMetadataSignatureVerificationCache.GetSignatureVerificationResult(keyStoreName, keyPath, isEnclaveEnabled, CMKSignature);

                if (signatureVerificationResult == null) {
                // We will simply bubble up the exception from VerifyColumnMasterKeyMetadata function.
                    isValidSignature = provider.VerifyColumnMasterKeyMetadata(keyPath, isEnclaveEnabled,
                        CMKSignature);

                    ColumnMasterKeyMetadataSignatureVerificationCache.AddSignatureVerificationResult(keyStoreName, keyPath, isEnclaveEnabled, CMKSignature, isValidSignature);
                }
                else {
                    isValidSignature = signatureVerificationResult.Value;
                }

            } catch (Exception e) {
                throw SQL.UnableToVerifyColumnMasterKeySignature(e);
            }

            if (!isValidSignature) {
                throw SQL.ColumnMasterKeySignatureVerificationFailed(keyPath);
            }
        }
    }
}
