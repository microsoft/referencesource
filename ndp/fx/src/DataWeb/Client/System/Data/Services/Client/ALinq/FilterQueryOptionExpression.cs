//---------------------------------------------------------------------
// <copyright file="FilterQueryOptionExpression.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Respresents a filter query option in resource bound expression tree.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;
    using System.Linq.Expressions;

    /// <summary>
    /// An resource specific expression representing a filter query option.
    /// </summary>
    internal class FilterQueryOptionExpression : QueryOptionExpression
    {
        /// <summary>predicate</summary>
        private Expression predicate;

        /// <summary>
        /// Creates a FilterQueryOptionExpression expression
        /// </summary>
        /// <param name="type">the return type of the expression</param>
        /// <param name="predicate">the predicate</param>
        internal FilterQueryOptionExpression(Type type, Expression predicate)
            : base((ExpressionType)ResourceExpressionType.FilterQueryOption, type)
        {
            this.predicate = predicate;
        }

        /// <summary>Gets the query option value.</summary>
        internal Expression Predicate
        {
            get
            {
                return this.predicate;
            }
        }
    }
}
