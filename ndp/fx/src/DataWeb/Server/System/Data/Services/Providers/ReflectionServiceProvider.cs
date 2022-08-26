//---------------------------------------------------------------------
// <copyright file="ReflectionServiceProvider.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides the interface definition for web data service
//      data sources.
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
    using System.Data.Services.Caching;
    using System.Data.Services.Common;
    using System.Diagnostics;
    using System.Linq;
    using System.Reflection;
    using System.Text;
    using System.Xml;

    #endregion Namespaces.

    /// <summary>
    /// Provides a reflection-based provider implementation.
    /// </summary>
    [DebuggerDisplay("ReflectionServiceProvider: {Type}")]
    internal class ReflectionServiceProvider : BaseServiceProvider
    {
        /// <summary>
        /// Initializes a new System.Data.Services.ReflectionServiceProvider instance.
        /// </summary>
        /// <param name="metadata">Metadata for this provider.</param>
        /// <param name="dataServiceInstance">data service instance.</param>
        internal ReflectionServiceProvider(MetadataCacheItem metadata, object dataServiceInstance)
            : base(metadata, dataServiceInstance)
        {
        }

        /// <summary>Gets a value indicating whether null propagation is required in expression trees.</summary>
        public override bool IsNullPropagationRequired
        {
            get { return true; }
        }

        /// <summary>Namespace name for the EDM container.</summary>
        public override string ContainerNamespace
        {
            get { return this.Type.Namespace; }
        }

        /// <summary>Name of the EDM container</summary>
        public override string ContainerName
        {
            get { return this.Type.Name; }
        }

        /// <summary>
        /// Gets the ResourceAssociationSet instance when given the source association end.
        /// </summary>
        /// <param name="resourceSet">Resource set of the source association end.</param>
        /// <param name="resourceType">Resource type of the source association end.</param>
        /// <param name="resourceProperty">Resource property of the source association end.</param>
        /// <returns>ResourceAssociationSet instance.</returns>
        public override ResourceAssociationSet GetResourceAssociationSet(ResourceSet resourceSet, ResourceType resourceType, ResourceProperty resourceProperty)
        {
            Debug.Assert(resourceSet != null, "resourceSet != null");
            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(resourceProperty != null, "resourceProperty != null");
            Debug.Assert(resourceType == DataServiceProviderWrapper.GetDeclaringTypeForProperty(resourceType, resourceProperty), "resourceType should be the declaring type for resourceProperty");

            ResourceType targetType = resourceProperty.ResourceType;
            Debug.Assert(targetType != null && targetType.ResourceTypeKind == ResourceTypeKind.EntityType, "targetType != null && targetType.ResourceTypeKind == ResourceTypeKind.EntityType");

            ResourceSet targetSet = InternalGetContainerForResourceType(targetType.InstanceType, this.EntitySets.Values);
            Debug.Assert(targetSet != null, "targetSet != null");

            string associationName = resourceType.Name + '_' + resourceProperty.Name;

            // EF associations are first-class, navigation properties come second. So you actually
            // define the two-way association first, then say that the nav props "go through" them.
            // For CLR, however, there is no such constraint - in fact, we're very happy with one-way (link) associations.
            // For one-way associations, the target property is always null.
            ResourceAssociationSetEnd sourceEnd = new ResourceAssociationSetEnd(resourceSet, resourceType, resourceProperty);
            ResourceAssociationSetEnd targetEnd = new ResourceAssociationSetEnd(targetSet, targetType, null);
            return new ResourceAssociationSet(associationName, sourceEnd, targetEnd);
        }

        /// <summary>
        /// Checks whether the specified <paramref name="type"/> is ordered.
        /// </summary>
        /// <param name="type">Type to check.</param>
        /// <returns>true if the type may be ordered; false otherwise.</returns>
        /// <remarks>
        /// The ordering may still fail at runtime; this method is currently
        /// used for cleaner error messages only.
        /// </remarks>
        public override bool GetTypeIsOrdered(Type type)
        {
            Debug.Assert(type != null, "type != null");
            if (typeof(IComparable).IsAssignableFrom(type))
            {
                return true;
            }
            else
            {
                return base.GetTypeIsOrdered(type);
            }
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
        public override IQueryable ApplyProjections(
            IQueryable source,
            ProjectionNode projection)
        {
            Debug.Assert(projection is RootProjectionNode, "We always get the special root node.");
            RootProjectionNode rootNode = (RootProjectionNode)projection;
            Debug.Assert(rootNode.OrderingInfo != null, "We always get non-null OrderingInfo");
            bool useBasicExpandProvider = ShouldUseBasicExpandProvider(rootNode);

            // We need the $skiptoken for top level result if it is paged, hence we need to use ApplyExpansions in that case
            if (useBasicExpandProvider || rootNode.OrderingInfo.IsPaged || rootNode.ProjectionsSpecified)
            {
                return new BasicExpandProvider(this.ProviderWrapper, true, true).ApplyProjections(source, projection);
            }

            // This pass-through implementation is appropriate for providers that fault-in on demand.
            return BasicExpandProvider.ApplyOrderSkipTakeOnTopLevelResultBeforeProjections(
                source,
                rootNode.OrderingInfo,
                rootNode.SkipCount,
                rootNode.TakeCount);
        }

        #region IDataServiceQueryProvider Methods

        /// <summary>
        /// Returns the collection of open properties name and value for the given resource instance.
        /// </summary>
        /// <param name="target">instance of the resource.</param>
        /// <returns>Returns the collection of open properties name and value for the given resource instance. Currently not supported for Reflection provider.</returns>
        public override IEnumerable<KeyValuePair<string, object>> GetOpenPropertyValues(object target)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Gets the value of the open property.
        /// </summary>
        /// <param name="target">instance of the resource type.</param>
        /// <param name="propertyName">name of the property.</param>
        /// <returns>the value of the open property. Currently this is not supported for Reflection provider.</returns>
        public override object GetOpenPropertyValue(object target, string propertyName)
        {
            throw new NotImplementedException();
        }

        #endregion IDataServiceQueryProvider Methods

        /// <summary>Checks whether the given property is a key property.</summary>
        /// <param name="property">property to check</param>
        /// <param name="keyKind">returns the key kind of the property, based on the heuristic it matches</param>
        /// <returns>true if this is a key property, else returns false</returns>
        internal static bool IsPropertyKeyProperty(PropertyInfo property, out ResourceKeyKind keyKind)
        {
            keyKind = (ResourceKeyKind)(-1);

            // Only primitive types are allowed to be keys.
            // Checks for generic to exclude Nullable<> value-type primitives, since we don't allows keys to be null.
            if (WebUtil.IsPrimitiveType(property.PropertyType) &&
                !property.PropertyType.IsGenericType)
            {
                DataServiceKeyAttribute keyAttribute = property.ReflectedType.GetCustomAttributes(true).OfType<DataServiceKeyAttribute>().FirstOrDefault();
                if (keyAttribute != null && keyAttribute.KeyNames.Contains(property.Name))
                {
                    keyKind = ResourceKeyKind.AttributedKey;
                    return true;
                }

                // For now, the key property must be {TypeName}Id or Id and the property
                // type must be primitive, since we do not support non-primitive types
                // as keys
                if (property.Name == property.DeclaringType.Name + "ID")
                {
                    keyKind = ResourceKeyKind.TypeNameId;
                    return true;
                }
                else if (property.Name == "ID")
                {
                    keyKind = ResourceKeyKind.Id;
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Checks whether the provider implements IUpdatable.
        /// </summary>
        /// <returns>returns true if the provider implements IUpdatable. otherwise returns false.</returns>
        internal override bool ImplementsIUpdatable()
        {
            return typeof(IUpdatable).IsAssignableFrom(this.Type);
        }

        /// <summary>Populates the metadata for this provider.</summary>
        /// <param name="knownTypes">Dictionary of known CLR to ResourceType entries, which is populated as metadata is built.</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <param name="entitySets">Dictionary of name to ResourceSet for entity sets, populated as metadata is built.</param>
        protected override void PopulateMetadata(
            IDictionary<Type, ResourceType> knownTypes,
            IDictionary<ResourceType, List<ResourceType>> childTypes,
            IDictionary<string, ResourceSet> entitySets)
        {
            Queue<ResourceType> unvisitedTypes = new Queue<ResourceType>();

            // Get the list of properties to be ignored.
            List<string> propertiesToBeIgnored = new List<string>(
                IgnorePropertiesAttribute.GetProperties(this.Type, true /*inherit*/, WebUtil.PublicInstanceBindingFlags));
            PropertyInfo[] properties = this.Type.GetProperties(WebUtil.PublicInstanceBindingFlags);
            foreach (PropertyInfo property in properties)
            {
                if (!propertiesToBeIgnored.Contains(property.Name) && property.CanRead && property.GetIndexParameters().Length == 0)
                {
                    Type elementType = BaseServiceProvider.GetIQueryableElement(property.PropertyType);
                    if (elementType != null)
                    {
                        // If the element type has key defined (in itself or one of its ancestors)
                        ResourceType resourceType = BuildHierarchyForEntityType(elementType, knownTypes, childTypes, unvisitedTypes, true /* entity type candidate */);
                        if (resourceType != null)
                        {
                            // We do not allow MEST scenario for reflection provider
                            foreach (KeyValuePair<string, ResourceSet> entitySetInfo in entitySets)
                            {
                                Type entitySetType = entitySetInfo.Value.ResourceType.InstanceType;
                                if (entitySetType.IsAssignableFrom(elementType))
                                {
                                    throw new InvalidOperationException(Strings.ReflectionProvider_MultipleEntitySetsForSameType(entitySetInfo.Value.Name, property.Name, entitySetType.FullName, resourceType.FullName));
                                }
                                else if (elementType.IsAssignableFrom(entitySetType))
                                {
                                    throw new InvalidOperationException(Strings.ReflectionProvider_MultipleEntitySetsForSameType(property.Name, entitySetInfo.Value.Name, resourceType.FullName, entitySetType.FullName));
                                }
                            }

                            // Add the entity set to the list of entity sets.
                            ResourceSet resourceContainer = new ResourceSet(property.Name, resourceType);
                            entitySets.Add(property.Name, resourceContainer);
                        }
                        else
                        {
                            throw new InvalidOperationException(Strings.ReflectionProvider_InvalidEntitySetProperty(property.Name, XmlConvert.EncodeName(((IDataServiceMetadataProvider)this).ContainerName)));
                        }
                    }
                }
            }

            // Populate the metadata for all the types in unvisited types 
            // and also their properties and populates metadata about property types
            PopulateMetadataForTypes(knownTypes, childTypes, unvisitedTypes, entitySets.Values);

            // At this point, we should have all the top level entity types and the complex types
            PopulateMetadataForDerivedTypes(knownTypes, childTypes, unvisitedTypes, entitySets.Values);

            // Populate and initialize the EntityPropertyMappingInfos for the data context
            foreach (ResourceType resourceType in knownTypes.Values)
            {
                resourceType.BuildReflectionEpmInfo(resourceType);
                resourceType.EpmInfoInitialized = true;
            }
        }

        /// <summary>
        /// Creates the IQueryable instance for the given resource set and returns it
        /// </summary>
        /// <param name="resourceContainer">resource set for which IQueryable instance needs to be created</param>
        /// <returns>returns the IQueryable instance for the given resource set</returns>
        protected override IQueryable GetResourceContainerInstance(ResourceSet resourceContainer)
        {
            Debug.Assert(resourceContainer != null, "resourceContainer != null");
            if (resourceContainer.ReadFromContextDelegate == null)
            {
                PropertyInfo propertyInfo = this.Type.GetProperty(resourceContainer.Name, WebUtil.PublicInstanceBindingFlags);
                MethodInfo getValueMethod = propertyInfo.GetGetMethod();

                // return ((TheContext)arg0).get_Property();
                Type[] parameterTypes = new Type[] { typeof(object) };
                System.Reflection.Emit.DynamicMethod readerMethod = new System.Reflection.Emit.DynamicMethod("queryable_reader", typeof(IQueryable), parameterTypes, false);
                var generator = readerMethod.GetILGenerator();
                generator.Emit(System.Reflection.Emit.OpCodes.Ldarg_0);
                generator.Emit(System.Reflection.Emit.OpCodes.Castclass, this.Type);
                generator.Emit(System.Reflection.Emit.OpCodes.Call, getValueMethod);
                generator.Emit(System.Reflection.Emit.OpCodes.Ret);
                resourceContainer.ReadFromContextDelegate = (Func<object, IQueryable>)readerMethod.CreateDelegate(typeof(Func<object, IQueryable>));
            }

            Debug.Assert(resourceContainer.ReadFromContextDelegate != null, "resourceContainer.ReadFromContextDelegate != null");
            return resourceContainer.ReadFromContextDelegate(this.CurrentDataSource);
        }

        /// <summary>
        /// Populate types for metadata specified by the provider
        /// </summary>
        /// <param name="userSpecifiedTypes">list of types specified by the provider</param>
        /// <param name="knownTypes">list of already known types</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <param name="entitySets">list of entity sets as specified in the data source type</param>
        protected override void PopulateMetadataForUserSpecifiedTypes(
            IEnumerable<Type> userSpecifiedTypes,
            IDictionary<Type, ResourceType> knownTypes,
            IDictionary<ResourceType, List<ResourceType>> childTypes,
            IEnumerable<ResourceSet> entitySets)
        {
            Queue<ResourceType> unvisitedTypes = new Queue<ResourceType>();
            foreach (Type type in userSpecifiedTypes)
            {
                ResourceType resourceType;
                if (TryGetType(knownTypes, type, out resourceType))
                {
                    continue;
                }

                if (IsEntityOrComplexType(type, knownTypes, childTypes, unvisitedTypes) == null)
                {
                    throw new InvalidOperationException(Strings.BadProvider_InvalidTypeSpecified(type.FullName));
                }
            }

            PopulateMetadataForTypes(knownTypes, childTypes, unvisitedTypes, entitySets);
        }

        /// <summary>
        /// Populate metadata for the given clr type.
        /// </summary>
        /// <param name="type">type whose metadata needs to be loaded.</param>
        /// <param name="knownTypes">list of already known resource types.</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <param name="entitySets">list of entity sets as specified in the data source.</param>
        /// <returns>resource type containing metadata for the given clr type.</returns>
        protected override ResourceType PopulateMetadataForType(
            Type type,
            IDictionary<Type, ResourceType> knownTypes,
            IDictionary<ResourceType, List<ResourceType>> childTypes,
            IEnumerable<ResourceSet> entitySets)
        {
            Queue<ResourceType> unvisitedTypes = new Queue<ResourceType>();
            ResourceType resourceType;
            if (!TryGetType(knownTypes, type, out resourceType))
            {
                resourceType = IsEntityOrComplexType(type, knownTypes, childTypes, unvisitedTypes);
                if (resourceType != null)
                {
                    PopulateMetadataForTypes(knownTypes, childTypes, unvisitedTypes, entitySets);
                }
            }

            return resourceType;
        }

        /// <summary>Checks whether the specified type is a complex type.</summary>
        /// <param name="type">Type to check.</param>
        /// <returns>
        /// true if the specified type is a complex type; false otherwise. Note
        /// that resources are not distinguished from complex types.
        /// </returns>
        private static bool IsComplexType(Type type)
        {
            Debug.Assert(type != null, "type != null");

            // Complex types are all types that contain public properties of primitive
            // types.
            //
            // We purposefully ignore certain known classes which fit this description
            // but we know are not meaningful for Astoria:
            // - System.Array:  what would get serialized would be Length, IsFixed, etc.
            // - Pointers:      we would otherwise serialize the pointer size
            // - COM object wrappers
            // - interface: since we will never know what the exact type of the instance will be.
            if (!type.IsVisible || type.IsArray || type.IsPointer || type.IsCOMObject || type.IsInterface ||
                type == typeof(IntPtr) || type == typeof(UIntPtr) || type == typeof(char) ||
                type == typeof(TimeSpan) || type == typeof(DateTimeOffset) || type == typeof(Uri) ||
                type.IsEnum)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Checks whether there is a key defined for the given type.
        /// </summary>
        /// <param name="type">type to check </param>
        /// <param name="entityTypeCandidate">
        /// Whether <paramref name="type"/> is being considered as a possible 
        /// entity type.
        /// </param>
        /// <returns>returns true if there are one or key properties present else returns false</returns>
        private static bool DoesTypeHaveKeyProperties(Type type, bool entityTypeCandidate)
        {
            Debug.Assert(type != null, "type != null");

            // Check for properties declared on this element only
            foreach (PropertyInfo property in type.GetProperties(WebUtil.PublicInstanceBindingFlags | BindingFlags.DeclaredOnly))
            {
                ResourceKeyKind keyKind;
                if (IsPropertyKeyProperty(property, out keyKind))
                {
                    if (keyKind == ResourceKeyKind.AttributedKey && !entityTypeCandidate)
                    {
                        throw new InvalidOperationException(Strings.ReflectionProvider_EntityTypeHasKeyButNoEntitySet(type.FullName));
                    }

                    if (!entityTypeCandidate)
                    {
                        return false;
                    }

                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Populates the metadata for the given unvisited types and all the associated types with this type
        /// </summary>
        /// <param name="knownTypes">list of known types</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <param name="unvisitedTypes">list of unvisited type</param>
        /// <param name="entitySets">Available entity sets.</param>
        private static void PopulateMetadataForTypes(
            IDictionary<Type, ResourceType> knownTypes, 
            IDictionary<ResourceType, List<ResourceType>> childTypes,
            Queue<ResourceType> unvisitedTypes, 
            IEnumerable<ResourceSet> entitySets)
        {
            Debug.Assert(knownTypes != null, "knownTypes != null");
            Debug.Assert(unvisitedTypes != null, "unvisitedTypes != null");
            Debug.Assert(entitySets != null, "entitySets != null");

            // Start walking down all the types
            while (unvisitedTypes.Count != 0)
            {
                // get the unvisited element
                ResourceType type = unvisitedTypes.Dequeue();

                // Go through all the properties and find out one or more complex types
                BuildTypeProperties(type, knownTypes, childTypes, unvisitedTypes, entitySets);
            }
        }

        /// <summary>
        /// Walks through the list of ancestors and finds the root base type and collects metadata for the entire chain of ancestors
        /// </summary>
        /// <param name="type">type whose ancestors metadata needs to be populated</param>
        /// <param name="knownTypes">list of already known types</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <param name="unvisitedTypes">list of unvisited types</param>
        /// <param name="entityTypeCandidate">Whether <paramref name="type"/> is a candidate to be an entity type.</param>
        /// <returns>return true if this given type is a entity type, otherwise returns false</returns>
        private static ResourceType BuildHierarchyForEntityType(
            Type type,
            IDictionary<Type, ResourceType> knownTypes,
            IDictionary<ResourceType, List<ResourceType>> childTypes,
            Queue<ResourceType> unvisitedTypes,
            bool entityTypeCandidate)
        {
            List<Type> ancestors = new List<Type>();

            if (!type.IsVisible)
            {
                return null;
            }

            if (CommonUtil.IsUnsupportedType(type))
            {
                // deriving from an unsupported type is not allowed
                throw new InvalidOperationException(Strings.BadProvider_UnsupportedType(type.FullName));
            }

            Type baseType = type;
            ResourceType baseResourceType = null;

            // Since this method is also used on property types, which can be interfaces,
            // Base types can be null
            while (baseType != null)
            {
                // Try and check if the base type is already loaded
                if (TryGetType(knownTypes, baseType, out baseResourceType))
                {
                    break;
                }

                ancestors.Add(baseType);
                baseType = baseType.BaseType;
            }

            if (baseResourceType == null)
            {
                // If entityTypeCandidate is false, then it means that the current type can't
                // be a entity type with keys. In other words, it must derive from an existing
                // type. Otherwise, its not an entity type
                if (entityTypeCandidate == false)
                {
                    return null;
                }

                // Find the last ancestor which has key defined
                for (int i = ancestors.Count - 1; i >= 0; i--)
                {
                    if (CommonUtil.IsUnsupportedType(ancestors[i]))
                    {
                        // deriving from an unsupported type is not allowed
                        throw new InvalidOperationException(Strings.BadProvider_UnsupportedAncestorType(type.FullName, ancestors[i].FullName));                        
                    }

                    if (DoesTypeHaveKeyProperties(ancestors[i], entityTypeCandidate))
                    {
                        break;
                    }

                    // Else this type is not interesting. Remove it from the ancestors list
                    ancestors.RemoveAt(i);
                }
            }
            else if (baseResourceType.ResourceTypeKind != ResourceTypeKind.EntityType)
            {
                return null;
            }
            else if (ancestors.Count == 0)
            {
                // we might have found the top level element.So just return
                return baseResourceType;
            }

            // For all the valid ancestors, add the type to the list of types encountered 
            // and unvisited types
            // its important that we enqueue the ancestors first, since when we populate member metadata
            // we can make sure that the base type is fully populated
            for (int i = ancestors.Count - 1; i >= 0; i--)
            {
                ResourceType entityType = ReflectionServiceProvider.CreateResourceType(ancestors[i], ResourceTypeKind.EntityType, baseResourceType, knownTypes, childTypes);
                unvisitedTypes.Enqueue(entityType);
                baseResourceType = entityType;
            }

            return baseResourceType;
        }

        /// <summary>
        /// Populates the metadata for the properties of the given resource type
        /// </summary>
        /// <param name="parentResourceType">resource type whose properties metadata needs to be populated</param>
        /// <param name="knownTypes">list of known types</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <param name="unvisitedTypes">list of unvisited type</param>
        /// <param name="entitySets">Available entity sets.</param>
        private static void BuildTypeProperties(
            ResourceType parentResourceType, 
            IDictionary<Type, ResourceType> knownTypes, 
            IDictionary<ResourceType, List<ResourceType>> childTypes,
            Queue<ResourceType> unvisitedTypes, 
            IEnumerable<ResourceSet> entitySets)
        {
            Debug.Assert(parentResourceType != null, "parentResourceType != null");
            Debug.Assert(knownTypes != null, "knownTypes != null");
            Debug.Assert(unvisitedTypes != null, "unvisitedTypes != null");
            Debug.Assert(entitySets != null, "entitySets != null");

            BindingFlags bindingFlags = WebUtil.PublicInstanceBindingFlags;

            // For non root types, we should only look for properties that are declared for this type
            if (parentResourceType.BaseType != null)
            {
                bindingFlags = bindingFlags | BindingFlags.DeclaredOnly;
            }

            HashSet<string> propertiesToBeIgnored = new HashSet<string>(IgnorePropertiesAttribute.GetProperties(parentResourceType.InstanceType, false /*inherit*/, bindingFlags), StringComparer.Ordinal);
            Debug.Assert(parentResourceType.IsOpenType == false, "ReflectionServiceProvider does not support Open types.");

            HashSet<string> etagPropertyNames = new HashSet<string>(LoadETagProperties(parentResourceType), StringComparer.Ordinal);

            ResourceKeyKind keyKind = (ResourceKeyKind)Int32.MaxValue;
            PropertyInfo[] properties = parentResourceType.InstanceType.GetProperties(bindingFlags);
            foreach (PropertyInfo property in properties)
            {
                // Ignore the properties which are specified in the IgnoreProperties attribute
                if (propertiesToBeIgnored.Contains(property.Name))
                {
                    continue;
                }

                if (property.CanRead && property.GetIndexParameters().Length == 0)
                {
                    ResourcePropertyKind kind = (ResourcePropertyKind)(-1);
                    ResourceKeyKind currentKeyKind = (ResourceKeyKind)(-1);
                    ResourceType resourceType;
                    Type resourcePropertyType = property.PropertyType;
                    ResourceSet container = null;
                    bool collection = false;

                    if (!TryGetType(knownTypes, resourcePropertyType, out resourceType))
                    {
                        Type collectionType = GetIEnumerableElement(property.PropertyType);
                        if (collectionType != null)
                        {
                            TryGetType(knownTypes, collectionType, out resourceType);

                            // Even if the above method returns false, we should set the
                            // following variable appropriately, so that we can use them below
                            collection = true;
                            resourcePropertyType = collectionType;
                        }
                    }

                    if (resourceType != null)
                    {
                        #region Already Known Type
                        if (resourceType.ResourceTypeKind == ResourceTypeKind.Primitive)
                        {
                            // Check for key property only on root types, since keys must be defined on the root types
                            if (parentResourceType.BaseType == null && parentResourceType.ResourceTypeKind == ResourceTypeKind.EntityType && IsPropertyKeyProperty(property, out currentKeyKind))
                            {
                                if ((int)currentKeyKind < (int)keyKind)
                                {
                                    if (parentResourceType.KeyProperties.Count != 0)
                                    {
                                        // Remove the existing property as key property - mark it as non key property
                                        parentResourceType.RemoveKeyProperties();
                                    }

                                    keyKind = currentKeyKind;
                                    kind = ResourcePropertyKind.Key | ResourcePropertyKind.Primitive;
                                }
                                else if ((int)currentKeyKind == (int)keyKind)
                                {
                                    Debug.Assert(currentKeyKind == ResourceKeyKind.AttributedKey, "This is the only way of specifying composite keys");
                                    kind = ResourcePropertyKind.Key | ResourcePropertyKind.Primitive;
                                }
                                else
                                {
                                    kind = ResourcePropertyKind.Primitive;
                                }
                            }
                            else
                            {
                                kind = ResourcePropertyKind.Primitive;
                            }
                        }
                        else if (resourceType.ResourceTypeKind == ResourceTypeKind.ComplexType)
                        {
                            kind = ResourcePropertyKind.ComplexType;
                        }
                        else if (resourceType.ResourceTypeKind == ResourceTypeKind.EntityType)
                        {
                            kind = collection ? ResourcePropertyKind.ResourceSetReference : ResourcePropertyKind.ResourceReference;
                        }
                        #endregion // Already Known Type
                    }
                    else
                    {
                        resourceType = IsEntityOrComplexType(resourcePropertyType, knownTypes, childTypes, unvisitedTypes);
                        if (resourceType != null)
                        {
                            if (resourceType.ResourceTypeKind == ResourceTypeKind.ComplexType)
                            {
                                kind = ResourcePropertyKind.ComplexType;
                            }
                            else
                            {
                                Debug.Assert(resourceType.ResourceTypeKind == ResourceTypeKind.EntityType, "Must be an entity type");
                                kind = collection ? ResourcePropertyKind.ResourceSetReference : ResourcePropertyKind.ResourceReference;
                            }
                        }
                    }

                    // if resource type is null OR
                    // if resource type is a collection of primitive or complex types OR
                    // if complex type has a property of entity type
                    if (resourceType == null ||
                        (resourceType.ResourceTypeKind != ResourceTypeKind.EntityType && collection) ||
                        (resourceType.ResourceTypeKind == ResourceTypeKind.EntityType && parentResourceType.ResourceTypeKind == ResourceTypeKind.ComplexType))
                    {
                        if (resourceType == null)
                        {
                            if (CommonUtil.IsUnsupportedType(resourcePropertyType))
                            {
                                throw new InvalidOperationException(Strings.BadProvider_UnsupportedPropertyType(property.Name, parentResourceType.FullName));
                            }

                            throw new InvalidOperationException(Strings.ReflectionProvider_InvalidProperty(property.Name, parentResourceType.FullName));
                        }
                        else
                        {
                            // collection of complex types not supported
                            throw new InvalidOperationException(Strings.ReflectionProvider_CollectionOfPrimitiveOrComplexNotSupported(property.Name, parentResourceType.FullName));
                        }
                    }

                    if (resourceType.ResourceTypeKind == ResourceTypeKind.EntityType)
                    {
                        container = InternalGetContainerForResourceType(resourcePropertyType, entitySets);
                        if (container == null)
                        {
                            throw new InvalidOperationException(Strings.ReflectionProvider_EntityPropertyWithNoEntitySet(parentResourceType.FullName, property.Name));
                        }
                    }

                    if (etagPropertyNames.Remove(property.Name))
                    {
                        kind |= ResourcePropertyKind.ETag;
                    }

                    ResourceProperty resourceProperty = new ResourceProperty(property.Name, kind, resourceType);
                    MimeTypeAttribute attribute = MimeTypeAttribute.GetMimeTypeAttribute(property);
                    if (attribute != null)
                    {
                        resourceProperty.MimeType = attribute.MimeType;
                    }

                    parentResourceType.AddProperty(resourceProperty);
                }
                else
                {
                    throw new InvalidOperationException(Strings.ReflectionProvider_InvalidProperty(property.Name, parentResourceType.FullName));
                }
            }

            if (parentResourceType.ResourceTypeKind == ResourceTypeKind.EntityType &&
                (parentResourceType.KeyProperties == null || parentResourceType.KeyProperties.Count == 0))
            {
                throw new InvalidOperationException(Strings.ReflectionProvider_KeyPropertiesCannotBeIgnored(parentResourceType.FullName));
            }

            if (etagPropertyNames.Count != 0)
            {
                throw new InvalidOperationException(Strings.ReflectionProvider_ETagPropertyNameNotValid(etagPropertyNames.ElementAt(0), parentResourceType.FullName));
            }
        }

        /// <summary>
        /// If the given type is a entity or complex type, it returns the resource type corresponding to the given type
        /// </summary>
        /// <param name="type">clr type</param>
        /// <param name="knownTypes">list of already known types</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <param name="unvisitedTypes">list of unvisited types</param>
        /// <returns>resource type corresponding to the given clr type, if the clr type is entity or complex</returns>
        private static ResourceType IsEntityOrComplexType(
            Type type,
            IDictionary<Type, ResourceType> knownTypes,
            IDictionary<ResourceType, List<ResourceType>> childTypes,
            Queue<ResourceType> unvisitedTypes)
        {
            // Ignore values types here. We do not support resources of values type (entity or complex)
            if (type.IsValueType || CommonUtil.IsUnsupportedType(type))
            {
                return null;
            }

            ResourceType resourceType = BuildHierarchyForEntityType(type, knownTypes, childTypes, unvisitedTypes, false /* entityTypeCandidate */);
            if (resourceType == null && IsComplexType(type))
            {
                resourceType = ReflectionServiceProvider.CreateResourceType(type, ResourceTypeKind.ComplexType, null, knownTypes, childTypes);
                unvisitedTypes.Enqueue(resourceType);
            }

            return resourceType;
        }

        /// <summary>Get the resource set for the given clr type.</summary>
        /// <param name="type">clr type for which resource set name needs to be returned</param>
        /// <param name="entitySets">Available entity sets to consider.</param>
        /// <returns>The container for its type, null if not found.</returns>
        private static ResourceSet InternalGetContainerForResourceType(Type type, IEnumerable<ResourceSet> entitySets)
        {
            Debug.Assert(type != null, "type != null");
            Debug.Assert(entitySets != null, "entitySets != null");

            // For each entity set, find out which one matches the type of this resource
            foreach (ResourceSet entitySetInfo in entitySets)
            {
                if (entitySetInfo.ResourceType.InstanceType.IsAssignableFrom(type))
                {
                    return entitySetInfo;
                }
            }

            return null;
        }

        /// <summary>
        /// Find out all the derived types in the list of assemblies and then populate metadata for those types
        /// </summary>
        /// <param name="knownTypes">list of known types</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <param name="unvisitedTypes">list of unvisited types</param>
        /// <param name="entitySets">Available entity sets.</param>
        private static void PopulateMetadataForDerivedTypes(
            IDictionary<Type, ResourceType> knownTypes,
            IDictionary<ResourceType, List<ResourceType>> childTypes,
            Queue<ResourceType> unvisitedTypes,
            IEnumerable<ResourceSet> entitySets)
        {
            Debug.Assert(knownTypes != null, "knownTypes != null");
            Debug.Assert(unvisitedTypes != null, "unvisitedTypes != null");
            Debug.Assert(entitySets != null, "entitySets != null");

            // Find all the root resource entity types
            List<ResourceType> rootTypes = new List<ResourceType>();
            foreach (ResourceType resourceType in knownTypes.Values)
            {
                if (resourceType.BaseType == null &&
                    resourceType.ResourceTypeKind == ResourceTypeKind.EntityType)
                {
                    rootTypes.Add(resourceType);
                }
            }

            // Use the default comparer, which calls Assembly.Equals (not a simple reference comparison).
            HashSet<Assembly> assemblies = new HashSet<Assembly>(EqualityComparer<Assembly>.Default);
            List<Type> derivedTypes = new List<Type>();

            // Walk through all the types in the assemblies and find all the derived types
            foreach (ResourceType resourceType in knownTypes.Values)
            {
                // No need to look into primitive types, as these live in system assemblies.
                if (resourceType.ResourceTypeKind == ResourceTypeKind.Primitive)
                {
                    continue;
                }

                Assembly assembly = resourceType.InstanceType.Assembly;
                //// ignore if the assembly has already been scanned
                if (assemblies.Contains(assembly))
                {
                    continue;
                }

                // Walk all the types in that assembly
                foreach (Type type in assembly.GetTypes())
                {
                    // skip all the non visible types or types which have generic parameters
                    if (!type.IsVisible || HasGenericParameters(type))
                    {
                        continue;
                    }

                    // Skip the type if its already loaded
                    if (knownTypes.ContainsKey(type))
                    {
                        continue;
                    }

                    // Check if this type dervies from any one of the root types
                    for (int i = 0; i < rootTypes.Count; i++)
                    {
                        if (rootTypes[i].InstanceType.IsAssignableFrom(type))
                        {
                            derivedTypes.Add(type);
                        }
                    }
                }

                assemblies.Add(assembly);
            }

            foreach (Type type in derivedTypes)
            {
                BuildHierarchyForEntityType(type, knownTypes, childTypes, unvisitedTypes, false /* entityTypeCandidate */);
                PopulateMetadataForTypes(knownTypes, childTypes, unvisitedTypes, entitySets);
            }
        }

        /// <summary>
        /// Loads the etag properties for the given resource type
        /// </summary>
        /// <param name="resourceType">resource type whose etag property names need to be loaded.</param>
        /// <returns>the list of properties that form the etag for the given resource type.</returns>
        private static IEnumerable<string> LoadETagProperties(ResourceType resourceType)
        {
            // if it is the root type, then we need to inherit the attribute from the base type.
            // otherwise, we need not, since if the base type already has it, the appropriate properties
            // must already have been marked as concurrency properties.
            bool inherit = resourceType.BaseType == null;

            // Read the etag attribute from the type and return it
            ETagAttribute[] attributes = (ETagAttribute[])resourceType.InstanceType.GetCustomAttributes(typeof(ETagAttribute), inherit);
            Debug.Assert(attributes.Length <= 1, "Only one attribute can be specified per type");

            if (attributes.Length == 1)
            {
                // Validate the property names
                // we may need to cache them instead of reading them everytime
                return attributes[0].PropertyNames;
            }

            return WebUtil.EmptyStringArray;
        }

        /// <summary>
        /// returns the new resource type instance
        /// </summary>
        /// <param name="type">backing clr type for the resource.</param>
        /// <param name="kind">kind of the resource.</param>
        /// <param name="baseType">base type of the resource.</param>
        /// <param name="knownTypes">list of already known types</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <returns>returns a new instance of the resource type containing all the metadata.</returns>
        private static ResourceType CreateResourceType(
            Type type,
            ResourceTypeKind kind,
            ResourceType baseType,
            IDictionary<Type, ResourceType> knownTypes,
            IDictionary<ResourceType, List<ResourceType>> childTypes)
        {
            ResourceType resourceType = new ResourceType(type, kind, baseType, type.Namespace, GetModelTypeName(type), type.IsAbstract);
            resourceType.IsOpenType = false;

            // We need to look at inherited attributes as well so we pass true for inherit argument. 
            if (type.GetCustomAttributes(typeof(HasStreamAttribute), true /* inherit */).Length == 1)
            {
                resourceType.IsMediaLinkEntry = true;
            }

            knownTypes.Add(type, resourceType);
            childTypes.Add(resourceType, null);
            if (baseType != null)
            {
                Debug.Assert(childTypes.ContainsKey(baseType), "childTypes.ContainsKey(baseType)");
                if (childTypes[baseType] == null)
                {
                    childTypes[baseType] = new List<ResourceType>();
                }

                childTypes[baseType].Add(resourceType);
            }

            return resourceType;
        }

        /// <summary>
        /// Gets the type name (without namespace) of the specified <paramref name="type"/>,
        /// appropriate as an externally-visible type name.
        /// </summary>
        /// <param name="type">Type to get name for.</param>
        /// <returns>The type name for <paramref name="type"/>.</returns>
        private static string GetModelTypeName(Type type)
        {
            Debug.Assert(type != null, "type != null");
            if (type.IsGenericType)
            {
                Type[] genericArguments = type.GetGenericArguments();
                StringBuilder builder = new StringBuilder(type.Name.Length * 2 * (1 + genericArguments.Length));
                if (type.IsNested)
                {
                    Debug.Assert(type.DeclaringType != null, "type.DeclaringType != null");
                    builder.Append(GetModelTypeName(type.DeclaringType));
                    builder.Append('_');
                }

                builder.Append(type.Name);
                builder.Append('[');
                for (int i = 0; i < genericArguments.Length; i++)
                {
                    if (i > 0)
                    {
                        builder.Append(' ');
                    }

                    string genericNamespace = WebUtil.GetModelTypeNamespace(genericArguments[i]);
                    if (!String.IsNullOrEmpty(genericNamespace))
                    {
                        builder.Append(genericNamespace);
                        builder.Append('.');
                    }

                    builder.Append(GetModelTypeName(genericArguments[i]));
                }

                builder.Append(']');
                return builder.ToString();
            }
            else if (type.IsNested)
            {
                Debug.Assert(type.DeclaringType != null, "type.DeclaringType != null");
                return GetModelTypeName(type.DeclaringType) + "_" + type.Name;
            }
            else
            {
                return type.Name;
            }
        }

        /// <summary>
        /// Checks whether the given type is a generic type with a generic parameter.
        /// </summary>
        /// <param name="type">type which needs to be checked.</param>
        /// <returns>Returns true, if the <paramref name="type"/> is generic and has generic parameters. Otherwise returns false.</returns>
        private static bool HasGenericParameters(Type type)
        {
            if (type.IsGenericType)
            {
                foreach (Type arg in type.GetGenericArguments())
                {
                    if (arg.IsGenericParameter)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Returns true if the subtree of expansions rooted in the specified <paramref name="expandedNode"/>
        /// contains either a filter or paging/maxresult constraint.
        /// </summary>
        /// <param name="expandedNode">The root of the expansions tree to inspect.</param>
        /// <returns>True if BasicExpandProvider should be used to process a query with this tree
        /// or false otherwise.</returns>
        private static bool ShouldUseBasicExpandProvider(ExpandedProjectionNode expandedNode)
        {
            foreach (ProjectionNode node in expandedNode.Nodes)
            {
                ExpandedProjectionNode childExpandedNode = node as ExpandedProjectionNode;
                if (childExpandedNode != null)
                {
                    if (childExpandedNode.HasFilterOrMaxResults)
                    {
                        return true;
                    }

                    if (ShouldUseBasicExpandProvider(childExpandedNode))
                    {
                        return true;
                    }
                }
            }

            return false;
        }
    }
}
