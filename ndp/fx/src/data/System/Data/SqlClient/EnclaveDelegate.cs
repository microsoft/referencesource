//------------------------------------------------------------------------------
// <copyright file="SqlCommand.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <owner current="true" primary="true">nivithla</owner>
// <owner current="true" primary="false">nivithla</owner>
//------------------------------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;

namespace System.Data.SqlClient {

    /// <summary>
    /// A delegate for communicating with secure enclave
    /// </summary>
    internal class EnclaveDelegate {

        private static readonly SqlAeadAes256CbcHmac256Factory SqlAeadAes256CbcHmac256Factory = new SqlAeadAes256CbcHmac256Factory();
        private static readonly string GetAttestationInfoQueryString = String.Format(@"Select GetTrustedModuleIdentityAndAttestationInfo({0}) as attestationInfo", 0);
        private static readonly EnclaveDelegate _EnclaveDelegate = new EnclaveDelegate();

        private static readonly string ClassName = "EnclaveDelegate";
        private static readonly string GetDecryptedKeysToBeSentToEnclaveName = "GetDecryptedKeysToBeSentToEnclave";
        private static readonly string GetSerializedAttestationParametersName = "GetSerializedAttestationParameters";
        private static readonly string ComputeQueryStringHashName = "ComputeQueryStringHash";
        
        private readonly Object _lock = new Object();

        //singleton instance
        public static EnclaveDelegate Instance { get { return _EnclaveDelegate; } }

        private EnclaveDelegate() { }

        /// <summary>
        /// Generate the byte package that needs to be sent to the enclave
        /// </summary>
        /// <param name="keysTobeSentToEnclave">Keys to be sent to enclave</param>
        /// <param name="enclaveType">enclave type</param>
        /// <param name="serverName">server name</param>
        /// <param name="enclaveAttestationUrl">url for attestation endpoint</param>
        /// <returns></returns>
        internal EnclavePackage GenerateEnclavePackage(Dictionary<int, SqlTceCipherInfoEntry> keysTobeSentToEnclave, string queryText, string enclaveType, string serverName, string enclaveAttestationUrl) {
            
            SqlEnclaveSession sqlEnclaveSession = null;
            long counter;
            try {
                GetEnclaveSession(enclaveType, serverName, enclaveAttestationUrl, out sqlEnclaveSession, out counter, throwIfNull: true);
            } catch (Exception e) {
                throw new RetriableEnclaveQueryExecutionException(e.Message, e);
            }

            List<ColumnEncryptionKeyInfo> decryptedKeysToBeSentToEnclave = GetDecryptedKeysToBeSentToEnclave(keysTobeSentToEnclave, serverName);
            byte[] queryStringHashBytes = ComputeQueryStringHash(queryText);
            byte[] keyBytePackage = GenerateBytePackageForKeys(counter, queryStringHashBytes, decryptedKeysToBeSentToEnclave);
            byte[] sessionKey = sqlEnclaveSession.GetSessionKey();
            byte[] encryptedBytePackage = EncryptBytePackage(keyBytePackage, sessionKey, serverName);
            byte[] enclaveSessionHandle = BitConverter.GetBytes(sqlEnclaveSession.SessionId);
            byte[] byteArrayToBeSentToEnclave = CombineByteArrays(new[] { enclaveSessionHandle, encryptedBytePackage });
            return new EnclavePackage(byteArrayToBeSentToEnclave, sqlEnclaveSession);
        }

        internal void InvalidateEnclaveSession(string enclaveType, string serverName, string EnclaveAttestationUrl, SqlEnclaveSession enclaveSession) {
            SqlColumnEncryptionEnclaveProvider sqlColumnEncryptionEnclaveProvider = GetEnclaveProvider(enclaveType);
            sqlColumnEncryptionEnclaveProvider.InvalidateEnclaveSession(serverName, EnclaveAttestationUrl, enclaveSession);
        }

        internal void GetEnclaveSession(string enclaveType, string serverName, string enclaveAttestationUrl, out SqlEnclaveSession sqlEnclaveSession) {
            long counter;
            GetEnclaveSession(enclaveType, serverName, enclaveAttestationUrl, out sqlEnclaveSession, out counter, throwIfNull:false);
        }

