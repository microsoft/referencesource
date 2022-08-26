//---------------------------------------------------------------------
// <copyright file="QueryComponents.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      The result from a expression to query components translation
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Linq.Expressions;

    #endregion Namespaces.

    /// <summary>Represents the components of query.</summary>
    internal class QueryComponents
    {
        #region Private fields.

        /// <summary> URI for query </summary>
        private readonly Uri uri;

        /// <summary> type </summary>
        private readonly Type lastSegmentType;

        /// <summary>Records the generated-to-source rewrites created.</summary>
        private readonly Dictionary<Expression, Expression> normalizerRewrites;

        /// <summary>selector Lambda Expression</summary>
        private readonly LambdaExpression projection;

        /// <summary> Version for query </summary>
        private Version version;

        #endregion Private fields.

        /// <summary>
        ///  Constructs a container for query components
        /// </summary>
        /// <param name="uri">URI for the query</param>
        /// <param name="version">Version for the query</param>
        /// <param name="lastSegmentType">Element type for the query</param>
        /// <param name="projection">selector Lambda Expression</param>
        /// <param name="normalizerRewrites">Records the generated-to-source rewrites created (possibly null).</param>
        internal QueryComponents(Uri uri, Version version, Type lastSegmentType, LambdaExpression projection, Dictionary<Expression, Expression> normalizerRewrites)
        {
            this.projection = projection;
            this.normalizerRewrites = normalizerRewrites;
            this.lastSegmentType = lastSegmentType;
            this.uri = uri;
            this.version = version;
        }

        #region Internal properties.

        /// <summary>The translated uri for a query</summary>
        internal Uri Uri
        {
            get
            {
                return this.uri;
            }
        }

        /// <summary>Records the generated-to-source rewrites created.</summary>
        internal Dictionary<Expression, Expression> NormalizerRewrites
        {
            get 
            { 
                return this.normalizerRewrites; 
            }
        }

        /// <summary>The projection expression for a query</summary>
        internal LambdaExpression Projection
        {
            get
            {
                return this.projection;
            }
        }

        /// <summary>The last segment type for query</summary>
        internal Type LastSegmentType
        {
            get
            {
                return this.lastSegmentType;
            }
        }

        /// <summary>The data service version associated with the uri</summary>
        internal Version Version
        {
            get
            {
                return this.version;
            }

#if !ASTORIA_LIGHT // Synchronous methods not available
            set
            {
                this.version = value;
            }
#endif
        }

        #endregion Internal properties.
    }
}
