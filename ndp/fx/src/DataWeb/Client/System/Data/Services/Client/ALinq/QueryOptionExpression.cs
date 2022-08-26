//---------------------------------------------------------------------
// <copyright file="QueryOptionExpression.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Respresents a query option in resource bound expression tree.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------
namespace System.Data.Services.Client
{
    using System;
    using System.Diagnostics;
    using System.Linq.Expressions;

    /// <summary>
    /// An resource specific expression representing a query option.
    /// </summary>
    internal abstract class QueryOptionExpression : Expression
    {
        /// <summary>
        /// Creates a QueryOptionExpression expression
        /// </summary>
        /// <param name="nodeType">expression node type</param>
        /// <param name="type">the return type of the expression</param>
#pragma warning disable 618
        internal QueryOptionExpression(ExpressionType nodeType, Type type) : base(nodeType, type)
        {
        }
#pragma warning restore 618

        /// <summary>
        /// Composes the <paramref name="previous"/> expression with this one when it's specified multiple times.
        /// </summary>
        /// <param name="previous"><see cref="QueryOptionExpression"/> to compose.</param>
        /// <returns>
        /// The expression that results from composing the <paramref name="previous"/> expression with this one.
        /// </returns>
        internal virtual QueryOptionExpression ComposeMultipleSpecification(QueryOptionExpression previous)
        {
            Debug.Assert(previous != null, "other != null");
            Debug.Assert(previous.GetType() == this.GetType(), "other.GetType == this.GetType() -- otherwise it's not the same specification");
            return this;
        }
    }
}
