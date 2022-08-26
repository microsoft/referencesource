//---------------------------------------------------------------------
// <copyright file="ParameterReplacerVisitor.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides an expression visitor that can replace a ParameterExpression.
// </summary>
//
// @owner  Microsoft, Microsoft
//---------------------------------------------------------------------
namespace System.Data.Services.Client
{
    using System.Diagnostics;
    using System.Linq.Expressions;

    /// <summary>Provides an expression visitor that can replace a <see cref="ParameterExpression"/>.</summary>
    internal class ParameterReplacerVisitor : ALinqExpressionVisitor
    {
        /// <summary>Expression to replace with.</summary>
        private Expression newExpression;

        /// <summary>Parameter to replace.</summary>
        private ParameterExpression oldParameter;

        /// <summary>Initializes a new <see cref="ParameterReplacerVisitor"/> instance.</summary>
        /// <param name="oldParameter">Parameter to replace.</param>
        /// <param name="newExpression">Expression to replace with.</param>
        private ParameterReplacerVisitor(ParameterExpression oldParameter, Expression newExpression)
        {
            this.oldParameter = oldParameter;
            this.newExpression = newExpression;
        }

        /// <summary>
        /// Replaces the occurences of <paramref name="oldParameter"/> for <paramref name="newExpression"/> in
        /// <paramref name="expression"/>.
        /// </summary>
        /// <param name="expression">Expression to perform replacement on.</param>
        /// <param name="oldParameter">Parameter to replace.</param>
        /// <param name="newExpression">Expression to replace with.</param>
        /// <returns>A new expression with the replacement performed.</returns>
        internal static Expression Replace(Expression expression, ParameterExpression oldParameter, Expression newExpression)
        {
            Debug.Assert(expression != null, "expression != null");
            Debug.Assert(oldParameter != null, "oldParameter != null");
            Debug.Assert(newExpression != null, "newExpression != null");
            return new ParameterReplacerVisitor(oldParameter, newExpression).Visit(expression);
        }

        /// <summary>ParameterExpression visit method.</summary>
        /// <param name="p">The ParameterExpression expression to visit</param>
        /// <returns>The visited ParameterExpression expression </returns>
        internal override Expression VisitParameter(ParameterExpression p)
        {
            if (p == this.oldParameter)
            {
                return this.newExpression;
            }
            else
            {
                return p;
            }
        }
    }
}
