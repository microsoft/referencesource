//---------------------------------------------------------------------
// <copyright file="IDataServiceUpdateProvider.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides the update interface for providers.
// </summary>
//
// @owner  pratikp
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    using System.Collections.Generic;

    /// <summary>
    /// This interface declares the methods required for passing
    /// etag values to the provider.
    /// </summary>
    public interface IDataServiceUpdateProvider : IUpdatable
    {
        /// <summary>
        /// Passes the etag value for the given resource.
        /// </summary>
        /// <param name="resourceCookie">cookie representing the resource.</param>
        /// <param name="checkForEquality">true if we need to compare the property values for equality. If false, then we need to compare values for non-equality.</param>
        /// <param name="concurrencyValues">list of the etag property names and its corresponding values.</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1006:DoNotNestGenericTypesInMemberSignatures", Justification = "need to pass in a collection of key value pair")]
        void SetConcurrencyValues(object resourceCookie, bool? checkForEquality, IEnumerable<KeyValuePair<string, object>> concurrencyValues);
    }
}
