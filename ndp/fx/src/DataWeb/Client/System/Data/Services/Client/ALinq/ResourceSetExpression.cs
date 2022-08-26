//---------------------------------------------------------------------
// <copyright file="ResourceSetExpression.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Respresents a resource set in resource bound expression tree.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------
namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Reflection;

    #endregion Namespaces.

    /// <summary>ResourceSet Expression</summary>
    [DebuggerDisplay("ResourceSetExpression {Source}.{MemberExpression}")]
    internal class ResourceSetExpression : ResourceExpression
    {
        #region Private fields.

        /// <summary>
        /// The (static) type of the resources in this resource set.
        /// The resource type can differ from this.Type if this expression represents a transparent scope.
        /// For example, in TransparentScope{Category, Product}, the true element type is Product.
        /// </summary>
        private readonly Type resourceType;

        /// <summary>property member name</summary>
        private readonly Expression member;

        /// <summary>key predicate</summary>
        private Dictionary<PropertyInfo, ConstantExpression> keyFilter;

        /// <summary>sequence query options</summary>
        private List<QueryOptionExpression> sequenceQueryOptions;

        /// <summary>enclosing transparent scope</summary>
        private TransparentAccessors transparentScope;

        #endregion Private fields.

        /// <summary>
        /// Creates a ResourceSet expression
        /// </summary>
        /// <param name="type">the return type of the expression</param>
        /// <param name="source">the source expression</param>
        /// <param name="memberExpression">property member name</param>
        /// <param name="resourceType">the element type of the resource set</param>
        /// <param name="expandPaths">expand paths for resource set</param>
        /// <param name="countOption">count query option for the resource set</param>
        /// <param name="customQueryOptions">custom query options for resourcse set</param>
        internal ResourceSetExpression(Type type, Expression source, Expression memberExpression, Type resourceType, List<string> expandPaths, CountOption countOption, Dictionary<ConstantExpression, ConstantExpression> customQueryOptions, ProjectionQueryOptionExpression projection)
            : base(source, source != null ? (ExpressionType)ResourceExpressionType.ResourceNavigationProperty : (ExpressionType)ResourceExpressionType.RootResourceSet, type, expandPaths, countOption, customQueryOptions, projection)
        {
            Debug.Assert(type != null, "type != null");
            Debug.Assert(memberExpression != null, "memberExpression != null");
            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(
                (source == null && memberExpression is ConstantExpression) ||
                (source != null && memberExpression is MemberExpression),
                "source is null with constant entity set name, or not null with member expression");

            this.member = memberExpression;
            this.resourceType = resourceType;
            this.sequenceQueryOptions = new List<QueryOptionExpression>();
        }

        #region Internal properties.

        /// <summary>
        /// Member for ResourceSet 
        /// </summary>
        internal Expression MemberExpression
        {
            get { return this.member; }
        }

        /// <summary>
        /// Type of resources contained in this ResourceSet - it's element type.
        /// </summary>
        internal override Type ResourceType
        {
            get { return this.resourceType; }
        }

        /// <summary>
        /// Is this ResourceSet enclosed in an anonymously-typed transparent scope produced by a SelectMany operation? 
        /// Applies to navigation ResourceSets.
        /// </summary>
        internal bool HasTransparentScope
        {
            get { return this.transparentScope != null; } 
        }

        /// <summary>
        /// The property accesses required to reference this ResourceSet and its source ResourceSet if a transparent scope is present.
        /// May be null. Use <see cref="HasTransparentScope"/> to test for the presence of a value.
        /// </summary>
        internal TransparentAccessors TransparentScope
        {
            get { return this.transparentScope;  }
            set { this.transparentScope = value; }
        }

        /// <summary>
        /// Has a key predicate restriction been applied to this ResourceSet?
        /// </summary>
        internal bool HasKeyPredicate
        {
            get { return this.keyFilter != null; }
        }

        /// <summary>
        /// The property name/required value pairs that comprise the key predicate (if any) applied to this ResourceSet.
        /// May be null. Use <see cref="HasKeyPredicate"/> to test for the presence of a value.
        /// </summary>
        internal Dictionary<PropertyInfo, ConstantExpression> KeyPredicate
        {
            get { return this.keyFilter; }
            set { this.keyFilter = value; }
        }

        /// <summary>
        /// A resource set produces at most 1 result if constrained by a key predicate
        /// </summary>
        internal override bool IsSingleton
        {
            get { return this.HasKeyPredicate; }
        }

        /// <summary>
        /// Have sequence query options (filter, orderby, skip, take), expand paths, projection
        /// or custom query options been applied to this resource set?
        /// </summary>
        internal override bool HasQueryOptions
        {
	        get 
            { 
                return this.sequenceQueryOptions.Count > 0 ||
                    this.ExpandPaths.Count > 0 ||
                    this.CountOption == CountOption.InlineAll ||        // value only count is not an option
                    this.CustomQueryOptions.Count > 0 ||
                    this.Projection != null;
            }
        }

        /// <summary>
        /// Filter query option for ResourceSet
        /// </summary>
        internal FilterQueryOptionExpression Filter
        {
            get
            {
                return this.sequenceQueryOptions.OfType<FilterQueryOptionExpression>().SingleOrDefault();
            }
        }

        /// <summary>
        /// OrderBy query option for ResourceSet
        /// </summary>
        internal OrderByQueryOptionExpression OrderBy
        {
            get { return this.sequenceQueryOptions.OfType<OrderByQueryOptionExpression>().SingleOrDefault(); }
        }

        /// <summary>
        /// Skip query option for ResourceSet
        /// </summary>
        internal SkipQueryOptionExpression Skip
        {
            get { return this.sequenceQueryOptions.OfType<SkipQueryOptionExpression>().SingleOrDefault(); }
        }

        /// <summary>
        /// Take query option for ResourceSet
        /// </summary>
        internal TakeQueryOptionExpression Take
        {
            get { return this.sequenceQueryOptions.OfType<TakeQueryOptionExpression>().SingleOrDefault(); }
        }

        /// <summary>
        /// Gets sequence query options for ResourcSet
        /// </summary>
        internal IEnumerable<QueryOptionExpression> SequenceQueryOptions
        {
            get { return this.sequenceQueryOptions.ToList(); }
        }

        /// <summary>Whether there are any query options for the sequence.</summary>
        internal bool HasSequenceQueryOptions
        {
            get { return this.sequenceQueryOptions.Count > 0; }
        }

        #endregion Internal properties.

        #region Internal methods.

        /// <summary>
        /// Cast ResourceSetExpression to new type
        /// </summary>
        internal override ResourceExpression CreateCloneWithNewType(Type type)
        {
            ResourceSetExpression rse = new ResourceSetExpression(
                type, 
                this.source, 
                this.MemberExpression, 
                TypeSystem.GetElementType(type),
                this.ExpandPaths.ToList(),
                this.CountOption,
                this.CustomQueryOptions.ToDictionary(kvp => kvp.Key, kvp => kvp.Value),
                this.Projection);
            rse.keyFilter = this.keyFilter;
            rse.sequenceQueryOptions = this.sequenceQueryOptions;
            rse.transparentScope = this.transparentScope;
            return rse;
        }

        /// <summary>
        /// Add query option to resource expression
        /// </summary>
        internal void AddSequenceQueryOption(QueryOptionExpression qoe)
        {
            Debug.Assert(qoe != null, "qoe != null");
            QueryOptionExpression old = this.sequenceQueryOptions.Where(o => o.GetType() == qoe.GetType()).FirstOrDefault();
            if (old != null)
            {
                qoe = qoe.ComposeMultipleSpecification(old);
                this.sequenceQueryOptions.Remove(old);
            }

            this.sequenceQueryOptions.Add(qoe);
        }

        /// <summary>
        /// Instructs this resource set expression to use the input reference expression from <paramref name="newInput"/> as it's
        /// own input reference, and to retarget the input reference from <paramref name="newInput"/> to this resource set expression.
        /// </summary>
        /// <param name="newInput">The resource set expression from which to take the input reference.</param>
        /// <remarks>Used exclusively by <see cref="ResourceBinder.RemoveTransparentScope"/>.</remarks>
        internal void OverrideInputReference(ResourceSetExpression newInput)
        {
            Debug.Assert(newInput != null, "Original resource set cannot be null");
            Debug.Assert(this.inputRef == null, "OverrideInputReference cannot be called if the target has already been referenced");

            InputReferenceExpression inputRef = newInput.inputRef;
            if (inputRef != null)
            {
                this.inputRef = inputRef;
                inputRef.OverrideTarget(this);
            }
        }

        #endregion Internal methods.

        /// <summary>
        /// Represents the property accesses required to access both 
        /// this resource set and its source resource/set (for navigations).
        /// 
        /// These accesses are required to reference resource sets enclosed
        /// in transparent scopes introduced by use of SelectMany. 
        /// </summary>
        /// <remarks>
        /// For example, this query:
        ///  from c in Custs where c.id == 1 
        ///  from o in c.Orders from od in o.OrderDetails select od
        ///  
        /// Translates to:
        ///  c.Where(c => c.id == 1)
        ///   .SelectMany(c => o, (c, o) => new $(c=c, o=o))
        ///   .SelectMany($ => $.o, ($, od) => od)
        /// 
        /// PatternRules.MatchPropertyProjectionSet identifies Orders as the target of the collector.
        /// PatternRules.MatchTransparentScopeSelector identifies the introduction of a transparent identifer.
        /// 
        /// A transparent accessor is associated with Orders, with 'c' being the source accesor, 
        /// and 'o' being the (introduced) accessor.
        /// </remarks>
        [DebuggerDisplay("{ToString()}")]
        internal class TransparentAccessors
        {
            #region Internal fields.

            /// <summary>
            /// The property reference that must be applied to reference this resource set
            /// </summary>
            internal readonly string Accessor;

            /// <summary>
            /// The property reference that must be applied to reference the source resource set.
            /// Note that this set's Accessor is NOT required to access the source set, but the
            /// source set MAY impose it's own Transparent Accessors
            /// </summary>
            internal readonly Dictionary<string, Expression> SourceAccessors;

            #endregion Internal fields.

            /// <summary>
            /// Constructs a new transparent scope with the specified set and source set accessors
            /// </summary>
            /// <param name="acc">The name of the property required to access the resource set</param>
            /// <param name="sourceAccesors">The names of the property required to access the resource set's sources.</param>
            internal TransparentAccessors(string acc, Dictionary<string, Expression> sourceAccesors)
            {
                Debug.Assert(!string.IsNullOrEmpty(acc), "Set accessor cannot be null or empty");
                Debug.Assert(sourceAccesors != null, "sourceAccesors != null");

                this.Accessor = acc;
                this.SourceAccessors = sourceAccesors;
            }

            /// <summary>Provides a string representation of this accessor.</summary>
            /// <returns>The text represntation of this accessor.</returns>
            public override string ToString()
            {
                string result = "SourceAccessors=[" + string.Join(",", this.SourceAccessors.Keys.ToArray());
                result += "] ->* Accessor=" + this.Accessor;
                return result;
            }
        }
    }
}
