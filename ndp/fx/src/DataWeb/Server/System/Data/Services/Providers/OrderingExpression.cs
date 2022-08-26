//---------------------------------------------------------------------
// <copyright file="OrderingExpression.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Describes a single ordering expression with sort order for 
//     $expands for a WCF Data Service.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    #region Namespaces
    using System.Linq.Expressions;
    #endregion

    /// <summary>
    /// Describes a single ordering expression along with sort order
    /// </summary>
    internal sealed class OrderingExpression
    {
        /// <summary>Ordering expression</summary>
        private readonly Expression orderingExpression;

        /// <summary>Order is ascending or descending</summary>
        private readonly bool isAscending;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="orderingExpression">Expression for ordering</param>
        /// <param name="isAscending">Order by ascending or descending</param>
        public OrderingExpression(Expression orderingExpression, bool isAscending)
        {
            this.orderingExpression = orderingExpression;
            this.isAscending = isAscending;
        }

        /// <summary>Ordering expression</summary>
        public Expression Expression
        {
            get
            {
                return this.orderingExpression;
            }
        }

        /// <summary>Ascending or descending</summary>
        public bool IsAscending
        {
            get
            {
                return this.isAscending;
            }
        }
    }
}
