//------------------------------------------------------------------------------
// <copyright file="ColumnMasterKeyMetadataSignatureVerificationCache.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <owner current="true" primary="true">nivithla</owner>
//------------------------------------------------------------------------------


namespace System.Data.SqlClient
{
    using System;
    using System.Runtime.Caching;
    using System.Text;
    using System.Threading; 

    /// <summary>
    /// Cache for storing result of signature verfication of CMK Metadata
    /// </summary>
    internal class ColumnMasterKeyMetadataSignatureVerificationCache
    {
        private const int _cacheSize = 2000; // Cache size in number of entries.
        private const int _cacheTrimThreshold = 300; // Threshold above the cache size when we start trimming.

        private const string _className = "ColumnMasterKeyMetadataSignatureVerificationCache";
        private const string _getSignatureVerificationResultMethodName = "GetSignatureVerificationResult";
        private const string _addSignatureVerificationResultMethodName = "AddSignatureVerificationResult";
        private const string _masterkeypathArgumentName = "masterKeyPath";
        private const string _keyStoreNameArgumentName = "keyStoreName";
        private const string _signatureName = "signature";
        private const string _cacheLookupKeySeparator = ":";
        
        private static readonly ColumnMasterKeyMetadataSignatureVerificationCache _signatureVerificationCache = new ColumnMasterKeyMetadataSignatureVerificationCache();

        //singleton instance
        internal static ColumnMasterKeyMetadataSignatureVerificationCache Instance { get { return _signatureVerificationCache; } }

        private readonly MemoryCache _cache;
        private int _inTrim = 0;

        private ColumnMasterKeyMetadataSignatureVerificationCache() {
            _cache = new MemoryCache(_className);
            _inTrim = 0;
        }

        /// <summary>
        /// Get signature verification result for given CMK metadata (KeystoreName, MasterKeyPath, allowEnclaveComputations) and a given signature
        /// </summary>
        /// <param name="keyStoreName">Key Store name for CMK</param>
        /// <param name="masterKeyPath">Key Path for CMK</param>
        /// <param name="allowEnclaveComputations">boolean indicating whether the key can be sent to enclave</param>
        /// <param name="signature">Signature for the CMK metadata</param>
        /// <returns>null if the data is not found in cache otherwise returns true/false indicating signature verification success/failure</returns>
        internal bool? GetSignatureVerificationResult(string keyStoreName, string masterKeyPath, bool allowEnclaveComputations, byte[] signature) {

            ValidateStringArgumentNotNullOrEmpty(masterKeyPath, _masterkeypathArgumentName, _getSignatureVerificationResultMethodName);
            ValidateStringArgumentNotNullOrEmpty(keyStoreName, _keyStoreNameArgumentName, _getSignatureVerificationResultMethodName);
            ValidateSignatureNotNullOrEmpty(signature, _getSignatureVerificationResultMethodName);

            string cacheLookupKey = GetCacheLookupKey(masterKeyPath, allowEnclaveComputations, signature, keyStoreName);

            return _cache.Get(cacheLookupKey) as bool?;
        }

        /// <summary>
        /// Add signature verification result for given CMK metadata (KeystoreName, MasterKeyPath, allowEnclaveComputations) and a given signature in the cache
        /// </summary>
        /// <param name="keyStoreName">Key Store name for CMK</param>
        /// <param name="masterKeyPath">Key Path for CMK</param>
        /// <param name="allowEnclaveComputations">boolean indicating whether the key can be sent to enclave</param>
        /// <param name="signature">Signature for the CMK metadata</param>
        /// <param name="result">result indicating signature verification success/failure</param>
        internal void AddSignatureVerificationResult(string keyStoreName, string masterKeyPath, bool allowEnclaveComputations, byte[] signature, bool result) {

            ValidateStringArgumentNotNullOrEmpty(masterKeyPath, _masterkeypathArgumentName, _addSignatureVerificationResultMethodName);
            ValidateStringArgumentNotNullOrEmpty(keyStoreName, _keyStoreNameArgumentName, _addSignatureVerificationResultMethodName);
            ValidateSignatureNotNullOrEmpty(signature, _addSignatureVerificationResultMethodName);

            string cacheLookupKey = GetCacheLookupKey(masterKeyPath, allowEnclaveComputations, signature, keyStoreName);

            TrimCacheIfNeeded();

            // By default evict after 10 days.
            _cache.Set(cacheLookupKey, result, DateTimeOffset.UtcNow.AddDays(10));
        }

        private void ValidateSignatureNotNullOrEmpty(byte[] signature, string methodName) {
            if (signature == null || signature.Length == 0) {
                if (null == signature) {
                    throw SQL.NullArgumentInternal(_signatureName, _className, methodName);
                }
                else {
                    throw SQL.EmptyArgumentInternal(_signatureName, _className, methodName);
                }
            }
        }

        private void ValidateStringArgumentNotNullOrEmpty(string stringArgValue, string stringArgName, string methodName) {
            if (string.IsNullOrWhiteSpace(stringArgValue)) {
                if (null == stringArgValue) {
                    throw SQL.NullArgumentInternal(stringArgName, _className, methodName);
                }
                else {
                    throw SQL.EmptyArgumentInternal(stringArgName, _className, methodName);
                }
            }
        }

        private void TrimCacheIfNeeded() {
            // If the size of the cache exceeds the threshold, set that we are in trimming and trim the cache accordingly.
            long currentCacheSize = _cache.GetCount();
            if ((currentCacheSize > _cacheSize + _cacheTrimThreshold) && (0 == Interlocked.CompareExchange(ref _inTrim, 1, 0))) {
                try {
                    _cache.Trim((int) (((double) (currentCacheSize - _cacheSize)/(double) currentCacheSize)*100));
                }
                finally {
                    Interlocked.CompareExchange(ref _inTrim, 0, 1);
                }
            }
        }

        private string GetCacheLookupKey(string masterKeyPath, bool allowEnclaveComputations, byte[] signature, string keyStoreName) {
            StringBuilder cacheLookupKeyBuilder = new StringBuilder(keyStoreName,
                capacity:
                    keyStoreName.Length + 
                    masterKeyPath.Length +
                    SqlSecurityUtility.GetBase64LengthFromByteLength(signature.Length) + 
                    3 /*separators*/ +
                    10 /*boolean value + somebuffer*/);

            cacheLookupKeyBuilder.Append(_cacheLookupKeySeparator);
            cacheLookupKeyBuilder.Append(masterKeyPath);
            cacheLookupKeyBuilder.Append(_cacheLookupKeySeparator);
            cacheLookupKeyBuilder.Append(allowEnclaveComputations);
            cacheLookupKeyBuilder.Append(_cacheLookupKeySeparator);
            cacheLookupKeyBuilder.Append(Convert.ToBase64String(signature));
            cacheLookupKeyBuilder.Append(_cacheLookupKeySeparator);
            string cacheLookupKey = cacheLookupKeyBuilder.ToString();
            return cacheLookupKey;
        }
    }
}
