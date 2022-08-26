//---------------------------------------------------------------------
// <copyright file="ProjectedWrapper.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a base class implementing IProjectedResult over 
//      projections.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Internal
{
    using System.Collections;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;

    /// <summary>Base class for all projected wrappers. The internal implementation of the <see cref="IProjectedResult"/>
    /// interface. We use this and the other ProjectedWrapper classes to project a subset of properties
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public abstract class ProjectedWrapper : IProjectedResult
    {
        #region Fields.
        /// <summary>Array of predefined projected wrappers for small number of properties. When we need to project
        /// more properties than these allow we will use the <see cref="ProjectedWrapperMany"/>.</summary>
        private static Type[] precreatedProjectedWrapperTypes = new Type[]
        {
            typeof(ProjectedWrapper0),
            typeof(ProjectedWrapper1),
            typeof(ProjectedWrapper2),
            typeof(ProjectedWrapper3),
            typeof(ProjectedWrapper4),
            typeof(ProjectedWrapper5),
            typeof(ProjectedWrapper6),
            typeof(ProjectedWrapper7),
            typeof(ProjectedWrapper8)
        };

        /// <summary>Array of projected propery names used for fast lookup based on the projected property index.</summary>
        private static string[] projectedPropertyNames = new string[]
        {
            "ProjectedProperty0",
            "ProjectedProperty1",
            "ProjectedProperty2",
            "ProjectedProperty3",
            "ProjectedProperty4",
            "ProjectedProperty5",
            "ProjectedProperty6",
            "ProjectedProperty7"
        };

        /// <summary>The full name of the type</summary>
        private string resourceTypeName;

        /// <summary>Text list of property names, in comma-separated format.</summary>
        /// <remarks>The position of each property name determines the index of the projected property
        /// which holds the value of that property.
        /// Some slot may be empty denoting that the projected property is not used for this instance.</remarks>
        private string propertyNameList;

        /// <summary>Parsed list property names.</summary>
        private string[] propertyNames;

        #endregion Fields.

        #region Public properties.

        /// <summary>The full name of the <see cref="System.Data.Services.Providers.ResourceType"/> which represents the type
        /// of this result.</summary>
        public string ResourceTypeName
        {
            get
            {
                return this.resourceTypeName;
            }

            set
            {
                Debug.Assert(value != null, "value != null");
                this.resourceTypeName = value;
            }
        }

        /// <summary>Text list of property names, in comma-separated format.</summary>
        public string PropertyNameList
        {
            get
            {
                return this.propertyNameList;
            }

            set
            {
                this.propertyNameList = WebUtil.CheckArgumentNull(value, "value");
                this.propertyNames = WebUtil.StringToSimpleArray(this.propertyNameList);
            }
        }

        #endregion Public properties.

        #region Methods.

        /// <summary>Gets the value for named property for the result.</summary>
        /// <param name="propertyName">Name of property for which to get the value.</param>
        /// <returns>The value for the named property of the result.</returns>
        public object GetProjectedPropertyValue(string propertyName)
        {
            WebUtil.CheckArgumentNull(propertyName, "propertyName");

            if (this.propertyNames == null)
            {
                throw new InvalidOperationException(Strings.BasicExpandProvider_ProjectedPropertiesNotInitialized);
            }

            int nameIndex = -1;
            for (int i = 0; i < this.propertyNames.Length; i++)
            {
                if (this.propertyNames[i] == propertyName)
                {
                    nameIndex = i;
                    break;
                }
            }

            return this.InternalGetProjectedPropertyValue(nameIndex);
        }

        /// <summary>Returns the type of the <see cref="ProjectedWrapper"/> with the specified number of projected properties.</summary>
        /// <param name="projectedPropertyCount">The number of properties to project in the wrapper.</param>
        /// <returns>The type of the projected wrapper to use (note that it might have room for more properties!)</returns>
        internal static Type GetProjectedWrapperType(int projectedPropertyCount)
        {
            if (projectedPropertyCount < precreatedProjectedWrapperTypes.Length)
            {
                return precreatedProjectedWrapperTypes[projectedPropertyCount];
            }
            else
            {
                return typeof(ProjectedWrapperMany);
            }
        }

        /// <summary>Creates an array of <see cref="MemberBinding"/> objects which bind the projected properties
        /// to the expressions passed in <paramref name="bindingExpressions"/>.</summary>
        /// <param name="bindingExpressions">Array of expressions to bind to properties on the projected wrapper.
        /// The expression at index 0 will be bound to the ResourceTypeName property.
        /// The expression at index 1 will be bound to the PropertyNameList property.
        /// The expression at index 2 + i will be bound to the ith projected property.</param>
        /// <param name="projectedWrapperType">The type of the projected wrapper to use. You should get this
        /// by calling the <see cref="ProjectedWrapper.GetProjectedWrapperType"/> method.</param>
        /// <returns>An array of bindings which bind the specified expression to the properties on the projected wrapper.</returns>
        internal static MemberBinding[] Bind(Expression[] bindingExpressions, Type projectedWrapperType)
        {
            Debug.Assert(bindingExpressions != null, "bindingExpression != null");
            Debug.Assert(projectedWrapperType != null, "projectedWrapperType != null");
            Debug.Assert(bindingExpressions.Length >= 2, "At least the ResourceTypeName and PropertyNameList properties must be bound.");
            Debug.Assert(
                GetProjectedWrapperType(bindingExpressions.Length - 2) == projectedWrapperType, 
                "The projected wrapper type will not fit the required number of properties.");

            int bindingsCount = bindingExpressions.Length;
            MemberBinding[] bindings;
            // The number of precreate types - 1 (they start with 0 properties) + ResourceTypeName + Description
            if (bindingsCount <= precreatedProjectedWrapperTypes.Length + 1)
            {
                // The number of properties fits into a single projected wrapper instance
                bindings = new MemberBinding[bindingsCount];
                BindResourceTypeAndPropertyNameList(projectedWrapperType, bindings, bindingExpressions);
                for (int propertyIndex = 0; propertyIndex < bindingsCount - 2; propertyIndex++)
                {
                    bindings[propertyIndex + 2] = BindToProjectedProperty(
                        projectedWrapperType,
                        propertyIndex,
                        bindingExpressions[propertyIndex + 2]);
                }
            }
            else
            {
                // The number of properties is too big - we need to use the linked list of ProjectedWrapperMany instances
                // So we return the number of properties we can bind + ResourceTypeName + Description + Next
                //   the rest of the bindings will be bound to the Next
                bindings = new MemberBinding[precreatedProjectedWrapperTypes.Length + 2];

                // Only fill the ResourceTypeName and Description on the first ProjectedWrapperMany
                //   the inner ones will never use it.
                BindResourceTypeAndPropertyNameList(projectedWrapperType, bindings, bindingExpressions);

                // And now call the recursive method to fill the rest.
                BindToProjectedWrapperMany(bindingExpressions, 2, bindings, 2);
            }

            return bindings;
        }

        /// <summary>If the specified resource is <see cref="IEnumerable"/> this method
        /// will returned a wrapped instance which will turn all special "null" instances of ProjectedWrapper
        /// in the enumeration results into the true null values. Otherwise this method simply
        /// returns the resource untouched.</summary>
        /// <param name="resource">The resource to wrap.</param>
        /// <returns>The original resource or wrapped resource if it was <see cref="IEnumerable"/>.</returns>
        /// <remarks>Note that we don't expect that the enumeration of results might return result
        /// which itself will be enumeration (nested enumerations). We handle this case through the
        /// ExpandedWrapper instance instead.</remarks>
        internal static object ProcessResultEnumeration(object resource)
        {
            IEnumerable enumerable;
            if (WebUtil.IsElementIEnumerable(resource, out enumerable))
            {
                return new EnumerableWrapper(enumerable);
            }
            else
            {
                return resource;
            }
        }

        /// <summary>Helper method which checks the specified resource and if it is the special
        /// "null" ProjectedWrapper value it will turn it into a true null. Otherwise it returns the original value.</summary>
        /// <param name="resource">The resource to check for nullness.</param>
        /// <returns>The original value, or null if the value was representing null value.</returns>
        internal static object ProcessResultInstance(object resource)
        {
            ProjectedWrapper projectedWrapper = resource as ProjectedWrapper;
            if (projectedWrapper != null && string.IsNullOrEmpty(projectedWrapper.resourceTypeName))
            {
                return null;
            }
            else
            {
                return resource;
            }
        }

        /// <summary>Unwraps <see cref="IEnumerator"/> which might be wrapped to report null values correctly.
        /// If the input is not wrapped it returns the original enumerator.</summary>
        /// <param name="enumerator">The enumerator to unwrap.</param>
        /// <returns>The unwrapped enumerator.</returns>
        internal static IEnumerator UnwrapEnumerator(IEnumerator enumerator)
        {
            EnumeratorWrapper wrapper = enumerator as EnumeratorWrapper;
            if (wrapper != null)
            {
                enumerator = wrapper.InnerEnumerator;
            }

            return enumerator;
        }

        /// <summary>Returns a wrapping <see cref="IQueryable"/> which will turn all special "null" instances
        /// of ProjectedWrapper in the enumeration results into true null values.</summary>
        /// <param name="queryable">The <see cref="IQueryable"/> to wrap.</param>
        /// <returns>Newly created wrapped for the specified <paramref name="queryable"/>.</returns>
        internal static IQueryable WrapQueryable(IQueryable queryable)
        {
            Debug.Assert(queryable != null, "queryable != null");
            return new QueryableWrapper(queryable);
        }

        /// <summary>Gets the value for the property specified by its index.</summary>
        /// <param name="propertyIndex">Index of the property for which to get the value.</param>
        /// <returns>The value for the property.</returns>
        protected abstract object InternalGetProjectedPropertyValue(int propertyIndex);

        /// <summary>Binds the ResourceTypeName and PropertyNameList properties to the first two expressions.</summary>
        /// <param name="projectedWrapperType">The type of the projected wrapper to bind to.</param>
        /// <param name="bindings">Items 0 and 1 will be filled with the bindings in this array.</param>
        /// <param name="bindingExpressions">The expressions to bind - only items 0 and 1 are used.</param>
        private static void BindResourceTypeAndPropertyNameList(
            Type projectedWrapperType, 
            MemberBinding[] bindings, 
            Expression[] bindingExpressions)
        {
            Debug.Assert(bindings.Length >= 2, "Must have at least two bindings.");
            Debug.Assert(bindingExpressions.Length >= 2, "Must have at least two expressions to bind.");
            Debug.Assert(typeof(ProjectedWrapper).IsAssignableFrom(projectedWrapperType), "Can't bind to type which is not a projected wrapper.");

            bindings[0] = Expression.Bind(projectedWrapperType.GetProperty("ResourceTypeName"), bindingExpressions[0]);
            bindings[1] = Expression.Bind(projectedWrapperType.GetProperty("PropertyNameList"), bindingExpressions[1]);
        }

        /// <summary>Binds specified expression to a projected propety of a given index.</summary>
        /// <param name="projectedWrapperType">The type of the projected wrapper to bind to.</param>
        /// <param name="propertyIndex">The index of the projected property to bind to.</param>
        /// <param name="expression">The expression to bind to the property.</param>
        /// <returns>The newly create binding expression.</returns>
        private static MemberAssignment BindToProjectedProperty(Type projectedWrapperType, int propertyIndex, Expression expression)
        {
            Debug.Assert(
                typeof(ProjectedWrapper).IsAssignableFrom(projectedWrapperType), 
                "Trying to bind to a type which is not a projected wrapper.");
            Debug.Assert(
                propertyIndex < projectedPropertyNames.Length, 
                "Trying to bind to a too big property index.");

            return Expression.Bind(
                projectedWrapperType.GetProperty(projectedPropertyNames[propertyIndex]),
                expression);
        }

        /// <summary>Binds projected epxressions to the <see cref="ProjectedWrapperMany"/> object.</summary>
        /// <param name="bindingExpressions">Array of expressions to bind.</param>
        /// <param name="expressionStartIndex">Index of the first expression in the <paramref name="bindingExpressions"/> to bind.</param>
        /// <param name="bindings">Array to fill with the bindings.</param>
        /// <param name="bindingStartIndex">Index of the first slot in <paramref name="bindings"/> to fill.</param>
        private static void BindToProjectedWrapperMany(
            Expression[] bindingExpressions, 
            int expressionStartIndex, 
            MemberBinding[] bindings, 
            int bindingStartIndex)
        {
            Debug.Assert(
                bindings.Length - bindingStartIndex == precreatedProjectedWrapperTypes.Length, 
                "We need space in the bindings to bind all the properties.");
            Debug.Assert(bindingExpressions.Length > expressionStartIndex, "At least one expression to bind.");

            int propertyIndex = 0;
            for (; propertyIndex < precreatedProjectedWrapperTypes.Length - 1 && propertyIndex + expressionStartIndex < bindingExpressions.Length; propertyIndex++)
            {
                bindings[bindingStartIndex + propertyIndex] = BindToProjectedProperty(
                    typeof(ProjectedWrapperMany), 
                    propertyIndex, 
                    bindingExpressions[expressionStartIndex + propertyIndex]);
            }

            if (bindingExpressions.Length > precreatedProjectedWrapperTypes.Length - 1 + expressionStartIndex)
            {
                int nextCount = bindingExpressions.Length - (precreatedProjectedWrapperTypes.Length - 1 + expressionStartIndex);
                if (nextCount > precreatedProjectedWrapperTypes.Length - 1)
                {
                    // If we're not going to fit into the next wrapper, we need one more for the Next property
                    nextCount = precreatedProjectedWrapperTypes.Length;
                }

                // We need more still - so create the next link on the Next property
                MemberBinding[] nextBindings = new MemberBinding[precreatedProjectedWrapperTypes.Length + 2];
                nextBindings[0] = Expression.Bind(
                    typeof(ProjectedWrapperMany).GetProperty("ResourceTypeName"), 
                    Expression.Constant(string.Empty, typeof(string)));
                nextBindings[1] = Expression.Bind(
                    typeof(ProjectedWrapperMany).GetProperty("PropertyNameList"),
                    Expression.Constant(string.Empty, typeof(string)));
                BindToProjectedWrapperMany(
                    bindingExpressions,
                    expressionStartIndex + precreatedProjectedWrapperTypes.Length - 1,
                    nextBindings,
                    2);
                Expression restProjectedWrapper = Expression.MemberInit(
                    Expression.New(typeof(ProjectedWrapperMany)),
                    nextBindings);
                bindings[bindingStartIndex + precreatedProjectedWrapperTypes.Length - 1] = Expression.Bind(
                    typeof(ProjectedWrapperMany).GetProperty("Next"),
                    restProjectedWrapper);
            }
            else
            {
                // This is the last one. We need to project something to the remaining properties
                //   and then the special End instance to the Next property.
                //   This is necessary to make Linq to Entities work as it fails when the same type is initialized
                //   multiple times in the same query and the initializions don't set the same properties in the same order.
                for (; propertyIndex < precreatedProjectedWrapperTypes.Length - 1; propertyIndex++)
                {
                    // Project empty strings for example (as they are very likely to easily round trip through and query system)
                    bindings[bindingStartIndex + propertyIndex] = BindToProjectedProperty(
                            typeof(ProjectedWrapperMany),
                            propertyIndex, 
                            Expression.Constant(string.Empty, typeof(string)));
                }

                // And then project the special End instance to the Next property
                //   we need to project something into the Next property, but we can't project ProjectedWrapperMany
                //   as Linq to Entities would require us to initialize all its properties (and thus we would end up in infinite loop).
                // Instead it is OK to project a different type (Since we never projected that one in this query yet).
                // Note that we need to initialize at least one property on the special type to avoid a problem
                // in Linq to Entities where it might fails with null-ref exception.
                bindings[bindingStartIndex + precreatedProjectedWrapperTypes.Length - 1] = Expression.Bind(
                    typeof(ProjectedWrapperMany).GetProperty("Next"),
                    Expression.MemberInit(
                        Expression.New(typeof(ProjectedWrapperManyEnd)),
                        Expression.Bind(
                            typeof(ProjectedWrapperManyEnd).GetProperty("ResourceTypeName"),
                            Expression.Constant(string.Empty, typeof(string)))));
            }
        }

        #endregion Methods.

        #region Enumeration wrappers
        /// <summary>Wrapper around <see cref="IQueryable"/> which replaces special "null" instances
        /// of ProjectedWrapper in the results with true null values.</summary>
        private sealed class QueryableWrapper : EnumerableWrapper, IQueryable
        {
            /// <summary>The <see cref="IQueryable"/> this object is wrapping.</summary>
            private readonly IQueryable queryable;

            /// <summary>Constructor.</summary>
            /// <param name="queryable">The queryable to wrap.</param>
            internal QueryableWrapper(IQueryable queryable) : base(queryable)
            {
                Debug.Assert(queryable != null, "queryable != null");
                this.queryable = queryable;
            }

            /// <summary>The type of the single element which is returned as a result of the query.</summary>
            public Type ElementType
            {
                get { return this.queryable.ElementType; }
            }

            /// <summary>The expression tree for this query.</summary>
            public Expression Expression
            {
                get { return this.queryable.Expression; }
            }

            /// <summary>The query provider - not support as it should never be called.</summary>
            public IQueryProvider Provider
            {
                get { throw Error.NotSupported(); }
            }
        }

        /// <summary>Wrapper around <see cref="IEnumerable"/> which replaces special "null" instances
        /// of ProjectedWrapper in the results with true null values.</summary>
        private class EnumerableWrapper : IEnumerable
        {
            /// <summary>The <see cref="IEnumerable"/> this object is wrapping.</summary>
            private readonly IEnumerable enumerable;

            /// <summary>Constructor.</summary>
            /// <param name="enumerable">The enumerable to wrap.</param>
            internal EnumerableWrapper(IEnumerable enumerable)
            {
                Debug.Assert(enumerable != null, "enumerable != null");
                this.enumerable = enumerable;
            }

            /// <summary>Gets a new enumerator.</summary>
            /// <returns>The newly created <see cref="IEnumerator"/>.</returns>
            IEnumerator IEnumerable.GetEnumerator()
            {
                return this.GetEnumerator();
            }

            /// <summary>Gets a new enumerator.</summary>
            /// <returns>The newly created <see cref="IEnumerator"/>.</returns>
            public IEnumerator GetEnumerator()
            {
                return new EnumeratorWrapper(this.enumerable.GetEnumerator());
            }
        }

        /// <summary>Wrapper around <see cref="IEnumerator"/> which replaces special "null" instances
        /// of ProjectedWrapper in the results with true null values.</summary>
        private sealed class EnumeratorWrapper : IEnumerator, IDisposable
        {
            /// <summary>The <see cref="IEnumerator"/> this object is wrapping.</summary>
            private readonly IEnumerator enumerator;

            /// <summary>Constructor.</summary>
            /// <param name="enumerator">The enumerator to wrap.</param>
            internal EnumeratorWrapper(IEnumerator enumerator)
            {
                Debug.Assert(enumerator != null, "enumerator != null");
                this.enumerator = enumerator;
            }

            /// <summary>Returns the current result on which the enumerator is positioned.</summary>
            public object Current
            {
                get { return ProjectedWrapper.ProcessResultInstance(this.enumerator.Current); }
            }

            /// <summary>Returns the inner enumerator thic object is wrapping.</summary>
            internal IEnumerator InnerEnumerator
            {
                get { return this.enumerator; }
            }

            /// <summary>Moves the enumerator to the next result.</summary>
            /// <returns>true if next result is available, false otherwise.</returns>
            public bool MoveNext()
            {
                return this.enumerator.MoveNext();
            }

            /// <summary>Resets the enumerator.</summary>
            public void Reset()
            {
                this.enumerator.Reset();
            }

            /// <summary>Disposes the object.</summary>
            public void Dispose()
            {
                WebUtil.Dispose(this.enumerator);
            }
        }
        #endregion
    }

    /// <summary>Provides a wrapper over result element with the ability to project a subset of properties.</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ProjectedWrapper0 : ProjectedWrapper
    {
        /// <summary>Gets the value for the property specified by its index.</summary>
        /// <param name="propertyIndex">Index of the property for which to get the value.</param>
        /// <returns>The value for the property.</returns>
        protected override object InternalGetProjectedPropertyValue(int propertyIndex)
        {
            throw Error.NotSupported();
        }
    }

    /// <summary>Provides a wrapper over result element with the ability to project a subset of properties.</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ProjectedWrapper1 : ProjectedWrapper
    {
        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty0 { get; set; }

        /// <summary>Gets the value for the property specified by its index.</summary>
        /// <param name="propertyIndex">Index of the property for which to get the value.</param>
        /// <returns>The value for the property.</returns>
        protected override object InternalGetProjectedPropertyValue(int propertyIndex)
        {
            if (propertyIndex == 0) return this.ProjectedProperty0;
            throw Error.NotSupported();
        }
    }

    /// <summary>Provides a wrapper over result element with the ability to project a subset of properties.</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ProjectedWrapper2 : ProjectedWrapper
    {
        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty1 { get; set; }

        /// <summary>Gets the value for the property specified by its index.</summary>
        /// <param name="propertyIndex">Index of the property for which to get the value.</param>
        /// <returns>The value for the property.</returns>
        protected override object InternalGetProjectedPropertyValue(int propertyIndex)
        {
            switch (propertyIndex)
            {
                case 0: return this.ProjectedProperty0;
                case 1: return this.ProjectedProperty1;
                default:
                   throw Error.NotSupported();
            }
        }
    }

    /// <summary>Provides a wrapper over result element with the ability to project a subset of properties.</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ProjectedWrapper3 : ProjectedWrapper
    {
        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty2 { get; set; }

        /// <summary>Gets the value for the property specified by its index.</summary>
        /// <param name="propertyIndex">Index of the property for which to get the value.</param>
        /// <returns>The value for the property.</returns>
        protected override object InternalGetProjectedPropertyValue(int propertyIndex)
        {
            switch (propertyIndex)
            {
                case 0: return this.ProjectedProperty0;
                case 1: return this.ProjectedProperty1;
                case 2: return this.ProjectedProperty2;
                default:
                    throw Error.NotSupported();
            }
        }
    }

    /// <summary>Provides a wrapper over result element with the ability to project a subset of properties.</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ProjectedWrapper4 : ProjectedWrapper
    {
        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty3 { get; set; }

        /// <summary>Gets the value for the property specified by its index.</summary>
        /// <param name="propertyIndex">Index of the property for which to get the value.</param>
        /// <returns>The value for the property.</returns>
        protected override object InternalGetProjectedPropertyValue(int propertyIndex)
        {
            switch (propertyIndex)
            {
                case 0: return this.ProjectedProperty0;
                case 1: return this.ProjectedProperty1;
                case 2: return this.ProjectedProperty2;
                case 3: return this.ProjectedProperty3;
                default:
                    throw Error.NotSupported();
            }
        }
    }

    /// <summary>Provides a wrapper over result element with the ability to project a subset of properties.</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ProjectedWrapper5 : ProjectedWrapper
    {
        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty3 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty4 { get; set; }

        /// <summary>Gets the value for the property specified by its index.</summary>
        /// <param name="propertyIndex">Index of the property for which to get the value.</param>
        /// <returns>The value for the property.</returns>
        protected override object InternalGetProjectedPropertyValue(int propertyIndex)
        {
            switch (propertyIndex)
            {
                case 0: return this.ProjectedProperty0;
                case 1: return this.ProjectedProperty1;
                case 2: return this.ProjectedProperty2;
                case 3: return this.ProjectedProperty3;
                case 4: return this.ProjectedProperty4;
                default:
                    throw Error.NotSupported();
            }
        }
    }

    /// <summary>Provides a wrapper over result element with the ability to project a subset of properties.</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ProjectedWrapper6 : ProjectedWrapper
    {
        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty3 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty4 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty5 { get; set; }

        /// <summary>Gets the value for the property specified by its index.</summary>
        /// <param name="propertyIndex">Index of the property for which to get the value.</param>
        /// <returns>The value for the property.</returns>
        protected override object InternalGetProjectedPropertyValue(int propertyIndex)
        {
            switch (propertyIndex)
            {
                case 0: return this.ProjectedProperty0;
                case 1: return this.ProjectedProperty1;
                case 2: return this.ProjectedProperty2;
                case 3: return this.ProjectedProperty3;
                case 4: return this.ProjectedProperty4;
                case 5: return this.ProjectedProperty5;
                default:
                    throw Error.NotSupported();
            }
        }
    }

    /// <summary>Provides a wrapper over result element with the ability to project a subset of properties.</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ProjectedWrapper7 : ProjectedWrapper
    {
        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty3 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty4 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty5 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty6 { get; set; }

        /// <summary>Gets the value for the property specified by its index.</summary>
        /// <param name="propertyIndex">Index of the property for which to get the value.</param>
        /// <returns>The value for the property.</returns>
        protected override object InternalGetProjectedPropertyValue(int propertyIndex)
        {
            switch (propertyIndex)
            {
                case 0: return this.ProjectedProperty0;
                case 1: return this.ProjectedProperty1;
                case 2: return this.ProjectedProperty2;
                case 3: return this.ProjectedProperty3;
                case 4: return this.ProjectedProperty4;
                case 5: return this.ProjectedProperty5;
                case 6: return this.ProjectedProperty6;
                default:
                    throw Error.NotSupported();
            }
        }
    }

    /// <summary>Provides a wrapper over result element with the ability to project a subset of properties.</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ProjectedWrapper8 : ProjectedWrapper
    {
        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty3 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty4 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty5 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty6 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty7 { get; set; }

        /// <summary>Gets the value for the property specified by its index.</summary>
        /// <param name="propertyIndex">Index of the property for which to get the value.</param>
        /// <returns>The value for the property.</returns>
        protected override object InternalGetProjectedPropertyValue(int propertyIndex)
        {
            switch (propertyIndex)
            {
                case 0: return this.ProjectedProperty0;
                case 1: return this.ProjectedProperty1;
                case 2: return this.ProjectedProperty2;
                case 3: return this.ProjectedProperty3;
                case 4: return this.ProjectedProperty4;
                case 5: return this.ProjectedProperty5;
                case 6: return this.ProjectedProperty6;
                case 7: return this.ProjectedProperty7;
                default:
                    throw Error.NotSupported();
            }
        }
    }

    /// <summary>Provides a wrapper over result element with the ability to project a subset of properties.</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public class ProjectedWrapperMany : ProjectedWrapper
    {
        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty3 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty4 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty5 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty6 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public object ProjectedProperty7 { get; set; }

        /// <summary>Gets or sets another instance of <see cref="ProjectedWrapperMany"/> which contains the set
        /// of next 8 projected properties (and potentially another link).</summary>
        public ProjectedWrapperMany Next { get; set; }

        /// <summary>Gets the value for the property specified by its index.</summary>
        /// <param name="propertyIndex">Index of the property for which to get the value.</param>
        /// <returns>The value for the property.</returns>
        protected override object InternalGetProjectedPropertyValue(int propertyIndex)
        {
            switch (propertyIndex)
            {
                case 0: return this.ProjectedProperty0;
                case 1: return this.ProjectedProperty1;
                case 2: return this.ProjectedProperty2;
                case 3: return this.ProjectedProperty3;
                case 4: return this.ProjectedProperty4;
                case 5: return this.ProjectedProperty5;
                case 6: return this.ProjectedProperty6;
                case 7: return this.ProjectedProperty7;
                default:
                    if (this.Next == null || propertyIndex < 0)
                    {
                        throw Error.NotSupported();
                    }
                    else
                    {
                        return this.Next.InternalGetProjectedPropertyValue(propertyIndex - 8);
                    }
            }
        }
    }

    /// <summary>Instance of this class is assigned to the last <see cref="ProjectedWrapperMany.Next"/> in the list.</summary>
    /// <remarks>This trick is necessary for Entity Framework to work correctly, as it can't project null into the Next property.</remarks>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ProjectedWrapperManyEnd : ProjectedWrapperMany
    {
        /// <summary>Gets the value for the property specified by its index.</summary>
        /// <param name="propertyIndex">Index of the property for which to get the value.</param>
        /// <returns>The value for the property.</returns>
        protected override object InternalGetProjectedPropertyValue(int propertyIndex)
        {
            // We should never get here - no properties are projected into this class (ever).
            throw Error.NotSupported();
        }
    }
}

