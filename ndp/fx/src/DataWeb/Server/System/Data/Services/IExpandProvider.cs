//---------------------------------------------------------------------
// <copyright file="IExpandProvider.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides an interface that supports the $expand WCF 
//      Data Service option for a store.
// </summary>
//
// @owner  mruiz
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System.Collections;
    using System.Collections.Generic;
    using System.Linq;

    /// <summary>
    /// This interface declares the methods required to support the $expand
    /// query option for a WCF Data Service.
    /// </summary>
    public interface IExpandProvider
    {
        /// <summary>Applies expansions to the specified <paramref name="queryable"/>.</summary>
        /// <param name="queryable"><see cref="IQueryable"/> object to expand.</param>
        /// <param name="expandPaths">A list of <see cref="ExpandSegmentCollection"/> paths to expand.</param>
        /// <returns>
        /// An <see cref="IEnumerable"/> object of the same type as the given <paramref name="queryable"/>,
        /// with the results including the specified <paramref name="expandPaths"/>.
        /// </returns>
        /// <remarks>
        /// This method may modify the <paramref name="expandPaths"/> to indicate which expansions
        /// are included.
        /// 
        /// The returned <see cref="IEnumerable"/> may implement the <see cref="IExpandedResult"/>
        /// interface to provide enumerable objects for the expansions; otherwise, the expanded
        /// information is expected to be found directly in the enumerated objects.
        /// </remarks>
        IEnumerable ApplyExpansions(IQueryable queryable, ICollection<ExpandSegmentCollection> expandPaths);
    }
}
