//---------------------------------------------------------------------
// <copyright file="AtomMaterializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Provides a class that can materialize ATOM entries into typed
// objects, while maintaining a log of changes done.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Data.Services.Common;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Linq;

    #endregion Namespaces.

    /// <summary>
    /// Use this class to invoke projection methods from AtomMaterializer.
    /// </summary>
    internal static class AtomMaterializerInvoker
    {
        /// <summary>Enumerates casting each element to a type.</summary>
        /// <typeparam name="T">Element type to enumerate over.</typeparam>
        /// <param name="source">Element source.</param>
        /// <returns>
        /// An IEnumerable&lt;T&gt; that iterates over the specified <paramref name="source"/>.
        /// </returns>
        /// <remarks>
        /// This method should be unnecessary with .NET 4.0 covariance support.
        /// </remarks>
        internal static IEnumerable<T> EnumerateAsElementType<T>(IEnumerable source)
        {
            return AtomMaterializer.EnumerateAsElementType<T>(source);
        }

        /// <summary>Creates a list to a target element type.</summary>
        /// <param name="materializer">Materializer used to flow link tracking.</param>
        /// <typeparam name="T">Element type to enumerate over.</typeparam>
        /// <typeparam name="TTarget">Element type for list.</typeparam>
        /// <param name="source">Element source.</param>
        /// <returns>
        /// An IEnumerable&lt;T&gt; that iterates over the specified <paramref name="source"/>.
        /// </returns>
        /// <remarks>
        /// This method should be unnecessary with .NET 4.0 covariance support.
        /// </remarks>
        internal static List<TTarget> ListAsElementType<T, TTarget>(object materializer, IEnumerable<T> source) where T : TTarget
        {
            Debug.Assert(materializer.GetType() == typeof(AtomMaterializer), "materializer.GetType() == typeof(AtomMaterializer)");
            return AtomMaterializer.ListAsElementType<T, TTarget>((AtomMaterializer)materializer, source);
        }

        /// <summary>Checks whether the entity on the specified <paramref name="path"/> is null.</summary>
        /// <param name="entry">Root entry for paths.</param>
        /// <param name="expectedType">Expected type for <paramref name="entry"/>.</param>
        /// <param name="path">Path to pull value for.</param>
        /// <returns>Whether the specified <paramref name="path"/> is null.</returns>
        /// <remarks>
        /// This method will not instantiate entity types on the path.
        /// </remarks>
        internal static bool ProjectionCheckValueForPathIsNull(
            object entry,
            Type expectedType,
            object path)
        {
            Debug.Assert(entry.GetType() == typeof(AtomEntry), "entry.GetType() == typeof(AtomEntry)");
            Debug.Assert(path.GetType() == typeof(ProjectionPath), "path.GetType() == typeof(ProjectionPath)");
            return AtomMaterializer.ProjectionCheckValueForPathIsNull((AtomEntry)entry, expectedType, (ProjectionPath)path);
        }

        /// <summary>Provides support for Select invocations for projections.</summary>
        /// <param name="materializer">Materializer under which projection is taking place.</param>
        /// <param name="entry">Root entry for paths.</param>
        /// <param name="expectedType">Expected type for <paramref name="entry"/>.</param>
        /// <param name="resultType">Expected result type.</param>
        /// <param name="path">Path to traverse.</param>
        /// <param name="selector">Selector callback.</param>
        /// <returns>An enumerable with the select results.</returns>
        internal static IEnumerable ProjectionSelect(
            object materializer,
            object entry,
            Type expectedType,
            Type resultType,
            object path,
            Func<object, object, Type, object> selector)
        {
            Debug.Assert(materializer.GetType() == typeof(AtomMaterializer), "materializer.GetType() == typeof(AtomMaterializer)");
            Debug.Assert(entry.GetType() == typeof(AtomEntry), "entry.GetType() == typeof(AtomEntry)");
            Debug.Assert(path.GetType() == typeof(ProjectionPath), "path.GetType() == typeof(ProjectionPath)");
            return AtomMaterializer.ProjectionSelect((AtomMaterializer)materializer, (AtomEntry)entry, expectedType, resultType, (ProjectionPath)path, selector);
        }

        /// <summary>Provides support for getting payload entries during projections.</summary>
        /// <param name="entry">Entry to get sub-entry from.</param>
        /// <param name="name">Name of sub-entry.</param>
        /// <returns>The sub-entry (never null).</returns>
        internal static AtomEntry ProjectionGetEntry(object entry, string name)
        {
            Debug.Assert(entry.GetType() == typeof(AtomEntry), "entry.GetType() == typeof(AtomEntry)");
            return AtomMaterializer.ProjectionGetEntry((AtomEntry)entry, name);
        }

        /// <summary>Initializes a projection-driven entry (with a specific type and specific properties).</summary>
        /// <param name="materializer">Materializer under which projection is taking place.</param>
        /// <param name="entry">Root entry for paths.</param>
        /// <param name="expectedType">Expected type for <paramref name="entry"/>.</param>
        /// <param name="resultType">Expected result type.</param>
        /// <param name="properties">Properties to materialize.</param>
        /// <param name="propertyValues">Functions to get values for functions.</param>
        /// <returns>The initialized entry.</returns>
        internal static object ProjectionInitializeEntity(
            object materializer,
            object entry,
            Type expectedType,
            Type resultType,
            string[] properties,
            Func<object, object, Type, object>[] propertyValues)
        {
            Debug.Assert(materializer.GetType() == typeof(AtomMaterializer), "materializer.GetType() == typeof(AtomMaterializer)");
            Debug.Assert(entry.GetType() == typeof(AtomEntry), "entry.GetType() == typeof(AtomEntry)");
            return AtomMaterializer.ProjectionInitializeEntity((AtomMaterializer)materializer, (AtomEntry)entry, expectedType, resultType, properties, propertyValues);
        }

        /// <summary>Projects a simple value from the specified <paramref name="path"/>.</summary>
        /// <param name="materializer">Materializer under which projection is taking place.</param>
        /// <param name="entry">Root entry for paths.</param>
        /// <param name="expectedType">Expected type for <paramref name="entry"/>.</param>
        /// <param name="path">Path to pull value for.</param>
        /// <returns>The value for the specified <paramref name="path"/>.</returns>
        /// <remarks>
        /// This method will not instantiate entity types, except to satisfy requests
        /// for payload-driven feeds or leaf entities.
        /// </remarks>
        internal static object ProjectionValueForPath(object materializer, object entry, Type expectedType, object path)
        {
            Debug.Assert(materializer.GetType() == typeof(AtomMaterializer), "materializer.GetType() == typeof(AtomMaterializer)");
            Debug.Assert(entry.GetType() == typeof(AtomEntry), "entry.GetType() == typeof(AtomEntry)");
            Debug.Assert(path.GetType() == typeof(ProjectionPath), "path.GetType() == typeof(ProjectionPath)");
            return AtomMaterializer.ProjectionValueForPath((AtomMaterializer)materializer, (AtomEntry)entry, expectedType, (ProjectionPath)path);
        }

        /// <summary>Materializes an entry with no special selection.</summary>
        /// <param name="materializer">Materializer under which materialization should take place.</param>
        /// <param name="entry">Entry with object to materialize.</param>
        /// <param name="expectedEntryType">Expected type for the entry.</param>
        /// <returns>The materialized instance.</returns>
        internal static object DirectMaterializePlan(object materializer, object entry, Type expectedEntryType)
        {
            Debug.Assert(materializer.GetType() == typeof(AtomMaterializer), "materializer.GetType() == typeof(AtomMaterializer)");
            Debug.Assert(entry.GetType() == typeof(AtomEntry), "entry.GetType() == typeof(AtomEntry)");
            return AtomMaterializer.DirectMaterializePlan((AtomMaterializer)materializer, (AtomEntry)entry, expectedEntryType);
        }

        /// <summary>Materializes an entry without including in-lined expanded links.</summary>
        /// <param name="materializer">Materializer under which materialization should take place.</param>
        /// <param name="entry">Entry with object to materialize.</param>
        /// <param name="expectedEntryType">Expected type for the entry.</param>
        /// <returns>The materialized instance.</returns>
        internal static object ShallowMaterializePlan(object materializer, object entry, Type expectedEntryType)
        {
            Debug.Assert(materializer.GetType() == typeof(AtomMaterializer), "materializer.GetType() == typeof(AtomMaterializer)");
            Debug.Assert(entry.GetType() == typeof(AtomEntry), "entry.GetType() == typeof(AtomEntry)");
            return AtomMaterializer.ShallowMaterializePlan((AtomMaterializer)materializer, (AtomEntry)entry, expectedEntryType);
        }
    }

    /// <summary>
    /// Use this class to materialize objects provided from an <see cref="AtomParser"/>.
    /// </summary>
    [DebuggerDisplay("AtomMaterializer {parser}")]
    internal class AtomMaterializer
    {
        #region Private fields.

        /// <summary>Associated context, used for resolving instances and types.</summary>
        private readonly DataServiceContext context;

        /// <summary>Expected type to materialize.</summary>
        private readonly Type expectedType;

        //// <summary>Whether missing properties should be ignored.</summary>
        ////private readonly bool ignoreMissingProperties;

        /// <summary>Log into which materialization activity is recorded.</summary>
        private readonly AtomMaterializerLog log;

        /// <summary>Function to materialize an entry and produce a value.</summary>
        private readonly ProjectionPlan materializeEntryPlan;

        /// <summary>
        /// Callback to be invoked when an object is materialized; receives the tag and object.
        /// </summary>
        private readonly Action<object, object> materializedObjectCallback;

        /// <summary>Merge behavior.</summary>
        private readonly MergeOption mergeOption;

        /// <summary>Collection->Next Link Table for nested links</summary>
        private readonly Dictionary<IEnumerable, DataServiceQueryContinuation> nextLinkTable;

        /// <summary>
        /// <see cref="AtomParser"/> from which content will be read.
        /// </summary>
        private readonly AtomParser parser;

        /// <summary>Current value being materialized; possibly null.</summary>
        private object currentValue;

        /// <summary>Whether missing properties should be ignored.</summary>
        /// <remarks>
        /// This should be readonly, but we make it writeable as a 
        /// work-around until server projections are implemented.
        /// </remarks>
        private bool ignoreMissingProperties;

        /// <summary>Target instance that the materializer expects to update.</summary>
        private object targetInstance;

        #endregion Private fields.

        #region Constructors.

        /// <summary>Initializes a new <see cref="AtomMaterializer"/> instance.</summary>
        /// <param name="parser"><see cref="AtomParser"/> from which content will be read.</param>
        /// <param name="context">Context into which instances will be materialized.</param>
        /// <param name="expectedType">Expected type to materialize.</param>
        /// <param name="ignoreMissingProperties">
        /// Whether missing properties on the type should be ignored (i.e., the payload
        /// is allowed to have properties that are not materialized and dropped instead).
        /// </param>
        /// <param name="mergeOption">Merge behavior.</param>
        /// <param name="log">Log into which materialization activity is recorded.</param>
        /// <param name="materializedObjectCallback">
        /// Callback to be invoked when an object is materialized; receives the tag and object.
        /// </param>
        /// <param name="queryComponents">Query components describingg processing of components; possibly null.</param>
        /// <param name="plan">Projection plan (if compiled in an earlier query).</param>
        internal AtomMaterializer(
            AtomParser parser, 
            DataServiceContext context, 
            Type expectedType, 
            bool ignoreMissingProperties, 
            MergeOption mergeOption, 
            AtomMaterializerLog log, 
            Action<object, object> materializedObjectCallback,
            QueryComponents queryComponents,
            ProjectionPlan plan)
        {
            Debug.Assert(context != null, "context != null");
            Debug.Assert(parser != null, "parser != null");
            Debug.Assert(log != null, "log != null");

            this.context = context;
            this.parser = parser;
            this.expectedType = expectedType;
            this.ignoreMissingProperties = ignoreMissingProperties;
            this.mergeOption = mergeOption;
            this.log = log;
            this.materializedObjectCallback = materializedObjectCallback;
            this.nextLinkTable = new Dictionary<IEnumerable, DataServiceQueryContinuation>(ReferenceEqualityComparer<IEnumerable>.Instance);
            this.materializeEntryPlan = plan ?? CreatePlan(queryComponents);
        }

        #endregion Constructors.

        #region Internal properties.

        /// <summary>DataServiceContext instance this materializer is bound to.</summary>
        internal DataServiceContext Context
        {
            get { return this.context; }
        }

        /// <summary>Function to materialize an entry and produce a value.</summary>
        internal ProjectionPlan MaterializeEntryPlan
        {
            get { return this.materializeEntryPlan; }
        }

        /// <summary>Target instance that the materializer expects to update.</summary>
        /// <remarks>
        /// This property is typically null, but will be set to an existing
        /// instance when POST-ing a value, so values can get merged into it.
        /// </remarks>
        internal object TargetInstance
        {
            get 
            { 
                return this.targetInstance;
            }

            set
            {
                Debug.Assert(value != null, "value != null -- otherwise we have no instance target.");
                this.targetInstance = value;
            }
        }

        /// <summary>Feed being materialized; possibly null.</summary>
        internal AtomFeed CurrentFeed
        {
            get
            {
                return this.parser.CurrentFeed;
            }
        }

        /// <summary>Entry being materialized; possibly null.</summary>
        internal AtomEntry CurrentEntry
        {
            get
            {
                return this.parser.CurrentEntry;
            }
        }

        /// <summary>Current value being materialized; possibly null.</summary>
        /// <remarks>
        /// This will typically be an entity if <see cref="CurrentEntry"/>
        /// is assigned, but may contain a string for example if a top-level
        /// primitive of type string is found.
        /// </remarks>
        internal object CurrentValue
        {
            get
            {
                return this.currentValue;
            }
        }

        /// <summary>Log into which materialization activity is recorded.</summary>
        internal AtomMaterializerLog Log
        {
            get { return this.log; }
        }

        /// <summary>Table storing the next links ----oicated with the current payload</summary>
        internal Dictionary<IEnumerable, DataServiceQueryContinuation> NextLinkTable
        {
            get { return this.nextLinkTable; }
        }

        /// <summary>Whether we have finished processing the current data stream.</summary>
        internal bool IsEndOfStream
        {
            get { return this.parser.DataKind == AtomDataKind.Finished; }
        }

        #endregion Internal properties.

        #region Projection support.

        /// <summary>Enumerates casting each element to a type.</summary>
        /// <typeparam name="T">Element type to enumerate over.</typeparam>
        /// <param name="source">Element source.</param>
        /// <returns>
        /// An IEnumerable&lt;T&gt; that iterates over the specified <paramref name="source"/>.
        /// </returns>
        /// <remarks>
        /// This method should be unnecessary with .NET 4.0 covariance support.
        /// </remarks>
        internal static IEnumerable<T> EnumerateAsElementType<T>(IEnumerable source)
        {
            Debug.Assert(source != null, "source != null");

            IEnumerable<T> typedSource = source as IEnumerable<T>;
            if (typedSource != null)
            {
                return typedSource;
            }
            else
            {
                return EnumerateAsElementTypeInternal<T>(source);
            }
        }

        /// <summary>Enumerates casting each element to a type.</summary>
        /// <typeparam name="T">Element type to enumerate over.</typeparam>
        /// <param name="source">Element source.</param>
        /// <returns>
        /// An IEnumerable&lt;T&gt; that iterates over the specified <paramref name="source"/>.
        /// </returns>
        /// <remarks>
        /// This method should be unnecessary with .NET 4.0 covariance support.
        /// </remarks>
        internal static IEnumerable<T> EnumerateAsElementTypeInternal<T>(IEnumerable source)
        {
            Debug.Assert(source != null, "source != null");

            foreach (object item in source)
            {
                yield return (T)item;
            }
        }

        /// <summary>Checks whether the specified type is a DataServiceCollection type (or inherits from one).</summary>
        /// <param name='type'>Type to check.</param>
        /// <returns>true if the type inherits from DataServiceCollection; false otherwise.</returns>
        internal static bool IsDataServiceCollection(Type type)
        {
            while (type != null)
            {
                if (type.IsGenericType && WebUtil.IsDataServiceCollectionType(type.GetGenericTypeDefinition()))
                {
                    return true;
                }

                type = type.BaseType;
            }

            return false;
        }

        /// <summary>Creates a list to a target element type.</summary>
        /// <param name="materializer">Materializer used to flow link tracking.</param>
        /// <typeparam name="T">Element type to enumerate over.</typeparam>
        /// <typeparam name="TTarget">Element type for list.</typeparam>
        /// <param name="source">Element source.</param>
        /// <returns>
        /// An IEnumerable&lt;T&gt; that iterates over the specified <paramref name="source"/>.
        /// </returns>
        /// <remarks>
        /// This method should be unnecessary with .NET 4.0 covariance support.
        /// </remarks>
        internal static List<TTarget> ListAsElementType<T, TTarget>(AtomMaterializer materializer, IEnumerable<T> source) where T : TTarget
        {
            Debug.Assert(materializer != null, "materializer != null");
            Debug.Assert(source != null, "source != null");

            List<TTarget> typedSource = source as List<TTarget>;
            if (typedSource != null)
            {
                return typedSource;
            }

            List<TTarget> list;
            IList sourceList = source as IList;
            if (sourceList != null)
            {
                list = new List<TTarget>(sourceList.Count);
            }
            else
            {
                list = new List<TTarget>();
            }

            foreach (T item in source)
            {
                list.Add((TTarget)item);
            }

            // We can flow the same continuation becaues they're immutable, and
            // we don't need to set the continuation property because List<T> doesn't
            // have one.
            DataServiceQueryContinuation continuation;
            if (materializer.nextLinkTable.TryGetValue(source, out continuation))
            {
                materializer.nextLinkTable[list] = continuation;
            }

            return list;
        }

        /// <summary>Checks whether the entity on the specified <paramref name="path"/> is null.</summary>
        /// <param name="entry">Root entry for paths.</param>
        /// <param name="expectedType">Expected type for <paramref name="entry"/>.</param>
        /// <param name="path">Path to pull value for.</param>
        /// <returns>Whether the specified <paramref name="path"/> is null.</returns>
        /// <remarks>
        /// This method will not instantiate entity types on the path.
        /// Note that if the target is a collection, the result is always false,
        /// as the model does not allow null feeds (but instead gets an empty
        /// collection, possibly with continuation tokens and such).
        /// </remarks>
        internal static bool ProjectionCheckValueForPathIsNull(
            AtomEntry entry,
            Type expectedType,
            ProjectionPath path)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(path != null, "path != null");

            if (path.Count == 0 || path.Count == 1 && path[0].Member == null)
            {
                return entry.IsNull;
            }

            bool result = false;
            AtomContentProperty atomProperty = null;
            List<AtomContentProperty> properties = entry.DataValues;
            for (int i = 0; i < path.Count; i++)
            {
                var segment = path[i];
                if (segment.Member == null)
                {
                    continue;
                }

                bool segmentIsLeaf = i == path.Count - 1;
                string propertyName = segment.Member;
                ClientType.ClientProperty property = ClientType.Create(expectedType).GetProperty(propertyName, false);
                atomProperty = GetPropertyOrThrow(properties, propertyName);
                ValidatePropertyMatch(property, atomProperty);
                if (atomProperty.Feed != null)
                {
                    Debug.Assert(segmentIsLeaf, "segmentIsLeaf -- otherwise the path generated traverses a feed, which should be disallowed");
                    result = false;
                }
                else
                {
                    Debug.Assert(
                        atomProperty.Entry != null,
                        "atomProperty.Entry != null -- otherwise a primitive property / complex type is being rewritte with a null check; this is only supported for entities and collection");
                    if (segmentIsLeaf)
                    {
                        result = atomProperty.Entry.IsNull;
                    }

                    properties = atomProperty.Entry.DataValues;
                    entry = atomProperty.Entry;
                }

                expectedType = property.PropertyType;
            }

            return result;
        }

        /// <summary>Provides support for Select invocations for projections.</summary>
        /// <param name="materializer">Materializer under which projection is taking place.</param>
        /// <param name="entry">Root entry for paths.</param>
        /// <param name="expectedType">Expected type for <paramref name="entry"/>.</param>
        /// <param name="resultType">Expected result type.</param>
        /// <param name="path">Path to traverse.</param>
        /// <param name="selector">Selector callback.</param>
        /// <returns>An enumerable with the select results.</returns>
        internal static IEnumerable ProjectionSelect(
            AtomMaterializer materializer,
            AtomEntry entry,
            Type expectedType,
            Type resultType,
            ProjectionPath path,
            Func<object, object, Type, object> selector)
        {
            ClientType entryType = entry.ActualType ?? ClientType.Create(expectedType);
            IEnumerable list = (IEnumerable)Util.ActivatorCreateInstance(typeof(List<>).MakeGenericType(resultType));
            AtomContentProperty atomProperty = null;
            ClientType.ClientProperty property = null;
            for (int i = 0; i < path.Count; i++)
            {
                var segment = path[i];
                if (segment.Member == null)
                {
                    continue;
                }

                string propertyName = segment.Member;
                property = entryType.GetProperty(propertyName, false);
                atomProperty = GetPropertyOrThrow(entry, propertyName);

                if (atomProperty.Entry != null)
                {
                    entry = atomProperty.Entry;
                    entryType = ClientType.Create(property.NullablePropertyType, false);
                } 
            }

            ValidatePropertyMatch(property, atomProperty);
            AtomFeed sourceFeed = atomProperty.Feed;
            Debug.Assert(
                sourceFeed != null, 
                "sourceFeed != null -- otherwise ValidatePropertyMatch should have thrown or property isn't a collection (and should be part of this plan)");

            Action<object, object> addMethod = GetAddToCollectionDelegate(list.GetType());
            foreach (var paramEntry in sourceFeed.Entries)
            {
                object projected = selector(materializer, paramEntry, property.CollectionType /* perhaps nested? */);
                addMethod(list, projected);
            }

            ProjectionPlan plan = new ProjectionPlan();
            plan.LastSegmentType = property.CollectionType;
            plan.Plan = selector;
            plan.ProjectedType = resultType;

            materializer.FoundNextLinkForCollection(list, sourceFeed.NextLink, plan);

            return list;
        }
        
        /// <summary>Provides support for getting payload entries during projections.</summary>
        /// <param name="entry">Entry to get sub-entry from.</param>
        /// <param name="name">Name of sub-entry.</param>
        /// <returns>The sub-entry (never null).</returns>
        internal static AtomEntry ProjectionGetEntry(AtomEntry entry, string name)
        {
            Debug.Assert(entry != null, "entry != null -- ProjectionGetEntry never returns a null entry, and top-level materialization shouldn't pass one in");

            AtomContentProperty property = GetPropertyOrThrow(entry, name);
            if (property.Entry == null)
            {
                throw new InvalidOperationException(Strings.AtomMaterializer_PropertyNotExpectedEntry(name, entry.Identity));
            }

            CheckEntryToAccessNotNull(property.Entry, name);

            return property.Entry;
        }

        /// <summary>Initializes a projection-driven entry (with a specific type and specific properties).</summary>
        /// <param name="materializer">Materializer under which projection is taking place.</param>
        /// <param name="entry">Root entry for paths.</param>
        /// <param name="expectedType">Expected type for <paramref name="entry"/>.</param>
        /// <param name="resultType">Expected result type.</param>
        /// <param name="properties">Properties to materialize.</param>
        /// <param name="propertyValues">Functions to get values for functions.</param>
        /// <returns>The initialized entry.</returns>
        internal static object ProjectionInitializeEntity(
            AtomMaterializer materializer,
            AtomEntry entry,
            Type expectedType,
            Type resultType,
            string[] properties,
            Func<object, object, Type, object>[] propertyValues)
        {
            if (entry == null || entry.IsNull)
            {
                throw new NullReferenceException(Strings.AtomMaterializer_EntryToInitializeIsNull(resultType.FullName));
            }

            object result;
            if (!entry.EntityHasBeenResolved)
            {
                AtomMaterializer.ProjectionEnsureEntryAvailableOfType(materializer, entry, resultType);
            }
            else if (!resultType.IsAssignableFrom(entry.ActualType.ElementType))
            {
                string message = Strings.AtomMaterializer_ProjectEntityTypeMismatch(
                    resultType.FullName,
                    entry.ActualType.ElementType.FullName,
                    entry.Identity);
                throw new InvalidOperationException(message);
            }

            result = entry.ResolvedObject;
            
            for (int i = 0; i < properties.Length; i++)
            {
                var property = entry.ActualType.GetProperty(properties[i], materializer.ignoreMissingProperties);
                object value = propertyValues[i](materializer, entry, expectedType);
                if (entry.ShouldUpdateFromPayload && ClientType.Create(property.NullablePropertyType, false).IsEntityType)
                {
                    materializer.Log.SetLink(entry, property.PropertyName, value);
                }

                bool isEntity = property.CollectionType == null || !ClientType.CheckElementTypeIsEntity(property.CollectionType);
                if (entry.ShouldUpdateFromPayload)
                {
                    if (isEntity)
                    {
                        property.SetValue(result, value, property.PropertyName, false);
                    }
                    else
                    {
                        IEnumerable valueAsEnumerable = (IEnumerable)value;
                        DataServiceQueryContinuation continuation = materializer.nextLinkTable[valueAsEnumerable];
                        Uri nextLinkUri = continuation == null ? null : continuation.NextLinkUri;
                        ProjectionPlan plan = continuation == null ? null : continuation.Plan;
                        materializer.MergeLists(entry, property, valueAsEnumerable, nextLinkUri, plan);
                    }
                }
                else if (!isEntity)
                {
                    materializer.FoundNextLinkForUnmodifiedCollection(property.GetValue(entry.ResolvedObject) as IEnumerable);
                }
            }

            return result;
        }

        /// <summary>Projects a simple value from the specified <paramref name="path"/>.</summary>
        /// <param name="materializer">Materializer under which projection is taking place.</param>
        /// <param name="entry">Root entry for paths.</param>
        /// <param name="expectedType">Expected type for <paramref name="entry"/>.</param>
        /// <param name="path">Path to pull value for.</param>
        /// <returns>The value for the specified <paramref name="path"/>.</returns>
        /// <remarks>
        /// This method will not instantiate entity types, except to satisfy requests
        /// for payload-driven feeds or leaf entities.
        /// </remarks>
        internal static object ProjectionValueForPath(AtomMaterializer materializer, AtomEntry entry, Type expectedType, ProjectionPath path)
        {
            Debug.Assert(materializer != null, "materializer != null");
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(path != null, "path != null");

            // An empty path indicates that we do a regular materialization.
            if (path.Count == 0 || path.Count == 1 && path[0].Member == null)
            {
                if (!entry.EntityHasBeenResolved)
                {
                    materializer.Materialize(entry, expectedType, /* includeLinks */ false);
                }

                return entry.ResolvedObject;
            }

            object result = null;
            AtomContentProperty atomProperty = null;
            List<AtomContentProperty> properties = entry.DataValues;
            for (int i = 0; i < path.Count; i++)
            {
                var segment = path[i];
                if (segment.Member == null)
                {
                    continue;
                }

                bool segmentIsLeaf = i == path.Count - 1;
                string propertyName = segment.Member;

                // only primitive values can be mapped so we don't need to apply mappings for non-leaf segments
                if (segmentIsLeaf)
                {
                    CheckEntryToAccessNotNull(entry, propertyName);
                    if (!entry.EntityPropertyMappingsApplied)
                    {
                        // EPM should happen only once per entry
                        ClientType attributeSourceType = MaterializeAtom.GetEntryClientType(entry.TypeName, materializer.context, expectedType, false);
                        ApplyEntityPropertyMappings(entry, attributeSourceType);
                    }
                }

                ClientType.ClientProperty property = ClientType.Create(expectedType).GetProperty(propertyName, false);
                atomProperty = GetPropertyOrThrow(properties, propertyName);

                ValidatePropertyMatch(property, atomProperty);

                AtomFeed feedValue = atomProperty.Feed;
                if (feedValue != null)
                {
                    Debug.Assert(segmentIsLeaf, "segmentIsLeaf -- otherwise the path generated traverses a feed, which should be disallowed");

                    // When we're materializing a feed as a leaf, we actually project each element.
                    Type collectionType = ClientType.GetImplementationType(segment.ProjectionType, typeof(ICollection<>));
                    if (collectionType == null)
                    {
                        collectionType = ClientType.GetImplementationType(segment.ProjectionType, typeof(IEnumerable<>));
                    }

                    Debug.Assert(
                        collectionType != null, 
                        "collectionType != null -- otherwise the property should never have been recognized as a collection");
                    
                    Type nestedExpectedType = collectionType.GetGenericArguments()[0];
                    Type feedType = segment.ProjectionType;
                    if (feedType.IsInterface || IsDataServiceCollection(feedType))
                    {
                        feedType = typeof(System.Collections.ObjectModel.Collection<>).MakeGenericType(nestedExpectedType);
                    }

                    IEnumerable list = (IEnumerable)Util.ActivatorCreateInstance(feedType);
                    MaterializeToList(materializer, list, nestedExpectedType, feedValue.Entries);

                    if (IsDataServiceCollection(segment.ProjectionType))
                    {
                        Type dataServiceCollectionType = WebUtil.GetDataServiceCollectionOfT(nestedExpectedType);
                        list = (IEnumerable)Util.ActivatorCreateInstance(
                            dataServiceCollectionType,
                            list, // items
                            TrackingMode.None); // tracking mode
                    }

                    ProjectionPlan plan = CreatePlanForShallowMaterialization(nestedExpectedType);
                    materializer.FoundNextLinkForCollection(list, feedValue.NextLink, plan);
                    result = list;
                }
                else if (atomProperty.Entry != null)
                {
                    // If this is a leaf, then we'll do a tracking, payload-driven
                    // materialization. If this isn't the leaf, then we'll 
                    // simply traverse through its properties.
                    if (segmentIsLeaf && !atomProperty.Entry.EntityHasBeenResolved && !atomProperty.IsNull)
                    {
                        materializer.Materialize(atomProperty.Entry, property.PropertyType, /* includeLinks */ false);
                    }

                    properties = atomProperty.Entry.DataValues;
                    result = atomProperty.Entry.ResolvedObject;
                    entry = atomProperty.Entry;
                }
                else
                {
                    if (atomProperty.Properties != null)
                    {
                        if (atomProperty.MaterializedValue == null && !atomProperty.IsNull)
                        {
                            ClientType complexType = ClientType.Create(property.PropertyType);
                            object complexInstance = Util.ActivatorCreateInstance(property.PropertyType);
                            MaterializeDataValues(complexType, atomProperty.Properties, materializer.ignoreMissingProperties, materializer.context);
                            ApplyDataValues(complexType, atomProperty.Properties, materializer.ignoreMissingProperties, materializer.context, complexInstance);
                            atomProperty.MaterializedValue = complexInstance;
                        }
                    }
                    else
                    {
                        MaterializeDataValue(property.NullablePropertyType, atomProperty, materializer.context);
                    }

                    properties = atomProperty.Properties;
                    result = atomProperty.MaterializedValue;
                }

                expectedType = property.PropertyType;
            }

            return result;
        }

        /// <summary>
        /// Ensures that an entry of <paramref name="requiredType"/> is 
        /// available on the specified <paramref name="entry"/>.
        /// </summary>
        /// <param name="materializer">Materilizer used for logging. </param>
        /// <param name="entry">Entry to ensure.</param>
        /// <param name="requiredType">Required type.</param>
        /// <remarks>
        /// As the 'Projection' suffix suggests, this method should only
        /// be used during projection operations; it purposefully avoid
        /// "source tree" type usage and POST reply entry resolution.
        /// </remarks>
        internal static void ProjectionEnsureEntryAvailableOfType(AtomMaterializer materializer, AtomEntry entry, Type requiredType)
        {
            Debug.Assert(materializer != null, "materializer != null");
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(
                materializer.targetInstance == null,
                "materializer.targetInstance == null -- projection shouldn't have a target instance set; that's only used for POST replies");

            if (entry.EntityHasBeenResolved)
            {
                if (!requiredType.IsAssignableFrom(entry.ResolvedObject.GetType()))
                {
                    throw new InvalidOperationException(
                        "Expecting type '" + requiredType + "' for '" + entry.Identity + "', but found " +
                        "a previously created instance of type '" + entry.ResolvedObject.GetType());
                }

                // Just check the type
                return;
            }

            if (entry.Identity == null)
            {
                throw Error.InvalidOperation(Strings.Deserialize_MissingIdElement);
            }

            if (!materializer.TryResolveAsCreated(entry) && 
                !materializer.TryResolveFromContext(entry, requiredType))
            {
                // The type is always required, so skip ResolveByCreating.
                materializer.ResolveByCreatingWithType(entry, requiredType);
            }
            else
            {
                if (!requiredType.IsAssignableFrom(entry.ResolvedObject.GetType()))
                {
                    throw Error.InvalidOperation(Strings.Deserialize_Current(requiredType, entry.ResolvedObject.GetType()));
                }
            }
        }

        /// <summary>Materializes an entry with no special selection.</summary>
        /// <param name="materializer">Materializer under which materialization should take place.</param>
        /// <param name="entry">Entry with object to materialize.</param>
        /// <param name="expectedEntryType">Expected type for the entry.</param>
        /// <returns>The materialized instance.</returns>
        internal static object DirectMaterializePlan(AtomMaterializer materializer, AtomEntry entry, Type expectedEntryType)
        {
            materializer.Materialize(entry, expectedEntryType, true);
            return entry.ResolvedObject;
        }

        /// <summary>Materializes an entry without including in-lined expanded links.</summary>
        /// <param name="materializer">Materializer under which materialization should take place.</param>
        /// <param name="entry">Entry with object to materialize.</param>
        /// <param name="expectedEntryType">Expected type for the entry.</param>
        /// <returns>The materialized instance.</returns>
        internal static object ShallowMaterializePlan(AtomMaterializer materializer, AtomEntry entry, Type expectedEntryType)
        {
            materializer.Materialize(entry, expectedEntryType, false);
            return entry.ResolvedObject;
        }

        /// <summary>
        /// Validates the specified <paramref name="property"/> matches 
        /// the parsed <paramref name="atomProperty"/>.
        /// </summary>
        /// <param name="property">Property as understood by the type system.</param>
        /// <param name="atomProperty">Property as parsed.</param>
        internal static void ValidatePropertyMatch(ClientType.ClientProperty property, AtomContentProperty atomProperty)
        {
            Debug.Assert(property != null, "property != null");
            Debug.Assert(atomProperty != null, "atomProperty != null");

            if (property.IsKnownType && (atomProperty.Feed != null || atomProperty.Entry != null))
            {
                throw Error.InvalidOperation(Strings.Deserialize_MismatchAtomLinkLocalSimple);
            }

            if (atomProperty.Feed != null && property.CollectionType == null)
            {
                throw Error.InvalidOperation(Strings.Deserialize_MismatchAtomLinkFeedPropertyNotCollection(property.PropertyName));
            }

            if (atomProperty.Entry != null && property.CollectionType != null)
            {
                throw Error.InvalidOperation(Strings.Deserialize_MismatchAtomLinkEntryPropertyIsCollection(property.PropertyName));
            }
        }

        #endregion Projection support.
        
        #region Internal methods.

        /// <summary>Reads the next value from the input content.</summary>
        /// <returns>true if another value is available after reading; false otherwise.</returns>
        /// <remarks>
        /// After invocation, the currentValue field (and CurrentValue property) will 
        /// reflect the value materialized from the parser; possibly null if the
        /// result is true (for null values); always null if the result is false.
        /// </remarks>
        internal bool Read()
        {
            this.currentValue = null;

            // links from last entry should be cleared
            this.nextLinkTable.Clear();
            while (this.parser.Read())
            {
                Debug.Assert(
                    this.parser.DataKind != AtomDataKind.None,
                    "parser.DataKind != AtomDataKind.None -- otherwise parser.Read() didn't update its state");
                Debug.Assert(
                    this.parser.DataKind != AtomDataKind.Finished,
                    "parser.DataKind != AtomDataKind.Finished -- otherwise parser.Read() shouldn't have returned true");

                switch (this.parser.DataKind)
                {
                    case AtomDataKind.Feed:
                    case AtomDataKind.FeedCount:
                        // Nothing to do.
                        break;
                    case AtomDataKind.Entry:
                        Debug.Assert(
                            this.parser.CurrentEntry != null,
                            "parser.CurrentEntry != null -- otherwise parser.DataKind shouldn't be Entry");
                        this.CurrentEntry.ResolvedObject = this.TargetInstance;
                        this.currentValue = this.materializeEntryPlan.Run(this, this.CurrentEntry, this.expectedType);
                        return true;
                    case AtomDataKind.PagingLinks:
                        // encountered paging links - this is the OUTER-MOST feed
                        // we don't need to do anything here, the CurrentFeed.NextLink should
                        // have already been set (either to an Uri or to null)
                        break;
                    default:
                        Debug.Assert(
                            this.parser.DataKind == AtomDataKind.Custom,
                            "parser.DataKind == AtomDataKind.Custom -- otherwise AtomMaterializer.Read switch is missing a case");

                        // V1 Compatibility Note:
                        // Top-level primitive types are allowed to have mixed content,
                        // and must be parsed with ReadCustomElementString.
                        //
                        // For complex types, we'll instead use the regular property
                        // reading path, with will throw on mixed content.
                        Type underlyingExpectedType = Nullable.GetUnderlyingType(this.expectedType) ?? this.expectedType;
                        ClientType targetType = ClientType.Create(underlyingExpectedType);
                        if (ClientConvert.IsKnownType(underlyingExpectedType))
                        {
                            // primitive type - we don't care about the namespace as per V1, but it should be in XmlConstants.DataWebNamespace ("http://schemas.microsoft.com/ado/2007/08/dataservices")
                            string elementText = this.parser.ReadCustomElementString();
                            if (elementText != null)
                            {
                                this.currentValue = ClientConvert.ChangeType(elementText, underlyingExpectedType);
                            }

                            return true;
                        }
                        else if (!targetType.IsEntityType && this.parser.IsDataWebElement)
                        {
                            // complex type - the namespace URI must be in dataweb namespace
                            AtomContentProperty property = this.parser.ReadCurrentPropertyValue();
                            if (property == null || property.IsNull)
                            {
                                this.currentValue = null;
                            }
                            else
                            {
                                this.currentValue = targetType.CreateInstance();
                                MaterializeDataValues(targetType, property.Properties, this.ignoreMissingProperties, this.context);
                                ApplyDataValues(targetType, property.Properties, this.ignoreMissingProperties, this.context, this.currentValue);
                            }

                            return true;
                        }

                        break;
                }
            }

            Debug.Assert(this.parser.DataKind == AtomDataKind.Finished, "parser.DataKind == AtomDataKind.None");
            Debug.Assert(this.parser.CurrentEntry == null, "parser.Current == null");
            return false;
        }

        /// <summary>Helper method for constructor of DataServiceCollection.</summary>
        /// <typeparam name="T">Element type for collection.</typeparam>
        /// <param name="from">The enumerable which has the continuation on it.</param>
        /// <param name="to">The DataServiceCollection to apply the continuation to.</param>
        internal void PropagateContinuation<T>(IEnumerable<T> from, DataServiceCollection<T> to)
        {
            DataServiceQueryContinuation continuation;
            if (this.nextLinkTable.TryGetValue(from, out continuation))
            {
                this.nextLinkTable.Add(to, continuation);
                Util.SetNextLinkForCollection(to, continuation);
            }
        }

        #endregion Internal methods.

        #region Private methods.

        /// <summary>
        /// Checks that the specified <paramref name="entry"/> isn't null.
        /// </summary>
        /// <param name="entry">Entry to check.</param>
        /// <param name="name">Name of entry being accessed.</param>
        private static void CheckEntryToAccessNotNull(AtomEntry entry, string name)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(name != null, "name != null");

            if (entry.IsNull)
            {
                throw new NullReferenceException(Strings.AtomMaterializer_EntryToAccessIsNull(name));
            }
        }

        /// <summary>Creates an entry materialization plan for a given projection.</summary>
        /// <param name="queryComponents">Query components for plan to materialize.</param>
        /// <returns>A materialization plan.</returns>
        private static ProjectionPlan CreatePlan(QueryComponents queryComponents)
        {
            // Can we have a primitive property as well?
            LambdaExpression projection = queryComponents.Projection;
            ProjectionPlan result;
            if (projection == null)
            {
                result = CreatePlanForDirectMaterialization(queryComponents.LastSegmentType);
            }
            else
            {
                result = ProjectionPlanCompiler.CompilePlan(projection, queryComponents.NormalizerRewrites);
                result.LastSegmentType = queryComponents.LastSegmentType;
            }

            return result;
        }

        /// <summary>Creates an entry materialization plan that is payload-driven.</summary>
        /// <param name="lastSegmentType">Segment type for the entry to materialize (typically last of URI in query).</param>
        /// <returns>A payload-driven materialization plan.</returns>
        private static ProjectionPlan CreatePlanForDirectMaterialization(Type lastSegmentType)
        {
            ProjectionPlan result = new ProjectionPlan();
            result.Plan = AtomMaterializerInvoker.DirectMaterializePlan;
            result.ProjectedType = lastSegmentType;
            result.LastSegmentType = lastSegmentType;
            return result;
        }

        /// <summary>Creates an entry materialization plan that is payload-driven and does not traverse expanded links.</summary>
        /// <param name="lastSegmentType">Segment type for the entry to materialize (typically last of URI in query).</param>
        /// <returns>A payload-driven materialization plan.</returns>
        private static ProjectionPlan CreatePlanForShallowMaterialization(Type lastSegmentType)
        {
            ProjectionPlan result = new ProjectionPlan();
            result.Plan = AtomMaterializerInvoker.ShallowMaterializePlan;
            result.ProjectedType = lastSegmentType;
            result.LastSegmentType = lastSegmentType;
            return result;
        }

        /// <summary>Gets a delegate that can be invoked to add an item to a collection of the specified type.</summary>
        /// <param name='listType'>Type of list to use.</param>
        /// <returns>The delegate to invoke.</returns>
        private static Action<object, object> GetAddToCollectionDelegate(Type listType)
        {
            Debug.Assert(listType != null, "listType != null");
            
            Type listElementType;
            MethodInfo addMethod = ClientType.GetAddToCollectionMethod(listType, out listElementType);
            ParameterExpression list = Expression.Parameter(typeof(object), "list");
            ParameterExpression item = Expression.Parameter(typeof(object), "element");
            Expression body = Expression.Call(Expression.Convert(list, listType), addMethod, Expression.Convert(item, listElementType));
#if ASTORIA_LIGHT
            LambdaExpression lambda = ExpressionHelpers.CreateLambda(body, list, item);
#else
            LambdaExpression lambda = Expression.Lambda(body, list, item);
#endif
            return (Action<object, object>)lambda.Compile();
        }

        /// <summary>
        /// Gets or creates a collection property on the specified <paramref name="instance"/>.
        /// </summary>
        /// <param name="instance">Instance on which to get/create the collection.</param>
        /// <param name="property">Collection property on the <paramref name="instance"/>.</param>
        /// <param name="collectionType">Type to use as a collection, possibly null.</param>
        /// <returns>
        /// The collection corresponding to the specified <paramref name="property"/>;
        /// never null.
        /// </returns>
        private static object GetOrCreateCollectionProperty(object instance, ClientType.ClientProperty property, Type collectionType)
        {
            Debug.Assert(instance != null, "instance != null");
            Debug.Assert(property != null, "property != null");
            Debug.Assert(property.CollectionType != null, "property.CollectionType != null -- otherwise property isn't a collection");

            // NOTE: in V1, we would have instantiated nested objects before setting them.
            object result;
            result = property.GetValue(instance);
            if (result == null)
            {
                if (collectionType == null)
                {
                    collectionType = property.PropertyType;
                    if (collectionType.IsInterface)
                    {
                        collectionType = typeof(System.Collections.ObjectModel.Collection<>).MakeGenericType(property.CollectionType);
                    }
                }

                result = Activator.CreateInstance(collectionType);
                property.SetValue(instance, result, property.PropertyName, false /* add */);
            }

            Debug.Assert(result != null, "result != null -- otherwise GetOrCreateCollectionProperty didn't fall back to creation");
            return result;
        }

        /// <summary>Materializes the result of a projection into a list.</summary>
        /// <param name="materializer">Materializer to use for the operation.</param>
        /// <param name="list">Target list.</param>
        /// <param name="nestedExpectedType">Expected type for nested object.</param>
        /// <param name="entries">Entries to materialize from.</param>
        /// <remarks>
        /// This method supports projections and as such does shallow payload-driven
        /// materialization of entities.
        /// </remarks>
        private static void MaterializeToList(
            AtomMaterializer materializer,
            IEnumerable list,
            Type nestedExpectedType,
            IEnumerable<AtomEntry> entries)
        {
            Debug.Assert(materializer != null, "materializer != null");
            Debug.Assert(list != null, "list != null");

            Action<object, object> addMethod = GetAddToCollectionDelegate(list.GetType());
            foreach (AtomEntry feedEntry in entries)
            {
                if (!feedEntry.EntityHasBeenResolved)
                {
                    materializer.Materialize(feedEntry, nestedExpectedType, /* includeLinks */ false);
                }

                addMethod(list, feedEntry.ResolvedObject);
            }
        }

        /// <summary>Materializes a single value.</summary>
        /// <param name="type">Type of value to set.</param>
        /// <param name="atomProperty">Property holding value.</param>
        /// <param name="context">Context under which value will be set.</param>
        /// <returns>true if the value was set; false if it wasn't (typically because it's a complex value).</returns>
        private static bool MaterializeDataValue(Type type, AtomContentProperty atomProperty, DataServiceContext context)
        {
            Debug.Assert(type != null, "type != null");
            Debug.Assert(atomProperty != null, "atomProperty != null");
            Debug.Assert(context != null, "context != null");

            string propertyTypeName = atomProperty.TypeName;
            string propertyValueText = atomProperty.Text;

            ClientType nestedElementType = null;
            Type underlyingType = Nullable.GetUnderlyingType(type) ?? type;
            bool knownType = ClientConvert.IsKnownType(underlyingType);
            if (!knownType)
            {
                nestedElementType = MaterializeAtom.GetEntryClientType(propertyTypeName, context, type, true);
                Debug.Assert(nestedElementType != null, "nestedElementType != null -- otherwise ReadTypeAttribute (or someone!) should throw");
                knownType = ClientConvert.IsKnownType(nestedElementType.ElementType);
            }

            if (knownType)
            {
                if (atomProperty.IsNull)
                {
                    if (!ClientType.CanAssignNull(type))
                    {
                        throw new InvalidOperationException(Strings.AtomMaterializer_CannotAssignNull(atomProperty.Name, type.FullName));
                    }

                    atomProperty.MaterializedValue = null;
                    return true;
                }
                else
                {
                    object value = propertyValueText;
                    if (propertyValueText != null)
                    {
                        value = ClientConvert.ChangeType(propertyValueText, (null != nestedElementType ? nestedElementType.ElementType : underlyingType));
                    }

                    atomProperty.MaterializedValue = value;
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Materializes the primitive data vlues in the given list of <paramref name="values"/>.
        /// </summary>
        /// <param name="actualType">Actual type for properties being materialized.</param>
        /// <param name="values">List of values to materialize.</param>
        /// <param name="ignoreMissingProperties">
        /// Whether properties missing from the client types should be ignored.
        /// </param>
        /// <param name="context">Data context, used for type resolution.</param>
        /// <remarks>
        /// Values are materialized in-place withi each <see cref="AtomContentProperty"/>
        /// instance.
        /// </remarks>
        private static void MaterializeDataValues(
            ClientType actualType, 
            List<AtomContentProperty> values,
            bool ignoreMissingProperties,
            DataServiceContext context)
        {
            Debug.Assert(actualType != null, "actualType != null");
            Debug.Assert(values != null, "values != null");
            Debug.Assert(context != null, "context != null");

            foreach (var atomProperty in values)
            {
                string propertyName = atomProperty.Name;
                
                var property = actualType.GetProperty(propertyName, ignoreMissingProperties); // may throw
                if (property == null)
                {
                    continue;
                }

                if (atomProperty.Feed == null && atomProperty.Entry == null)
                {
                    bool materialized = MaterializeDataValue(property.NullablePropertyType, atomProperty, context);
                    if (!materialized && property.CollectionType != null)
                    {
                        // client object property is collection implying nested collection of complex objects
                        throw Error.NotSupported(Strings.ClientType_CollectionOfNonEntities);
                    }
                }
            }
        }

        /// <summary>Applies a data value to the specified <paramref name="instance"/>.</summary>
        /// <param name="type">Type to which a property value will be applied.</param>
        /// <param name="property">Property with value to apply.</param>
        /// <param name="ignoreMissingProperties">
        /// Whether properties missing from the client types should be ignored.
        /// </param>
        /// <param name="context">Data context, used for type resolution.</param>
        /// <param name="instance">Instance on which value will be applied.</param>
        private static void ApplyDataValue(ClientType type, AtomContentProperty property, bool ignoreMissingProperties, DataServiceContext context, object instance)
        {
            Debug.Assert(type != null, "type != null");
            Debug.Assert(property != null, "property != null");
            Debug.Assert(context != null, "context != null");
            Debug.Assert(instance != null, "instance != context");

            var prop = type.GetProperty(property.Name, ignoreMissingProperties);
            if (prop == null)
            {
                return;
            }

            if (property.Properties != null)
            {
                if (prop.IsKnownType ||
                    ClientConvert.IsKnownType(MaterializeAtom.GetEntryClientType(property.TypeName, context, prop.PropertyType, true).ElementType))
                {
                    // The error message is a bit odd, but it's compatible with V1.
                    throw Error.InvalidOperation(Strings.Deserialize_ExpectingSimpleValue);
                }

                // Complex type.
                bool needToSet = false;
                ClientType complexType = ClientType.Create(prop.PropertyType);
                object complexInstance = prop.GetValue(instance);
                if (complexInstance == null)
                {
                    complexInstance = complexType.CreateInstance();
                    needToSet = true;
                }

                MaterializeDataValues(complexType, property.Properties, ignoreMissingProperties, context);
                ApplyDataValues(complexType, property.Properties, ignoreMissingProperties, context, complexInstance);

                if (needToSet)
                {
                    prop.SetValue(instance, complexInstance, property.Name, true /* allowAdd? */);
                }
            }
            else
            {
                prop.SetValue(instance, property.MaterializedValue, property.Name, true /* allowAdd? */);
            }
        }

        /// <summary>
        /// Applies the values of the specified <paramref name="properties"/> to a
        /// given <paramref name="instance"/>.
        /// </summary>
        /// <param name="type">Type to which properties will be applied.</param>
        /// <param name="properties">Properties to assign to the specified <paramref name="instance"/>.</param>
        /// <param name="ignoreMissingProperties">
        /// Whether properties missing from the client types should be ignored.
        /// </param>
        /// <param name="context">Data context, used for type resolution.</param>
        /// <param name="instance">Instance on which values will be applied.</param>
        private static void ApplyDataValues(ClientType type, IEnumerable<AtomContentProperty> properties, bool ignoreMissingProperties, DataServiceContext context, object instance)
        {
            Debug.Assert(type != null, "type != null");
            Debug.Assert(properties != null, "properties != null");
            Debug.Assert(context != null, "properties != context");
            Debug.Assert(instance != null, "instance != context");

            foreach (var p in properties)
            {
                ApplyDataValue(type, p, ignoreMissingProperties, context, instance);
            }
        }

        /// <summary>
        /// Sets a value on the property identified by the given <paramref name="path"/>.
        /// </summary>
        /// <param name="values">Starting list of properties to set.</param>
        /// <param name="path">List of paths, delimited by '/' characters.</param>
        /// <param name="value">Value to set in text form.</param>
        /// <param name="typeName">Name of type of value to set.</param>
        private static void SetValueOnPath(List<AtomContentProperty> values, string path, string value, string typeName)
        {
            Debug.Assert(values != null, "values != null");
            Debug.Assert(path != null, "path != null");

            bool existing = true;
            AtomContentProperty property = null;
            foreach (string step in path.Split('/'))
            {
                if (values == null)
                {
                    Debug.Assert(property != null, "property != null -- if values is null then this isn't the first step");
                    property.EnsureProperties();
                    values = property.Properties;
                }

                property = values.Where(v => v.Name == step).FirstOrDefault();
                if (property == null)
                {
                    AtomContentProperty newProperty = new AtomContentProperty();
                    existing = false;
                    newProperty.Name = step;
                    values.Add(newProperty);
                    property = newProperty;
                }
                else
                {
                    // If any property along the way was already marked as null in the payload,
                    // we can exit early as there will be nothing to do in any
                    // further nested types.
                    if (property.IsNull)
                    {
                        // 
                        return;
                    }
                }

                values = property.Properties;
            }

            Debug.Assert(property != null, "property != null -- property path should have at least one segment");
            
            // Content wins, therefore only set values if the property didn't exist before.
            if (existing == false)
            {
                property.TypeName = typeName;
                property.Text = value;
            }
        }

        /// <summary>
        /// Applies all available Entity Property Mappings on the specified <paramref name="entry"/>.
        /// </summary>
        /// <param name="entry">ATOM entry on which to apply entity property mappings.</param>
        /// <param name="entryType">Client type used to read EPM info from.</param>
        private static void ApplyEntityPropertyMappings(AtomEntry entry, ClientType entryType)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(entry.Tag is XElement, "entry.Tag is XElement");
            Debug.Assert(entryType != null, "entryType != null -- othewise how would we know to apply property mappings (note that for projections entry.ActualType may be different that entryType)?");
            Debug.Assert(!entry.EntityPropertyMappingsApplied, "!entry.EntityPropertyMappingsApplied -- EPM should happen only once per entry");

            if (entryType.HasEntityPropertyMappings)
            {
                // NOTE: this is an XElement-based version of the code.
                // There is a streaming version which has been since removed at:
                // - %sdxroot%\ndp\fx\src\DataWeb\Client\System\Data\Services\Client\Epm\EpmContentDeserializer.cs
                // - %sdxroot%\ndp\fx\src\DataWeb\Client\System\Data\Services\Client\Epm\EpmReaderState.cs
                XElement entryElement = entry.Tag as XElement;
                Debug.Assert(entryElement != null, "entryElement != null");
                ApplyEntityPropertyMappings(entry, entryElement, entryType.EpmTargetTree.SyndicationRoot);
                ApplyEntityPropertyMappings(entry, entryElement, entryType.EpmTargetTree.NonSyndicationRoot);
            }

            entry.EntityPropertyMappingsApplied = true;
        }

        /// <summary>
        /// Applies Entity Property Mappings on the specified 
        /// <paramref name="entry"/> given a <paramref name="target"/> root.
        /// </summary>
        /// <param name="entry">ATOM entry on which to apply entity property mappings.</param>
        /// <param name="entryElement">XML tree for the specified.</param>
        /// <param name="target">Target for mapping.</param>
        private static void ApplyEntityPropertyMappings(AtomEntry entry, XElement entryElement, EpmTargetPathSegment target)
        {
            Debug.Assert(target != null, "target != null");
            Debug.Assert(!target.HasContent, "!target.HasContent");

            // 
            Stack<System.Data.Services.Common.EpmTargetPathSegment> segments = new Stack<System.Data.Services.Common.EpmTargetPathSegment>();
            Stack<XElement> elements = new Stack<XElement>();

            segments.Push(target);
            elements.Push(entryElement);

            while (segments.Count > 0)
            {
                System.Data.Services.Common.EpmTargetPathSegment segment = segments.Pop();
                XElement element = elements.Pop();
                if (segment.HasContent)
                {
                    var node = element.Nodes().Where(n => n.NodeType == XmlNodeType.Text || n.NodeType == XmlNodeType.SignificantWhitespace).FirstOrDefault();
                    string elementValue = (node == null) ? null : ((XText)node).Value;
                    Debug.Assert(segment.EpmInfo != null, "segment.EpmInfo != null -- otherwise segment.HasValue should be false");

                    string path = segment.EpmInfo.Attribute.SourcePath;
                    string typeName = (string)element.Attribute(XName.Get(XmlConstants.AtomTypeAttributeName, XmlConstants.DataWebMetadataNamespace));

                    // 
                    SetValueOnPath(entry.DataValues, path, elementValue, typeName);
                }

                foreach (var item in segment.SubSegments)
                {
                    if (item.IsAttribute)
                    {
                        string localName = item.SegmentName.Substring(1);
                        var attribute = element.Attribute(XName.Get(localName, item.SegmentNamespaceUri));
                        if (attribute != null)
                        {
                            // NOTE: nulls can't be expressed on the attribute; null values
                            // are required to always be part of the main content instead.
                            SetValueOnPath(entry.DataValues, item.EpmInfo.Attribute.SourcePath, attribute.Value, null);
                        }
                    }
                    else
                    {
                        var child = element.Element(XName.Get(item.SegmentName, item.SegmentNamespaceUri));
                        if (child != null)
                        {
                            segments.Push(item);
                            elements.Push(child);
                        }
                    }
                }

                Debug.Assert(segments.Count == elements.Count, "segments.Count == elements.Count -- otherwise they're out of sync");
            }
        }

        /// <summary>Gets a property from the specified <paramref name="properties"/> list, throwing if not found.</summary>
        /// <param name="properties">List to get value from.</param>
        /// <param name="propertyName">Property name to look up.</param>
        /// <returns>The specified property (never null).</returns>
        private static AtomContentProperty GetPropertyOrThrow(List<AtomContentProperty> properties, string propertyName)
        {
            AtomContentProperty atomProperty = null;
            if (properties != null)
            {
                atomProperty = properties.Where(p => p.Name == propertyName).FirstOrDefault();
            }

            if (atomProperty == null)
            {
                throw new InvalidOperationException(Strings.AtomMaterializer_PropertyMissing(propertyName));
            }

            Debug.Assert(atomProperty != null, "atomProperty != null");
            return atomProperty;
        }

        /// <summary>Gets a property from the specified <paramref name="entry"/>, throwing if not found.</summary>
        /// <param name="entry">Entry to get value from.</param>
        /// <param name="propertyName">Property name to look up.</param>
        /// <returns>The specified property (never null).</returns>
        private static AtomContentProperty GetPropertyOrThrow(AtomEntry entry, string propertyName)
        {
            AtomContentProperty atomProperty = null;
            var properties = entry.DataValues;
            if (properties != null)
            {
                atomProperty = properties.Where(p => p.Name == propertyName).FirstOrDefault();
            }

            if (atomProperty == null)
            {
                throw new InvalidOperationException(Strings.AtomMaterializer_PropertyMissingFromEntry(propertyName, entry.Identity));
            }

            Debug.Assert(atomProperty != null, "atomProperty != null");
            return atomProperty;
        }

        /// <summary>Merges a list into the property of a given <paramref name="entry"/>.</summary>
        /// <param name="entry">Entry to merge into.</param>
        /// <param name="property">Property on entry to merge into.</param>
        /// <param name="list">List of materialized values.</param>
        /// <param name="nextLink">Next link for feed from which the materialized values come from.</param>
        /// <param name="plan">Projection plan for the list.</param>
        /// <remarks>
        /// This method will handle entries that shouldn't be updated correctly.
        /// </remarks>
        private void MergeLists(AtomEntry entry, ClientType.ClientProperty property, IEnumerable list, Uri nextLink, ProjectionPlan plan)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(entry.ResolvedObject != null, "entry.ResolvedObject != null");
            Debug.Assert(property != null, "property != null");
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(plan != null || nextLink == null, "plan != null || nextLink == null");

            // Simple case: the list is of the target type, and the resolved entity
            // has null; we can simply assign the collection. No merge required.
            if (entry.ShouldUpdateFromPayload && 
                property.NullablePropertyType == list.GetType() && 
                property.GetValue(entry.ResolvedObject) == null)
            {
                property.SetValue(entry.ResolvedObject, list, property.PropertyName, false /* allowAdd */);
                this.FoundNextLinkForCollection(list, nextLink, plan);

                foreach (object item in list)
                {
                    this.log.AddedLink(entry, property.PropertyName, item);
                }

                return;
            }

            this.ApplyItemsToCollection(entry, property, list, nextLink, plan);
        }

        /// <summary>Tries to resolve the object as the target one in a POST refresh.</summary>
        /// <param name="entry">Entry to resolve.</param>
        /// <returns>true if the entity was resolved; false otherwise.</returns>
        private bool TryResolveAsTarget(AtomEntry entry)
        {
            if (entry.ResolvedObject == null)
            {
                return false;
            }

            // The only case when the entity hasn't been resolved but
            // it has already been set is when the target instance
            // was set directly to refresh a POST.
            Debug.Assert(
                entry.ResolvedObject == this.TargetInstance,
                "entry.ResolvedObject == this.TargetInstance -- otherwise there we ResolveOrCreateInstance more than once on the same entry");
            Debug.Assert(
                this.mergeOption == MergeOption.OverwriteChanges || this.mergeOption == MergeOption.PreserveChanges,
                "MergeOption.OverwriteChanges and MergeOption.PreserveChanges are the only expected values during SaveChanges");
            entry.ActualType = ClientType.Create(entry.ResolvedObject.GetType());
            this.log.FoundTargetInstance(entry);
            entry.ShouldUpdateFromPayload = this.mergeOption == MergeOption.PreserveChanges ? false : true;
            entry.EntityHasBeenResolved = true;
            return true;
        }

        /// <summary>Tries to resolve the object as one from the context (only if tracking is enabled).</summary>
        /// <param name="entry">Entry to resolve.</param>
        /// <param name="expectedEntryType">Expected entry type for the specified <paramref name="entry"/>.</param>
        /// <returns>true if the entity was resolved; false otherwise.</returns>
        private bool TryResolveFromContext(AtomEntry entry, Type expectedEntryType)
        {
            // We should either create a new instance or grab one from the context.
            bool tracking = this.mergeOption != MergeOption.NoTracking;
            if (tracking)
            {
                EntityStates state;
                entry.ResolvedObject = this.context.TryGetEntity(entry.Identity, entry.ETagText, this.mergeOption, out state);
                if (entry.ResolvedObject != null)
                {
                    if (!expectedEntryType.IsInstanceOfType(entry.ResolvedObject))
                    {
                        throw Error.InvalidOperation(Strings.Deserialize_Current(expectedEntryType, entry.ResolvedObject.GetType()));
                    }

                    entry.ActualType = ClientType.Create(entry.ResolvedObject.GetType());
                    entry.EntityHasBeenResolved = true;

                    // Note that deleted items will have their properties overwritten even
                    // if PreserveChanges is used as a merge option.
                    entry.ShouldUpdateFromPayload =
                        this.mergeOption == MergeOption.OverwriteChanges ||
                        (this.mergeOption == MergeOption.PreserveChanges && state == EntityStates.Unchanged) ||
                        (this.mergeOption == MergeOption.PreserveChanges && state == EntityStates.Deleted);
                    this.log.FoundExistingInstance(entry);

                    return true;
                }
            }

            return false;
        }

        /// <summary>"Resolved" the entity in the <paramref name="entry"/> by instantiating it.</summary>
        /// <param name="entry">Entry to resolve.</param>
        /// <param name="type">Type to create.</param>
        /// <remarks>
        /// After invocation, entry.ResolvedObject is exactly of type <paramref name="type"/>.
        /// </remarks>
        private void ResolveByCreatingWithType(AtomEntry entry, Type type)
        {
            Debug.Assert(
                entry.ResolvedObject == null,
                "entry.ResolvedObject == null -- otherwise we're about to overwrite - should never be called");
            entry.ActualType = ClientType.Create(type);
            entry.ResolvedObject = Activator.CreateInstance(type);
            entry.CreatedByMaterializer = true;
            entry.ShouldUpdateFromPayload = true;
            entry.EntityHasBeenResolved = true;
            this.log.CreatedInstance(entry);
        }

        /// <summary>"Resolved" the entity in the <paramref name="entry"/> by instantiating it.</summary>
        /// <param name="entry">Entry to resolve.</param>
        /// <param name="expectedEntryType">Type expected by the projection.</param>
        /// <remarks>
        /// This method allows the expected entry to be overriden by either a callback
        /// or by the type read by the parser.
        /// </remarks>
        private void ResolveByCreating(AtomEntry entry, Type expectedEntryType)
        {
            Debug.Assert(
                entry.ResolvedObject == null,
                "entry.ResolvedObject == null -- otherwise we're about to overwrite - should never be called");

            ClientType actualType = MaterializeAtom.GetEntryClientType(entry.TypeName, this.context, expectedEntryType, true);

            // We can't assert that actualType.HasKeys, as there are cases where keys won't be defined, and we still
            // support deserializing them.
            Debug.Assert(actualType != null, "actualType != null -- otherwise ClientType.Create returned a null value");
            this.ResolveByCreatingWithType(entry, actualType.ElementType);
        }

        /// <summary>Tries to resolve the object from those created in this materialization session.</summary>
        /// <param name="entry">Entry to resolve.</param>
        /// <returns>true if the entity was resolved; false otherwise.</returns>
        private bool TryResolveAsCreated(AtomEntry entry)
        {
            AtomEntry existingEntry;
            if (!this.log.TryResolve(entry, out existingEntry))
            {
                return false;
            }

            Debug.Assert(
                existingEntry.ResolvedObject != null, 
                "existingEntry.ResolvedObject != null -- how did it get there otherwise?");
            entry.ActualType = existingEntry.ActualType;
            entry.ResolvedObject = existingEntry.ResolvedObject;
            entry.CreatedByMaterializer = existingEntry.CreatedByMaterializer;
            entry.ShouldUpdateFromPayload = existingEntry.ShouldUpdateFromPayload;
            entry.EntityHasBeenResolved = true;
            return true;
        }

        /// <summary>Resolved or creates an instance on the specified <paramref name="entry"/>.</summary>
        /// <param name="entry">Entry on which to resolve or create an instance.</param>
        /// <param name="expectedEntryType">Expected type for the <paramref name="entry"/>.</param>
        /// <remarks>
        /// After invocation, the ResolvedObject value of the <paramref name="entry"/>
        /// will be assigned, along with the ActualType value.
        /// </remarks>
        private void ResolveOrCreateInstance(AtomEntry entry, Type expectedEntryType)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(expectedEntryType != null, "expectedEntryType != null");
            Debug.Assert(entry.EntityHasBeenResolved == false, "entry.EntityHasBeenResolved == false");

            // This will be the case when TargetInstance has been set.
            if (!this.TryResolveAsTarget(entry))
            {
                if (entry.Identity == null)
                {
                    throw Error.InvalidOperation(Strings.Deserialize_MissingIdElement);
                }

                if (!this.TryResolveAsCreated(entry))
                {
                    if (!this.TryResolveFromContext(entry, expectedEntryType))
                    {
                        this.ResolveByCreating(entry, expectedEntryType);
                    }
                }
            }

            Debug.Assert(entry.ActualType != null, "entry.ActualType != null");
            Debug.Assert(entry.ResolvedObject != null, "entry.ResolvedObject != null");
            Debug.Assert(entry.EntityHasBeenResolved, "entry.EntityHasBeenResolved");

            return;
        }

        /// <summary>
        /// Applies the values of a nested <paramref name="feed"/> to the collection
        /// <paramref name="property"/> of the specified <paramref name="entry"/>.
        /// </summary>
        /// <param name="entry">Entry with collection to be modified.</param>
        /// <param name="property">Collection property on the entry.</param>
        /// <param name="feed">Values to apply onto the collection.</param>
        /// <param name="includeLinks">Whether links that are expanded should be materialized.</param>
        private void ApplyFeedToCollection(
            AtomEntry entry,
            ClientType.ClientProperty property,
            AtomFeed feed,
            bool includeLinks)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(property != null, "property != null");
            Debug.Assert(feed != null, "feed != null");

            ClientType collectionType = ClientType.Create(property.CollectionType);
            foreach (AtomEntry feedEntry in feed.Entries)
            {
                this.Materialize(feedEntry, collectionType.ElementType, includeLinks);
            }

            ProjectionPlan continuationPlan = includeLinks ? CreatePlanForDirectMaterialization(property.CollectionType) : CreatePlanForShallowMaterialization(property.CollectionType);
            this.ApplyItemsToCollection(entry, property, feed.Entries.Select(e => e.ResolvedObject), feed.NextLink, continuationPlan);
        }

        /// <summary>
        /// Applies the values of the <paramref name="items"/> enumeration to the
        /// <paramref name="property"/> of the specified <paramref name="entry"/>.
        /// </summary>
        /// <param name="entry">Entry with collection to be modified.</param>
        /// <param name="property">Collection property on the entry.</param>
        /// <param name="items">Values to apply onto the collection.</param>
        /// <param name="nextLink">Next link for collection continuation.</param>
        /// <param name="continuationPlan">Projection plan for collection continuation.</param>
        private void ApplyItemsToCollection(
            AtomEntry entry,
            ClientType.ClientProperty property,
            IEnumerable items,
            Uri nextLink,
            ProjectionPlan continuationPlan)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(property != null, "property != null");
            Debug.Assert(items != null, "items != null");

            object collection = entry.ShouldUpdateFromPayload ? GetOrCreateCollectionProperty(entry.ResolvedObject, property, null) : null;
            ClientType collectionType = ClientType.Create(property.CollectionType);
            foreach (object item in items)
            {
                if (!collectionType.ElementType.IsAssignableFrom(item.GetType()))
                {
                    string message = Strings.AtomMaterializer_EntryIntoCollectionMismatch(
                        item.GetType().FullName,
                        collectionType.ElementType.FullName);
                    throw new InvalidOperationException(message);
                }

                if (entry.ShouldUpdateFromPayload)
                {
                    property.SetValue(collection, item, property.PropertyName, true /* allowAdd? */);
                    this.log.AddedLink(entry, property.PropertyName, item);
                }
            }

            if (entry.ShouldUpdateFromPayload)
            {
                this.FoundNextLinkForCollection(collection as IEnumerable, nextLink, continuationPlan);
            }
            else
            {
                this.FoundNextLinkForUnmodifiedCollection(property.GetValue(entry.ResolvedObject) as IEnumerable);
            }

            // Remove the extra items from the collection as necessary.
            if (this.mergeOption == MergeOption.OverwriteChanges || this.mergeOption == MergeOption.PreserveChanges)
            {
                var itemsToRemove =
                    from x in this.context.GetLinks(entry.ResolvedObject, property.PropertyName)
                    where MergeOption.OverwriteChanges == this.mergeOption || EntityStates.Added != x.State
                    select x.Target;
                itemsToRemove = itemsToRemove.Except(EnumerateAsElementType<object>(items));
                foreach (var item in itemsToRemove)
                {
                    if (collection != null)
                    {
                        property.RemoveValue(collection, item);
                    }

                    this.log.RemovedLink(entry, property.PropertyName, item);
                }
            }
        }

        /// <summary>Records the fact that a rel='next' link was found for the specified <paramref name="collection"/>.</summary>
        /// <param name="collection">Collection to add link to.</param>
        /// <param name="link">Link (possibly null).</param>
        /// <param name="plan">Projection plan for the collection (null allowed only if link is null).</param>
        private void FoundNextLinkForCollection(IEnumerable collection, Uri link, ProjectionPlan plan)
        {
            Debug.Assert(plan != null || link == null, "plan != null || link == null");

            if (collection != null && !this.nextLinkTable.ContainsKey(collection))
            {
                DataServiceQueryContinuation continuation = DataServiceQueryContinuation.Create(link, plan);
                this.nextLinkTable.Add(collection, continuation);
                Util.SetNextLinkForCollection(collection, continuation);
            }
        }

        /// <summary>Records the fact that a <paramref name="collection"/> was found but won't be modified.</summary>
        /// <param name="collection">Collection to add link to.</param>
        private void FoundNextLinkForUnmodifiedCollection(IEnumerable collection)
        {
            if (collection != null && !this.nextLinkTable.ContainsKey(collection))
            {
                this.nextLinkTable.Add(collection, null);
            }
        }

        /// <summary>Materializes the specified <paramref name="entry"/>.</summary>
        /// <param name="entry">Entry with object to materialize.</param>
        /// <param name="expectedEntryType">Expected type for the entry.</param>
        /// <param name="includeLinks">Whether links that are expanded should be materialized.</param>
        /// <remarks>This is a payload-driven materialization process.</remarks>
        private void Materialize(AtomEntry entry, Type expectedEntryType, bool includeLinks)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(entry.DataValues != null, "entry.DataValues != null -- otherwise not correctly initialized");
            Debug.Assert(
                entry.ResolvedObject == null || entry.ResolvedObject == this.targetInstance,
                "entry.ResolvedObject == null || entry.ResolvedObject == this.targetInstance -- otherwise getting called twice");
            Debug.Assert(expectedEntryType != null, "expectedType != null");

            // ResolvedObject will already be assigned when we have a TargetInstance, for example.
            this.ResolveOrCreateInstance(entry, expectedEntryType);
            Debug.Assert(entry.ResolvedObject != null, "entry.ResolvedObject != null -- otherwise ResolveOrCreateInstnace didn't do its job");

            this.MaterializeResolvedEntry(entry, includeLinks);
        }

        /// <summary>Materializes the specified <paramref name="entry"/>.</summary>
        /// <param name="entry">Entry with object to materialize.</param>
        /// <param name="includeLinks">Whether links that are expanded should be materialized.</param>
        /// <remarks>This is a payload-driven materialization process.</remarks>
        private void MaterializeResolvedEntry(AtomEntry entry, bool includeLinks)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(entry.ResolvedObject != null, "entry.ResolvedObject != null -- otherwise not resolved/created!");

            ClientType actualType = entry.ActualType;

            if (!entry.EntityPropertyMappingsApplied)
            {
                // EPM should happen only once per entry
                ApplyEntityPropertyMappings(entry, entry.ActualType);
            }

            // Note that even if ShouldUpdateFromPayload is false, we will still be creating
            // nested instances (but not their links), so they show up in the data context
            // entries. This keeps this code compatible with V1 behavior.
            MaterializeDataValues(actualType, entry.DataValues, this.ignoreMissingProperties, this.context);

            foreach (var e in entry.DataValues)
            {
                var prop = actualType.GetProperty(e.Name, this.ignoreMissingProperties);
                if (prop == null)
                {
                    continue;
                }

                if (entry.ShouldUpdateFromPayload == false && e.Entry == null && e.Feed == null)
                {
                    // Skip non-links for ShouldUpdateFromPayload.
                    continue;
                }

                if (!includeLinks && (e.Entry != null || e.Feed != null))
                {
                    continue;
                }

                ValidatePropertyMatch(prop, e);

                AtomFeed feedValue = e.Feed;
                if (feedValue != null)
                {
                    Debug.Assert(includeLinks, "includeLinks -- otherwise we shouldn't be materializing this entry");
                    this.ApplyFeedToCollection(entry, prop, feedValue, includeLinks);
                }
                else if (e.Entry != null)
                {
                    if (!e.IsNull)
                    {
                        Debug.Assert(includeLinks, "includeLinks -- otherwise we shouldn't be materializing this entry");
                        this.Materialize(e.Entry, prop.PropertyType, includeLinks);
                    }

                    if (entry.ShouldUpdateFromPayload)
                    {
                        prop.SetValue(entry.ResolvedObject, e.Entry.ResolvedObject, e.Name, true /* allowAdd? */);
                        this.log.SetLink(entry, prop.PropertyName, e.Entry.ResolvedObject);
                    }
                }
                else
                {
                    Debug.Assert(entry.ShouldUpdateFromPayload, "entry.ShouldUpdateFromPayload -- otherwise we're about to set a property we shouldn't");
                    ApplyDataValue(actualType, e, this.ignoreMissingProperties, this.context, entry.ResolvedObject);
                }
            }

            Debug.Assert(entry.ResolvedObject != null, "entry.ResolvedObject != null -- otherwise we didn't do any useful work");
            if (this.materializedObjectCallback != null)
            {
                this.materializedObjectCallback(entry.Tag, entry.ResolvedObject);
            }
        }

        #endregion Private methods.
    }
}
