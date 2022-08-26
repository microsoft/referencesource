//---------------------------------------------------------------------
// <copyright file="ProjectionQueryOptionExpression.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Respresents a projection query option in resource bound expression tree.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq.Expressions;

    #endregion Namespaces.

    /// <summary>
    /// An resource specific expression representing a projection query option.
    /// </summary>
    internal class ProjectionQueryOptionExpression : QueryOptionExpression
    {
        #region Private fields.

        /// <summary>projection expression to evaluate on client on results from server to materialize type</summary>
        private readonly LambdaExpression lambda;

        /// <summary>projection paths to send to the server</summary>
        private readonly List<string> paths;

        #endregion Private fields.

        /// <summary>
        /// Creates a ProjectionQueryOption expression
        /// </summary>
        /// <param name="type">the return type of the expression</param>
        /// <param name="lambda">projection expression</param>
        /// <param name="paths">Projection paths for the query option</param>
        internal ProjectionQueryOptionExpression(Type type, LambdaExpression lambda, List<string> paths)
            : base((ExpressionType)ResourceExpressionType.ProjectionQueryOption, type)
        {
            Debug.Assert(type != null, "type != null");
            Debug.Assert(lambda != null, "lambda != null");
            Debug.Assert(paths != null, "paths != null");

            this.lambda = lambda;
            this.paths = paths;
        }

        #region Internal properties.

        /// <summary>
        /// expression for the projection
        /// </summary>
        internal LambdaExpression Selector
        {
            get
            {
                return this.lambda;
            }
        }

        /// <summary>
        /// expression for the projection
        /// </summary>
        internal List<string> Paths
        {
            get
            {
                return this.paths;
            }
        }

        #endregion Internal properties.
    }
}
