//---------------------------------------------------------------------
// <copyright file="OrderingInfo.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Describes ordering information for each expanded entity set
//      for $expands request for a WCF Data Service.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    #region Namespaces
    using System.Collections.Generic;
    using System.Collections.ObjectModel;

    #endregion

    /// <summary>
    /// Describes ordering information for each entity set
    /// for $expand request for a WCF Data Service.
    /// </summary>
    internal sealed class OrderingInfo
    {
        /// <summary>Is the expanded entity set paged</summary>
        private readonly bool paged;

        /// <summary>Collection of ordering expressions</summary>
        private readonly List<OrderingExpression> orderingExpressions;

        /// <summary>Constructor</summary>
        /// <param name="paged">Whether top level entity set is paged</param>
        internal OrderingInfo(bool paged)
        {
            this.paged = paged;
            this.orderingExpressions = new List<OrderingExpression>();
        }

        /// <summary>Is the expaded entity set paged</summary>
        public bool IsPaged
        {
            get
            {
                return this.paged;
            }
        }

        /// <summary>Gives the collection of ordering expressions for a request</summary>
        public ReadOnlyCollection<OrderingExpression> OrderingExpressions
        {
            get
            {
                return this.orderingExpressions.AsReadOnly();
            }
        }

        /// <summary>Adds a single OrderingExpression to the collection</summary>
        /// <param name="orderingExpression">Ordering expression to add</param>
        internal void Add(OrderingExpression orderingExpression)
        {
            this.orderingExpressions.Add(orderingExpression);
        }
    }
}
