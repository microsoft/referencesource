//---------------------------------------------------------------------
// <copyright file="RootProjectionNode.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Represents the root of the projection tree
//      for queries with $expand and/or $select.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    #region Namespaces
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;
    #endregion

    /// <summary>Internal class which implements the root of the projection tree.</summary>
    /// <remarks>This class is used to carry information required by our V1 providers
    /// to able able to fall back to the V1 behavior of using the <see cref="IExpandProvider"/> interface.</remarks>
    [DebuggerDisplay("RootProjectionNode {PropertyName}")]
    internal class RootProjectionNode : ExpandedProjectionNode
    {
        #region Private fields
        /// <summary>The collection of expand paths.</summary>
        /// <remarks>Used by V1 providers to pass the V1 way of representing description
        /// of expands in the query to the <see cref="IExpandProvider"/>.</remarks>
        private readonly List<ExpandSegmentCollection> expandPaths;

        /// <summary>The base resource type for all entities in this query.</summary>
        /// <remarks>This is usually the base resource type of the resource set as well,
        /// but it can happen that it's a derived type of the resource set base type.</remarks>
        private readonly ResourceType baseResourceType;
        #endregion

        #region Constructor
        /// <summary>Creates new root node for the projection tree.</summary>
        /// <param name="resourceSetWrapper">The resource set of the root level of the query.</param>
        /// <param name="orderingInfo">The ordering info for this node. null means no ordering to be applied.</param>
        /// <param name="filter">The filter for this node. null means no filter to be applied.</param>
        /// <param name="skipCount">Number of results to skip. null means no results to be skipped.</param>
        /// <param name="takeCount">Maximum number of results to return. null means return all available results.</param>
        /// <param name="maxResultsExpected">Maximum number of expected results. Hint that the provider should return
        /// at least maxResultsExpected + 1 results (if available).</param>
        /// <param name="expandPaths">The list of expanded paths.</param>
        /// <param name="baseResourceType">The resource type for all entities in this query.</param>
        internal RootProjectionNode(
            ResourceSetWrapper resourceSetWrapper,
            OrderingInfo orderingInfo,
            Expression filter,
            int? skipCount,
            int? takeCount,
            int? maxResultsExpected,
            List<ExpandSegmentCollection> expandPaths,
            ResourceType baseResourceType)
            : base(
                String.Empty,
                null,
                resourceSetWrapper,
                orderingInfo,
                filter,
                skipCount,
                takeCount,
                maxResultsExpected)
        {
            Debug.Assert(baseResourceType != null, "baseResourceType != null");

            this.expandPaths = expandPaths;
            this.baseResourceType = baseResourceType;
        }
        #endregion

        #region Internal properties
        /// <summary>The resource type in which all the entities expanded by this segment will be of.</summary>
        /// <remarks>This is usually the resource type of the <see cref="ResourceSetWrapper"/> for this node,
        /// but it can also be a derived type of that resource type.
        /// This can happen if navigation property points to a resource set but uses a derived type.
        /// It can also happen if service operation returns entities from a given resource set
        /// but it returns derived types.</remarks>
        internal override ResourceType ResourceType
        {
            get
            {
                return this.baseResourceType;
            }
        }

        /// <summary>The collection of expand paths.</summary>
        /// <remarks>Used by V1 providers to pass the V1 way of representing description
        /// of expands in the query to the <see cref="IExpandProvider"/>.</remarks>
        internal List<ExpandSegmentCollection> ExpandPaths
        {
            get
            {
                return this.expandPaths;
            }
        }

        /// <summary>Flag which is set when the ExpandPaths property should be used to determine the expanded
        /// properties to serialize instead of using the ProjectedNode tree.</summary>
        /// <remarks>This flag is set if the old IExpandProvider was used to process expansions and thus it could have
        /// modified the ExpandPaths, in which case the serialization needs to use that to comply with the provider.
        /// Note that this can never be set to true if projections where used in the query, so in that case
        /// there's no possiblity for the ExpandPaths to differ from the ProjectedNode tree.
        /// If projections are in place, we only apply the expansion removal caused by "*" projection expressions
        /// to the tree and leave the ExpandPaths unaffected, as they should not be used in that case.</remarks>
        internal bool UseExpandPathsForSerialization
        {
            get;
            set;
        }

        /// <summary>Flag used to mark that projections were used in the query.</summary>
        internal bool ProjectionsSpecified 
        { 
            get; 
            set; 
        }

        /// <summary>Returns true if there are any expansions in this tree.</summary>
        internal bool ExpansionsSpecified 
        { 
            get; 
            set; 
        }
        #endregion
    }
}