        private void GetEnclaveSession(string enclaveType, string serverName, string enclaveAttestationUrl, out SqlEnclaveSession sqlEnclaveSession, out long counter, bool throwIfNull) {
            SqlColumnEncryptionEnclaveProvider sqlColumnEncryptionEnclaveProvider = GetEnclaveProvider(enclaveType);
            sqlColumnEncryptionEnclaveProvider.GetEnclaveSession(serverName, enclaveAttestationUrl, out sqlEnclaveSession, out counter);

            if (throwIfNull) {
                if (sqlEnclaveSession == null) throw SQL.NullEnclaveSessionDuringQueryExecution(enclaveType, enclaveAttestationUrl);
            }
        }

        internal SqlEnclaveAttestationParameters GetAttestationParameters(string enclaveType, string serverName, string enclaveAttestationUrl) {
            SqlColumnEncryptionEnclaveProvider sqlColumnEncryptionEnclaveProvider = GetEnclaveProvider(enclaveType);
            return sqlColumnEncryptionEnclaveProvider.GetAttestationParameters();
        }

        internal byte[] GetSerializedAttestationParameters(SqlEnclaveAttestationParameters sqlEnclaveAttestationParameters, string enclaveType) {
            byte[] attestationProtocolBytes = null;
            byte[] attestationProtocolInputLengthBytes = null;
            byte[] clientDHPublicKeyLengthBytes = null;
            int attestationProtocolInt = sqlEnclaveAttestationParameters.Protocol;
 
            // attestation protocol
            attestationProtocolBytes = GetUintBytes(enclaveType, attestationProtocolInt, "attestationProtocol");

            if (attestationProtocolBytes == null) { 
                throw SQL.NullArgumentInternal("attestationProtocolBytes", ClassName, GetSerializedAttestationParametersName);
            }
            
            // attestationProtocolInput
            byte[] attestationProtocolInputBytes = sqlEnclaveAttestationParameters.GetInput();
            
            // attestationProtocolInput length
            attestationProtocolInputLengthBytes = GetUintBytes(enclaveType, attestationProtocolInputBytes.Length, "attestationProtocolInputLength");

            if (attestationProtocolInputLengthBytes == null) { 
                throw SQL.NullArgumentInternal("attestationProtocolInputLengthBytes", ClassName, GetSerializedAttestationParametersName);
            }

            // clientDHPublicKey
            byte[] clientDHPublicKey = sqlEnclaveAttestationParameters.ClientDiffieHellmanKey.Key.Export(CngKeyBlobFormat.EccPublicBlob);
            
            // clientDHPublicKey length
            clientDHPublicKeyLengthBytes =  GetUintBytes(enclaveType, clientDHPublicKey.Length, "clientDHPublicKeyLength");

            if (clientDHPublicKeyLengthBytes == null) { 
                throw SQL.NullArgumentInternal("clientDHPublicKeyLengthBytes", ClassName, GetSerializedAttestationParametersName);
            }

            return CombineByteArrays(new[] { attestationProtocolBytes, attestationProtocolInputLengthBytes, attestationProtocolInputBytes, clientDHPublicKeyLengthBytes, clientDHPublicKey});
        }

        private byte[] GetUintBytes(string enclaveType, int intValue, string variableName) {
            try {
                uint attestationProtocol = Convert.ToUInt32(intValue);
                return BitConverter.GetBytes(attestationProtocol);
            }
            catch (Exception e) {
                throw SQL.InvalidAttestationParameterUnableToConvertToUnsignedInt(
                    variableName, intValue, enclaveType, e);
            }
        }

        /// <summary>
        /// Create a new enclave session
        /// </summary>
        /// <param name="enclaveType">enclave type</param>
        /// <param name="serverName">servername</param>
        /// <param name="attestationUrl">attestation url for attestation service endpoint</param>
        /// <param name="attestationInfo">attestation info from SQL Server</param>
        /// <param name="attestationParameters">attestation parameters</param>
        internal void CreateEnclaveSession(string enclaveType, string serverName, string attestationUrl, 
            byte[] attestationInfo, SqlEnclaveAttestationParameters attestationParameters) {

            lock (_lock) {
                SqlColumnEncryptionEnclaveProvider sqlColumnEncryptionEnclaveProvider = GetEnclaveProvider(enclaveType);
                long counter;
                SqlEnclaveSession sqlEnclaveSession = null;
                sqlColumnEncryptionEnclaveProvider.GetEnclaveSession(serverName, attestationUrl, out sqlEnclaveSession, out counter);

                if (sqlEnclaveSession != null) {
                    return;
                }

                sqlColumnEncryptionEnclaveProvider.CreateEnclaveSession(attestationInfo, attestationParameters.ClientDiffieHellmanKey, attestationUrl, serverName, out sqlEnclaveSession, out counter);

                if (sqlEnclaveSession == null) throw SQL.NullEnclaveSessionReturnedFromProvider(enclaveType, attestationUrl);
            }
        }

