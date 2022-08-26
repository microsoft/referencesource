//---------------------------------------------------------------------
// <copyright file="IDataServicePagingProvider.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides an interface that supports custom paging by
//      the data service providers.
// </summary>
//
// @owner  wbasheer
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    #region Namespaces
    using System.Collections;
    using System.Linq;
    #endregion

    /// <summary>
    /// When exposed by a provider, this interface is used to provide custom paging for the clients.
    /// </summary>
    public interface IDataServicePagingProvider
    {
        /// <summary>
        /// Return the next-page token to put in the $skiptoken query option, or null if no continuation is required. 
        /// </summary>
        /// <param name="enumerator">Enumerator for which continuation token is being requested.</param>
        /// <returns>Continuation token as a collection of primitive types.</returns>
        object[] GetContinuationToken(IEnumerator enumerator);

        /// <summary>
        /// Gives the continuation token ($skiptoken) from the request URI, parsed into primitive values, to the provider.   
        /// </summary>
        /// <param name="query">Query for which continuation token is being provided.</param>
        /// <param name="resourceType">Resource type of the result on which skip token is to be applied.</param>
        /// <param name="continuationToken">Continuation token parsed into primitive typed values.</param>
        void SetContinuationToken(IQueryable query, ResourceType resourceType, object[] continuationToken);
    }
}