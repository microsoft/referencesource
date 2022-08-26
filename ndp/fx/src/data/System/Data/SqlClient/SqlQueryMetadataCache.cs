//------------------------------------------------------------------------------
// <copyright file="SqlQueryMetadataCache.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <owner current="true" primary="true">panant</owner>
// <owner current="true" primary="false">skaushi</owner>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.Caching;
    using System.Text;
    using System.Threading;

    /// <summary>
    /// <para> Implements a cache of query paramater metadata that is used to avoid the extra roundtrip to the server for every execution of the same query.</para>
    /// </summary>
    sealed internal class SqlQueryMetadataCache {
        const int CacheSize = 2000; // Cache size in number of entries.
        const int CacheTrimThreshold = 300; // Threshold above the cache size when we start trimming.

        private readonly MemoryCache _cache;
        private static readonly SqlQueryMetadataCache _singletonInstance = new SqlQueryMetadataCache();
        private int _inTrim = 0;
        private long _cacheHits = 0;
        private long _cacheMisses = 0;

#if DEBUG
        private bool _sleepOnTrim = false;
#endif

        private SqlQueryMetadataCache() {
            _cache = new MemoryCache("SqlQueryMetadataCache");
            _inTrim = 0;
            _cacheHits = 0;
            _cacheMisses = 0;
        }

        internal static SqlQueryMetadataCache GetInstance() {
            return _singletonInstance;
        }

        /// <summary>
        /// <para> Retrieves the query metadata for a specific query from the cache.</para>
        /// </summary>
        internal bool GetQueryMetadataIfExists(SqlCommand sqlCommand) {
            // Return immediately if caching is disabled.
            if (!SqlConnection.ColumnEncryptionQueryMetadataCacheEnabled){
                    return false;
            }

            // Check the cache to see if we have the MD for this query cached.
            string cacheLookupKey = GetCacheLookupKeyFromSqlCommand(sqlCommand);
            if (cacheLookupKey == null) {
                IncrementCacheMisses();
                return false;
            }

            Dictionary<string, SqlCipherMetadata> ciperMetadataDictionary = _cache.Get(cacheLookupKey) as Dictionary<string, SqlCipherMetadata>;
            
            // If we had a cache miss just return false.
            if (ciperMetadataDictionary == null) {
                IncrementCacheMisses();
                return false;
            }

            // Iterate over all the parameters and try to get their cipher MD.
            foreach (SqlParameter param in sqlCommand.Parameters) {
                SqlCipherMetadata paramCiperMetadata;
                bool found = ciperMetadataDictionary.TryGetValue(param.ParameterNameFixed, out paramCiperMetadata);

                // If we failed to identify the encryption for a specific parameter, clear up the cipher MD of all parameters and exit.
                if (!found) {
                    foreach (SqlParameter paramToCleanup in sqlCommand.Parameters) {
                        paramToCleanup.CipherMetadata = null;
                    }

                    IncrementCacheMisses();
                    return false;
                }

                // Cached cipher MD should never have an initialized algorithm since this would contain the key.
                Debug.Assert(paramCiperMetadata == null || !paramCiperMetadata.IsAlgorithmInitialized());

                // We were able to identify the cipher MD for this parameter, so set it on the param.
                param.CipherMetadata = paramCiperMetadata;
            }

            // Create a copy of the cipherMD in order to load the key.
            // The key shouldn't be loaded in the cached version for security reasons.
            foreach (SqlParameter param in sqlCommand.Parameters) {
                SqlCipherMetadata cipherMdCopy = null;

                if (param.CipherMetadata != null) {
                    cipherMdCopy = new SqlCipherMetadata(
                        param.CipherMetadata.EncryptionInfo,
                        0,
                        param.CipherMetadata.CipherAlgorithmId,
                        param.CipherMetadata.CipherAlgorithmName,
                        param.CipherMetadata.EncryptionType,
                        param.CipherMetadata.NormalizationRuleVersion);
                }

                param.CipherMetadata = cipherMdCopy;

                if (cipherMdCopy != null) { 
                    // Try to get the encryption key. If the key information is stale, this might fail.
                    // In this case, just fail the cache lookup.
                    try {
                        SqlSecurityUtility.DecryptSymmetricKey(cipherMdCopy, sqlCommand.Connection.DataSource);
                    }
                    catch (Exception ex) {
                        // Invalidate the cache entry.
                        InvalidateCacheEntry(sqlCommand);

                        // If we get one of the expected exceptions, just fail the cache lookup, otherwise throw.
                        if (ex is SqlException || ex is ArgumentException || ex is ArgumentNullException) {
                            foreach (SqlParameter paramToCleanup in sqlCommand.Parameters) {
                                paramToCleanup.CipherMetadata = null;
                            }

                            IncrementCacheMisses();
                            return false;
                        }

                        throw;
                    }
                }
            }

            IncrementCacheHits();
            return true;
        }

        /// <summary>
        /// <para> Add the metadata for a specific query to the cache.</para>
        /// </summary>
        internal void AddQueryMetadata(SqlCommand sqlCommand, bool ignoreQueriesWithReturnValueParams) {
            // Return immediately if caching is disabled.
            if (!SqlConnection.ColumnEncryptionQueryMetadataCacheEnabled){
                return;
            }

            // We don't want to cache parameter metadata for commands with ReturnValue because there is no way for the client to verify that the cached information is still valid.
            // ReturnStatus is fine because it is always plaintext, but we cannot distinguish between the two at RPC time (they are both ReturnValue parameters), but only when the TDS tokens with the result come back.
            // Therefore we want to postpone populating the cache for any queries that have a ReturnValue parameter until we get the return tokens from TDS.
            // Check if we have a ReturnValue parameter and simply exit unless the caller wants to include queries with return values.
            // Only stored procs can have a real ReturnValue so just check for these.
            if (sqlCommand.CommandType == CommandType.StoredProcedure) {
                foreach (SqlParameter param in sqlCommand.Parameters) {
                    // If we have a return value parameter don't cache the query MD.
                    // We will cache it after we have confirmed it is looking for ReturnStatus and not ReturnValue.
                    if (param.Direction == ParameterDirection.ReturnValue && ignoreQueriesWithReturnValueParams) {
                        sqlCommand.CachingQueryMetadataPostponed = true;
                        return;
                    }
                }
            }

            // Construct the entry and put it in the cache.
            string cacheLookupKey = GetCacheLookupKeyFromSqlCommand(sqlCommand);
            if (cacheLookupKey == null) {
                return;
            }

            Dictionary<string, SqlCipherMetadata> ciperMetadataDictionary = new Dictionary<string, SqlCipherMetadata>(sqlCommand.Parameters.Count);

            // Create a copy of the cipherMD that doesn't have the algorithm and put it in the cache.
            foreach (SqlParameter param in sqlCommand.Parameters) {
                SqlCipherMetadata cipherMdCopy = null;
                if (param.CipherMetadata != null) {
                    cipherMdCopy = new SqlCipherMetadata(
                        param.CipherMetadata.EncryptionInfo,
                        0,
                        param.CipherMetadata.CipherAlgorithmId,
                        param.CipherMetadata.CipherAlgorithmName,
                        param.CipherMetadata.EncryptionType,
                        param.CipherMetadata.NormalizationRuleVersion);
                }

                // Cached cipher MD should never have an initialized algorithm since this would contain the key.
                Debug.Assert(cipherMdCopy == null || !cipherMdCopy.IsAlgorithmInitialized());

                ciperMetadataDictionary.Add(param.ParameterNameFixed, cipherMdCopy);
            }

            // If the size of the cache exceeds the threshold, set that we are in trimming and trim the cache accordingly.
            long currentCacheSize = _cache.GetCount();
            if ((currentCacheSize > CacheSize + CacheTrimThreshold) && (0 == Interlocked.CompareExchange(ref _inTrim, 1, 0))) {
                try
                {
#if DEBUG
                    if (_sleepOnTrim) {
                        Thread.Sleep(TimeSpan.FromSeconds(10));
                    }
#endif
                    _cache.Trim((int)(((double)(currentCacheSize - CacheSize) / (double)currentCacheSize) * 100));
                }
                finally {
                    Interlocked.CompareExchange(ref _inTrim, 0, 1);
                }
            }

            // By default evict after 10 hours.
            _cache.Set(cacheLookupKey, ciperMetadataDictionary, DateTimeOffset.UtcNow.AddHours(10));
        }

        /// <summary>
        /// <para> Remove the metadata for a specific query from the cache.</para>
        /// </summary>
        internal void InvalidateCacheEntry(SqlCommand sqlCommand) {
            string cacheLookupKey = GetCacheLookupKeyFromSqlCommand(sqlCommand);
            if (cacheLookupKey == null) {
                return;
            }
            
            _cache.Remove(cacheLookupKey);
        }


        /// <summary>
        /// Increments the counter for the cache hits in the query metadata cache.
        /// </summary>
        private void IncrementCacheHits() {
            Interlocked.Increment(ref _cacheHits);
        }

        /// <summary>
        /// Increments the counter for the cache misses in the query metadata cache.
        /// </summary>
        private void IncrementCacheMisses() {
            Interlocked.Increment(ref _cacheMisses);
        }

        /// <summary>
        /// Resets the counters for the cache hits and misses in the query metadata cache.
        /// </summary>
        private void ResetCacheCounts() {
            _cacheHits = 0;
            _cacheMisses = 0;
        }

        private String GetCacheLookupKeyFromSqlCommand(SqlCommand sqlCommand) {
            const int SqlIdentifierLength = 128;

            SqlConnection connection = sqlCommand.Connection;

            // Return null if we have no connection.
            if (connection == null) {
                return null;
            }

            StringBuilder cacheLookupKeyBuilder = new StringBuilder(connection.DataSource, capacity: connection.DataSource.Length + SqlIdentifierLength + sqlCommand.CommandText.Length + 6);
            cacheLookupKeyBuilder.Append(":::");
            // Pad database name to 128 characters to avoid any false cache matches because of weird DB names.
            cacheLookupKeyBuilder.Append(connection.Database.PadRight(SqlIdentifierLength));
            cacheLookupKeyBuilder.Append(":::");
            cacheLookupKeyBuilder.Append(sqlCommand.CommandText);

            return cacheLookupKeyBuilder.ToString();
        }
    }
}
