//---------------------------------------------------------------------
// <copyright file="DataServiceRequestOfT.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// typed request object
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Diagnostics;

    #endregion Namespaces.

    /// <summary>
    /// Holds a Uri and type for the request.
    /// </summary>
    /// <typeparam name="TElement">The type to construct for the request results</typeparam>
    public sealed class DataServiceRequest<TElement> : DataServiceRequest
    {
        #region Private fields.

        /// <summary>The UriTranslateResult for the request</summary>
        private readonly QueryComponents queryComponents;

        /// <summary>The ProjectionPlan for the request (if precompiled in a previous page).</summary>
        private readonly ProjectionPlan plan;

        #endregion Private fields.

        #region Constructors.

        /// <summary>Create a request for a specific Uri</summary>
        /// <param name="requestUri">The URI for the request.</param>
        public DataServiceRequest(Uri requestUri)
        {
            Util.CheckArgumentNull(requestUri, "requestUri");
            Type elementType = typeof(TElement);
            elementType = ClientConvert.IsKnownType(elementType) ? elementType : TypeSystem.GetElementType(elementType);
            this.queryComponents = new QueryComponents(requestUri, Util.DataServiceVersionEmpty, elementType, null, null);
        }

        /// <summary>Create a request for a specific Uri</summary>
        /// <param name="queryComponents">The query components for the request</param>
        /// <param name="plan">Projection plan to reuse (possibly null).</param>
        internal DataServiceRequest(QueryComponents queryComponents, ProjectionPlan plan)
        {
            Debug.Assert(queryComponents != null, "queryComponents != null");
            
            this.queryComponents = queryComponents;
            this.plan = plan;
        }

        #endregion Constructors.

        /// <summary>Element Type</summary>
        public override Type ElementType
        {
            get { return typeof(TElement); }
        }

        /// <summary>The URI for the request.</summary>
        public override Uri RequestUri
        {
            get { return this.queryComponents.Uri; }
        }

        /// <summary>The ProjectionPlan for the request, if precompiled in a previous page; null otherwise.</summary>
        internal override ProjectionPlan Plan
        {
            get
            {
                return this.plan;
            }
        }

        /// <summary>The TranslateResult associated with this request</summary>
        internal override QueryComponents QueryComponents
        {
            get
            {
                return this.queryComponents;
            }
        }
    }
}
