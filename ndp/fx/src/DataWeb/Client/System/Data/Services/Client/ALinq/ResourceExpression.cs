//---------------------------------------------------------------------
// <copyright file='ResourceExpression.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Base class for expressions representing resources
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Linq.Expressions;

    #endregion Namespaces.

    /// <summary>
    /// The counting option for the resource expression
    /// </summary>
    internal enum CountOption
    {
        /// <summary>No counting</summary>
        None,

        /// <summary>Translates to the $count segment.</summary>
        ValueOnly,

        /// <summary>Translates to the $inlinecount=allpages query option</summary>
        InlineAll
    }

    /// <summary>
    /// Abstract base class for expressions that support Query Options
    /// </summary>
    internal abstract class ResourceExpression : Expression
    {
        #region Fields.

        /// <summary>Source expression.</summary>
        protected readonly Expression source;

        /// <summary>Singleton InputReferenceExpression that should be used to indicate a reference to this element of the resource path</summary>
        protected InputReferenceExpression inputRef;

        /// <summary>expand paths</summary>
        private List<string> expandPaths;

        /// <summary>The count query option for the resource set</summary>
        private CountOption countOption;

        /// <summary>custom query options</summary>
        private Dictionary<ConstantExpression, ConstantExpression> customQueryOptions;

        private ProjectionQueryOptionExpression projection;

        #endregion Fields.

        /// <summary>
        /// Creates a Resource expression
        /// </summary>
        /// <param name="type">the return type of the expression</param>
        /// <param name="customQueryOptions">The custom query options</param>
        /// <param name="expandPaths">the expand paths</param>
        /// <param name="nodeType">the node type</param>
        /// <param name="countOption">the count option</param>
#pragma warning disable 618
        internal ResourceExpression(Expression source, ExpressionType nodeType, Type type, List<string> expandPaths, CountOption countOption, Dictionary<ConstantExpression, ConstantExpression> customQueryOptions, ProjectionQueryOptionExpression projection)
            : base(nodeType, type)
        {
            this.expandPaths = expandPaths ?? new List<string>();
            this.countOption = countOption;
            this.customQueryOptions = customQueryOptions ?? new Dictionary<ConstantExpression, ConstantExpression>(ReferenceEqualityComparer<ConstantExpression>.Instance);
            this.projection = projection;
            this.source = source;
        }
#pragma warning restore 618

        abstract internal ResourceExpression CreateCloneWithNewType(Type type);

        abstract internal bool HasQueryOptions { get; }

        /// <summary>
        /// Resource type for this expression (for sets, this is the element type).
        /// Never null.
        /// </summary>
        internal abstract Type ResourceType { get; }

        /// <summary>
        /// Does this expression produce at most 1 resource?
        /// </summary>
        abstract internal bool IsSingleton { get; }

        /// <summary>
        /// Expand query option for ResourceSet
        /// </summary>
        internal virtual List<string> ExpandPaths
        {
            get { return this.expandPaths; }
            set { this.expandPaths = value; }
        }

        /// <summary>
        /// Count query option for ResourceSet
        /// </summary>
        internal virtual CountOption CountOption
        {
            get { return this.countOption; }
            set { this.countOption = value; }
        }

        /// <summary>
        /// custom query options for ResourceSet
        /// </summary>
        internal virtual Dictionary<ConstantExpression, ConstantExpression> CustomQueryOptions
        {
            get { return this.customQueryOptions; }
            set { this.customQueryOptions = value; }
        }

        /// <summary>Description of the projection on a resource.</summary>
        /// <remarks>
        /// This property is set by the ProjectionAnalyzer component (so it
        /// mutates this instance), or by the ResourceBinder when it clones
        /// a ResourceExpression.
        /// </remarks>
        internal ProjectionQueryOptionExpression Projection
        {
            get { return this.projection; }
            set { this.projection = value; }
        }

        /// <summary>
        /// Gets the source expression.
        /// </summary>
        internal Expression Source
        {
            get
            {
                return this.source;
            }
        }

        /// <summary>
        /// Creates an <see cref="InputReferenceExpression"/> that refers to this component of the resource path. 
        /// The returned expression is guaranteed to be reference-equal (object.ReferenceEquals)
        /// to any other InputReferenceExpression that also refers to this resource path component.
        /// </summary>
        /// <returns>The InputReferenceExpression that refers to this resource path component</returns>
        internal InputReferenceExpression CreateReference()
        {
            if (this.inputRef == null)
            {
                this.inputRef = new InputReferenceExpression(this);
            }

            return this.inputRef;
        }
    }
}
