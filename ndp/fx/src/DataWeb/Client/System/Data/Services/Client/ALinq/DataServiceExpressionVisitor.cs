//---------------------------------------------------------------------
// <copyright file="DataServiceExpressionVisitor.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Expression Visitors for Linq to URI translator
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------
namespace System.Data.Services.Client
{
    #region Namespaces.

    using System.Diagnostics;
    using System.Linq.Expressions;

    #endregion Namespaces.

    /// <summary>
    /// Specific Vistior base class for the DataServiceQueryProvider.  
    /// </summary>
    internal abstract class DataServiceALinqExpressionVisitor : ALinqExpressionVisitor
    {
        /// <summary>
        /// Main visit method.
        /// </summary>
        /// <param name="exp">Expression to visit</param>
        /// <returns>Visited expression</returns>
        internal override Expression Visit(Expression exp)
        {
            if (exp == null)
            {
                return null;
            }

            switch ((ResourceExpressionType)exp.NodeType)
            {
                case ResourceExpressionType.RootResourceSet:
                case ResourceExpressionType.ResourceNavigationProperty:
                    return this.VisitResourceSetExpression((ResourceSetExpression)exp);
                case ResourceExpressionType.ResourceNavigationPropertySingleton:
                    return this.VisitNavigationPropertySingletonExpression((NavigationPropertySingletonExpression)exp);
                case ResourceExpressionType.InputReference:
                    return this.VisitInputReferenceExpression((InputReferenceExpression)exp);
                default:
                    return base.Visit(exp);
            }
        }

        /// <summary>
        /// ResourceSetExpression visit method.
        /// </summary>
        /// <param name="rse">ResourceSetExpression expression to visit</param>
        /// <returns>Visited ResourceSetExpression expression</returns>
        internal virtual Expression VisitResourceSetExpression(ResourceSetExpression rse)
        {
            Expression source = this.Visit(rse.Source);

            if (source != rse.Source)
            {
                rse = new ResourceSetExpression(rse.Type, source, rse.MemberExpression, rse.ResourceType, rse.ExpandPaths, rse.CountOption, rse.CustomQueryOptions, rse.Projection);
            }

            return rse;
        }

        /// <summary>
        /// NavigationPropertySingletonExpressionvisit method.
        /// </summary>
        /// <param name="npse">NavigationPropertySingletonExpression expression to visit</param>
        /// <returns>Visited NavigationPropertySingletonExpression expression</returns>
        internal virtual Expression VisitNavigationPropertySingletonExpression(NavigationPropertySingletonExpression npse)
        {
            Expression source = this.Visit(npse.Source);

            if (source != npse.Source)
            {
                npse = new NavigationPropertySingletonExpression(npse.Type, source, npse.MemberExpression, npse.MemberExpression.Type, npse.ExpandPaths, npse.CountOption, npse.CustomQueryOptions, npse.Projection);
            }

            return npse;
        }

        /// <summary>
        /// Visit an <see cref="InputReferenceExpression"/>, producing a new InputReferenceExpression
        /// based on the visited form of the <see cref="ResourceSetExpression"/> that is referenced by
        /// the InputReferenceExpression argument, <paramref name="ire"/>.
        /// </summary>
        /// <param name="ire">InputReferenceExpression expression to visit</param>
        /// <returns>Visited InputReferenceExpression expression</returns>
        internal virtual Expression VisitInputReferenceExpression(InputReferenceExpression ire)
        {
            Debug.Assert(ire != null, "ire != null -- otherwise caller never should have visited here");
            ResourceExpression re = (ResourceExpression)this.Visit(ire.Target);
            return re.CreateReference();
        }
    }
}
