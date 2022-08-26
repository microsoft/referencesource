//---------------------------------------------------------------------
// <copyright file="BasicExpandProvider.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a helper class to implement $expand functionality
//      with filters by rewriting queries and implementing custom
//      result enumerators.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Data.Services.Client;
    using System.Data.Services.Common;
    using System.Data.Services.Internal;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Reflection;
    using System.Text;

    #endregion Namespaces.

    /// <summary>
    /// Provides a helper class to implement $expand functionality
    /// with filters by rewriting queries and implementing custom
    /// result enumerators.
    /// </summary>
    internal class BasicExpandProvider : IProjectionProvider
    {
        #region Private fields.

        /// <summary>Queryable.OrderBy method info</summary>
        private static readonly MethodInfo OrderByMethodInfo = typeof(Queryable)
                                          .GetMethods()
                                          .First(m => m.Name == "OrderBy" && m.IsGenericMethod == true && m.GetParameters().Count() == 2);

        /// <summary>Queryable.ThenBy method info</summary>
        private static readonly MethodInfo ThenByMethodInfo = typeof(Queryable)
                                          .GetMethods()
                                          .First(m => m.Name == "ThenBy" && m.IsGenericMethod == true && m.GetParameters().Count() == 2);

        /// <summary>Queryable.OrderByDescending method info</summary>
        private static readonly MethodInfo OrderByDescendingMethodInfo = typeof(Queryable)
                                          .GetMethods()
                                          .First(m => m.Name == "OrderByDescending" && m.IsGenericMethod == true && m.GetParameters().Count() == 2);

        /// <summary>Queryable.ThenByDescending method info</summary>
        private static readonly MethodInfo ThenByDescendingMethodInfo = typeof(Queryable)
                                          .GetMethods()
                                          .First(m => m.Name == "ThenByDescending" && m.IsGenericMethod == true && m.GetParameters().Count() == 2);

        /// <summary>Queryable.Skip method info</summary>
        private static readonly MethodInfo SkipMethodInfo = typeof(Queryable)
                                          .GetMethods()
                                          .First(m => m.Name == "Skip" && m.IsGenericMethod == true && m.GetParameters().Count() == 2);

        /// <summary>Queryable.Take method info</summary>
        private static readonly MethodInfo TakeMethodInfo = typeof(Queryable)
                                          .GetMethods()
                                          .First(m => m.Name == "Take" && m.IsGenericMethod == true && m.GetParameters().Count() == 2);

        /// <summary>Whether all values are automatically expanded in the model.</summary>
        private readonly bool expanded;

        /// <summary>Whether the provider of the IQueryable requires usage
        /// casts to System.Object when assigning to properties of that type/
        /// For example Linq to Entities requires us not to cast here as it only supports
        /// casting of primitive types. On the other hand Linq to Objects requires casting
        /// since otherwise it would generate wrong IL.</summary>
        private readonly bool castToObject;

        /// <summary>Full provider.</summary>
        private readonly DataServiceProviderWrapper provider;

        #endregion Private fields.

        /// <summary>Initializes a new <see cref="BasicExpandProvider" /> instance.</summary>
        /// <param name="provider">Full provider.</param>
        /// <param name="expanded">Whether all values are automatically expanded in the model.</param>
        /// <param name="castToObject">Whether the provider of the IQueryable requires usage
        /// casts to System.Object when assigning to properties of that type.
        /// For example Linq to Entities requires us not to cast here as it only supports
        /// casting of primitive types. On the other hand Linq to Objects requires casting
        /// since otherwise it would generate wrong IL.</param>
        internal BasicExpandProvider(DataServiceProviderWrapper provider, bool expanded, bool castToObject)
        {
            Debug.Assert(provider != null, "provider != null");
            this.provider = provider;
            this.expanded = expanded;
            this.castToObject = castToObject;
        }

        /// <summary>Provider for metadata.</summary>
        internal DataServiceProviderWrapper Provider
        {
            get { return this.provider; }
        }

        /// <summary>Applies expansions and projections to the specified <paramref name="source"/>.</summary>
        /// <param name="source"><see cref="IQueryable"/> object to expand and apply projections to.</param>
        /// <param name="projection">The root node of the tree which describes
        /// the projections and expansions to be applied to the <paramref name="source"/>.</param>
        /// <returns>
        /// An <see cref="IQueryable"/> object, with the results including 
        /// the expansions and projections specified in <paramref name="projection"/>. 
        /// </returns>
        /// <remarks>
        /// The returned <see cref="IQueryable"/> may implement the <see cref="IExpandedResult"/> interface 
        /// to provide enumerable objects for the expansions; otherwise, the expanded
        /// information is expected to be found directly in the enumerated objects. If paging is 
        /// requested by providing a non-empty list in <paramref name="projection"/>.OrderingInfo then
        /// it is expected that the topmost <see cref="IExpandedResult"/> would have a $skiptoken property 
        /// which will be an <see cref="IExpandedResult"/> in itself and each of it's sub-properties will
        /// be named SkipTokenPropertyXX where XX represents numbers in increasing order starting from 0. Each of 
        /// SkipTokenPropertyXX properties will be used to generated the $skiptoken to support paging.
        /// If projections are required, the provider may choose to return <see cref="IQueryable"/>
        /// which returns instances of <see cref="IProjectedResult"/>. In that case property values are determined
        /// by calling the <see cref="IProjectedResult.GetProjectedPropertyValue"/> method instead of
        /// accessing properties of the returned object directly.
        /// If both expansion and projections are required, the provider may choose to return <see cref="IQueryable"/>
        /// of <see cref="IExpandedResult"/> which in turn returns <see cref="IProjectedResult"/> from its
        /// <see cref="IExpandedResult.ExpandedElement"/> property.
        /// </remarks>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        public IQueryable ApplyProjections(
            IQueryable source,
            ProjectionNode projection)
        {
            Debug.Assert(projection is RootProjectionNode, "We always get the special root node.");
            RootProjectionNode rootNode = (RootProjectionNode)projection;

            ExpandNode root = ExpandNode.BuildExpansionAndProjectionTree(this, rootNode);
            bool queryIsV1Compatible = 
                root.IsV1Compatible && 
                this.provider.IsV1Provider && 
                !rootNode.ProjectionsSpecified && 
                !rootNode.OrderingInfo.IsPaged;

            root.AssignTypeForExpected(source.ElementType, false /* singleResult */);

            // The new way is to have OrderBy before any Select
            if (!queryIsV1Compatible)
            {
                source = ApplyOrderSkipTakeOnTopLevelResultBeforeProjections(
                    source,
                    rootNode.OrderingInfo,
                    rootNode.SkipCount,
                    rootNode.TakeCount);
            }

            IQueryable query = root.BuildProjectionQuery(source);
            
            // The old way is to have OrderBy after Select (this is here for backward compat)
            if (queryIsV1Compatible)
            {
                query = ApplyOrderSkipTakeOnTopLevelResultAfterProjection(
                    query,
                    rootNode.OrderingInfo,
                    rootNode.SkipCount,
                    rootNode.TakeCount,
                    root);
            }

            if (!WebUtil.IsExpandedWrapperType(query.ElementType))
            {
                // If we don't use ExpandedWrapper on the root level we need to wrap the queryable
                // to process special "null" ProjectedWrapper values and turn them into true nulls
                // so that serialization doesn't have to deal with this.
                return ProjectedWrapper.WrapQueryable(query);
            }
            else
            {
                // If the top level uses ExpandedWrapper we need to wrap the returned IQueryable to expose the
                // IExpandedResult directly from the IEnumerator instead from each result instance.
                // Note that we don't need to wrap again for special "null" ProjectedWrapper values
                // as the ExpandedWrapper will post process the values it returns for this feature as well.
                Type resultQueryable = typeof(ExpandedQueryable<>).MakeGenericType(query.ElementType);
                object[] args = new object[] { query };
                return (IQueryable)Activator.CreateInstance(resultQueryable, args);
            }
        }

        /// <summary>
        /// Applies the ordering, skip and take on top level query result set before projections were applied
        /// </summary>
        /// <param name="query">Current query expression</param>
        /// <param name="orderingInfo">Top level ordering information</param>
        /// <param name="skipCount">Elements to skip</param>
        /// <param name="takeCount">Count of elements to return</param>
        /// <param name="root">The root <see cref="ExpandNode"/> where we store information about the shape
        /// of projections of the root level.</param>
        /// <returns><paramref name="query"/> appended with OrderBy/ThenBy, Skip and Top expressions</returns>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        internal static IQueryable ApplyOrderSkipTakeOnTopLevelResultAfterProjection(
            IQueryable query,
            OrderingInfo orderingInfo,
            int? skipCount,
            int? takeCount,
            ExpandNode root)
        {
            Debug.Assert(root != null, "root != null");

            if (orderingInfo != null)
            {
                Expression expression = query.Expression;
                Type elementType = ReflectionServiceProvider.GetIEnumerableElement(expression.Type);

                ParameterExpression parameter = Expression.Parameter(elementType, "p");
                Expression replaceExpression = parameter;
                if (root.RequiresExpandedWrapper)
                {
                    Debug.Assert(
                        WebUtil.IsExpandedWrapperType(elementType),
                        "If the node requires expanded wrapper it should have the ExpandedWrapper element type.");

                    // If the query is using expanded wrapper we need to replace the parameter in the OrderBy expressions
                    // with the access to the ExpandedElement instead of the wrapper itself.
                    replaceExpression = Expression.Property(parameter, "ExpandedElement");
                }

                bool first = true;
                foreach (OrderingExpression o in orderingInfo.OrderingExpressions)
                {
                    LambdaExpression e = (LambdaExpression)o.Expression;
                    LambdaExpression orderer;

                    orderer = Expression.Lambda(ParameterReplacerVisitor.Replace(e.Body, e.Parameters[0], replaceExpression), parameter);

                    query = (IQueryable) typeof(BasicExpandProvider).GetMethod("InvokeOrderBy", BindingFlags.Static | BindingFlags.NonPublic)
                                                                    .MakeGenericMethod(query.ElementType, orderer.Body.Type)
                                                                    .Invoke(null, new object[] { query, orderer, first, o.IsAscending });
                    first = false;
                }
            }

            return ApplySkipTakeOnTopLevelResult(query, skipCount, takeCount);
        }

        /// <summary>
        /// Applies the ordering, skip and take on top level query result set once projections where applied
        /// </summary>
        /// <param name="query">Current query expression</param>
        /// <param name="orderingInfo">Top level ordering information</param>
        /// <param name="skipCount">Elements to skip</param>
        /// <param name="takeCount">Count of elements to return</param>
        /// <returns><paramref name="query"/> appended with OrderBy/ThenBy, Skip and Top expressions</returns>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        internal static IQueryable ApplyOrderSkipTakeOnTopLevelResultBeforeProjections(
            IQueryable query,
            OrderingInfo orderingInfo,
            int? skipCount,
            int? takeCount)
        {
            if (orderingInfo != null)
            {
                Expression expression = query.Expression;
                Type elementType = ReflectionServiceProvider.GetIEnumerableElement(expression.Type);

                ParameterExpression parameter = Expression.Parameter(elementType, "p");

                bool first = true;
                foreach (OrderingExpression o in orderingInfo.OrderingExpressions)
                {
                    LambdaExpression e = (LambdaExpression)o.Expression;
                    LambdaExpression orderer;

                    orderer = Expression.Lambda(
                        ParameterReplacerVisitor.Replace(e.Body, e.Parameters[0], parameter),
                        parameter);

                    query = (IQueryable) typeof(BasicExpandProvider).GetMethod("InvokeOrderBy", BindingFlags.Static | BindingFlags.NonPublic)
                                                                    .MakeGenericMethod(query.ElementType, orderer.Body.Type)
                                                                    .Invoke(null, new object[] { query, orderer, first, o.IsAscending });
                    first = false;
                }
            }

            return ApplySkipTakeOnTopLevelResult(query, skipCount, takeCount);
        }

        /// <summary>
        /// Applies the skip and take on top level query result set
        /// </summary>
        /// <param name="query">Current query expression</param>
        /// <param name="skipCount">Elements to skip</param>
        /// <param name="takeCount">Count of elements to return</param>
        /// <returns><paramref name="query"/> appended with Skip and Top expressions</returns>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        internal static IQueryable ApplySkipTakeOnTopLevelResult(
            IQueryable query,
            int? skipCount,
            int? takeCount)
        {
            if (skipCount.HasValue)
            {
                query = (IQueryable) typeof(BasicExpandProvider).GetMethod("InvokeSkipOrTake", BindingFlags.Static | BindingFlags.NonPublic)
                                                                .MakeGenericMethod(query.ElementType)
                                                                .Invoke(null, new object[] { query, skipCount.Value, true });
            }

            if (takeCount.HasValue)
            {
                query = (IQueryable) typeof(BasicExpandProvider).GetMethod("InvokeSkipOrTake", BindingFlags.Static | BindingFlags.NonPublic)
                                                                .MakeGenericMethod(query.ElementType)
                                                                .Invoke(null, new object[] { query, takeCount.Value, false });
            }

            return query;
        }

        /// <summary>
        /// Invokes Queryable.OrderBy/ThenBy in a strongly typed way for a weakly typed <paramref name="query"/>
        /// </summary>
        /// <typeparam name="TSource">Source type</typeparam>
        /// <typeparam name="TKey">Type of expression to order by</typeparam>
        /// <param name="query">Input query</param>
        /// <param name="orderExpression">Expression representing orderby clause</param>
        /// <param name="firstOrder">Is this first ordering expression in a list</param>
        /// <param name="isAscending">Ordering is ascending or descending</param>
        /// <returns>Query with orderby/thenby appended to it</returns>
        private static IQueryable InvokeOrderBy<TSource, TKey>(IQueryable query, LambdaExpression orderExpression, bool firstOrder, bool isAscending)
        {
            MethodInfo orderingMethod = firstOrder ? 
                        (isAscending ? BasicExpandProvider.OrderByMethodInfo : BasicExpandProvider.OrderByDescendingMethodInfo) : 
                        (isAscending ? BasicExpandProvider.ThenByMethodInfo : BasicExpandProvider.ThenByDescendingMethodInfo);
            
            return (IQueryable)orderingMethod
                                .MakeGenericMethod(typeof(TSource), typeof(TKey))
                                .Invoke(null, new object[] { (IQueryable<TSource>)query, (Expression<Func<TSource, TKey>>)orderExpression });
        }

        /// <summary>
        ///  Invokes Queryable.Skip/Take in a strongly typed way for a weakly typed <paramref name="query"/>
        /// </summary>
        /// <typeparam name="TSource">Source type</typeparam>
        /// <param name="query">Input query</param>
        /// <param name="count">Value of skip or top parameter</param>
        /// <param name="isSkip">Is it Skip or Top</param>
        /// <returns>Query with appended Skip/Top invocation</returns>
        private static IQueryable InvokeSkipOrTake<TSource>(IQueryable query, int count, bool isSkip)
        {
            MethodInfo skipOrTakeMethod = (isSkip ? BasicExpandProvider.SkipMethodInfo : BasicExpandProvider.TakeMethodInfo).MakeGenericMethod(typeof(TSource));
            return (IQueryable)skipOrTakeMethod.Invoke(null, new object[] { (IQueryable<TSource>)query, count });
        }

        #region Inner types.

        /// <summary>Queryable element for results with expanded properties.</summary>
        /// <typeparam name="TWrapper">Wrapper type with expanded properties.</typeparam>
        internal class ExpandedQueryable<TWrapper> : IQueryable where TWrapper : IExpandedResult
        {
            /// <summary>Source enumeration with wrapped properties.</summary>
            private IQueryable<TWrapper> source;

            /// <summary>Initializes a new ExpandedEnumerable instance.</summary>
            /// <param name="source">Source for enumeration.</param>
            public ExpandedQueryable(IQueryable<TWrapper> source)
            {
                Debug.Assert(source != null, "source != null");
                this.source = source;
            }

            /// <summary>The type of a single result.</summary>
            public Type ElementType
            {
                get { return this.source.ElementType; }
            }

            /// <summary>The query expression.</summary>
            public Expression Expression
            {
                get { return this.source.Expression; }
            }

            /// <summary>The query provider - not supported as nobody should call this.</summary>
            public IQueryProvider Provider
            {
                get { throw Error.NotSupported(); }
            }

            /// <summary>Gets an enumerator object for results.</summary>
            /// <returns>An enumerator object for results.</returns>
            IEnumerator IEnumerable.GetEnumerator()
            {
                return this.GetEnumerator();
            }

            /// <summary>Gets an enumerator object for results.</summary>
            /// <returns>An enumerator object for results.</returns>
            public IEnumerator GetEnumerator()
            {
                IEnumerator<TWrapper> enumerator = this.source.GetEnumerator();
                return new ExpandedEnumerator<TWrapper>(enumerator);
            }
        }

        /// <summary>Base class for the generic ExpandedEnumerator so that we can easily recognize instances of that class
        /// without usage of reflection (used during serialization and thus needs to be as fast as possible).</summary>
        /// <remarks>Note that we can't implement the ExpandedEnumerator as non-generic because the V1 version used
        /// the inner enumerator as generic and called the MoveNext/Current and so methods on the generic version.
        /// We also need the generic version for the IExpandedResult unwrapping to work.</remarks>
        internal abstract class ExpandedEnumerator
        {
            /// <summary>Determines if the specified enumerator is the ExpandedWrapper enumerator wrapper or the ProjectedWrapper enumerator wrapper
            /// and if so returns the unwrapped inner enumerator.</summary>
            /// <param name="enumerator">The enumerator to unwrap</param>
            /// <returns>Unwrapped enumerator if it was wrapped or the original enumerator if now wrapping occured.</returns>
            internal static IEnumerator UnwrapEnumerator(IEnumerator enumerator)
            {
                // Note that only one wrapper could have been applied. If the query only used projections and no expansions then we wrap it
                //   with the ProjectedWrapper enumerator wrapper. If the query had expansions we wrap it wil ExpandedWrapper enumerator wrapper
                //   which also includes the functionality of the ProjectedWrapper enumerator wrapper.
                ExpandedEnumerator expandedEnumerator = enumerator as ExpandedEnumerator;
                if (expandedEnumerator != null)
                {
                    return expandedEnumerator.GetInnerEnumerator();
                }
                else
                {
                    return ProjectedWrapper.UnwrapEnumerator(enumerator);
                }
            }

            /// <summary>Virtual method to unwrap the inner enumerator as it's stored in the generic version of this class.</summary>
            /// <returns>The inner enumerator.</returns>
            protected abstract IEnumerator GetInnerEnumerator();
        }

        /// <summary>Use this class to enumerate elements that can be expanded.</summary>
        /// <typeparam name="TWrapper">Wrapper type with expanded properties.</typeparam>
        internal class ExpandedEnumerator<TWrapper> : ExpandedEnumerator, IEnumerator, IDisposable, IExpandedResult where TWrapper : IExpandedResult
        {
            /// <summary>Enumerator for wrapper instances.</summary>
            private IEnumerator<TWrapper> e;

            /// <summary>Initializes a new ExpandedEnumerator instance.</summary>
            /// <param name="enumerator">Source for enumeration.</param>
            internal ExpandedEnumerator(IEnumerator<TWrapper> enumerator)
            {
                WebUtil.CheckArgumentNull(enumerator, "enumerator");
                this.e = enumerator;
            }

            /// <summary>Element with expanded properties.</summary>
            public object ExpandedElement
            {
                get { return this.Current; }
            }

            /// <summary>Element with expanded properties.</summary>
            public object Current
            {
                get { return this.e.Current.ExpandedElement; }
            }

            /// <summary>Gets an expanded property for the specified <paramref name="name"/>.</summary>
            /// <param name="name">Name of property to return.</param>
            /// <returns>The expanded property value with the specified <paramref name="name"/>.</returns>
            public object GetExpandedPropertyValue(string name)
            {
                return this.e.Current.GetExpandedPropertyValue(name);
            }

            /// <summary>Moves to the next element.</summary>
            /// <returns>true if an element is available after the move; false otherwise.</returns>
            public bool MoveNext()
            {
                return this.e.MoveNext();
            }

            /// <summary>Resets the enumerator to the beginning of the sequence.</summary>
            public void Reset()
            {
                throw Error.NotImplemented();
            }

            /// <summary>Releases resources.</summary>
            public void Dispose()
            {
                this.e.Dispose();
                GC.SuppressFinalize(this);
            }

            /// <summary>Virtual method to unwrap the inner enumerator as it's stored in the generic version of this class.</summary>
            /// <returns>The inner enumerator.</returns>
            protected override IEnumerator GetInnerEnumerator()
            {
                return this.e;
            }
        }

        /// <summary>Class which represents one projected property. Used as a pair of name and resource property.</summary>
        [DebuggerDisplay("ProjectedProperty {Name}")]
        internal class ProjectedProperty
        {
            /// <summary>Name of the property to project</summary>
            private readonly string name;

            /// <summary>The resource property to project.</summary>
            /// <remarks>If this field is null, it means we are to project open-type property.</remarks>
            private readonly ResourceProperty property;

            /// <summary>Creates a new instance of the projected property for a given property.</summary>
            /// <param name="name">The name of the property to project.</param>
            /// <param name="property">The <see cref="ResourceProperty"/> to project. For open properties this is null.</param>
            public ProjectedProperty(string name, ResourceProperty property)
            {
                Debug.Assert(name != null, "name != null");
                Debug.Assert(property == null || String.Equals(property.Name, name, StringComparison.Ordinal), "The name of the property must match the specified name.");
                this.name = name;
                this.property = property;
            }

            /// <summary>Name of the property to project</summary>
            public string Name
            {
                get { return this.name; }
            }

            /// <summary>The resource property to project.</summary>
            /// <remarks>If this field is null, it means we are to project open-type property.</remarks>
            public ResourceProperty Property
            {
                get { return this.property; }
            }

            /// <summary><see cref="EqualityComparer&lt;ProjectedProperty&gt;"/> for <see cref="ProjectedProperty"/> which treats
            /// two instance equal if they have the same name and the same resource property.</summary>
            public class ProjectedPropertyEqualityComparer : IEqualityComparer<ProjectedProperty>
            {
                #region IEqualityComparer<ProjectedProperty> Members

                /// <summary>Compares two <see cref="ProjectedProperty"/> instances for equality.</summary>
                /// <param name="x">First instance.</param>
                /// <param name="y">Second instance.</param>
                /// <returns>true if the instances are equal, that means if they have the same name
                /// and the same resource property.</returns>
                public bool Equals(ProjectedProperty x, ProjectedProperty y)
                {
                    return String.Equals(x.name, y.name, StringComparison.Ordinal) && x.property == y.property;
                }

                /// <summary>Computes a simple hash code.</summary>
                /// <param name="obj">The instance to compute the hash code for.</param>
                /// <returns>The hash code.</returns>
                public int GetHashCode(ProjectedProperty obj)
                {
                    return obj.name.GetHashCode() + (obj.property == null ? 0 : obj.property.GetHashCode());
                }

                #endregion
            }
        }

        /// <summary>Use this class to build a tree structure over a list of paths.</summary>
        [DebuggerDisplay("ExpandNode {Node}")]
        internal class ExpandNode
        {
            #region Private fields.

            /// <summary>Enumerable.Select method info</summary>
            private static readonly MethodInfo SelectMethodInfoEnumerable = typeof(Enumerable)
                                                            .GetMethods(BindingFlags.Static | BindingFlags.Public)
                                                            .Single(IsMethodEnumerableSelect);

            /// <summary>Enumerable.Where method info</summary>
            private static readonly MethodInfo WhereMethodInfoEnumerable = typeof(Enumerable)
                                                            .GetMethods(BindingFlags.Static | BindingFlags.Public)
                                                            .Single(IsMethodEnumerableWhere);
                                                      
            /// <summary>Enumerable.Take method info</summary>
            private static readonly MethodInfo TakeMethodInfoEnumerable = typeof(Enumerable)
                                                            .GetMethod("Take", BindingFlags.Static | BindingFlags.Public);

            /// <summary>Expanded projection node for this node.</summary>
            private readonly ExpandedProjectionNode Node;

            /// <summary>Provider with metadata for operations.</summary>
            private readonly BasicExpandProvider expandProvider;

            /// <summary>All child expand nodes for this segment.</summary>
            private List<ExpandNode> children;

            /// <summary>The element type that will be projected from this segment.</summary>
            private Type elementType;

            /// <summary>The model (never wrapped) type to be used in an strongly-typed IEnumerable for this segment.</summary>
            private Type enumeratedType;

            /// <summary>Whether there are any filters within this node. MaxResults are considered filters for processing.</summary>
            private bool hasFilterWithin;

            /// <summary>Whether this segment represents a single element rather than a collection.</summary>
            private bool singleResult;
            
            /// <summary>Ordering info for this node</summary>
            private OrderingInfo orderingInfo;
            
            /// <summary>Is skip token needed for current segment</summary>
            private bool needSkipToken;

            /// <summary>Set to true if this node is the root node.</summary>
            private bool isRoot;

            /// <summary>Set to true if this expand node and all its children represent a query which could be generated
            /// by the V1 version.</summary>
            /// <remarks>This is used to generate queires compatible with V1 when the query is V1 compatible.</remarks>
            private bool isV1Compatible;

            /// <summary>Whether this segment should be projected into an expanded wrapper.</summary>
            /// <remarks>Segments require a wrapper because the model doesn't auto-wire
            /// up (as in the EDM case), or because they have children with
            /// filters (and thus they need to project filtered children),
            /// or because they have wrapped children (and thus they need
            /// to project the custom projected type).</remarks>
            private bool requiresExpandedWrapper;

            /// <summary>List of all <see cref="ResourceType"/>s possibly returned by this segment (before projections).</summary>
            /// <remarks>The list is ordered such that if you imagine the tree of inheritance the root is always the first
            /// item in the list. The rule is that for each two types A and B,
            /// if A is ancestor in the inheritance hierarchy to B, then A goes before B. Types which don't have inheritance
            /// relationship (one is not the ancestor of the other) are in undefined order (we don't care).
            /// The list represents the entire inheritance hierarchy of types which can appear in the given resource set
            /// for this segment.</remarks>
            private List<ResourceType> resourceTypes;

            /// <summary>List of properties to be projected on this segment.</summary>
            /// <remarks>The list may hold several <see cref="ProjectedProperty"/> instances with the same name.
            /// In that case it means that there are different properties declared on different types in the inheritance
            /// tree which have the same name. Only one of them can be an open property (without the <see cref="ResourceProperty"/>).
            /// 
            /// We need multiple instances for the same property name in cases where the properties have differnt types.
            /// In that case we can't project them onto the same property on the <see cref="ProjectedWrapper"/>
            /// since some providers (for example Linq to Entities) would reject such query (the properties might have different 
            /// CLR types and the provider might complain that we are projecting differnt types for the same property, although
            /// in reality we always project into "object", the provides sometimes don't see that).
            /// 
            /// If a property is to be projected, there's going to be an some instance of <see cref="ProjectedProperty"/>
            /// which applies to every type the property can be projected on. If there's is no matching instance for a given
            /// type in the inheritance hierarchy it means that type can't project that property at all (and we project
            /// null of the correct CLR type instead).
            /// 
            /// The order in this list is important. It defines the order in which the properties are projected onto the
            /// ProjectedWrapper types. If a given property has index #i in this list it's going to be projected
            /// as #ith ProjectedProperty on the ProjectedWrapper.
            /// 
            /// If this field is null it means no projections are defined for this segment and thus the entire resource
            /// should be returned from the query. If the field is non-null only the properties listed in it should be projected
            /// from the query.
            /// 
            /// We must store a full list of all properties possibly projected on all types in this segment. The reason is
            /// so that we can assign them unique indeces (the order of this list). Since after the projection we won't be able
            /// to refer to them by names anymore. After the projection we won't know which exact type the resource was
            /// so we need a way to refer to the projected property without the knowledge of the exact type.</remarks>
            private List<ProjectedProperty> projectedProperties;

            /// <summary>Candidate set of properties to project.</summary>
            /// <remarks>We fill this set as we go through all the different ways to determine
            /// properties which need to be projected.
            /// Once we're done we will move the content of this set to the <see cref="projectedProperties"/> list
            /// and this fix the order of the projected properties.
            /// We use a set since we want to speed up the process of finding duplicates as it can happen
            /// quite often that we try to add the same property several times.
            /// During the phase where we build this set we use this field to mark nodes which require projection.
            /// Nodes which have this field null don't require projections and will project the entire resource always.</remarks>
            private HashSet<ProjectedProperty> projectedPropertyCandidates;

            /// <summary>Set of names of open-properties we are to project.</summary>
            /// <remarks>This is a companion set to the <see cref="projectedPropertyCandidates"/> set.
            /// We need this list to search for these properties on derived types, since if the property
            /// is actually declared on the derived type we need to add it as a declared property as well.</remarks>
            private HashSet<string> projectedOpenPropertyNames;

            #endregion Private fields.

            /// <summary>Initalizes a new <see cref="ExpandNode"/>.</summary>
            /// <param name="node">Node being described.</param>
            /// <param name="provider">Provider for expansion flags and metadata.</param>
            internal ExpandNode(ExpandedProjectionNode node, BasicExpandProvider provider)
            {
                Debug.Assert(node != null, "node != null");
                Debug.Assert(provider != null, "provider != null");
                this.Node = node;
                this.expandProvider = provider;
                this.orderingInfo = this.Node.OrderingInfo;
                this.needSkipToken = NeedSkipTokenVisitor.IsSkipTokenRequired(this.orderingInfo);
                this.children = new List<ExpandNode>();
                this.isV1Compatible = true;

                if (!node.ProjectAllProperties)
                {
                    this.projectedPropertyCandidates =
                        new HashSet<ProjectedProperty>(new ProjectedProperty.ProjectedPropertyEqualityComparer());
                    this.projectedOpenPropertyNames = new HashSet<string>(StringComparer.Ordinal);
                }
            }

            /// <summary>Provider with metadata for operations.</summary>
            internal BasicExpandProvider ExpandProvider
            {
                get
                {
                    return this.expandProvider;
                }
            }

            /// <summary>Type to be projected for this node in the model (possibly wrapped).</summary>
            internal Type ProjectedType
            {
                get
                {
                    if (this.singleResult)
                    {
                        return this.elementType;
                    }
                    else
                    {
                        return typeof(IEnumerable<>).MakeGenericType(this.elementType);
                    }
                }
            }

            /// <summary>Ordering information needed for root node</summary>
            internal OrderingInfo OrderingInfo
            {
                get
                {
                    return this.orderingInfo;
                }
            }

            /// <summary>Description to initialize the wrapper with.</summary>
            internal string WrapperDescription
            {
                get
                {
                    StringBuilder result = new StringBuilder();
                    foreach (var child in this.children)
                    {
                        if (result.Length > 0)
                        {
                            result.Append(',');
                        }

                        result.Append(child.Node.PropertyName);
                    }
                    
                    return result.ToString();
                }
            }

            /// <summary>The base resource type for this segmenr.</summary>
            /// <remarks>All resources reported by this segment will be of this or some of its derived types.</remarks>
            internal ResourceType BaseResourceType
            {
                get { return this.Node.ResourceType; }
            }

            /// <summary>Set to true if this expand node and all its children represent a query which could be generated
            /// by the V1 version.</summary>
            /// <remarks>This is used to generate queires compatible with V1 when the query is V1 compatible.</remarks>
            internal bool IsV1Compatible
            {
                get { return this.isV1Compatible; }
            }

            /// <summary>Returns true if this node requires a ExpandedWrapper to be used to represent the result.</summary>
            /// <remarks>If both exapnede and projected wrappers are required we return ExpandedWrapper which stores
            /// the result element as a ProjectedWrapper.</remarks>
            internal bool RequiresExpandedWrapper
            {
                get { return this.requiresExpandedWrapper; }
            }

            /// <summary>Returns true if this node requires a ProjectedWrapper to be used to represent the result.</summary>
            /// <remarks>If the node also requires an ExpandedWrapper then the actuall type fo the result will be
            /// the ExpandedWrapper with its ExpandedElement property of type ProjectedWrapper.</remarks>
            private bool RequiresProjectedWrapper
            {
                get { return this.projectedProperties != null; }
            }

            /// <summary>Returns true if this node requires some kind of wrapper for its result.</summary>
            private bool RequiresWrapper
            {
                get { return this.RequiresExpandedWrapper || this.RequiresProjectedWrapper; }
            }

            /// <summary>Returns true if this expansion will need standard paging applied.</summary>
            private bool NeedsStandardPaging
            {
                get
                {
                    return !this.singleResult &&
                           !this.isRoot &&
                           this.Node.OrderingInfo != null &&
                           this.Node.OrderingInfo.IsPaged;
                }
            }

            /// <summary>Builds a tree of <see cref="ExpandNode"/> items and the list of projected properties on them.</summary>
            /// <param name="provider">Expand provider for nodes.</param>
            /// <param name="rootProjectionNode">The root projection node to start with.</param>
            /// <returns>The newly create root expand node.</returns>
            internal static ExpandNode BuildExpansionAndProjectionTree(
                BasicExpandProvider provider,
                ExpandedProjectionNode rootProjectionNode)
            {
                Debug.Assert(provider != null, "provider != null");
                Debug.Assert(rootProjectionNode != null, "projection != null");

                // Create new root expand node
                ExpandNode expandNode = new ExpandNode(rootProjectionNode, provider);
                expandNode.isRoot = true;
                expandNode.CreateChildren();

                // And apply projections to it
                expandNode.ApplyProjections();

                return expandNode;
            }

            /// <summary>Creates children expand nodes for this node.</summary>
            internal void CreateChildren()
            {
                // On inner nodes - if the node uses projections or ordering (as that implies paging) it is not V1 compatible
                if (!this.isRoot && (!this.Node.ProjectAllProperties || this.OrderingInfo != null))
                {
                    this.isV1Compatible = false;
                }

                // Walk child expanded nodes and create our representation for them
                foreach (ProjectionNode node in this.Node.Nodes)
                {
                    ExpandedProjectionNode expandedProjectionNode = node as ExpandedProjectionNode;
                    if (expandedProjectionNode != null)
                    {
                        // Create new expand node
                        ExpandNode expandNode = new ExpandNode(expandedProjectionNode, this.ExpandProvider);
                        this.children.Add(expandNode);
                        expandNode.CreateChildren();
                        this.isV1Compatible &= expandNode.isV1Compatible;
                    }
                }
            }

            /// <summary>Adds the specified property to the list of projected properties on this node.</summary>
            /// <param name="property">The <see cref="ResourceProperty"/> to project.</param>
            internal void AddProjectedProperty(ResourceProperty property)
            {
                this.AddProjectedProperty(property.Name, property);
            }

            /// <summary>Adds the specified property to the list of projected properties on this node.</summary>
            /// <param name="propertyName">The name of the property to project.</param>
            /// <param name="property">The <see cref="ResourceProperty"/> to project, can be null for open-type properties.</param>
            internal void AddProjectedProperty(string propertyName, ResourceProperty property)
            {
                this.projectedPropertyCandidates.Add(new ProjectedProperty(propertyName, property));
                if (property == null)
                {
                    this.projectedOpenPropertyNames.Add(propertyName);
                }
            }

            /// <summary>Finds a child <see cref="ExpandNode"/> with the specified name.</summary>
            /// <param name="name">The name of the node (navigation property) for which to look.</param>
            /// <returns>The child <see cref="ExpandNode"/> or null if none was found.</returns>
            internal ExpandNode FindChild(string name)
            {
                foreach (ExpandNode child in this.children)
                {
                    if (String.Equals(child.Node.PropertyName, name, StringComparison.Ordinal))
                    {
                        return child;
                    }
                }

                return null;
            }

            /// <summary>Applies projection segments onto the node creating list of projected properties 
            /// and types for this node. It calls itself recursively to apply projections to the
            /// entire subtree.</summary>
            internal void ApplyProjections()
            {
                // If we are to project all properties for this segment, just leave it at the default
                //   which is to not use projection wrapper for this segment.
                if (!this.Node.ProjectAllProperties)
                {
                    // Populate the list of resource types for this segment (in no particular order yet)
                    this.resourceTypes = new List<ResourceType>();
                    this.resourceTypes.Add(this.BaseResourceType);
                    if (this.ExpandProvider.Provider.HasDerivedTypes(this.BaseResourceType))
                    {
                        this.resourceTypes.AddRange(this.ExpandProvider.Provider.GetDerivedTypes(this.BaseResourceType));
                    }

                    // Sort the list so that less inherited types are before more inherited ones
                    this.resourceTypes.Sort(
                        new Comparison<ResourceType>((x, y) => (x == y) ? 0 : (x.IsAssignableFrom(y) ? -1 : 1)));

                    // First apply all projection segments (these are mandatory since they are required by the query)
                    foreach (ProjectionNode node in this.Node.Nodes)
                    {
                        if (node.Property == null || node.Property.TypeKind != ResourceTypeKind.EntityType)
                        {
                            // Only project primitive, complex or open properites (as open properties are never nav. properties)
                            Debug.Assert(
                                !(node is ExpandedProjectionNode), 
                                "Projected simple, complex or open property must not be represented by expansion node.");
                            this.AddProjectedProperty(node.PropertyName, node.Property);
                        }
                        else
                        {
                            // It's a navigation property - try to find its expand segment
                            ExpandedProjectionNode expandedProjectionNode = node as ExpandedProjectionNode;
                            if (expandedProjectionNode != null)
                            {
                                // If we have an expand node for it, follow it
                                // But also add it to the list as we don't know if we will need to wrap it 
                                // in the expand wrapper or not yet. 
                                // If we are going to wrap this node (not the child)
                                //   for expansions, we don't need to project the nav. property, as the expanded wrapper
                                //   will do that for us. So in that case we will remove the projection property from the list.
                                // But if we're not going to wrap this node for expansion, we need the nav. property
                                //   projected so that we can navigate over it (since we are expanding it).
                                this.AddProjectedProperty(node.Property);
                            }
                            else
                            {
                                // If we don't have an expand segment we "should" project the nav. property, 
                                //   but since there's no expand we don't actually need any value 
                                //   to produce the result (which is just a link)
                                //   so there's no need to actually project anything (we don't need to value to create the link).
                                // So leave it out - serialization will not ask for it
                            }
                        }
                    }

                    // Add all Key properties since we will need them to generate the resource identity during serialization.
                    // Note that we only need to walk them on the base type, derived types can't modify the list of key properties.
                    foreach (ResourceProperty keyProperty in this.BaseResourceType.KeyProperties)
                    {
                        this.AddProjectedProperty(keyProperty);
                    }

                    // Add all properties as required by the ordering info if paging is on and we will not explicitely project
                    //   this $skiptoken property. In this case we need the properties accessed by the OrderBy expression
                    //   to be projected so that the skiptoken can be generated from them during serialization.
                    // Note that applying ordering info to projections migh affect projections on child nodes
                    //   but never the other way round. So first we apply ordering to the parent and then to children.
                    //   This way we can fix the order of projected properties once we apply ordering on a given level.
                    if (this.OrderingInfo != null && this.OrderingInfo.IsPaged && !this.needSkipToken)
                    {
                        foreach (OrderingExpression orderingExpression in this.OrderingInfo.OrderingExpressions)
                        {
                            LambdaExpression lambda = (LambdaExpression)orderingExpression.Expression;
                            Dictionary<Expression, ExpandNode> expandNodeAnnotations =
                                ExpandNodeAnnotationVisitor.AnnotateExpression(lambda.Body, lambda.Parameters[0], this);
                            AddPropertyAccessesAsProjectedPropertiesVisitor.
                                AddPropertyAccessesAsProjectedProperties(lambda.Body, expandNodeAnnotations);
                        }
                    }

                    // The IDataServiceStreamProvider API expects the full MLE instance.  If there exists an MLE type in the
                    // hierarchy, we need to query for the full of entities from the hierarchy. During serialization we will
                    // only serialize out the projected fields.
                    bool foundMediaLinkEntryInHierarchy = false;

                    // Walk the list of types and infer projected properties as required by ETags and Epm.
                    foreach (ResourceType resourceType in this.resourceTypes)
                    {
                        // Add all the ETag properties for this type since we will need them to generate ETag during serialization.
                        foreach (ResourceProperty etagProperty in this.ExpandProvider.Provider.GetETagProperties(this.Node.ResourceSetWrapper.Name, resourceType))
                        {
                            this.AddProjectedProperty(etagProperty);
                        }

                        // Add all the Epm properties for this type since we will need them to be able to fill all the mappings
                        //   during serialization.
                        if (resourceType.HasEntityPropertyMappings)
                        {
                            foreach (EpmSourcePathSegment epmSegment in resourceType.EpmSourceTree.Root.SubProperties)
                            {
                                // Note that we intentionally add entire complex type properties and not the subproperties for those
                                //   as we can't project just partial complex types. So even if the Epm needs just one property from
                                //   a complex type, the entire complex type will be projected.
                                // Once we actually support projecting into the complex types (from the query) we might modify
                                //   this here to project only the properties requried by Epm.
                                ResourceProperty property = resourceType.TryResolvePropertyName(epmSegment.PropertyName);
                                this.AddProjectedProperty(epmSegment.PropertyName, property);
                            }
                        }

                        if (resourceType.IsMediaLinkEntry)
                        {
                            foundMediaLinkEntryInHierarchy = true;
                        }
                    }

                    // Walk the types again and resolve open-type properties
                    foreach (ResourceType resourceType in this.resourceTypes)
                    {
                        // Walk all the open properties we are to project and check if some of them is not declared on this type
                        foreach (string projectedOpenPropertyName in this.projectedOpenPropertyNames)
                        {
                            ResourceProperty property = resourceType.TryResolvePropertyName(projectedOpenPropertyName);

                            // If the property is declared on this type, add the declared version as well.
                            if (property != null)
                            {
                                this.AddProjectedProperty(projectedOpenPropertyName, property);
                            }
                        }
                    }

                    if (!foundMediaLinkEntryInHierarchy)
                    {
                        // Now fix some arbitrary order of the properties (the order itself doesn't matter,
                        //   but it needs to be fixed as it determines the indeces for projected wrappers).
                        // This allows us to find the right ProjectedProperty on the projected wrapper
                        //   for a given property.
                        this.projectedProperties = new List<ProjectedProperty>(this.projectedPropertyCandidates.Count);
                        this.projectedProperties.AddRange(this.projectedPropertyCandidates);
                    }
                    else
                    {
                        // We found an MLE type in the hierarchy.  We are intentionally throwing away the work done above in this method.
                        // The reason for doing this is methods on the IDataServiceStreamProvider API expects the full MLE instances.
                        // If this.projectedProperties is not null, the query we construct will come back with partial entities in
                        // the form of ProjectedWrapper objects. And passing the ProjectedWrapper instances to the stream provider
                        // breaks the API contract.
                        // By leaving this.projectedProperties null, the query we construct will retrieve the full MLE entity which we
                        // can successfully pass to the stream provider.  During serialization we will only serialize out the projected
                        // properties.
                        Debug.Assert(this.projectedProperties == null, "By default the entire resource should be projected.");
                    }
                }
                else
                {
                    Debug.Assert(this.projectedProperties == null, "By default the entire resource should be projected.");
                }

                // Now walk child expanded nodes and apply projections to them as well
                foreach (ExpandNode child in this.children)
                {
                    child.ApplyProjections();
                }
            }

            /// <summary>----ings type information given the specified type.</summary>
            /// <param name="enumeratedType">Type expected for enumeration.</param>
            /// <param name="singleResult">true if a single result is expected of this node; false otherwise.</param>
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Maintainability", "CA1500:VariableNamesShouldNotMatchFieldNames", MessageId = "enumeratedType", Justification = "1:1 mapping")]
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Maintainability", "CA1500:VariableNamesShouldNotMatchFieldNames", MessageId = "singleResult", Justification = "1:1 mapping")]
            internal void AssignTypeForExpected(Type enumeratedType, bool singleResult)
            {
                this.enumeratedType = enumeratedType;
                this.singleResult = singleResult;
              
                bool foundChildWithWrapper = false;
                bool foundChildWithFilter = false;

                foreach (ExpandNode child in this.children)
                {
                    Debug.Assert(child.Node != null, "child.Segment != null -- only the root has a null segment");
                    ResourceProperty property = child.Node.Property;
                    if (property == null)
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BasicExpandProvider_ExpandNotSupportedForOpenProperties);
                    }

                    Type segmentType = property.ResourceType.InstanceType;
                    child.AssignTypeForExpected(segmentType, property.Kind != ResourcePropertyKind.ResourceSetReference);
                    if (child.RequiresWrapper)
                    {
                        foundChildWithWrapper = true;
                    }

                    if (child.hasFilterWithin)
                    {
                        foundChildWithFilter = true;
                    }
                }

                // The segment node needs filter if either a filter is present or 
                // page size or max results is set when the result count is non-single
                bool needFilterForSelf = !this.isRoot && 
                                        (this.Node.Filter != null || (this.Node.MaxResultsExpected.HasValue && !this.singleResult));

                this.hasFilterWithin = needFilterForSelf || foundChildWithFilter;

                this.requiresExpandedWrapper =
                    this.needSkipToken ||                   // We wrap to project $skiptoken expressions
                    foundChildWithWrapper ||                // We wrap to project custom types.
                    foundChildWithFilter ||                 // We wrap to project filtered results.
                    (!this.ExpandProvider.expanded && this.children.Count > 0); // We wrap to implement expansion.

                if (this.RequiresExpandedWrapper)
                {
                    foreach (ExpandNode child in this.children)
                    {
                        // If the child was projected explicitely (has projection segment)
                        // remove it from the list of projected properties as we're going to use expanded wrapper to project it
                        if (this.projectedProperties != null)
                        {
                            int projectedPropertyIndex = this.GetProjectedPropertyIndex(child.Node.Property);
                            if (projectedPropertyIndex != -1)
                            {
                                Debug.Assert(this.RequiresProjectedWrapper, "The parent segment must use projected wrapper, if some of its children where projected explicitely.");

                                // Note that this changes the order of projected properties
                                //   but we shouldn't have used that yet anyway.
                                this.projectedProperties.RemoveAt(projectedPropertyIndex);
                            }
                        }
                    }
                }

                if (this.RequiresWrapper)
                {
                    this.SetWrapperElementType();
                }
                else
                {
                    this.elementType = this.enumeratedType;
                }
            }

            /// <summary>Builds the projection to apply expansion for the specified <paramref name="query"/>.</summary>
            /// <param name="query">Query to build for.</param>
            /// <returns>A new query with the expansions performed inside.</returns>
            internal IQueryable BuildProjectionQuery(IQueryable query)
            {
                Debug.Assert(query != null, "query != null");
                Debug.Assert(this.isRoot, "BuildProjectionQuery is being called on a non-root node.");
                MethodCallExpression call = this.BuildProjectionExpression(query.Expression) as MethodCallExpression;
                
                // Sometimes in reflection scenarios, the projection simply results in the original expression being returned
                // back because the whole graph is already present in memory, for such cases we simply return the original query
                if (call != null && !this.singleResult && this.RequiresWrapper)
                {
                    Debug.Assert(
                        call.Method.Name == "Select",
                        call.Method.Name + " == 'Select' -- otherwise last expression must be a .Select projection.");
                    LambdaExpression selector = call.Arguments[1].NodeType == ExpressionType.Quote ?
                                                (LambdaExpression)((UnaryExpression)call.Arguments[1]).Operand :
                                                (LambdaExpression)call.Arguments[1];
                    return RequestUriProcessor.InvokeSelectForTypes(query, this.elementType, selector);
                }
                else
                {
                    return query;
                }
            }

            /// <summary>Returns the index of the projected property.</summary>
            /// <param name="property">The property to look for.</param>
            /// <returns>The index of the projected property. This is the index under which the property
            /// was projected into the ProjectedWrapper.</returns>
            internal int GetProjectedPropertyIndex(ResourceProperty property)
            {
                Debug.Assert(property != null, "property != null");
                Debug.Assert(this.projectedProperties != null, "Trying to find a projected property when no projections were applied.");

                return this.projectedProperties.FindIndex(
                    projectedProperty => projectedProperty.Property == property);
            }

            /// <summary>
            /// Creates a LambdaExpression and can be used when the delegate type is not known at compile time.
            /// </summary>
            /// <param name="delegateType">A Type that represents a delegate type.</param>
            /// <param name="body">An Expression to set the Body property equal to.</param>
            /// <param name="parameters">An IEnumerable&lt;T&gt; that contains ParameterExpression objects to use to populate the Parameters collection.</param>
            /// <returns>
            /// An object that represents a lambda expression which has the NodeType property equal to Lambda and the 
            /// Body and Parameters properties set to the specified values.
            /// </returns>
            private static LambdaExpression BuildLambdaExpression(Type delegateType, Expression body, IEnumerable<ParameterExpression> parameters)
            {
                Debug.Assert(delegateType != null, "delegateType != null");
                Debug.Assert(body != null, "body != null");
                return Expression.Lambda(delegateType, body, parameters);
            }

            /// <summary>Checks whether the specified method is the IEnumerable.Select() with Func`T,T2.</summary>
            /// <param name="m">Method to check.</param>
            /// <returns>true if this is the method; false otherwise.</returns>
            private static bool IsMethodEnumerableSelect(MethodInfo m)
            {
                return IsNamedMethodSecondArgumentPredicate(m, "Select");
            }

            /// <summary>Checks whether the specified method is the IEnumerable.Where() with Func`T,bool.</summary>
            /// <param name="m">Method to check.</param>
            /// <returns>true if this is the method; false otherwise.</returns>
            private static bool IsMethodEnumerableWhere(MethodInfo m)
            {
                return IsNamedMethodSecondArgumentPredicate(m, "Where");
            }

            /// <summary>Checks whether the specified method takes a Func`T,bool as its second argument.</summary>
            /// <param name="m">Method to check.</param>
            /// <param name="name">Expected name of method.</param>
            /// <returns>true if this is the method; false otherwise.</returns>
            private static bool IsNamedMethodSecondArgumentPredicate(MethodInfo m, string name)
            {
                Debug.Assert(m != null, "m != null");
                Debug.Assert(!String.IsNullOrEmpty(name), "!String.IsNullOrEmpty(name)");
                if (m.Name == name)
                {
                    ParameterInfo[] p = m.GetParameters();
                    if (p != null &&
                        p.Length == 2 &&
                        p[0].ParameterType.IsGenericType &&
                        p[1].ParameterType.IsGenericType)
                    {
                        Type functionParameter = p[1].ParameterType;
                        return functionParameter.IsGenericType && functionParameter.GetGenericArguments().Length == 2;
                    }
                }

                return false;
            }

            /// <summary>
            /// Create the accessor expression for a property (both CLR and late-bound)
            /// </summary>
            /// <param name="source">Expression representing the instance on which to access a property</param>
            /// <param name="resourceType">The <see cref="ResourceType"/> of the <paramref name="source"/> on which to access the property.</param>
            /// <param name="property">The <see cref="ResourceProperty"/> to access.</param>
            /// <param name="nullPropagationRequired">If set to true, nulls should be propagated through the property accesses,
            /// this means that if the source is null, the result will evaluate to null without accessing the property.
            /// If set to false the property is accessed directly.</param>
            /// <returns>An expression to access a property (singleton or set). The returned expression will always
            /// have the CLR type of the property being accessed.</returns>
            private static Expression AccessProperty(Expression source, ResourceType resourceType, ResourceProperty property, bool nullPropagationRequired)
            {
                Debug.Assert(source != null, "source != null");
                Debug.Assert(resourceType != null, "resourceType != null");
                Debug.Assert(property != null, "property != null");
                Debug.Assert(resourceType.InstanceType.IsAssignableFrom(source.Type), "Trying to access a property on expression of a wrong type.");

                Expression expression;
                if (property.CanReflectOnInstanceTypeProperty)
                {
                    expression = Expression.Property(source, resourceType.GetPropertyInfo(property));
                }
                else
                {
                    if (property.Kind == ResourcePropertyKind.ResourceSetReference)
                    {
                        MethodInfo getter = DataServiceProviderMethods.GetSequenceValueMethodInfo.MakeGenericMethod(property.ResourceType.InstanceType);
                        expression = Expression.Call(null /* instance */, getter, source, Expression.Constant(property));
                    }
                    else
                    {
                        expression = Expression.Call(
                            null /*instance*/,
                            DataServiceProviderMethods.GetValueMethodInfo,
                            source,
                            Expression.Constant(property));
                        expression = Expression.Convert(expression, property.Type);
                    }
                }

                if (nullPropagationRequired)
                {
                    // If the expression is of a primitive type (for example int)
                    //   we need to change its type to Nullable so that if the source is null
                    //   we can return null.
                    if (!WebUtil.TypeAllowsNull(expression.Type))
                    {
                        expression = Expression.Convert(expression, WebUtil.GetTypeAllowingNull(expression.Type));
                    }

                    expression = Expression.Condition(
                        Expression.Equal(source, Expression.Constant(null, source.Type)),
                        Expression.Constant(null, expression.Type),
                        expression);
                }

                Debug.Assert(property.Type.IsAssignableFrom(expression.Type), "The returned expression is not of the type of the property being accessed.");
                return expression;
            }

            /// <summary>
            /// Create the accessor expression for an open property
            /// </summary>
            /// <param name="source">Expression representing the instance on which to access a property</param>
            /// <param name="propertyName">The name of the property to access.</param>
            /// <returns>An expression to access a property. This always returns an expression of type System.Object.</returns>
            private static Expression AccessOpenProperty(Expression source, string propertyName)
            {
                Debug.Assert(source != null, "source != null");
                Debug.Assert(propertyName != null, "propertyName != null");

                Expression expression = Expression.Call(
                    null /*instance*/,
                    OpenTypeMethods.GetValueOpenPropertyMethodInfo,
                    source,
                    Expression.Constant(propertyName));
                Debug.Assert(expression.Type == typeof(object), "Open properties should always return 'object' type.");

                return expression;
            }

            /// <summary>
            /// Creates a <see cref="MemberAssignment"/> expression that binds an expression to the named property.
            /// </summary>
            /// <param name="type">The type to look for the property on.</param>
            /// <param name="propertyName">Name of property on the element type.</param>
            /// <param name="source">Source expression for property.</param>
            /// <returns>
            /// A <see cref="MemberAssignment"/> expression that binds an expression to the named property.
            /// </returns>
            private static MemberAssignment BindByName(Type type, string propertyName, Expression source)
            {
                Debug.Assert(propertyName != null, "propertyName != null");
                Debug.Assert(source != null, "source != null");
                MemberInfo member = type.GetProperty(propertyName);
                return Expression.Bind(member, source);
            }

            /// <summary>Applies the segment filter to the specified expression.</summary>
            /// <param name="expression">Expression to apply filter on.</param>
            /// <returns>The expression with the filter applied.</returns>
            private Expression ApplySegmentFilter(Expression expression)
            {
                // Collections filter with .Where, and singletons conditionally evaluate to null.
                Type expressionElementType =
                    this.singleResult ?
                    expression.Type :
                    ReflectionServiceProvider.GetIEnumerableElement(expression.Type);

                if (!this.isRoot)
                {
                    if (this.singleResult && this.Node.Filter != null)
                    {
                        LambdaExpression filterLambda = (LambdaExpression)this.Node.Filter;
                        expression = RequestQueryProcessor.ComposePropertyNavigation(
                            expression, filterLambda, this.ExpandProvider.Provider.NullPropagationRequired);
                    }
                    else if (!this.singleResult)
                    {
                        // The filter should be applied before the .Take operation.
                        if (this.Node.Filter != null)
                        {
                            MethodInfo method = ExpandNode.WhereMethodInfoEnumerable.MakeGenericMethod(expressionElementType);
                            Expression[] arguments = new Expression[] { expression, this.Node.Filter };
                            expression = Expression.Call(null, method, arguments);
                        }

                        // If top level MaxResults configuration setting is on, we only request 1 more than max results limit
                        if (this.Node.MaxResultsExpected.HasValue && this.Node.ResourceSetWrapper.PageSize == 0)
                        {
                            MethodInfo method = ExpandNode.TakeMethodInfoEnumerable.MakeGenericMethod(expressionElementType);
                            ConstantExpression count = Expression.Constant((int)this.Node.MaxResultsExpected + 1, typeof(int));
                            Expression[] arguments = new Expression[] { expression, count };
                            expression = Expression.Call(null, method, arguments);
                        }
                    }
                }

                return expression;
            }

            /// <summary>Builds the projection for this segment.</summary>
            /// <param name="expression">Expected expression for this segment if it had no filtering or wrapping.</param>
            /// <returns>The expression for this segment, possibly a filter and/or wrapped version of 
            /// <paramref name="expression"/>.</returns>
            private Expression BuildProjectionExpression(Expression expression)
            {
                Debug.Assert(expression != null, "expression != null");
                expression = this.ApplySegmentFilter(expression);
                expression = this.ApplyOrderTakeOnInnerSegment(expression);

                // If this segment requires a wrapper, project its child properties by passing
                // the expression for the property as the parameter.
                if (this.RequiresWrapper)
                {
                    Type expressionElementType =
                        this.singleResult ?
                        expression.Type :
                        ReflectionServiceProvider.GetIEnumerableElement(expression.Type);

                    Expression parameter = this.singleResult ? expression : Expression.Parameter(expressionElementType, "p");
                    Expression body;
                    if (this.RequiresExpandedWrapper)
                    {
                        body = this.BuildProjectionExpressionForExpandedWrapper(parameter);
                    }
                    else
                    {
                        Debug.Assert(this.RequiresProjectedWrapper, "this.RequiresProjectedWrapper");
                        body = this.BuildProjectionExpressionForProjectedWrapper(parameter);
                    }

                    if (this.singleResult)
                    {
                        expression = body;
                    }
                    else
                    {
                        Expression source = expression;
                        Type[] typeArguments = new Type[] { parameter.Type, body.Type };
                        Type delegateType = typeof(Func<,>).MakeGenericType(typeArguments);
                        LambdaExpression selector = BuildLambdaExpression(delegateType, body, new ParameterExpression[] { (ParameterExpression)parameter });
                        Expression[] arguments = new Expression[] { source, selector };
                        MethodInfo method = ExpandNode.SelectMethodInfoEnumerable.MakeGenericMethod(expressionElementType, this.elementType);
                        expression = Expression.Call(null, method, arguments);
                    }
                }

                return expression;
            }

            /// <summary>Builds projection expression which projects the result as ExpandedWrapper to enable
            /// on-demand expansion of navigation properties.</summary>
            /// <param name="source">Expression which evaluates to the element from which to project.</param>
            /// <returns>Projection expression which projects the result as ExpandedWrapper.</returns>
            private Expression BuildProjectionExpressionForExpandedWrapper(Expression source)
            {
                Debug.Assert(source != null, "source != null");
                Debug.Assert(this.RequiresExpandedWrapper, "this.RequiresExpandedWrapper");

                MemberBinding[] bindings = new MemberBinding[this.children.Count + 2 + (this.needSkipToken ? 1 : 0)];
                bindings[0] = BindByName(
                    this.elementType, 
                    "ExpandedElement", 
                    this.BuildProjectionExpressionForProjectedWrapper(source));
                bindings[1] = BindByName(this.elementType, "Description", Expression.Constant(this.WrapperDescription));
                for (int i = 0; i < this.children.Count; i++)
                {
                    ExpandNode node = this.children[i];

                    // If we need to perform null checks (null propagation is required) we need to do the null check
                    //   on the outer level around any possible Select/OrderBy/ThenBy/Take calls possibly generated
                    //   by the BuildProjectionExpression.
                    Expression propertyAccess = node.BuildProjectionExpression(
                        AccessProperty(
                            source,
                            this.BaseResourceType,
                            node.Node.Property,
                            false));

                    // We need to check for null in cases where the source can be null.
                    // That is if the source represents a single result (in which case it represents a nav. property
                    //   which may be null). It can also happen if the source is the root, in which case URI path
                    //   may have specified single result wrapper in an enumerable, but that result can be null.
                    //   (for example /Customers(0)/BestFriend?$expand...)
                    // Note that it is possible to get null on !singleResult but only on the root level (this.isRoot).
                    //   We don't perform the null check in that case since that is our behavior in V1
                    //   and we want to remain 100% backward compatible.
                    if (this.singleResult && this.ExpandProvider.Provider.NullPropagationRequired)
                    {
                        // Need to make sure the result type is nullable.
                        // But expanded properties are always of entity type or projected/expanded wrappers which all
                        //   are reference types and thus nullables.
                        Debug.Assert(
                            WebUtil.TypeAllowsNull(propertyAccess.Type), 
                            "Expanded property should be projected into a nullable type.");

                        // We are to project a null. Some providers don't support projecting null of arbitrary type 
                        //   (for example Linq to Entities) so we will assume that providers which require null propagation 
                        //   are able to project arbitrary typed nulls and providers which don't want null propagation 
                        //   don't support projecting arbitrary typed nulls.
                        //   In this case the provider already requested null propagation, so it better support projecting
                        //   arbitrary typed null.
                        // We may end up projecting null as a value of ResourceSetReference property. This can in theory break
                        //   the serializers as they expect a non-null (possibly empty) IEnumerable instead. But note that
                        //   if we project null here, we also project null into the ExpandedElement property
                        //   (see the same conditional expression in BuildProjectionExpressionForProjectedWrapper)
                        //   and thus the serializers will recognize this value as null and won't try to expand its properties.
                        //   (Note that there's an assert in the serializers which verifies, that for ResourceSetReference 
                        //   properties they do get non-null IEnumerable if they need to).
                        propertyAccess = Expression.Condition(
                            Expression.Equal(source, Expression.Constant(null, source.Type)),
                            Expression.Constant(null, propertyAccess.Type),
                            propertyAccess);
                    }
                    
                    bindings[i + 2] = BindByName(
                        this.elementType, 
                        "ProjectedProperty" + i.ToString(System.Globalization.CultureInfo.InvariantCulture), 
                        propertyAccess);
                }

                if (this.needSkipToken)
                {
                    Debug.Assert(!this.singleResult, "We don't need sorting for single results");
                    Debug.Assert(this.orderingInfo != null, "Must have ordering info to require skip token");

                    Type projectedSkipTokenType = this.elementType.GetGenericArguments().Skip(this.children.Count + 1).First();

                    MemberBinding[] skipTokenPropertyBindings = new MemberBinding[this.OrderingInfo.OrderingExpressions.Count + 2];
                    skipTokenPropertyBindings[0] = BindByName(projectedSkipTokenType, "ExpandedElement", Expression.Constant((string)null, typeof(string)));
                    StringBuilder skipTokenDescription = new StringBuilder();
                    for (int i = 0; i < this.OrderingInfo.OrderingExpressions.Count; i++)
                    {
                        skipTokenDescription.Append(XmlConstants.SkipTokenPropertyPrefix + i.ToString(System.Globalization.CultureInfo.InvariantCulture)).Append(",");

                        LambdaExpression orderLamba = (LambdaExpression)this.OrderingInfo.OrderingExpressions[i].Expression;
                        Expression orderExpression = ParameterReplacerVisitor.Replace(orderLamba.Body, orderLamba.Parameters[0], source);
                        MemberInfo member = projectedSkipTokenType.GetProperty("ProjectedProperty" + i.ToString(System.Globalization.CultureInfo.InvariantCulture));
                        skipTokenPropertyBindings[i + 2] = Expression.Bind(member, orderExpression);
                    }

                    skipTokenPropertyBindings[1] = BindByName(projectedSkipTokenType, "Description", Expression.Constant(skipTokenDescription.Remove(skipTokenDescription.Length - 1, 1).ToString()));

                    Expression skipTokenBody = Expression.MemberInit(Expression.New(projectedSkipTokenType), skipTokenPropertyBindings);

                    bindings[1] = BindByName(
                        this.elementType, 
                        "Description", 
                        Expression.Constant((this.WrapperDescription.Length == 0 ? "" : (this.WrapperDescription + ",")) + XmlConstants.HttpQueryStringSkipToken));
                    bindings[this.children.Count + 2] = BindByName(this.elementType, "ProjectedProperty" + this.children.Count.ToString(System.Globalization.CultureInfo.InvariantCulture), skipTokenBody);
                }

                return Expression.MemberInit(Expression.New(this.elementType), bindings);
            }

            /// <summary>Builds projection expression which projects all properties which we need
            /// based on active projections for this node. It uses ProjectedWrapper if necessary.</summary>
            /// <param name="source">Expression which evaluates to the element from which to project the properties.</param>
            /// <returns>Projection expression which projects all required properties from the <paramref name="source"/>.</returns>
            private Expression BuildProjectionExpressionForProjectedWrapper(Expression source)
            {
                Debug.Assert(!this.RequiresProjectedWrapper || this.projectedProperties != null, "Must have a list of projected properties if projection wrapper is required.");
                if (!this.RequiresProjectedWrapper)
                {
                    // Project everything - so do not use any wrapper
                    return source;
                }
                else
                {
                    Type projectedWrapperType = ProjectedWrapper.GetProjectedWrapperType(this.projectedProperties.Count);

                    // This holds the expression built so far (for all the types walked so far)
                    Expression expression = null;

                    Debug.Assert(this.resourceTypes != null, "We should have filled a list of resource types already.");
                    Debug.Assert(this.resourceTypes.Count > 0, "We should have at least the base type filled.");

                    // Walk the list of resource types (starting with the base and in the inheritance order)
                    //   and generate the "(resource is Type) ? new ProjectedWrapper() { ... } : nextTypeExpr" expression
                    foreach (ResourceType resourceType in this.resourceTypes)
                    {
                        // Prepare the bindings for the particular type
                        StringBuilder propertyNameList = new StringBuilder();
                        Expression[] bindingExpressions = new Expression[this.projectedProperties.Count + 2];
                        for (int propertyIndex = 0; propertyIndex < this.projectedProperties.Count; propertyIndex++)
                        {
                            ProjectedProperty projectedProperty = this.projectedProperties[propertyIndex];
                            ResourceProperty resourceProperty = resourceType.TryResolvePropertyName(projectedProperty.Name);

                            Expression sourceExpression = Expression.TypeAs(source, resourceType.InstanceType);

                            // Now we need to decide if we are going to project the property or not (for this type).
                            // The rules are:
                            // - If the projectedProperty is for a declared property (Property != null) 
                            //   - If we found "a" declared property of the same name on the current type
                            //     - If the declared properties are the same -> project the property (non-open)
                            //     - Else the properties are different and thus potentially of different types -> don't project
                            //   - Else the type doesn't have such property, so don't project it
                            // - Else the projectedProperty is for an open property (Property == null)
                            //   - If we found "a" declared property of the same name on the current type -> don't project
                            //     we will project that property as a declared property instead (it should be in the list already).
                            //   - Else no such property declared on the type -> project it as an open property.
                            Expression propertyAccess = null;
                            if (projectedProperty.Property != null)
                            {
                                if (resourceProperty != null)
                                {
                                    if (resourceProperty == projectedProperty.Property)
                                    {
                                        // Project as declared property (non-open)
                                        propertyAccess = AccessProperty(sourceExpression, resourceType, resourceProperty, false);

                                        // We must cast to nullable as we don't always project the specified property
                                        //   and thus we must be able to project a null for it instead.
                                        if (!WebUtil.TypeAllowsNull(propertyAccess.Type))
                                        {
                                            propertyAccess = Expression.Convert(propertyAccess, WebUtil.GetTypeAllowingNull(resourceProperty.Type));
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if (resourceProperty == null && resourceType.IsOpenType)
                                {
                                    // Project as open property - if the type is open that is
                                    propertyAccess = AccessOpenProperty(sourceExpression, projectedProperty.Name);
                                }
                            }

                            if (propertyIndex > 0)
                            {
                                propertyNameList.Append(',');
                            }

                            // If we haven't projected anything yet, project null instead
                            if (propertyAccess == null)
                            {
                                if (projectedProperty.Property != null)
                                {
                                    // Strongly typed null if we have a declared property
                                    propertyAccess = Expression.Constant(null, WebUtil.GetTypeAllowingNull(projectedProperty.Property.Type));
                                }
                                else
                                {
                                    // Open type null for open properties
                                    propertyAccess = Expression.Constant(null, typeof(object));
                                }

                                // We haven't projected the property, so don't include it in the property name list (basically mark this
                                //   ProjectedProperty as "not in use").
                            }
                            else
                            {
                                // We have projected the property, so include its name in the projected wrapper's property name list
                                propertyNameList.Append(projectedProperty.Name);
                            }

                            // For some providers we need to cast to object now since we are projecting into a property of type object.
                            if (this.ExpandProvider.castToObject && propertyAccess.Type != typeof(object))
                            {
                                propertyAccess = Expression.Convert(propertyAccess, typeof(object));
                            }

                            bindingExpressions[propertyIndex + 2] = propertyAccess;
                        }

#if DEBUG
                        {
                            // Verification of projections
                            // We should have projected each name exactly once (other occurences of the same name should be not-used slots)
                            string[] propertyNames = propertyNameList.ToString().Split(',');
                            foreach (string projectedPropertyName in projectedProperties.Select(p => p.Name).Distinct(StringComparer.Ordinal))
                            {
                                // It is possible for properties on derived types not to be projected at all
                                // on the base type (for example). That's why it can happen that some names won't be in the list at all.
                                Debug.Assert(propertyNames.Where(name => name == projectedPropertyName).Count() <= 1,
                                    "Each property name to be projected must be listed in the property name list at most once.");
                            }
                        }
#endif

                        // The ResourceTypeName property
                        // If the provider doesn't require null propagation
                        //   it also means (at least we assume) that it can't project arbitrary typed null
                        //   So instead of projecting (ProjectedWrapper)null, we use a small trick where we project
                        //   and empty string as the type name of the ProjectedWrapper. Later during serialization
                        //   we then recognize this and treat such ProjectedWrapper instance as null.
                        if (!this.ExpandProvider.Provider.NullPropagationRequired)
                        {
                            bindingExpressions[0] = Expression.Condition(
                                Expression.Equal(
                                    source,
                                    Expression.Constant(null, source.Type)),
                                Expression.Constant(string.Empty, typeof(string)),
                                Expression.Constant(resourceType.FullName, typeof(string)));
                        }
                        else
                        {
                            bindingExpressions[0] = Expression.Constant(resourceType.FullName, typeof(string));
                        }

                        // The PropertyNameList property
                        bindingExpressions[1] = Expression.Constant(propertyNameList.ToString());

                        Expression initExpression = Expression.MemberInit(
                            Expression.New(projectedWrapperType),
                            ProjectedWrapper.Bind(bindingExpressions, projectedWrapperType));

                        if (expression == null)
                        {
                            expression = initExpression;
                        }
                        else
                        {
                            // Generate the (source is resourceType) ? new ProjectedWrapper() { resourceType... } : expression
                            Expression typeIsExpression = this.ExpandProvider.Provider.IsV1Provider ?
                                (Expression)Expression.TypeIs(source, resourceType.InstanceType) :
                                Expression.Call(
                                    null, 
                                    DataServiceProviderMethods.TypeIsMethodInfo, 
                                    source, 
                                    Expression.Constant(resourceType));
                            Debug.Assert(typeIsExpression.Type == typeof(bool), "The type test expression should be of type bool.");
                            expression = Expression.Condition(typeIsExpression, initExpression, expression);
                        }

                        Debug.Assert(expression.Type == projectedWrapperType, "The projection expression is of a wrong type.");
                    }

                    Debug.Assert(expression != null, "We should have generated some projection expression.");

                    // If this node represents a single result we need to make sure that we project "null"
                    //   if the source of the projection was null
                    if (this.ExpandProvider.Provider.NullPropagationRequired)
                    {
                        Debug.Assert(WebUtil.TypeAllowsNull(expression.Type), "We should only project types which are nullable.");
                        expression = Expression.Condition(
                            Expression.Equal(source, Expression.Constant(null, source.Type)),
                            Expression.Constant(null, expression.Type),
                            expression);
                    }

                    return expression;
                }
            }

            /// <summary>
            /// Applies OrderBy/ThenBy and Take on inner segments
            /// </summary>
            /// <param name="expression">Input expression</param>
            /// <returns>Output expression with OrderBy/Take possibly added to it</returns>
            private Expression ApplyOrderTakeOnInnerSegment(Expression expression)
            {
                if (this.NeedsStandardPaging)
                {
                    // Since not a single result, infer element type from enumerable
                    Type expressionElementType = ReflectionServiceProvider.GetIEnumerableElement(expression.Type);

                    ParameterExpression parameter = Expression.Parameter(expressionElementType, "p");

                    bool first = true;

                    // Order the segment based on OrderingInfo provided in ExpandSegment.
                    Debug.Assert(this.OrderingInfo.IsPaged, "Paging should be enabled for current segment.");
                    foreach (OrderingExpression oi in this.OrderingInfo.OrderingExpressions)
                    {
                        string chosenFunction = first == true ? (oi.IsAscending ? "OrderBy" : "OrderByDescending") : (oi.IsAscending ? "ThenBy" : "TheyByDescending");
                        LambdaExpression orderLambda = (LambdaExpression)oi.Expression;

                        Expression updatedOrderingBody = ParameterReplacerVisitor.Replace(
                            orderLambda.Body,
                            orderLambda.Parameters[0],
                            parameter);

                        expression = Expression.Call(
                                        typeof(Enumerable),
                                        chosenFunction,
                                        new Type[] { expressionElementType, updatedOrderingBody.Type },
                                        expression,
                                        Expression.Lambda(updatedOrderingBody, parameter));
                        first = false;
                    }

                    // Ask for PageSize number of elements.
                    Debug.Assert(this.Node.MaxResultsExpected.HasValue, "Paging is on for this node, so it should have the MaxResultsExpected set as well.");
                    return Expression.Call(
                                    typeof(Enumerable),
                                    "Take",
                                    new Type[] { expressionElementType },
                                    expression,
                                    Expression.Constant(this.Node.MaxResultsExpected, typeof(int)));
                }

                return expression;
            }

            /// <summary>Sets the expanded wrapper element type on this node.</summary>
            private void SetWrapperElementType()
            {
                // Build a wrapper of the required cardinality.
                Debug.Assert(this.RequiresWrapper, "this.RequiresWrapper -- otherwise no need to call SetWrapperElementType");

                if (this.RequiresExpandedWrapper)
                {
                    // 1 ExpandedElement type + * each expanded child's type + 1? $skiptoken type
                    Type[] typeArguments = new Type[1 + this.children.Count + (this.needSkipToken ? 1 : 0)];
                    int typeArgumentIndex = 0;

                    typeArguments[typeArgumentIndex++] = this.RequiresProjectedWrapper ? this.GetProjectedWrapperType() : this.enumeratedType;

                    for (int i = 0; i < this.children.Count; i++)
                    {
                        typeArguments[typeArgumentIndex++] = this.children[i].ProjectedType;
                    }

                    if (this.needSkipToken)
                    {
                        Debug.Assert(this.OrderingInfo != null, "Must have ordering info to require a skip token");

                        Type[] skipTokenTypes = new Type[this.OrderingInfo.OrderingExpressions.Count + 1];

                        // We don't use the ExpandedElement on the $skiptoken value, 
                        // so we'll project a null string always (roundtrips better then null object)
                        skipTokenTypes[0] = typeof(string);  
                        for (int i = 0; i < this.OrderingInfo.OrderingExpressions.Count; i++)
                        {
                            skipTokenTypes[i + 1] = ((LambdaExpression)this.OrderingInfo.OrderingExpressions[i].Expression).Body.Type;
                        }

                        typeArguments[typeArgumentIndex++] = WebUtil.GetWrapperType(skipTokenTypes, Strings.BasicExpandProvider_SDP_UnsupportedOrderingExpressionBreadth);
                    }

                    this.elementType = WebUtil.GetWrapperType(typeArguments, Strings.BasicExpandProvider_UnsupportedExpandBreadth);
                }
                else
                {
                    this.elementType = this.GetProjectedWrapperType();
                }
            }

            /// <summary>Returns the projected wrapper element type on this node.</summary>
            /// <returns>The type of the projected wrapper used for this node.</returns>
            private Type GetProjectedWrapperType()
            {
                Debug.Assert(this.RequiresProjectedWrapper, "The node must have some projections for it to use projected wrapper.");
                return ProjectedWrapper.GetProjectedWrapperType(this.projectedProperties.Count);
            }
        }

        #region ExpandNodeAnnotationVisitor
        /// <summary>Expression tree visitor which annotates the tree with <see cref="ExpandNode"/>s.</summary>
        /// <remarks>Given expression is annotated with <see cref="ExpandNode"/> if it evaluates
        /// to an instance of the resource which belongs to the resource set represented by the <see cref="ExpandNode"/>.
        /// For example in query /Customers?$expand=BestFriend all expressions which evaluate to
        /// customer.BestFriend will be annotated with the <see cref="ExpandNode"/> representing the expanded
        /// BestFriend nav. property.
        /// This visitor doesn't modify the expression tree at all.</remarks>
        internal class ExpandNodeAnnotationVisitor : PropertyAccessVisitor
        {
            /// <summary>The parameter expression which is the parameter for the expression tree and evaluates to
            /// the resource instances.</summary>
            private ParameterExpression parameter;

            /// <summary><see cref="ExpandNode"/> into which belongs the resource which the <see cref="parameter"/>
            /// evaluates to.</summary>
            private ExpandNode parameterExpandNode;

            /// <summary>The created annotations.</summary>
            /// <remarks>Annotations are stored as a dictionary where the expression is the key and the value is the
            /// <see cref="ExpandNode"/>.
            /// Note that since this annotation is entirely computed out of the subtree and not from the parents
            /// of the expression subtree it is safe to annotate like this.
            /// Even if a given expression is referenced multiple times from the same tree its annotation
            /// will be the same in all cases.</remarks>
            private Dictionary<Expression, ExpandNode> expandNodeAnnotations;

            /// <summary>Constructor</summary>
            /// <param name="parameter">The parameter expression which evaluates to the resource.</param>
            /// <param name="parameterExpandNode">The <see cref="ExpandNode"/> into which the resource from 
            /// <paramref name="parameter"/> belongs to.</param>
            private ExpandNodeAnnotationVisitor(ParameterExpression parameter, ExpandNode parameterExpandNode)
            {
                this.parameter = parameter;
                this.parameterExpandNode = parameterExpandNode;
                this.expandNodeAnnotations = new Dictionary<Expression, ExpandNode>(ReferenceEqualityComparer<Expression>.Instance);
            }

            /// <summary>Annotates specified expression with the respective <see cref="ExpandNode"/>s.</summary>
            /// <param name="expression">The expression to annotate.</param>
            /// <param name="parameter">The parameter expression which evaluates to the base resource instance.</param>
            /// <param name="parameterExpandNode">The <see cref="ExpandNode"/> which represents the base resource set.</param>
            /// <returns>Annotations stored as a dictionary.</returns>
            /// <remarks>The expression is not modified at all.</remarks>
            internal static Dictionary<Expression, ExpandNode> AnnotateExpression(
                Expression expression, 
                ParameterExpression parameter, 
                ExpandNode parameterExpandNode)
            {
                ExpandNodeAnnotationVisitor visitor = new ExpandNodeAnnotationVisitor(parameter, parameterExpandNode);
                visitor.Visit(expression);
                return visitor.expandNodeAnnotations;
            }

            /// <summary>Visits a conditional node in the expression tree.</summary>
            /// <param name="c">The conditional expression to process.</param>
            /// <returns>Possibly new conditional expression with which to replace the <paramref name="c"/> in the tree with.</returns>
            /// <remarks>This class needs to override this method since it needs to "merge" the <see cref="ExpandNode"/> annotations
            /// from the true and false branches of the conditional expression.
            /// It assumes that only one branch will actually have some annotation.</remarks>
            internal override Expression VisitConditional(ConditionalExpression c)
            {
                base.VisitConditional(c);
                ExpandNode trueAnnotation = this.GetExpandNodeAnnotation(c.IfTrue);
                ExpandNode falseAnnotation = this.GetExpandNodeAnnotation(c.IfFalse);

                if (trueAnnotation == null || falseAnnotation == null)
                {
                    if (trueAnnotation != null || falseAnnotation != null)
                    {
                        this.SetExpandNodeAnnotation(c, trueAnnotation != null ? trueAnnotation : falseAnnotation);
                    }
                }
                else
                {
                    Debug.Assert(trueAnnotation == falseAnnotation, "True and False branches of a conditional expression have different asigned expand nodes.");
                }

                return c;
            }

            /// <summary>UnaryExpression visit method</summary>
            /// <param name="u">The UnaryExpression expression to visit</param>
            /// <returns>The visited UnaryExpression expression </returns>
            /// <remarks>This method simply propagates the annotation from the operand of the unary expression
            /// to the unary expression itself.</remarks>
            internal override Expression VisitUnary(UnaryExpression u)
            {
                base.VisitUnary(u);

                switch (u.NodeType)
                {
                    case ExpressionType.Convert:
                    case ExpressionType.ConvertChecked:
                    case ExpressionType.Quote:
                    case ExpressionType.TypeAs:
                        ExpandNode operandAnnotation = this.GetExpandNodeAnnotation(u.Operand);
                        if (operandAnnotation != null)
                        {
                            this.SetExpandNodeAnnotation(u, operandAnnotation);
                        }

                        break;
                    default:
                        break;
                }

                return u;
            }

            /// <summary>ParameterExpression visit method</summary>
            /// <param name="p">The ParameterExpression expression to visit</param>
            /// <returns>The visited ParameterExpression expression </returns>
            internal override Expression VisitParameter(ParameterExpression p)
            {
                if (p == this.parameter)
                {
                    this.SetExpandNodeAnnotation(p, this.parameterExpandNode);
                }

                return p;
            }

            /// <summary>Dervied class will override them method to process any property accesses found in the tree.</summary>
            /// <param name="propertyName">The name of the property being accessed.</param>
            /// <param name="operandExpression">The expression on which the property is being accessed.
            /// The implementation may choose to return a different expression through this ref parameter.
            /// If the method returns true, the <paramref name="accessExpression"/> is null and the method
            /// changed this parameter, the caller will replace the operand in the original property
            /// access with the new expression provided in this parameter. The way the property is accessed
            /// and its name remains the same.</param>
            /// <param name="accessExpression">The entire expression of the property access.
            /// The implementation may choose to return a different expression through this ref parameter.
            /// If the method returns true and this parameter is not null the caller will replace the entire
            /// property access expression with the new one passed in this parameter.</param>
            /// <returns>If the method returns false it means that it is not interested in this property access,
            /// and the processing of the tree will continue by examining the children of the property access expression.
            /// If the method returns true the caller looks at the returned value of <paramref name="accessExpression"/>.
            /// If it is not-null it will replace the entire property access expression with it.
            /// If it's null it will just replace the operand of the property access with the <paramref name="operandExpression"/>.
            /// If the implementation wants to skip this property access without modification it should return true
            /// and not modify the ref parameters.</returns>
            /// <remarks>If the method returns true the caller will not continue walking the children of the property
            /// access expression. It's the responsibility of the implementation to do so if it requires such
            /// functionality.</remarks>
            protected override bool ProcessPropertyAccess(string propertyName, ref Expression operandExpression, ref Expression accessExpression)
            {
                this.Visit(operandExpression);

                ExpandNode operandExpandNode = this.GetExpandNodeAnnotation(operandExpression);
                if (operandExpandNode != null)
                {
                    ExpandNode childExpandNode = operandExpandNode.FindChild(propertyName);
                    if (childExpandNode != null)
                    {
                        this.SetExpandNodeAnnotation(accessExpression, childExpandNode);
                    }
                }

                return true;
            }

            /// <summary>Helper method to determine annotation for a given expression.</summary>
            /// <param name="expression">Expression to determine annotation for.</param>
            /// <returns>Annotation for the <paramref name="expression"/> or null if no annotation was found.</returns>
            private ExpandNode GetExpandNodeAnnotation(Expression expression)
            {
                ExpandNode annotation;
                if (this.expandNodeAnnotations.TryGetValue(expression, out annotation))
                {
                    return annotation;
                }
                else
                {
                    return null;
                }
            }

            /// <summary>Helper method to set an annotation for a given expression.</summary>
            /// <param name="expression">Expression to set an annotation on.</param>
            /// <param name="annotation">The annotation to set.</param>
            private void SetExpandNodeAnnotation(Expression expression, ExpandNode annotation)
            {
#if DEBUG
                ExpandNode existingAnnotation;
                this.expandNodeAnnotations.TryGetValue(expression, out existingAnnotation);
                Debug.Assert(existingAnnotation == null || existingAnnotation == annotation, "Same expression can't have two different assigned expand nodes.");
#endif
                this.expandNodeAnnotations[expression] = annotation;
            }
        }
        #endregion

        #region AddPropertyAccessesAsProjectedPropertiesVisitor
        /// <summary>Expression tree visitor which finds all property accesses and marks the affected properties
        /// for projection.</summary>
        /// <remarks>The visitor doesn't change the expression tree at all.
        /// Visitor recognizes all ways a WCF Data Services uses to access properties.</remarks>
        internal class AddPropertyAccessesAsProjectedPropertiesVisitor : PropertyAccessVisitor
        {
            /// <summary>Annotations for the specified expression tree.</summary>
            private Dictionary<Expression, ExpandNode> expandNodeAnnotations;

            /// <summary>Constructor</summary>
            /// <param name="expandNodeAnnotations">Annotation to use.</param>
            private AddPropertyAccessesAsProjectedPropertiesVisitor(
                Dictionary<Expression, ExpandNode> expandNodeAnnotations)
            {
                this.expandNodeAnnotations = expandNodeAnnotations;
            }

            /// <summary>Inspects the specified <paramref name="expression"/> using the <paramref name="expandNodeAnnotations"/>
            /// and adds all accessed properties to the list of candidates for projection on their respective
            /// <see cref="ExpandNode"/>s.</summary>
            /// <param name="expression">The expression to inspect.</param>
            /// <param name="expandNodeAnnotations">Annotation for the <paramref name="expression"/> computed
            /// as a result of <see cref="ExpandNodeAnnotationVisitor.AnnotateExpression"/>.</param>
            internal static void AddPropertyAccessesAsProjectedProperties(
                Expression expression,
                Dictionary<Expression, ExpandNode> expandNodeAnnotations)
            {
                AddPropertyAccessesAsProjectedPropertiesVisitor visitor =
                    new AddPropertyAccessesAsProjectedPropertiesVisitor(expandNodeAnnotations);
                visitor.Visit(expression);
            }

            /// <summary>Dervied class will override them method to process any property accesses found in the tree.</summary>
            /// <param name="propertyName">The name of the property being accessed.</param>
            /// <param name="operandExpression">The expression on which the property is being accessed.
            /// The implementation may choose to return a different expression through this ref parameter.
            /// If the method returns true, the <paramref name="accessExpression"/> is null and the method
            /// changed this parameter, the caller will replace the operand in the original property
            /// access with the new expression provided in this parameter. The way the property is accessed
            /// and its name remains the same.</param>
            /// <param name="accessExpression">The entire expression of the property access.
            /// The implementation may choose to return a different expression through this ref parameter.
            /// If the method returns true and this parameter is not null the caller will replace the entire
            /// property access expression with the new one passed in this parameter.</param>
            /// <returns>If the method returns false it means that it is not interested in this property access,
            /// and the processing of the tree will continue by examining the children of the property access expression.
            /// If the method returns true the caller looks at the returned value of <paramref name="accessExpression"/>.
            /// If it is not-null it will replace the entire property access expression with it.
            /// If it's null it will just replace the operand of the property access with the <paramref name="operandExpression"/>.
            /// If the implementation wants to skip this property access without modification it should return true
            /// and not modify the ref parameters.</returns>
            /// <remarks>If the method returns true the caller will not continue walking the children of the property
            /// access expression. It's the responsibility of the implementation to do so if it requires such
            /// functionality.</remarks>
            protected override bool ProcessPropertyAccess(string propertyName, ref Expression operandExpression, ref Expression accessExpression)
            {
                this.Visit(operandExpression);

                ExpandNode operandExpandNode;
                if (this.expandNodeAnnotations.TryGetValue(operandExpression, out operandExpandNode))
                {
                    ResourceProperty property = operandExpandNode.BaseResourceType.TryResolvePropertyName(propertyName);
                    operandExpandNode.AddProjectedProperty(propertyName, property);
                }

                return true;
            }
        }
        #endregion

        #endregion Inner types.
    }
}
