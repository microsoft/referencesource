//---------------------------------------------------------------------
// <copyright file="MetadataCache.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a class to cache metadata information.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Caching
{
    using System;
    using System.Collections.Generic;
    using System.Data.EntityClient;
    using System.Data.Objects;
    using System.Diagnostics;
    using System.Threading;

    /// <summary>
    /// Use this class to cache metadata through MetadataCacheItem instances.
    /// </summary>
    internal static class MetadataCache
    {
        /// <summary>AppDomain-wide cache for metadata items.</summary>
        private static Dictionary<MetadataCacheKey, MetadataCacheItem> cache = new Dictionary<MetadataCacheKey, MetadataCacheItem>(new MetadataCacheKey.Comparer());

        /// <summary>Reader/writer lock for AppDomain <see cref="cache"/>.</summary>
        private static ReaderWriterLockSlim cacheLock = new ReaderWriterLockSlim(LockRecursionPolicy.NoRecursion);

        /// <summary>Adds a new cache item, and returns the item that is put in the cache.</summary>
        /// <param name="serviceType">Type of service with metadata being cached.</param>
        /// <param name="dataContextInstance">
        /// Data context instance being cached, possibly segmenting the cache 
        /// space for <paramref name="serviceType"/>.
        /// </param>
        /// <param name="item">Item being added.</param>
        /// <returns>The item being put in the cache (possibly an existing one).</returns>
        /// <remarks>This method is thread-safe but not re-entrant.</remarks>
        internal static MetadataCacheItem AddCacheItem(Type serviceType, object dataContextInstance, MetadataCacheItem item)
        {
            Debug.Assert(serviceType != null, "serviceType != null");
            Debug.Assert(dataContextInstance != null, "dataContextInstance != null");
            Debug.Assert(item != null, "item != null");

            MetadataCacheKey key = new MetadataCacheKey(serviceType, dataContextInstance as ObjectContext);
            MetadataCacheItem result;
            cacheLock.EnterWriteLock();
            try
            {
                // If another thread beat the current thread, we return the
                // previously created item, which has a higher chance of
                // having survived a garbage collection already.
                if (!cache.TryGetValue(key, out result))
                {
                    cache.Add(key, item);
                    result = item;
                }
            }
            finally
            {
                cacheLock.ExitWriteLock();
            }

            Debug.Assert(result != null, "result != null -- a null item is never returned.");
            Debug.Assert(
                result == TryLookup(serviceType, dataContextInstance),
                "result == TryLookup(serviceType, dataContextInstance) -- instance from cache is being returned.");

            return result;
        }

        /// <summary>Tries to look up metadata for the specifed service type and context instance.</summary>
        /// <param name="serviceType">Type of service with metadata being cached.</param>
        /// <param name="dataContextInstance">
        /// Data context instance being cached, possibly segmenting the cache 
        /// space for <paramref name="serviceType"/>.
        /// </param>
        /// <returns>The cached metadata item, if one exists.</returns>
        /// <remarks>This method is thread-safe but not re-entrant.</remarks>
        internal static MetadataCacheItem TryLookup(Type serviceType, object dataContextInstance)
        {
            Debug.Assert(serviceType != null, "serviceType != null");
            Debug.Assert(dataContextInstance != null, "dataContextInstance != null");

            MetadataCacheKey key = new MetadataCacheKey(serviceType, dataContextInstance as ObjectContext);
            MetadataCacheItem result;
            cacheLock.EnterReadLock();
            try
            {
                cache.TryGetValue(key, out result);
            }
            finally
            {
                cacheLock.ExitReadLock();
            }

            return result;
        }

        /// <summary>This type is used as the key in the metadata cache.</summary>
        internal struct MetadataCacheKey
        {
            /// <summary>Connection string used to segment service type.</summary>
            private readonly string dataContextConnection;

            /// <summary>Hash code for this instance.</summary>
            private readonly int hashCode;

            /// <summary>Service type.</summary>
            private readonly Type serviceType;

            /// <summary>Initializes a new MetadataCacheKey instance.</summary>
            /// <param name='serviceType'>Service type for key.</param>
            /// <param name='dataContextInstance'>Data context instace for key, possibly null.</param>
            internal MetadataCacheKey(Type serviceType, ObjectContext dataContextInstance)
            {
                Debug.Assert(serviceType != null, "serviceType != null");
                this.serviceType = serviceType;
                this.dataContextConnection = null;
                this.hashCode = this.serviceType.GetHashCode();
                
                if (dataContextInstance != null)
                {
                    EntityConnection connection = dataContextInstance.Connection as EntityConnection;
                    if (connection != null)
                    {
                        this.dataContextConnection = new EntityConnectionStringBuilder(connection.ConnectionString).Metadata;
                        this.hashCode ^= this.dataContextConnection.GetHashCode();
                    }
                }
            }

            /// <summary>Comparer for metadata cache keys.</summary>
            internal class Comparer : IEqualityComparer<MetadataCacheKey>
            {
                /// <summary>Compares the specified keys.</summary>
                /// <param name="x">First key.</param>
                /// <param name="y">Second key.</param>
                /// <returns>true if <paramref name="x"/> equals <paramref name="y"/>, false otherwise.</returns>
                public bool Equals(MetadataCacheKey x, MetadataCacheKey y)
                {
                    return x.dataContextConnection == y.dataContextConnection && x.serviceType == y.serviceType;
                }

                /// <summary>Gets the hash code for the object.</summary>
                /// <param name="obj">Object.</param>
                /// <returns>The hash code for this key.</returns>
                public int GetHashCode(MetadataCacheKey obj)
                {
                    return obj.hashCode;
                }
            }
        }
    }
}