        private SqlColumnEncryptionEnclaveProvider GetEnclaveProvider(string enclaveType) {
            if (SqlConnection.sqlColumnEncryptionEnclaveProviderConfigurationManager == null)
                throw SQL.EnclaveProvidersNotConfiguredForEnclaveBasedQuery();

            var sqlColumnEncryptionEnclaveProvider =
                SqlConnection.sqlColumnEncryptionEnclaveProviderConfigurationManager.GetSqlColumnEncryptionEnclaveProvider(
                    enclaveType);

            if (sqlColumnEncryptionEnclaveProvider == null) throw SQL.EnclaveProviderNotFound(enclaveType);
            return sqlColumnEncryptionEnclaveProvider;
        }

        /// <summary>
        /// Decrypt the keys that need to be sent to the enclave
        /// </summary>
        /// <param name="keysTobeSentToEnclave">Keys that need to sent to the enclave</param>
        /// <param name="sqlConnection">active connection</param>
        /// <returns></returns>
        private List<ColumnEncryptionKeyInfo> GetDecryptedKeysToBeSentToEnclave(Dictionary<int, SqlTceCipherInfoEntry> keysTobeSentToEnclave, string serverName) {
            List<ColumnEncryptionKeyInfo> decryptedKeysToBeSentToEnclave = new List<ColumnEncryptionKeyInfo>();

            foreach (SqlTceCipherInfoEntry cipherInfo in keysTobeSentToEnclave.Values) {
                SqlClientSymmetricKey sqlClientSymmetricKey = null;
                SqlEncryptionKeyInfo? encryptionkeyInfoChosen = null;
                SqlSecurityUtility.DecryptSymmetricKey(cipherInfo, serverName, out sqlClientSymmetricKey,
                    out encryptionkeyInfoChosen);

                if (sqlClientSymmetricKey == null) throw SQL.NullArgumentInternal("sqlClientSymmetricKey", ClassName, GetDecryptedKeysToBeSentToEnclaveName);
                if (cipherInfo.ColumnEncryptionKeyValues == null) throw SQL.NullArgumentInternal("ColumnEncryptionKeyValues", ClassName, GetDecryptedKeysToBeSentToEnclaveName);
                if (!(cipherInfo.ColumnEncryptionKeyValues.Count > 0)) throw SQL.ColumnEncryptionKeysNotFound();

                //cipherInfo.CekId is always 0, hence used cipherInfo.ColumnEncryptionKeyValues[0].cekId. Even when cek has multiple ColumnEncryptionKeyValues
                //the cekid and the plaintext value will remain the same, what varies is the encrypted cek value, since the cek can be encrypted by 
                //multiple CMKs
                decryptedKeysToBeSentToEnclave.Add(new ColumnEncryptionKeyInfo(sqlClientSymmetricKey.RootKey,
                    cipherInfo.ColumnEncryptionKeyValues[0].databaseId,
                    cipherInfo.ColumnEncryptionKeyValues[0].cekMdVersion, cipherInfo.ColumnEncryptionKeyValues[0].cekId));
            }
            return decryptedKeysToBeSentToEnclave;
        }

        /// <summary>
        /// Generate a byte package consisting of decrypted keys and some headers expected by the enclave
        /// </summary>
        /// <param name="enclaveSessionCounter">counter to avoid replay attacks</param>
        /// <param name="keys"></param>
        /// <returns></returns>
        private byte[] GenerateBytePackageForKeys(long enclaveSessionCounter, byte[] queryStringHashBytes, List<ColumnEncryptionKeyInfo> keys) {

            //Format GUID | counter | queryStringHash | key[1]id | key[1]Bytes | ...... key[n]id | key[n]bytes
            Guid guid = Guid.NewGuid();
            byte[] guidBytes = guid.ToByteArray();
            byte[] counterBytes = BitConverter.GetBytes(enclaveSessionCounter);

            int lengthOfByteArrayToAllocate = guidBytes.Length;
            lengthOfByteArrayToAllocate += counterBytes.Length;
            lengthOfByteArrayToAllocate += queryStringHashBytes.Length;

            foreach (ColumnEncryptionKeyInfo key in keys) {
                lengthOfByteArrayToAllocate += key.GetLengthForSerialization();
            }

            byte[] bytePackage = new byte[lengthOfByteArrayToAllocate];
            int startOffset = 0;

            Buffer.BlockCopy(guidBytes, 0, bytePackage, startOffset, guidBytes.Length);
            startOffset += guidBytes.Length;

            Buffer.BlockCopy(counterBytes, 0, bytePackage, startOffset, counterBytes.Length);
            startOffset += counterBytes.Length;

            Buffer.BlockCopy(queryStringHashBytes, 0, bytePackage, startOffset, queryStringHashBytes.Length);
            startOffset += queryStringHashBytes.Length;

            foreach (ColumnEncryptionKeyInfo key in keys) {
                startOffset = key.SerializeToBuffer(bytePackage, startOffset);
            }

            return bytePackage;
        }

