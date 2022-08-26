//---------------------------------------------------------------------
// <copyright file="SkipQueryOptionExpression.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Respresents a skip query option in resource bound expression tree.
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
    /// An resource specific expression representing a skip query option.
    /// </summary>
    [DebuggerDisplay("SkipQueryOptionExpression {SkipAmount}")]
    internal class SkipQueryOptionExpression : QueryOptionExpression
    {
        /// <summary>amount to skip</summary>
        private ConstantExpression skipAmount;

        /// <summary>
        /// Creates a SkipQueryOption expression
        /// </summary>
        /// <param name="type">the return type of the expression</param>
        /// <param name="skipAmount">the query option value</param>
        internal SkipQueryOptionExpression(Type type, ConstantExpression skipAmount)
            : base((ExpressionType)ResourceExpressionType.SkipQueryOption, type)
        {
            this.skipAmount = skipAmount;
        }

        /// <summary>
        /// query option value
        /// </summary>
        internal ConstantExpression SkipAmount
        {
            get
            {
                return this.skipAmount;
            }
        }

        /// <summary>
        /// Composes the <paramref name="previous"/> expression with this one when it's specified multiple times.
        /// </summary>
        /// <param name="previous"><see cref="QueryOptionExpression"/> to compose.</param>
        /// <returns>
        /// The expression that results from composing the <paramref name="previous"/> expression with this one.
        /// </returns>
        internal override QueryOptionExpression ComposeMultipleSpecification(QueryOptionExpression previous)
        {
            Debug.Assert(previous != null, "other != null");
            Debug.Assert(previous.GetType() == this.GetType(), "other.GetType == this.GetType() -- otherwise it's not the same specification");
            Debug.Assert(this.skipAmount != null, "this.skipAmount != null");
            Debug.Assert(
                this.skipAmount.Type == typeof(int),
                "this.skipAmount.Type == typeof(int) -- otherwise it wouldn't have matched the Enumerable.Skip(source, int count) signature");
            int thisValue = (int)this.skipAmount.Value;
            int previousValue = (int)((SkipQueryOptionExpression)previous).skipAmount.Value;
            return new SkipQueryOptionExpression(this.Type, Expression.Constant(thisValue + previousValue, typeof(int)));
        }
    }
}
