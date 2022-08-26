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

    /// <summary>
    /// Abstract base class for all column encryption Key Store providers. It exposes two functions
    ///		1. DecryptColumnEncryptionKey - This is the function used by SqlClient under the covers to decrypt encrypted column encryption key blob.
    ///		2. EncryptColumnEncryptionKey - This will be used by client tools that generate DDL for customers
    ///     3. SignColumnMasterKeyMetadata - This will be used by client tools that generate Column Master Keys (CMK) for customers
    ///     4. VerifyColumnMasterKeyMetadata - This will be used by SqlClient under the covers to verify the CMKs received from SQL Server
    /// </summary>
    public abstract class SqlColumnEncryptionKeyStoreProvider
    {
        /// <summary>
        /// This function must be implemented by the corresponding Key Store providers. This function should use an asymmetric key identified by the key path
        /// and decrypt an encrypted column encryption key with a given encryption algorithm.
        /// </summary>
        /// <param name="masterKeyPath">Complete path of an asymmetric key. Path format is specific to a key store provider.</param>
        /// <param name="encryptionAlgorithm">Asymmetric Key Encryption Algorithm</param>
        /// <param name="encryptedColumnEncryptionKey">Encrypted Column Encryption Key</param>
        /// <returns>Plain text column encryption key</returns>
        public abstract byte[] DecryptColumnEncryptionKey(string masterKeyPath, string encryptionAlgorithm, byte[] encryptedColumnEncryptionKey);

        /// <summary>
        /// This function must be implemented by the corresponding Key Store providers. This function should use an asymmetric key identified by a key path
        /// and encrypt a plain text column encryption key with a given asymmetric key encryption algorithm.
        /// </summary>
        /// <param name="keyPath">Complete path of an asymmetric key. Path format is specific to a key store provider.</param>
        /// <param name="encryptionAlgorithm">Asymmetric Key Encryption Algorithm</param>
        /// <param name="columnEncryptionKey">Plain text column encryption key to be encrypted</param>
        /// <returns>Encrypted column encryption key</returns>
        public abstract byte[] EncryptColumnEncryptionKey(string masterKeyPath, string encryptionAlgorithm, byte[] columnEncryptionKey);

        /// <summary>
        /// This function must be implemented by the corresponding Key Store providers that wish to use enclaves with Always Encrypted.
        /// This function has a default implementation in the base class that throws NotImplementedException to ensure that 
        /// it does not break applications that rely on an old API
        /// Digitally sign the specified column master key metadata: the key path and the property indicating whether column master key supports enclave computations.
        /// </summary>
        /// <param name="masterKeyPath">Complete path of an asymmetric key. Path format is specific to a key store provider.</param>
        /// <param name="allowEnclaveComputations">Boolean indicating whether this key can be sent to trusted enclave</param>
        /// <returns>Encrypted column encryption key</returns>
        public virtual byte[] SignColumnMasterKeyMetadata(string masterKeyPath, bool allowEnclaveComputations)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// This function must be implemented by the corresponding Key Store providers that wish to use enclaves with Always Encrypted.
        /// This function has a default implementation in the base class that throws NotImplementedException to ensure that 
        /// it does not break applications that rely on an old API
        /// Verifies whether the specified signature is valid for the column master key with the specified metadata properties: the key path and the property indicating whether column master key supports enclave computations.
        /// </summary>
        /// <param name="masterKeyPath">Complete path of an asymmetric key. Path format is specific to a key store provider.</param>
        /// <param name="allowEnclaveComputations">Boolean indicating whether this key can be sent to trusted enclave</param>
        /// <param name="signature">Signature for the master key metadata</param>
        /// <returns>Boolean indicating whether the master key metadata can be verified based on the provided signature</returns>
        public virtual bool VerifyColumnMasterKeyMetadata(string masterKeyPath, bool allowEnclaveComputations, byte[] signature)
        {
            throw new NotImplementedException();
        }
    }
}
