//---------------------------------------------------------------------
// <copyright file="NavigationPropertySingletonExpression.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Respresents a navigation to a singleton property.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------
namespace System.Data.Services.Client
{
    #region Private fields.

    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;

    #endregion Private fields.

    /// <summary>Expression for a navigation property into a single entity (eg: Customer.BestFriend).</summary>
    internal class NavigationPropertySingletonExpression : ResourceExpression
    {
        #region Private fields.

        /// <summary>property member name</summary>
        private readonly Expression memberExpression;

        /// <summary> resource type</summary>
        private readonly Type resourceType;

        #endregion Private fields.

        /// <summary>
        /// Creates a NavigationPropertySingletonExpression expression
        /// </summary>
        /// <param name="type">the return type of the expression</param>
        /// <param name="source">the source expression</param>
        /// <param name="memberExpression">property member name</param>
        /// <param name="resourceType">resource type for expression</param>
        /// <param name="expandPaths">expand paths for resource set</param>
        /// <param name="countOption">count option for the resource set</param>
        /// <param name="customQueryOptions">custom query options for resourcse set</param>
        /// <param name="projection">projection expression</param>
        internal NavigationPropertySingletonExpression(Type type, Expression source, Expression memberExpression, Type resourceType, List<string> expandPaths, CountOption countOption, Dictionary<ConstantExpression, ConstantExpression> customQueryOptions, ProjectionQueryOptionExpression projection)
            : base(source, (ExpressionType)ResourceExpressionType.ResourceNavigationPropertySingleton, type, expandPaths, countOption, customQueryOptions, projection)
        {
            Debug.Assert(memberExpression != null, "memberExpression != null");
            Debug.Assert(resourceType != null, "resourceType != null");

            this.memberExpression = memberExpression;
            this.resourceType = resourceType;
        }

        /// <summary>
        /// Gets the member expression.
        /// </summary>
        internal MemberExpression MemberExpression
        {
            get
            {
                return (MemberExpression)this.memberExpression;
            }
        }

        /// <summary>
        /// The resource type of the singe instance produced by this singleton navigation.
        /// </summary>
        internal override Type ResourceType
        {
            get { return this.resourceType; }
        }

        /// <summary>
        /// Singleton navigation properties always produce at most 1 result
        /// </summary>
        internal override bool IsSingleton
        {
            get { return true; }
        }

        /// <summary>
        /// Does Singleton navigation have query options.
        /// </summary>
        internal override bool HasQueryOptions
        {
            get
            {
                return this.ExpandPaths.Count > 0 ||
                    this.CountOption == CountOption.InlineAll || 
                    this.CustomQueryOptions.Count > 0  || 
                    this.Projection != null;
            }
        }

        /// <summary>
        /// Cast changes the type of the ResourceExpression
        /// </summary>
        /// <param name="type">new type</param>
        /// <returns>new NavigationPropertySingletonExpression</returns>
        internal override ResourceExpression CreateCloneWithNewType(Type type)
        {
            return new NavigationPropertySingletonExpression(
                type, 
                this.source, 
                this.MemberExpression,
                TypeSystem.GetElementType(type),
                this.ExpandPaths.ToList(),
                this.CountOption,
                this.CustomQueryOptions.ToDictionary(kvp => kvp.Key, kvp => kvp.Value),
                this.Projection);
        }
    }
}