        /// <summary>
        /// Encrypt the byte package containing keys with the session key
        /// </summary>
        /// <param name="bytePackage">byte package containing keys</param>
        /// <param name="sessionKey">session key used to encrypt the package</param>
        /// <param name="serverName">server hosting the enclave</param>
        /// <returns></returns>
        private byte[] EncryptBytePackage(byte[] bytePackage, byte[] sessionKey, string serverName) {
            if (sessionKey == null) throw SQL.NullArgumentInternal("sessionKey", ClassName, "EncryptBytePackage");
            if (sessionKey.Length == 0) throw SQL.EmptyArgumentInternal("sessionKey", ClassName, "EncryptBytePackage");
            //bytePackage is created internally in this class and is guaranteed to be non null and non empty

            try {
                SqlClientSymmetricKey symmetricKey = new SqlClientSymmetricKey(sessionKey);
                SqlClientEncryptionAlgorithm sqlClientEncryptionAlgorithm =
                    SqlAeadAes256CbcHmac256Factory.Create(symmetricKey, SqlClientEncryptionType.Randomized,
                        SqlAeadAes256CbcHmac256Algorithm.AlgorithmName);
                return sqlClientEncryptionAlgorithm.EncryptData(bytePackage);
            } catch (Exception e) {
                throw SQL.FailedToEncryptRegisterRulesBytePackage(e);
            }
        }

        /// <summary>
        /// Combine the array of given byte arrays into one
        /// </summary>
        /// <param name="byteArraysToCombine">byte arrays to be combined</param>
        /// <returns></returns>
        private byte[] CombineByteArrays(byte[][] byteArraysToCombine) {
            byte[] combinedArray = new byte[byteArraysToCombine.Sum(ba => ba.Length)];
            int offset = 0;
            foreach (byte[] byteArray in byteArraysToCombine) {
                Buffer.BlockCopy(byteArray, 0, combinedArray, offset, byteArray.Length);
                offset += byteArray.Length;
            }
            return combinedArray;
        }

        private byte[] ComputeQueryStringHash(string queryString)
        {
            // Validate the input parameters
            if (string.IsNullOrWhiteSpace(queryString)) {
                string argumentName = "queryString";
                if (null == queryString)
                {
                    throw SQL.NullArgumentInternal(argumentName, ClassName, ComputeQueryStringHashName);
                }
                else
                {
                    throw SQL.EmptyArgumentInternal(argumentName, ClassName, ComputeQueryStringHashName);
                }
            }

            byte[] queryStringBytes = Encoding.Unicode.GetBytes(queryString);

            // Compute hash 
            byte[] hash;
            using (SHA256Cng sha256 = new SHA256Cng())
            {
                sha256.TransformFinalBlock(queryStringBytes, 0, queryStringBytes.Length);
                hash = sha256.Hash;
            }
            return hash;
        }

        /// <summary>
        /// Exception when executing a enclave based Always Encrypted query 
        /// </summary>
        internal class RetriableEnclaveQueryExecutionException : Exception {
            internal RetriableEnclaveQueryExecutionException(string message, Exception innerException) : base(message, innerException) { }
        }

        /// <summary>
        /// Class encapsulating necessary information about the byte package that needs to be sent to the enclave
        /// </summary>
        internal class EnclavePackage {

            public SqlEnclaveSession EnclaveSession { get;}
            public byte[] EnclavePackageBytes { get;}

            /// <summary>
            /// Constructor
            /// </summary>
            /// <param name="enclavePackageBytes">byte package to be sent to enclave</param>
            /// <param name="enclaveSession"> enclave session to be used</param>
            internal EnclavePackage(byte[] enclavePackageBytes, SqlEnclaveSession enclaveSession) {
                EnclavePackageBytes = enclavePackageBytes;
                EnclaveSession = enclaveSession;
            }
        }
    }
}
