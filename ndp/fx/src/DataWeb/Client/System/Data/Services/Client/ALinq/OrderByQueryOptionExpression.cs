//---------------------------------------------------------------------
// <copyright file="OrderByQueryOptionExpression.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Respresents a OrderBy query option in resource bound expression tree.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------
namespace System.Data.Services.Client
{
    using System;
    using System.Collections.Generic;
    using System.Linq.Expressions;

    /// <summary>
    /// An resource specific expression representing a OrderBy query option.
    /// </summary>
    internal class OrderByQueryOptionExpression : QueryOptionExpression
    {
        /// <summary> selectors for OrderBy query option</summary>
        private List<Selector> selectors;

        /// <summary>
        /// Creates a OrderByQueryOptionExpression expression
        /// </summary>
        /// <param name="type">the return type of the expression</param>
        /// <param name="selectors">selectors for orderby expression</param>
        internal OrderByQueryOptionExpression(Type type, List<Selector> selectors)
            : base((ExpressionType)ResourceExpressionType.OrderByQueryOption, type)
        {
            this.selectors = selectors; 
        }

        /// <summary>
        /// Selectors for OrderBy expression
        /// </summary>
        internal List<Selector> Selectors
        {
            get
            {
                return this.selectors;
            }
        }

        /// <summary>
        /// Structure for selectors.  Holds lambda expression + flag indicating desc.
        /// </summary>
        internal struct Selector
        {
            /// <summary>
            /// lambda expression for selector
            /// </summary>
            internal readonly Expression Expression;

            /// <summary>
            /// flag indicating if descending
            /// </summary>
            internal readonly bool Descending;

            /// <summary>
            /// Creates a Selector
            /// </summary>
            /// <param name="e">lambda expression for selector</param>
            /// <param name="descending">flag indicating if descending</param>
            internal Selector(Expression e, bool descending)
            {
                this.Expression = e;
                this.Descending = descending;
            }
        }
    }
}
