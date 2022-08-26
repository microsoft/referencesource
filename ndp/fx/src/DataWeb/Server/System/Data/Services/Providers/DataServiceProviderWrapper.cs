//---------------------------------------------------------------------
// <copyright file="DataServiceProviderWrapper.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides the wrapper over IDataServiceMetadataProvider and IDataServiceQueryProvider calls
//      so that we can bunch of validation in one place.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Data.Services.Serializers;
    using System.Diagnostics;
    using System.Linq;
    using System.Xml;
    using System.Data.Services.Caching;
    using System.Reflection;

    #endregion Namespaces.

    /// <summary>Schema version compliance of the metadata.</summary>
    internal enum MetadataEdmSchemaVersion
    {
        /// <summary>EDM v1.0 compliant.</summary>
        Version1Dot0,

        /// <summary>EDM v1.1 compliant.</summary>
        Version1Dot1,

        /// <summary>EDM v1.2 compliant.</summary>
        Version1Dot2,

        /// <summary>EDM v2.0 compliant.</summary>
        Version2Dot0,
    }

    /// <summary>
    /// Class to abstract IDataServiceMetadataProvider and IDataServiceQueryProvider, 
    /// hence making sure all the metadata and query provider calls are made via this class.
    /// 
    /// Each request must create a new instance of this class because a 
    /// request is the defined scope of metadata consistency.
    /// </summary>
    internal class DataServiceProviderWrapper
    {
        #region Private Fields

        /// <summary>
        /// Object reference that represents true. This is a workaround to the issue that 'Dictionary&lt;string, bool&gt;' will require
        /// JIT compilation at runtime for precompiled assemblies.  So we use 'Dictionary&lt;string, object&gt;' instead.
        /// </summary>
        private static readonly object RefTrue = new object();

        /// <summary>
        /// Object reference that represents false. This is a workaround to the issue that 'Dictionary&lt;string, bool&gt;' will require
        /// JIT compilation at runtime for precompiled assemblies.  So we use 'Dictionary&lt;string, object&gt;' instead.
        /// </summary>
        private static readonly object RefFalse = new object();

        /// <summary>
        /// Empty open property values
        /// </summary>
        private static readonly IEnumerable<KeyValuePair<string, object>> EmptyOpenPropertyValues = new KeyValuePair<string, object>[0];

        /// <summary>
        /// Metadata to be used by the service provider wrapper.
        /// </summary>
        private readonly MetadataCacheItem metadata;

        /// <summary>
        /// metadata provider instance.
        /// </summary>
        private IDataServiceMetadataProvider metadataProvider;

        /// <summary>
        /// metadata provider instance.
        /// </summary>
        private IDataServiceQueryProvider queryProvider;

        #endregion Private Fields

        #region Constructors

        /// <summary>
        /// Creates a new instance of DataServiceProviderWrapper instance.
        /// </summary>
        /// <param name="metadata">Metadata to be used by the service provider wrapper.</param>
        /// <param name="metadataProvider">instance of the metadata provider.</param>
        /// <param name="queryProvider">instance of the query provider.</param>
        internal DataServiceProviderWrapper(MetadataCacheItem metadata, IDataServiceMetadataProvider metadataProvider, IDataServiceQueryProvider queryProvider)
        {
            Debug.Assert(metadata != null, "metadata != null");
            Debug.Assert(metadataProvider != null, "metadataProvider != null");
            Debug.Assert(queryProvider != null, "queryProvider != null");
            Debug.Assert(
                metadataProvider is BaseServiceProvider && queryProvider is BaseServiceProvider || (metadata.ResourceSetWrapperCache.Count == 0 && metadata.VisibleTypeCache.Count == 0),
                "For V1 providers, both metadata and query providers must be BaseServiceProvider, otherwise the metadata cache must be empty for IDSP providers");

            this.metadata = metadata;
            this.metadataProvider = metadataProvider;
            this.queryProvider = queryProvider;
        }

        #endregion Constructors

        #region IDataServiceQueryProvider Properties

        /// <summary>The data source from which data is provided.</summary>
        public object CurrentDataSource
        {
            get { return this.queryProvider.CurrentDataSource; }
        }

        /// <summary>Gets a value indicating whether null propagation is required in expression trees.</summary>
        public bool NullPropagationRequired
        {
            get { return this.queryProvider.IsNullPropagationRequired; }
        }

        #endregion IDataServiceQueryProvider Properties

        #region IDataServiceMetadataProvider Properties

        /// <summary>Namespace name for the container.</summary>
        public string ContainerNamespace
        {
            get
            {
                string containerNamespace = this.metadataProvider.ContainerNamespace;

                // 752636 [Breaking Change]: Reflection Provider should not allow null ContainerNamespace
                // In V1 the reflection provider allows the namespace to be null. Fixing this would be a breaking change.
                // We will skip this check for V1 providers for now.
                if (string.IsNullOrEmpty(containerNamespace) && !this.IsV1Provider)
                {
                    throw new InvalidOperationException(Strings.DataServiceProviderWrapper_ContainerNamespaceMustNotBeNullOrEmpty);
                }

                return containerNamespace;
            }
        }

        /// <summary>Name of the container</summary>
        public string ContainerName
        {
            get
            {
                string containerName = this.metadataProvider.ContainerName;
                if (string.IsNullOrEmpty(containerName))
                {
                    throw new InvalidOperationException(Strings.DataServiceProviderWrapper_ContainerNameMustNotBeNullOrEmpty);
                }

                return containerName;
            }
        }

        /// <summary>
        /// Gets all visible containers.
        /// WARNING!!! This property can only be called for the $metadata path because it enumerates through all resource sets.
        /// Calling it from outside of the $metadata path would break our IDSP contract.
        /// </summary>
        public IEnumerable<ResourceSetWrapper> ResourceSets
        {
            get 
            {
                var resourceSets = this.metadataProvider.ResourceSets;
                if (resourceSets != null)
                {
                    HashSet<string> resourceSetNames = new HashSet<string>(EqualityComparer<string>.Default);

                    foreach (ResourceSet resourceSet in resourceSets)
                    {
                        // verify that the name of the resource set is unique
                        AddUniqueNameToSet(
                            resourceSet != null ? resourceSet.Name : null, 
                            resourceSetNames, 
                            Strings.DataServiceProviderWrapper_MultipleEntitySetsWithSameName(resourceSet.Name));

                        // For IDSP, we want to make sure the metadata object instance stay the same within
                        // a request because we do reference comparisons.  Note the provider can return 
                        // different metadata instances within the same request.  The the Validate*() methods
                        // will make sure to return the first cached instance.
                        ResourceSetWrapper resourceSetWrapper = this.ValidateResourceSet(resourceSet);
                        if (resourceSetWrapper != null)
                        {
                            yield return resourceSetWrapper;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Returns all types in this data source
        /// WARNING!!! This property can only be called for the $metadata path because it enumerates through all resource types.
        /// Calling it from outside of the $metadata path would break our IDSP contract.
        /// </summary>
        public IEnumerable<ResourceType> Types
        {
            get 
            {
                var types = this.metadataProvider.Types;
                if (types != null)
                {
                    HashSet<string> resourceTypeNames = new HashSet<string>(EqualityComparer<string>.Default);

                    foreach (ResourceType resourceType in types)
                    {
                        // verify that the name of the resource type is unique
                        AddUniqueNameToSet(
                            resourceType != null ? resourceType.Name : null, 
                            resourceTypeNames, 
                            Strings.DataServiceProviderWrapper_MultipleResourceTypesWithSameName(resourceType.Name));

                        // For IDSP, we want to make sure the metadata object instance stay the same within
                        // a request because we do reference comparisons.  Note the provider can return 
                        // different metadata instances within the same request.  The the Validate*() methods
                        // will make sure to return the first cached instance.
                        ResourceType type = this.ValidateResourceType(resourceType);
                        if (type != null)
                        {
                            yield return type;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Returns all the visible service operations in this data source.
        /// WARNING!!! This property can only be called for the $metadata path because it enumerates through all service operations.
        /// Calling it from outside of the $metadata path would break our IDSP contract.
        /// </summary>
        public IEnumerable<ServiceOperationWrapper> ServiceOperations
        {
            get 
            {
                var serviceOperations = this.metadataProvider.ServiceOperations;
                if (serviceOperations != null)
                {
                    HashSet<string> serviceOperationNames = new HashSet<string>(EqualityComparer<string>.Default);

                    foreach (ServiceOperation serviceOperation in serviceOperations)
                    {
                        // verify that the name of the service operation is unique
                        AddUniqueNameToSet(
                            serviceOperation != null ? serviceOperation.Name : null, 
                            serviceOperationNames,
                            Strings.DataServiceProviderWrapper_MultipleServiceOperationsWithSameName(serviceOperation.Name));

                        // For IDSP, we want to make sure the metadata object instance stay the same within
                        // a request because we do reference comparisons.  Note the provider can return 
                        // different metadata instances within the same request.  The the Validate*() methods
                        // will make sure to return the first cached instance.
                        ServiceOperationWrapper serviceOperationWrapper = this.ValidateServiceOperation(serviceOperation);
                        if (serviceOperationWrapper != null)
                        {
                            yield return serviceOperationWrapper;
                        }
                    }
                }
            }
        }

        #endregion IDataServiceMetadataProvider Properties

        #region Properties

        /// <summary>
        /// Cached configuration with access rights info.
        /// </summary>
        internal DataServiceConfiguration Configuration
        {
            [DebuggerStepThrough]
            get { return this.metadata.Configuration; }
        }

#if DEBUG
        /// <summary>
        /// Used for verifying expression generated for queries, checks if all the 
        /// </summary>
        internal bool AreAllResourceTypesNonOpen
        {
            get
            {
                return !this.VisibleTypeCache.Values.Any(rt => rt.IsOpenType);
            }
        }
#endif

        /// <summary>
        /// Returns true if the data provider is a V1 provider i.e. ReflectionServiceProvider or ObjectContextServiceProvider.
        /// Otherwise returns false.
        /// </summary>
        internal bool IsV1Provider
        {
            get { return this.metadataProvider is BaseServiceProvider; }
        }

        /// <summary>
        /// Returns the <see cref="IProjectionProvider"/> for this provider
        /// </summary>
        /// <returns>The <see cref="IProjectionProvider"/> for this provider</returns>
        /// <remarks>Note that this will only return non-null on V1 providers
        /// in which case it returns our V1 provider's implementation of this interface.
        /// In all other cases this returns null as we don't allow custom implementation of this interface yet.</remarks>
        internal IProjectionProvider ProjectionProvider
        {
            get
            {
                if (this.IsV1Provider)
                {
                    return (IProjectionProvider)this.metadataProvider;
                }
                else
                {
                    return null;
                }
            }
        }

        /// <summary>
        /// Keep track of the calculated visibility of resource types.
        /// </summary>
        private Dictionary<string, ResourceType> VisibleTypeCache
        {
            [DebuggerStepThrough]
            get { return this.metadata.VisibleTypeCache; }
        }

        /// <summary>
        /// Maps resource set names to ResourceSetWrappers.
        /// </summary>
        private Dictionary<string, ResourceSetWrapper> ResourceSetWrapperCache
        {
            [DebuggerStepThrough]
            get { return this.metadata.ResourceSetWrapperCache; }
        }

        /// <summary>
        /// Maps service operation names to ServiceOperationWrappers.
        /// </summary>
        private Dictionary<string, ServiceOperationWrapper> ServiceOperationWrapperCache
        {
            [DebuggerStepThrough]
            get { return this.metadata.ServiceOperationWrapperCache; }
        }

        /// <summary>
        /// Maps names to ResourceAssociationSets.
        /// </summary>
        private Dictionary<string, ResourceAssociationSet> ResourceAssociationSetCache
        {
            [DebuggerStepThrough]
            get { return this.metadata.ResourceAssociationSetCache; }
        }

        /// <summary>
        /// Mapes "resourceSetName_resourceTypeName" to the list of visible properties from the set.
        /// </summary>
        private Dictionary<string, List<ResourceProperty>> ResourcePropertyCache
        {
            [DebuggerStepThrough]
            get { return this.metadata.ResourcePropertyCache; }
        }

        /// <summary>
        /// Mapes "resourceSetName_resourceTypeName" to boolean of whether resourceType is allowed for resourceSet
        /// </summary>
        private Dictionary<string, object> EntityTypeDisallowedForSet
        {
            [DebuggerStepThrough]
            get { return this.metadata.EntityTypeDisallowedForSet; }
        }

        #endregion Properties

        #region IDataServiceQueryProvider Methods

#if DEBUG
        /// <summary>
        /// Returns the IQueryable that represents the container.
        /// </summary>
        /// <param name="resourceSet">resource set representing the entity set.</param>
        /// <returns>
        /// An IQueryable that represents the container; null if there is 
        /// no container for the specified name.
        /// </returns>
        public IQueryable GetQueryRootForResourceSet(ResourceSetWrapper resourceSet, IDataService dataService)
#else
        /// <summary>
        /// Returns the IQueryable that represents the container.
        /// </summary>
        /// <param name="resourceSet">resource set representing the entity set.</param>
        /// <returns>
        /// An IQueryable that represents the container; null if there is 
        /// no container for the specified name.
        /// </returns>
        public IQueryable GetQueryRootForResourceSet(ResourceSetWrapper resourceSet)
#endif
        {
            Debug.Assert(resourceSet != null, "resourceSet != null");
#if DEBUG
            dataService.ProcessingPipeline.AssertDebugStateDuringRequestProcessing(dataService);
#endif
            return this.queryProvider.GetQueryRootForResourceSet(resourceSet.ResourceSet);
        }

        /// <summary>Gets the <see cref="ResourceType"/> for the specified <paramref name="instance"/>.</summary>
        /// <param name="instance">Instance to extract a <see cref="ResourceType"/> from.</param>
        /// <returns>The <see cref="ResourceType"/> that describes this <paramref name="instance"/> in this provider.</returns>
        public ResourceType GetResourceType(object instance)
        {
            Debug.Assert(instance != null, "instance != null");
            return this.ValidateResourceType(this.queryProvider.GetResourceType(instance));
        }

        /// <summary>
        /// Get the value of the strongly typed property.
        /// </summary>
        /// <param name="target">instance of the type declaring the property.</param>
        /// <param name="resourceProperty">resource property describing the property.</param>
        /// <param name="resourceType">Resource type to which the property belongs.</param>
        /// <returns>value for the property.</returns>
        public object GetPropertyValue(object target, ResourceProperty resourceProperty, ResourceType resourceType)
        {
            Debug.Assert(target != null, "target != null");
            Debug.Assert(resourceProperty != null, "resourceProperty != null");
            Debug.Assert(resourceProperty.IsReadOnly, "resourceProperty.IsReadOnly");
            if (resourceProperty.CanReflectOnInstanceTypeProperty)
            {
                try
                {
                    if (resourceType == null)
                    {
                        resourceType = this.GetResourceType(target);
                    }

                    Debug.Assert(resourceType != null, "resourceType != null");
                    PropertyInfo propertyInfo = resourceType.GetPropertyInfo(resourceProperty);
                    Debug.Assert(propertyInfo != null, "propertyInfo != null");
                    return propertyInfo.GetGetMethod().Invoke(target, null);
                }
                catch (TargetInvocationException exception)
                {
                    ErrorHandler.HandleTargetInvocationException(exception);
                    throw;
                }
            }
            else
            {
                return this.queryProvider.GetPropertyValue(target, resourceProperty);
            }
        }

        /// <summary>
        /// Get the value of the open property.
        /// </summary>
        /// <param name="target">instance of the type declaring the open property.</param>
        /// <param name="propertyName">name of the open property.</param>
        /// <returns>value for the open property.</returns>
        public object GetOpenPropertyValue(object target, string propertyName)
        {
            Debug.Assert(target != null, "target != null");
            Debug.Assert(!string.IsNullOrEmpty(propertyName), "!string.IsNullOrEmpty(propertyName)");
            return this.queryProvider.GetOpenPropertyValue(target, propertyName);
        }

        /// <summary>
        /// Get the name and values of all the properties defined in the given instance of an open type.
        /// </summary>
        /// <param name="target">instance of a open type.</param>
        /// <returns>collection of name and values of all the open properties.</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1006:DoNotNestGenericTypesInMemberSignatures", Justification = "need to return a collection of key value pair")]
        public IEnumerable<KeyValuePair<string, object>> GetOpenPropertyValues(object target)
        {
            Debug.Assert(target != null, "target != null");
            IEnumerable<KeyValuePair<string, object>> result = this.queryProvider.GetOpenPropertyValues(target);
            if (result == null)
            {
                return EmptyOpenPropertyValues;
            }

            return result;
        }

        /// <summary>
        /// Invoke the given service operation and returns the results.
        /// </summary>
        /// <param name="serviceOperation">service operation to invoke.</param>
        /// <param name="parameters">value of parameters to pass to the service operation.</param>
        /// <returns>returns the result of the service operation. If the service operation returns void, then this should return null.</returns>
        public object InvokeServiceOperation(ServiceOperationWrapper serviceOperation, object[] parameters)
        {
            Debug.Assert(serviceOperation != null, "serviceOperation != null");
            return this.queryProvider.InvokeServiceOperation(serviceOperation.ServiceOperation, parameters);
        }

        #endregion IDataServiceQueryProvider Methods

        #region IDataServiceMetadataProvider Methods

        /// <summary>Given the specified name, tries to find a resource set.</summary>
        /// <param name="name">Name of the resource set to resolve.</param>
        /// <returns>Resolved resource set, possibly null.</returns>
        public ResourceSetWrapper TryResolveResourceSet(string name)
        {
            Debug.Assert(!string.IsNullOrEmpty(name), "!string.IsNullOrEmpty(name)");

            // For IDSP, we want to make sure the metadata object instance stay the same within
            // a request because we do reference comparisons.
            ResourceSetWrapper resourceSetWrapper;
            if (this.ResourceSetWrapperCache.TryGetValue(name, out resourceSetWrapper))
            {
                return resourceSetWrapper;
            }

            ResourceSet resourceSet;
            if (this.metadataProvider.TryResolveResourceSet(name, out resourceSet))
            {
                return this.ValidateResourceSet(resourceSet);
            }

            return null;
        }

        /// <summary>
        /// Gets the ResourceAssociationSet instance when given the source association end.
        /// </summary>
        /// <param name="resourceSet">Resource set of the source association end.</param>
        /// <param name="resourceType">Resource type of the source association end.</param>
        /// <param name="resourceProperty">Resource property of the source association end.</param>
        /// <returns>ResourceAssociationSet instance.</returns>
        public ResourceAssociationSet GetResourceAssociationSet(ResourceSetWrapper resourceSet, ResourceType resourceType, ResourceProperty resourceProperty)
        {
            Debug.Assert(resourceSet != null, "resourceSet != null");
            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(resourceProperty != null, "resourceProperty != null");

            // If the association set has already been cached, use the cached copy
            resourceType = GetDeclaringTypeForProperty(resourceType, resourceProperty);
            string associationSetKey = resourceSet.Name + '_' + resourceType.FullName + '_' + resourceProperty.Name;
            ResourceAssociationSet associationSet;
            if (this.ResourceAssociationSetCache.TryGetValue(associationSetKey, out associationSet))
            {
                return associationSet;
            }

            // Get the association set from the underlying provider.
            associationSet = this.metadataProvider.GetResourceAssociationSet(resourceSet.ResourceSet, resourceType, resourceProperty);
            if (associationSet != null)
            {
                ResourceAssociationSetEnd thisEnd = associationSet.GetResourceAssociationSetEnd(resourceSet, resourceType, resourceProperty);
                ResourceAssociationSetEnd relatedEnd = associationSet.GetRelatedResourceAssociationSetEnd(resourceSet, resourceType, resourceProperty);
                ResourceSetWrapper relatedSet = this.ValidateResourceSet(relatedEnd.ResourceSet);
                if (relatedSet == null)
                {
                    // If the related set is not visible, the association set is also not visible.
                    associationSet = null;
                }
                else
                {
                    ResourceType relatedType = this.ValidateResourceType(relatedEnd.ResourceType);
                    ResourceProperty relatedProperty = null;
                    if (relatedEnd.ResourceProperty != null)
                    {
                        relatedProperty = relatedType.TryResolvePropertyName(relatedEnd.ResourceProperty.Name);
                    }

                    // For IDSP, we want to make sure the metadata object instance stay the same within a request because we 
                    // do reference comparisons.  Note if the provider returns a ResourceAssociationSet with different instances
                    // of ResourceSet, ResourceType and ResourceProperty than what we've seen earlier in the same request, we
                    // create a new instance of the association set using the metadata objects we've already cached.
                    // If the metadata change should cause a failure, we want the IDSP to detect it and fail. At the Astoria runtime
                    // layer we assume the metadata objects to remain constant throughout the request.
                    resourceType = this.ValidateResourceType(thisEnd.ResourceType);
                    if (thisEnd.ResourceSet != resourceSet.ResourceSet ||
                        thisEnd.ResourceType != resourceType ||
                        thisEnd.ResourceProperty != resourceProperty ||
                        relatedEnd.ResourceSet != relatedSet.ResourceSet ||
                        relatedEnd.ResourceType != relatedType ||
                        relatedEnd.ResourceProperty != relatedProperty)
                    {
                        associationSet = new ResourceAssociationSet(
                            associationSet.Name,
                            new ResourceAssociationSetEnd(resourceSet.ResourceSet, resourceType, resourceProperty),
                            new ResourceAssociationSetEnd(relatedSet.ResourceSet, relatedType, relatedProperty));
                    }
                }
            }

            this.ResourceAssociationSetCache.Add(associationSetKey, associationSet);
            return associationSet;
        }

        /// <summary>Given the specified name, tries to find a type.</summary>
        /// <param name="name">Name of the type to resolve.</param>
        /// <returns>Resolved resource type, possibly null.</returns>
        public ResourceType TryResolveResourceType(string name)
        {
            Debug.Assert(!string.IsNullOrEmpty(name), "!string.IsNullOrEmpty(name)");

            // For IDSP, we want to make sure the metadata object instance stay the same within
            // a request because we do reference comparisons.
            ResourceType resourceType;
            if (this.VisibleTypeCache.TryGetValue(name, out resourceType))
            {
                return resourceType;
            }

            if (this.metadataProvider.TryResolveResourceType(name, out resourceType))
            {
                return this.ValidateResourceType(resourceType);
            }

            return null;
        }

        /// <summary>
        /// The method must return a collection of all the types derived from <paramref name="resourceType"/>.
        /// The collection returned should NOT include the type passed in as a parameter.
        /// An implementer of the interface should return null if the type does not have any derived types (ie. null == no derived types).
        /// </summary>
        /// <param name="resourceType">Resource to get derived resource types from.</param>
        /// <returns>
        /// A collection of resource types (<see cref="ResourceType"/>) derived from the specified <paramref name="resourceType"/> 
        /// or null if there no types derived from the specified <paramref name="resourceType"/> exist.
        /// </returns>
        public IEnumerable<ResourceType> GetDerivedTypes(ResourceType resourceType)
        {
            Debug.Assert(resourceType != null, "resourceType != null");

            var derivedTypes = this.metadataProvider.GetDerivedTypes(resourceType);
            if (derivedTypes != null)
            {
                foreach (ResourceType derivedType in derivedTypes)
                {
                    ResourceType type = this.ValidateResourceType(derivedType);
                    if (type != null)
                    {
                        yield return type;
                    }
                }
            }
        }

        /// <summary>
        /// Returns true if <paramref name="resourceType"/> represents an Entity Type which has derived Entity Types, else false.
        /// </summary>
        /// <param name="resourceType">instance of the resource type in question.</param>
        /// <returns>True if <paramref name="resourceType"/> represents an Entity Type which has derived Entity Types, else false.</returns>
        public bool HasDerivedTypes(ResourceType resourceType)
        {
            Debug.Assert(this.ValidateResourceType(resourceType) != null, "resourceType must be read-only and visible.");
            return this.metadataProvider.HasDerivedTypes(resourceType);
        }

        /// <summary>Given the specified name, tries to find a service operation.</summary>
        /// <param name="name">Name of the service operation to resolve.</param>
        /// <returns>Resolved service operation, possibly null.</returns>
        public ServiceOperationWrapper TryResolveServiceOperation(string name)
        {
            Debug.Assert(!string.IsNullOrEmpty(name), "!string.IsNullOrEmpty(name)");

            // For IDSP, we want to make sure the metadata object instance stay the same within
            // a request because we do reference comparisons.
            ServiceOperationWrapper serviceOperationWrapper;
            if (this.ServiceOperationWrapperCache.TryGetValue(name, out serviceOperationWrapper))
            {
                return serviceOperationWrapper;
            }

            ServiceOperation serviceOperation;
            if (this.metadataProvider.TryResolveServiceOperation(name, out serviceOperation))
            {
                return this.ValidateServiceOperation(serviceOperation);
            }

            return null;
        }

        #endregion IDataServiceMetadataProvider Methods

        #region Internal Methods

        /// <summary>
        /// Gets the resource type which the resource property is declared on.
        /// </summary>
        /// <param name="resourceType">resource type to start looking</param>
        /// <param name="resourceProperty">resource property in question</param>
        /// <returns>actual resource type that declares the property</returns>
        internal static ResourceType GetDeclaringTypeForProperty(ResourceType resourceType, ResourceProperty resourceProperty)
        {
            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(resourceProperty != null, "resourceProperty != null");

            while (resourceType != null)
            {
                if (resourceType.TryResolvePropertiesDeclaredOnThisTypeByName(resourceProperty.Name) != null)
                {
                    break;
                }

                resourceType = resourceType.BaseType;
            }

            Debug.Assert(resourceType != null, "resourceType != null");
            return resourceType;
        }

        /// <summary>Disposes of the metadata and query providers.</summary>
        internal void DisposeDataSource()
        {
            Debug.Assert(this.queryProvider != null, "this.queryProvider != null");
            Debug.Assert(this.metadataProvider != null, "this.metadataProvider != null");

            WebUtil.Dispose(this.metadataProvider);

            // If the same instance implements IDataServiceMetadataProvider and IDataServiceQueryProvider interface,
            // we call dispose only once.
            if (this.metadataProvider != this.queryProvider)
            {
                WebUtil.Dispose(this.queryProvider);
            }

            this.metadataProvider = null;
            this.queryProvider = null;
        }

        /// <summary>
        /// Iterates through the resource sets, service operations and resource types to pre-populate the metadata cache item.
        /// </summary>
        internal void PopulateMetadataCacheItemForV1Provider()
        {
            Debug.Assert(this.IsV1Provider, "this.IsV1Provider");

            // This is only called when we initialize the service for the first time.
            // The Count extention method will cause the iterator to instantiate the wrapper classes.
            this.ServiceOperations.Count();
            this.Types.Count();

            foreach (ResourceSetWrapper resourceSet in this.ResourceSets)
            {
                // Derived types of a visible type are visible
                foreach (ResourceType derivedType in this.GetDerivedTypes(resourceSet.ResourceType))
                {
                    this.GetResourceProperties(resourceSet, derivedType);
                    this.IsEntityTypeDisallowedForSet(resourceSet, derivedType);
                }

                // Base types of a visible type are visible
                ResourceType resourceType = resourceSet.ResourceType;
                while (resourceType != null)
                {
                    this.GetResourceProperties(resourceSet, resourceType);
                    this.IsEntityTypeDisallowedForSet(resourceSet, resourceType);
                    resourceType = resourceType.BaseType;
                }
            }
        }

        /// <summary>
        /// Gets the target container for the given navigation property, source container and the source resource type
        /// </summary>
        /// <param name="sourceContainer">source entity set.</param>
        /// <param name="sourceResourceType">source resource type.</param>
        /// <param name="navigationProperty">navigation property.</param>
        /// <returns>target container that the navigation property refers to.</returns>
        internal ResourceSetWrapper GetContainer(ResourceSetWrapper sourceContainer, ResourceType sourceResourceType, ResourceProperty navigationProperty)
        {
            ResourceAssociationSet associationSet = this.GetResourceAssociationSet(sourceContainer, sourceResourceType, navigationProperty);
            if (associationSet != null)
            {
                ResourceAssociationSetEnd relatedEnd = associationSet.GetRelatedResourceAssociationSetEnd(sourceContainer, sourceResourceType, navigationProperty);
                return this.ValidateResourceSet(relatedEnd.ResourceSet);
            }

            return null;
        }

        /// <summary>
        /// For a V1 provider checks the Epm compatiblity in order to write the appropriate 
        /// versioning header for metadata requests
        /// </summary>
        /// <returns>true if the provider is V1 compatible, false otherwise</returns>
        internal bool GetEpmCompatiblityForV1Provider()
        {
            Debug.Assert(this.IsV1Provider, "Must be a V1 provider to call this function");
            return (this.metadataProvider as BaseServiceProvider).EpmIsV1Compatible;
        }

        /// <summary>
        /// Return the list of ETag properties for a given type in the context of a given container
        /// </summary>
        /// <param name="containerName">Name of the container to use for context (for MEST-enabled providers)</param>
        /// <param name="resourceType">Type to get the ETag properties for</param>
        /// <returns>A collection of the properties that form the ETag for the given type in the given container</returns>
        internal IList<ResourceProperty> GetETagProperties(string containerName, ResourceType resourceType)
        {
            Debug.Assert(containerName != null || !(this.metadataProvider is ObjectContextServiceProvider), "ContainerName is required for MEST-enabled provider (EFx provider)");
            Debug.Assert(resourceType != null && resourceType.ResourceTypeKind == ResourceTypeKind.EntityType, "Resource should be non-null and of an entity type");

            // Note only primitive properties can be part of an etag, they are always visible.
            ObjectContextServiceProvider efxProvider = this.metadataProvider as ObjectContextServiceProvider;
            return efxProvider == null ? resourceType.ETagProperties :
                                         efxProvider.GetETagProperties(containerName, resourceType);
        }

        /// <summary>
        /// Gets the visible resource properties for <paramref name="resourceType"/> from <paramref name="resourceSet"/>.
        /// We cache the list of visible resource properties so we don't have to calculate it repeatedly when serializing feeds.
        /// </summary>
        /// <param name="resourceSet">Resource set in question.</param>
        /// <param name="resourceType">Resource type in question.</param>
        /// <returns>List of visible resource properties from the given resource set and resource type.</returns>
        internal IEnumerable<ResourceProperty> GetResourceProperties(ResourceSetWrapper resourceSet, ResourceType resourceType)
        {
            Debug.Assert(resourceType != null, "resourceType != null");
            if (resourceType.ResourceTypeKind == ResourceTypeKind.EntityType)
            {
                Debug.Assert(resourceSet != null, "resourceSet != null");
                string key = resourceSet.Name + '_' + resourceType.FullName;
                List<ResourceProperty> properties;
                if (!this.ResourcePropertyCache.TryGetValue(key, out properties))
                {
                    properties = new List<ResourceProperty>();
                    foreach (ResourceProperty property in resourceType.Properties)
                    {
                        if (property.TypeKind == ResourceTypeKind.EntityType && this.GetContainer(resourceSet, resourceType, property) == null)
                        {
                            continue;
                        }

                        properties.Add(property);
                    }

                    this.ResourcePropertyCache.Add(key, properties);
                }

                return properties;
            }
            else
            {
                return resourceType.Properties;
            }
        }

        /// <summary>
        /// Write the metadata document.
        /// </summary>
        /// <param name="serializer">instance of the metadata serializer.</param>
        /// <param name="writer">xml writer in which we need to write the metadata document.</param>
        /// <param name="service">Data service instance.</param>
        internal void WriteMetadataDocument(MetadataSerializer serializer, XmlWriter writer, IDataService service)
        {
            Debug.Assert(serializer != null, "serializer != null");
            Debug.Assert(writer != null, "writer != null");

            BaseServiceProvider internalProvider = this.metadataProvider as BaseServiceProvider;
            ObjectContextServiceProvider efxProvider = this.metadataProvider as ObjectContextServiceProvider;

            // always v1.1+ schemas for custom providers
            MetadataEdmSchemaVersion metadataEdmSchemaVersion = internalProvider == null ? MetadataEdmSchemaVersion.Version1Dot1 : internalProvider.EdmSchemaVersion;

            if (efxProvider == null)
            {
                serializer.GenerateMetadata(metadataEdmSchemaVersion, service);
            }
            else
            {
                efxProvider.GetMetadata(writer, this, service);
            }
        }

        /// <summary>
        /// Check if the given type can be ordered. If not, throw an exception.
        /// </summary>
        /// <param name="clrType">clr type which needs to checked for ordering.</param>
        internal void CheckIfOrderedType(Type clrType)
        {
            // For known providers check for sort-ability for better error reporting
            BaseServiceProvider baseProvider = this.metadataProvider as BaseServiceProvider;
            if (baseProvider != null &&
                !baseProvider.GetTypeIsOrdered(clrType))
            {
                string resourceTypeName = WebUtil.GetTypeName(clrType);
                throw DataServiceException.CreateBadRequestError(Strings.RequestQueryParser_OrderByDoesNotSupportType(resourceTypeName));
            }
        }

        /// <summary>
        /// Checks whether the current data provider is a V1 provider or not.
        /// </summary>
        /// <returns>Returns true if the current data source is a V1 provider. Otherwise, returns false.</returns>
        internal bool IsV1ProviderAndImplementsUpdatable()
        {
            BaseServiceProvider baseServiceProvider = this.metadataProvider as BaseServiceProvider;
            if (baseServiceProvider != null &&
                baseServiceProvider.ImplementsIUpdatable())
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Retrieve an implementation of a data service interface (ie. IUpdatable, IExpandProvider,etc)
        /// </summary>
        /// <typeparam name="T">The type representing the requested interface</typeparam>
        /// <param name="dataService">Data service instance</param>
        /// <returns>An object implementing the requested interface, or null if not available</returns>
        internal T GetService<T>(IDataService dataService) where T : class
        {
            Debug.Assert(dataService != null, "dataService != null");
            Debug.Assert(dataService.Provider == this, "dataService.Provider == this");
            Debug.Assert(typeof(T) != typeof(IDataServiceMetadataProvider), "typeof(T) != typeof(IDataServiceMetadataProvider)");
            Debug.Assert(typeof(T) != typeof(IDataServiceQueryProvider), "typeof(T) != typeof(IDataServiceQueryProvider)");
            Debug.Assert(typeof(T).IsVisible, "Trying to ask the service for non-public interface.");

#if DEBUG
            dataService.ProcessingPipeline.AssertDebugStateDuringRequestProcessing(dataService);
            dataService.ProcessingPipeline.HasInstantiatedProviderInterfaces = true;
#endif
            // *NOTE* to limit test surface area, IDataServiceStreamProvider && IExpandProvider are the only custom implementation
            // we support for ObjectContextServiceProvider.
            // Should remove this in the future.
            if (this.metadataProvider is ObjectContextServiceProvider && typeof(T) != typeof(IDataServiceStreamProvider) && typeof(T) != typeof(IExpandProvider))
            {
                // Return internal implementation of the interface if there is one.
                return WebUtil.GetService<T>(this.metadataProvider);
            }

            // 1. Check if subclass of DataService<T> implements IServiceProvider. If it does, then call IServiceProvider.GetService()
            // with the appropriate type. If it doesn’t proceed to Step 2
            //    a. If IServiceProvider.GetService() returns something, then go ahead and use that instance
            //    b. If IServiceProvider.GetService() doesn't return anything, proceed to Step 2.
            T result = WebUtil.GetService<T>(dataService.Instance);
            if (result != null)
            {
                return result;
            }

            // 2. Check if the T (where T is the type from DataService<T>) implements the interface.
            //    a. If yes, use that.
            //    b. If no, proceed to Step 3
            result = this.CurrentDataSource as T;
            if (result != null)
            {
                // Since IDataServiceUpdateProvider derives from IUpdatable, we need to make sure that
                // when asked for IUpdatable, this method checks only for IUpdatable, and returns null
                // if the type returns IDataServiceUpdateProvider.
                if (typeof(T) != typeof(IUpdatable) || ((result as IDataServiceUpdateProvider) == null))
                {
                    return result;
                }
            }

            // 3. Check if the data service provider is a V1 provider
            //    a. If yes, return an internal implementation if we can find one.
            //    b. If no, then the provider doesn’t support the current interface functionality.
            if (this.IsV1Provider)
            {
                // Look for internal implementation for the interface
                return WebUtil.GetService<T>(this.metadataProvider);
            }

            return null;
        }

        /// <summary>
        /// We do not allow entity sets to contain any derived type with navigation properties in the GET path
        /// </summary>
        /// <param name="resourceSet">entity set containing the resource type.</param>
        /// <param name="resourceType">entity type in the hierarchy returned by <paramref name="resourceSet"/>.</param>
        /// <returns>True if the derived type has any navigation properties.</returns>
        internal bool IsEntityTypeDisallowedForSet(ResourceSetWrapper resourceSet, ResourceType resourceType)
        {
            Debug.Assert(resourceSet != null, "resourceSet != null");
            Debug.Assert(resourceType != null, "resourceType != null");

            object result;
            string key = resourceSet.Name + '_' + resourceType.FullName;
            if (this.EntityTypeDisallowedForSet.TryGetValue(key, out result))
            {
                Debug.Assert(result == DataServiceProviderWrapper.RefTrue || result == DataServiceProviderWrapper.RefFalse, "result must be either RefTrue or RefFalse.");
                return result == DataServiceProviderWrapper.RefTrue ? true : false;
            }

            ResourceType baseType = resourceSet.ResourceType;
            if (baseType != resourceType && baseType.IsAssignableFrom(resourceType) && this.HasNavigationProperties(resourceSet, resourceType))
            {
                this.EntityTypeDisallowedForSet.Add(key, DataServiceProviderWrapper.RefTrue);
                return true;
            }
            else
            {
                this.EntityTypeDisallowedForSet.Add(key, DataServiceProviderWrapper.RefFalse);
                return false;
            }
        }

        /// <summary>
        /// Validates if the container should be visible and is not read only. If the container rights
        /// are set to None the container should not be visible.
        /// </summary>
        /// <param name="resourceSet">Resource set to be validated.</param>
        /// <returns>Validated container, null if the container is not supposed to be visible.</returns>
        internal ResourceSetWrapper ValidateResourceSet(ResourceSet resourceSet)
        {
            ResourceSetWrapper resourceSetWrapper = null;
            if (resourceSet != null)
            {
                // For IDSP, we want to make sure the metadata object instance stay the same within
                // a request because we do reference comparisons.  Note the provider can return 
                // different metadata instances within the same request.  The the Validate*() methods
                // will make sure to return the first cached instance.
                if (this.ResourceSetWrapperCache.TryGetValue(resourceSet.Name, out resourceSetWrapper))
                {
                    return resourceSetWrapper;
                }

                resourceSetWrapper = new ResourceSetWrapper(resourceSet, this.ValidateResourceType(resourceSet.ResourceType));
                resourceSetWrapper.ApplyConfiguration(this.Configuration);
                if (!resourceSetWrapper.IsVisible)
                {
                    resourceSetWrapper = null;
                }

                this.ResourceSetWrapperCache[resourceSet.Name] = resourceSetWrapper;
            }

            return resourceSetWrapper;
        }

        #endregion Internal Methods

        #region Private Methods

        /// <summary>
        /// Throws if resource type is not sealed.
        /// </summary>
        /// <param name="resourceType">resource type to inspect.</param>
        private static void ValidateResourceTypeReadOnly(ResourceType resourceType)
        {
            Debug.Assert(resourceType != null, "resourceType != null");
            if (!resourceType.IsReadOnly)
            {
                throw new DataServiceException(500, Strings.DataServiceProviderWrapper_ResourceTypeNotReadonly(resourceType.FullName));
            }
        }

        /// <summary>
        /// This is a common method for checking uniqe names across entity sets, resource types and service operations.
        /// </summary>
        /// <param name="name">Name to be added to set.</param>
        /// <param name="names">Set containing already verified names.</param>
        /// <param name="exceptionString">String for exception to be thrown if the name is not unique.</param>
        private static void AddUniqueNameToSet(string name, HashSet<string> names, string exceptionString)
        {
            if (name != null)
            {
                if (names.Contains(name))
                {
                    throw new DataServiceException(500, exceptionString);
                }

                names.Add(name);
            }
        }

        /// <summary>Validates that <paramref name="resourceType"/> is cached and read only.</summary>
        /// <param name="resourceType">Resource type to be validated.</param>
        /// <returns>Validated resource type, null if the resource type is not supposed to be visible.</returns>
        private ResourceType ValidateResourceType(ResourceType resourceType)
        {
            if (resourceType != null)
            {
                // For IDSP, we want to make sure the metadata object instance stay the same within
                // a request because we do reference comparisons.  Note the provider can return 
                // different metadata instances within the same request.  The the Validate*() methods
                // will make sure to return the first cached instance.
                ResourceType cachedType;
                if (this.VisibleTypeCache.TryGetValue(resourceType.FullName, out cachedType))
                {
                    return cachedType;
                }

                ValidateResourceTypeReadOnly(resourceType);
                this.VisibleTypeCache[resourceType.FullName] = resourceType;
                return resourceType;
            }

            return null;
        }

        /// <summary>
        /// Validates if the service operation should be visible and is read only. If the service operation
        /// rights are set to None the service operation should not be visible.
        /// </summary>
        /// <param name="serviceOperation">Service operation to be validated.</param>
        /// <returns>Validated service operation, null if the service operation is not supposed to be visible.</returns>
        private ServiceOperationWrapper ValidateServiceOperation(ServiceOperation serviceOperation)
        {
            ServiceOperationWrapper serviceOperationWrapper = null;
            if (serviceOperation != null)
            {
                // For IDSP, we want to make sure the metadata object instance stay the same within
                // a request because we do reference comparisons.  Note the provider can return 
                // different metadata instances within the same request.  The the Validate*() methods
                // will make sure to return the first cached instance.
                if (this.ServiceOperationWrapperCache.TryGetValue(serviceOperation.Name, out serviceOperationWrapper))
                {
                    return serviceOperationWrapper;
                }

                serviceOperationWrapper = new ServiceOperationWrapper(serviceOperation);
                serviceOperationWrapper.ApplyConfiguration(this.Configuration, this);
                if (!serviceOperationWrapper.IsVisible)
                {
                    serviceOperationWrapper = null;
                }

                this.ServiceOperationWrapperCache[serviceOperation.Name] = serviceOperationWrapper;
            }

            return serviceOperationWrapper;
        }

        /// <summary>
        /// Checks whether the resource type from the resource set has visible navigation properties or not.
        /// </summary>
        /// <param name="resourceSet">resource set instance to inspect.</param>
        /// <param name="resourceType">resource type to inspect.</param>
        /// <returns>Returns true if the resource type has one or more visible navigation properties from the resource set. Otherwise returns false.</returns>
        private bool HasNavigationProperties(ResourceSetWrapper resourceSet, ResourceType resourceType)
        {
            foreach (ResourceProperty property in resourceType.PropertiesDeclaredOnThisType)
            {
                if (property.TypeKind != ResourceTypeKind.EntityType)
                {
                    continue;
                }

                if (this.GetResourceAssociationSet(resourceSet, resourceType, property) != null)
                {
                    return true;
                }
            }

            return false;
        }

        #endregion Private Methods
    }
}
