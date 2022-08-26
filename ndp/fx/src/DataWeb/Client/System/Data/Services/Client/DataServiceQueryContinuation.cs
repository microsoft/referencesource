//---------------------------------------------------------------------
// <copyright file="DataServiceQueryContinuation.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Provides a class to represent the continuation of a query.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq.Expressions;
    using System.Text;
    using System.Reflection;

    #endregion Namespaces.

    /// <summary>Use this class to represent the continuation of a query.</summary>
    [DebuggerDisplay("{NextLinkUri}")]
    public abstract class DataServiceQueryContinuation
    {
        #region Private fields.

        /// <summary>URI to next page of data.</summary>
        private readonly Uri nextLinkUri;
        
        /// <summary>Projection plan for results of next page.</summary>
        private readonly ProjectionPlan plan;

        #endregion Private fields.

        #region Constructors.

        /// <summary>Initializes a new <see cref="DataServiceQueryContinuation"/> instance.</summary>
        /// <param name="nextLinkUri">URI to next page of data.</param>
        /// <param name="plan">Projection plan for results of next page.</param>
        internal DataServiceQueryContinuation(Uri nextLinkUri, ProjectionPlan plan)
        {
            Debug.Assert(nextLinkUri != null, "nextLinkUri != null");
            Debug.Assert(plan != null, "plan != null");

            this.nextLinkUri = nextLinkUri;
            this.plan = plan;
        }

        #endregion Contructors.

        #region Properties.

        /// <summary>The URI to the next page of data.</summary>
        public Uri NextLinkUri
        {
            get { return this.nextLinkUri; }
        }

        /// <summary>Type of element to be paged over.</summary>
        internal abstract Type ElementType
        {
            get;
        }

        /// <summary>Projection plan for the next page of data; null if not available.</summary>
        internal ProjectionPlan Plan
        {
            get { return this.plan; }
        }

        #endregion Properties.

        #region Methods.

        /// <summary>Provides a string representation of this continuation.</summary>
        /// <returns>String representation.</returns>
        public override string ToString()
        {
            return this.NextLinkUri.ToString();
        }

        /// <summary>Creates a new <see cref="DataServiceQueryContinuation"/> instance.</summary>
        /// <param name="nextLinkUri">Link to next page of data (possibly null).</param>
        /// <param name="plan">Plan to materialize the data (only null if nextLinkUri is null).</param>
        /// <returns>A new continuation object; null if nextLinkUri is null.</returns>
        internal static DataServiceQueryContinuation Create(Uri nextLinkUri, ProjectionPlan plan)
        {
            Debug.Assert(plan != null || nextLinkUri == null, "plan != null || nextLinkUri == null");

            if (nextLinkUri == null)
            {
                return null;
            }

            var constructors = typeof(DataServiceQueryContinuation<>).MakeGenericType(plan.ProjectedType).GetConstructors(BindingFlags.NonPublic | BindingFlags.Instance);
            Debug.Assert(constructors.Length == 1, "constructors.Length == 1");
            object result = Util.ConstructorInvoke(constructors[0], new object[] { nextLinkUri, plan });
            return (DataServiceQueryContinuation)result;
        }

        /// <summary>
        /// Initializes a new <see cref="QueryComponents"/> instance that can 
        /// be used for this continuation.
        /// </summary>
        /// <returns>A new initializes <see cref="QueryComponents"/>.</returns>
        internal QueryComponents CreateQueryComponents()
        {
            QueryComponents result = new QueryComponents(this.NextLinkUri, Util.DataServiceVersionEmpty, this.Plan.LastSegmentType, null, null);
            return result;
        }

        #endregion Methods.
    }

    /// <summary>Use this class to represent the continuation of a query.</summary>
    /// <typeparam name="T">Element type.</typeparam>
    public sealed class DataServiceQueryContinuation<T> : DataServiceQueryContinuation
    {
        #region Contructors.

        /// <summary>Initializes a new typed instance.</summary>
        /// <param name="nextLinkUri">URI to next page of data.</param>
        /// <param name="plan">Projection plan for results of next page.</param>
        internal DataServiceQueryContinuation(Uri nextLinkUri, ProjectionPlan plan)
            : base(nextLinkUri, plan)
        {
        }

        #endregion Contructors.

        #region Properties.

        /// <summary>Type of element to be paged over.</summary>
        internal override Type ElementType
        {
            get { return typeof(T); }
        }

        #endregion Properties.
    }
}
