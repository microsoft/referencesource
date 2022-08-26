//---------------------------------------------------------------------
// <copyright file="BaseServiceProvider.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Abstract Class which contains the common code for ObjectContextServiceProvider
//      and ReflectionServiceProvider
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
    using System.Diagnostics;
    using System.Globalization;
    using System.Linq;
    using System.Reflection;
    using System.ServiceModel.Web;

    #endregion Namespaces.

    /// <summary>Provides a reflection-based provider implementation.</summary>
    internal abstract class BaseServiceProvider : IDataServiceMetadataProvider, IDataServiceQueryProvider, IDisposable, IProjectionProvider, IServiceProvider
    {
        /// <summary>Bindings Flags to be used for reflection.</summary>
        protected const BindingFlags ResourceContainerBindingFlags = WebUtil.PublicInstanceBindingFlags;

        /// <summary>Instance from which data is provided.</summary>
        private object instance;

        /// <summary>Metadata to be used by the service provider.</summary>
        private MetadataCacheItem metadata;

        /// <summary>instance of the service to invoke service operations.</summary>
        private object dataServiceInstance;

        /// <summary>
        /// Reference back to the provider wrapper.
        /// </summary>
        private DataServiceProviderWrapper providerWrapper;

        /// <summary>
        /// Initializes a new System.Data.Services.BaseServiceProvider instance.
        /// </summary>
        /// <param name="metadata">Metadata for this provider.</param>
        /// <param name="dataServiceInstance">data service instance.</param>
        protected BaseServiceProvider(MetadataCacheItem metadata, object dataServiceInstance)
        {
            WebUtil.CheckArgumentNull(metadata, "metadata");
            WebUtil.CheckArgumentNull(dataServiceInstance, "dataServiceInstance");
            this.metadata = metadata;
            this.dataServiceInstance = dataServiceInstance;
        }

        #region IDataServiceQueryProvider Properties

        /// <summary>Returns the instance from which data is provided.</summary>
        public object CurrentDataSource
        {
            [DebuggerStepThrough]
            get
            {
                // Many debuggers will try to display this property, and we don't want to trigger an assertion.
                Debug.Assert(
                    System.Diagnostics.Debugger.IsAttached || this.instance != null,
                    "this.instance != null -- otherwise CurrentDataSource is accessed before initialization or after disposal.");
                return this.instance;
            }

            set
            {
                WebUtil.CheckArgumentNull(value, "value");
                this.instance = value;
            }
        }

        /// <summary>Gets a value indicating whether null propagation is required in expression trees.</summary>
        public abstract bool IsNullPropagationRequired
        {
            get;
        }

        #endregion IDataServiceQueryProvider Properties

        #region IDataServiceMetadataProvider Properties

        /// <summary>Namespace name for the EDM container.</summary>
        public abstract string ContainerNamespace
        {
            get;
        }

        /// <summary>Name of the EDM container</summary>
        public abstract string ContainerName
        {
            get;
        }

        /// <summary>Gets all available containers.</summary>
        /// <returns>An enumerable object with all available containers.</returns>
        public IEnumerable<ResourceSet> ResourceSets
        {
            get { return this.EntitySets.Values; }
        }

        /// <summary>Returns all the types in this data source</summary>
        public IEnumerable<ResourceType> Types
        {
            get { return this.metadata.TypeCache.Values; }
        }

        /// <summary>Returns all known service operations.</summary>
        public IEnumerable<ServiceOperation> ServiceOperations
        {
            get
            {
                foreach (ServiceOperation serviceOperation in this.metadata.ServiceOperations.Values)
                {
                    yield return serviceOperation;
                }
            }
        }

        #endregion IDataServiceMetadataProvider Properties

        /// <summary>EDM version to which metadata is compatible.</summary>
        /// <remarks>
        /// For example, a service operation of type Void is not acceptable 1.0 CSDL,
        /// so it should use 1.1 CSDL instead. Similarly, OpenTypes are supported
        /// in 1.2 and not before.
        /// </remarks>
        internal MetadataEdmSchemaVersion EdmSchemaVersion
        {
            get { return this.metadata.EdmSchemaVersion; }
        }

        /// <summary>Whether all EPM properties serialize in an Astoria V1-compatible way.</summary>
        /// <remarks>
        /// This property is false if any property has KeepInContent set to false.
        /// </remarks>
        internal bool EpmIsV1Compatible
        {
            [DebuggerStepThrough]
            get { return this.metadata.EpmIsV1Compatible; }
        }

        /// <summary>
        /// Reference back to the provider wrapper.
        /// </summary>
        internal DataServiceProviderWrapper ProviderWrapper
        {
            set
            {
                this.providerWrapper = value;
            }

            get
            {
                Debug.Assert(this.providerWrapper != null, "this.providerWrapper != null");
                return this.providerWrapper;
            }
        }

        /// <summary>Returns the list of entity sets.</summary>
        protected IDictionary<string, ResourceSet> EntitySets
        {
            [DebuggerStepThrough]
            get { return this.metadata.EntitySets; }
        }

        /// <summary>Target type for the data provider </summary>
        protected Type Type
        {
            [DebuggerStepThrough]
            get { return this.metadata.Type; }
        }

        /// <summary>Cache of resource properties per type.</summary>
        private Dictionary<Type, ResourceType> TypeCache
        {
            [DebuggerStepThrough]
            get { return this.metadata.TypeCache; }
        }

        /// <summary>Cache of immediate derived types per type.</summary>
        private Dictionary<ResourceType, List<ResourceType>> ChildTypesCache
        {
            [DebuggerStepThrough]
            get { return this.metadata.ChildTypesCache; }
        }

        #region Public Methods

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
        public abstract IQueryable ApplyProjections(
            IQueryable source,
            ProjectionNode projection);

        /// <summary>
        /// Gets the ResourceAssociationSet instance when given the source association end.
        /// </summary>
        /// <param name="resourceSet">Resource set of the source association end.</param>
        /// <param name="resourceType">Resource type of the source association end.</param>
        /// <param name="resourceProperty">Resource property of the source association end.</param>
        /// <returns>ResourceAssociationSet instance.</returns>
        public abstract ResourceAssociationSet GetResourceAssociationSet(ResourceSet resourceSet, ResourceType resourceType, ResourceProperty resourceProperty);

        /// <summary>
        /// Checks whether the specified <paramref name="type"/> is ordered.
        /// </summary>
        /// <param name="type">Type to check.</param>
        /// <returns>true if the type may be ordered; false otherwise.</returns>
        /// <remarks>
        /// The ordering may still fail at runtime; this method is currently
        /// used for cleaner error messages only.
        /// </remarks>
        public virtual bool GetTypeIsOrdered(Type type)
        {
            Debug.Assert(type != null, "type != null");
            return type == typeof(object) || WebUtil.IsPrimitiveType(type);
        }

        /// <summary>
        /// Returns the requested service
        /// </summary>
        /// <param name="serviceType">type of service you are requesting for.</param>
        /// <returns>returns the instance of the requested service.</returns>
        public virtual object GetService(Type serviceType)
        {
            if (typeof(IDataServiceMetadataProvider) == serviceType ||
                typeof(IDataServiceQueryProvider) == serviceType ||
                typeof(IProjectionProvider) == serviceType)
            {
                return this;
            }

            return null;
        }

        /// <summary>Releases the current data source object as necessary.</summary>
        void IDisposable.Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        #endregion Public Methods

        #region IDataServiceQueryProvider Methods

        /// <summary>
        /// Returns the IQueryable that represents the container.
        /// </summary>
        /// <param name="container">resource set representing the entity set.</param>
        /// <returns>
        /// An IQueryable that represents the container; null if there is 
        /// no container for the specified name.
        /// </returns>
        public IQueryable GetQueryRootForResourceSet(ResourceSet container)
        {
            Debug.Assert(container != null, "resourceContainer != null");
            return this.GetResourceContainerInstance(container);
        }

        /// <summary>Gets the <see cref="ResourceType"/> for the specified <paramref name="resource"/>.</summary>
        /// <param name="resource">Instance to extract a <see cref="ResourceType"/> from.</param>
        /// <returns>The <see cref="ResourceType"/> that describes this <paramref name="resource"/> in this provider.</returns>
        public ResourceType GetResourceType(object resource)
        {
            Debug.Assert(resource != null, "instance != null");
            return this.GetNonPrimitiveType(resource.GetType());
        }

        /// <summary>
        /// Get the value of the strongly typed property.
        /// </summary>
        /// <param name="target">instance of the type declaring the property.</param>
        /// <param name="resourceProperty">resource property describing the property.</param>
        /// <returns>value for the property.</returns>
        public object GetPropertyValue(object target, ResourceProperty resourceProperty)
        {
            Debug.Assert(target != null, "target != null");
            Debug.Assert(resourceProperty != null, "resourceProperty != null");

            try
            {
                PropertyInfo propertyInfo = this.GetResourceType(target).GetPropertyInfo(resourceProperty);
                Debug.Assert(propertyInfo != null, "propertyInfo != null");
                return propertyInfo.GetGetMethod().Invoke(target, null);
            }
            catch (TargetInvocationException exception)
            {
                ErrorHandler.HandleTargetInvocationException(exception);
                throw;
            }
        }

        /// <summary>
        /// Gets the value of the open property.
        /// </summary>
        /// <param name="target">instance of the resource type.</param>
        /// <param name="propertyName">name of the property.</param>
        /// <returns>the value of the open property. If the property is not present, return null.</returns>
        public abstract object GetOpenPropertyValue(object target, string propertyName);

        /// <summary>
        /// Get the name and values of all the properties defined in the given instance of an open type.
        /// </summary>
        /// <param name="target">instance of a open type.</param>
        /// <returns>collection of name and values of all the open properties.</returns>
        public abstract IEnumerable<KeyValuePair<string, object>> GetOpenPropertyValues(object target);

        /// <summary>
        /// Invoke the given service operation instance.
        /// </summary>
        /// <param name="serviceOperation">metadata for the service operation to invoke.</param>
        /// <param name="parameters">list of parameters to pass to the service operation.</param>
        /// <returns>returns the result by the service operation instance.</returns>
        public object InvokeServiceOperation(ServiceOperation serviceOperation, object[] parameters)
        {
            return ((MethodInfo)serviceOperation.CustomState).Invoke(
                    this.dataServiceInstance,
                    BindingFlags.Instance | BindingFlags.Instance | BindingFlags.FlattenHierarchy,
                    null,
                    parameters,
                    Globalization.CultureInfo.InvariantCulture);
        }

        #endregion IDataServiceQueryProvider Methods

        #region IDataServiceMetadataProvider Methods

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
            Debug.Assert(this.ChildTypesCache.ContainsKey(resourceType), "this.ChildTypesCache.ContainsKey(resourceType)");

            List<ResourceType> childTypes = this.ChildTypesCache[resourceType];
            if (childTypes != null)
            {
                foreach (ResourceType childType in childTypes)
                {
                    yield return childType;

                    foreach (ResourceType descendantType in this.GetDerivedTypes(childType))
                    {
                        yield return descendantType;
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
            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(this.ChildTypesCache.ContainsKey(resourceType), "this.ChildTypesCache.ContainsKey(resourceType)");
            Debug.Assert(this.ChildTypesCache[resourceType] == null || this.ChildTypesCache[resourceType].Count > 0, "this.ChildTypesCache[resourceType] == null || this.ChildTypesCache[resourceType].Count > 0");

            return this.ChildTypesCache[resourceType] != null;
        }

        /// <summary>Given the specified name, tries to find a resource set.</summary>
        /// <param name="name">Name of the resource set to resolve.</param>
        /// <param name="resourceSet">Returns the resolved resource set, null if no resource set for the given name was found.</param>
        /// <returns>True if resource set with the given name was found, false otherwise.</returns>
        public bool TryResolveResourceSet(string name, out ResourceSet resourceSet)
        {
            Debug.Assert(!string.IsNullOrEmpty(name), "!string.IsNullOrEmpty(name)");
            return this.EntitySets.TryGetValue(name, out resourceSet);
        }

        /// <summary>Given the specified name, tries to find a service operation.</summary>
        /// <param name="name">Name of the service operation to resolve.</param>
        /// <param name="serviceOperation">Returns the resolved service operation, null if no service operation was found for the given name.</param>
        /// <returns>True if we found the service operation for the given name, false otherwise.</returns>
        public bool TryResolveServiceOperation(string name, out ServiceOperation serviceOperation)
        {
            Debug.Assert(!string.IsNullOrEmpty(name), "!string.IsNullOrEmpty(name)");
            return this.metadata.ServiceOperations.TryGetValue(name, out serviceOperation);
        }

        /// <summary>Given the specified name, tries to find a type.</summary>
        /// <param name="name">Name of the type to resolve.</param>
        /// <param name="resourceType">Returns the resolved resource type, null if no resource type for the given name was found.</param>
        /// <returns>True if we found the resource type for the given name, false otherwise.</returns>
        public bool TryResolveResourceType(string name, out ResourceType resourceType)
        {
            Debug.Assert(!string.IsNullOrEmpty(name), "!string.IsNullOrEmpty(name)");
            Debug.Assert(this.metadata != null, "this.metadata != null");
            Debug.Assert(this.TypeCache != null, "this.TypeCache != null");
            foreach (ResourceType t in this.TypeCache.Values)
            {
                if (t.FullName == name)
                {
                    resourceType = t;
                    return true;
                }
            }

            resourceType = null;
            return false;
        }

        #endregion IDataServiceMetadataProvider Methods

        #region Internal Methods

        /// <summary>
        /// Returns the type of the IEnumerable if the type implements IEnumerable interface; null otherwise.
        /// </summary>
        /// <param name="type">type that needs to be checked</param>
        /// <returns>Element type if the type implements IEnumerable, else returns null</returns>
        internal static Type GetIEnumerableElement(Type type)
        {
            return GetGenericInterfaceElementType(type, IEnumerableTypeFilter);
        }

        /// <summary>
        /// Returns the "T" in the IQueryable of T implementation of type.
        /// </summary>
        /// <param name="type">Type to check.</param>
        /// <param name="typeFilter">filter against which the type is checked</param>
        /// <returns>
        /// The element type for the generic IQueryable interface of the type,
        /// or null if it has none or if it's ambiguous.
        /// </returns>
        internal static Type GetGenericInterfaceElementType(Type type, TypeFilter typeFilter)
        {
            Debug.Assert(type != null, "type != null");
            Debug.Assert(!type.IsGenericTypeDefinition, "!type.IsGenericTypeDefinition");

            if (typeFilter(type, null))
            {
                return type.GetGenericArguments()[0];
            }

            Type[] queriables = type.FindInterfaces(typeFilter, null);
            if (queriables != null && queriables.Length == 1)
            {
                return queriables[0].GetGenericArguments()[0];
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Checks whether the provider implements IUpdatable.
        /// </summary>
        /// <returns>returns true if the provider implements IUpdatable. otherwise returns false.</returns>
        internal abstract bool ImplementsIUpdatable();

        /// <summary>Adds service operations based on methods of the specified type.</summary>
        /// <param name="type">Type with methods to add.</param>
        internal void AddOperationsFromType(Type type)
        {
            Debug.Assert(type != null, "type != null");
            foreach (MethodInfo methodInfo in type.GetMethods(WebUtil.PublicInstanceBindingFlags | BindingFlags.FlattenHierarchy))
            {
                if (methodInfo.GetCustomAttributes(typeof(WebGetAttribute), true).Length != 0)
                {
                    this.AddServiceOperation(methodInfo, XmlConstants.HttpMethodGet);
                }
                else if (methodInfo.GetCustomAttributes(typeof(WebInvokeAttribute), true).Length != 0)
                {
                    this.AddServiceOperation(methodInfo, XmlConstants.HttpMethodPost);
                }
            }
        }

        /// <summary>Populates the metadata for the given provider.</summary>
        internal void PopulateMetadata()
        {
            Debug.Assert(this.metadata != null, "this.metadata != null -- otherwise we don't have an item to populate");
            this.PopulateMetadata(this.TypeCache, this.ChildTypesCache, this.EntitySets);
        }
        
        /// <summary>
        /// Applies access rights to entity sets
        /// </summary>
        /// <param name="configuration">Data service configuration instance with access right info.</param>
        internal void ApplyConfiguration(DataServiceConfiguration configuration)
        {
            Debug.Assert(configuration != null, "configuration != null");

            this.PopulateMetadataForUserSpecifiedTypes(configuration.GetKnownTypes(), this.TypeCache, this.ChildTypesCache, this.EntitySets.Values);
            this.CheckConfigurationConsistency(this.instance, configuration);
        }

        /// <summary>Make all the metadata readonly</summary>
        internal void MakeMetadataReadonly()
        {
            foreach (ResourceSet container in this.ResourceSets)
            {
                container.SetReadOnly();
            }

            foreach (ResourceType resourceType in this.Types)
            {
                resourceType.SetReadOnly();
                if (!resourceType.EpmIsV1Compatible)
                {
                    this.metadata.EpmIsV1Compatible = false;
                }
            }

            foreach (ServiceOperation operation in this.ServiceOperations)
            {
                operation.SetReadOnly();
            }
        }

        #endregion Internal Methods

        #region Protected methods.

        /// <summary>
        /// Returns the type of the IQueryable if the type implements IQueryable interface
        /// </summary>
        /// <param name="type">clr type on which IQueryable check needs to be performed.</param>
        /// <returns>Element type if the property type implements IQueryable, else returns null</returns>
        protected static Type GetIQueryableElement(Type type)
        {
            return GetGenericInterfaceElementType(type, IQueryableTypeFilter);
        }

        /// <summary>
        /// Find the corresponding ResourceType for a given Type, primitive or not
        /// </summary>
        /// <param name="knownTypes">Non-primitive types to search</param>
        /// <param name="type">Type to look for</param>
        /// <param name="resourceType">Corresponding ResourceType, if found</param>
        /// <returns>True if type found, false otherwise</returns>
        protected static bool TryGetType(IDictionary<Type, ResourceType> knownTypes, Type type, out ResourceType resourceType)
        {
            Debug.Assert(knownTypes != null, "knownTypes != null");
            Debug.Assert(type != null, "type != null");

            resourceType = ResourceType.GetPrimitiveResourceType(type);

            if (resourceType == null)
            {
                knownTypes.TryGetValue(type, out resourceType);
            }

            return resourceType != null;
        }

        /// <summary>Checks that the applied configuration is consistent.</summary>
        /// <param name='dataSourceInstance'>Instance of the data source for the provider.</param>
        /// <param name="configuration">Data service configuration instance with access right info.</param>
        /// <remarks>At this point in initialization, metadata trimming hasn't taken place.</remarks>
        protected virtual void CheckConfigurationConsistency(object dataSourceInstance, DataServiceConfiguration configuration)
        {
        }

        /// <summary>Releases the current data source object as necessary.</summary>
        /// <param name="disposing">
        /// Whether this method is called from an explicit call to Dispose by 
        /// the consumer, rather than during finalization.
        /// </param>
        protected virtual void Dispose(bool disposing)
        {
            WebUtil.Dispose(this.instance);
            this.instance = null;
        }

        /// <summary>
        /// Returns the resource type of the given instance and validates that the instance returns a single resource.
        /// </summary>
        /// <param name="resource">clr instance of a resource.</param>
        /// <returns>resource type of the given instance.</returns>
        protected ResourceType GetSingleResource(object resource)
        {
            ResourceType resourceType = this.ProviderWrapper.GetResourceType(resource);
            return resourceType;
        }

        /// <summary>
        /// Creates the object query for the given resource set and returns it
        /// </summary>
        /// <param name="resourceContainer">resource set for which IQueryable instance needs to be created</param>
        /// <returns>returns the IQueryable instance for the given resource set</returns>
        protected abstract IQueryable GetResourceContainerInstance(ResourceSet resourceContainer);

        /// <summary>
        /// Populates the metadata for the given provider
        /// </summary>
        /// <param name="knownTypes">list of known types</param>
        /// <param name="childTypes">list of known types and their immediate children</param>
        /// <param name="entitySets">list of entity sets</param>
        protected abstract void PopulateMetadata(
            IDictionary<Type, ResourceType> knownTypes,
            IDictionary<ResourceType, List<ResourceType>> childTypes,
            IDictionary<string, ResourceSet> entitySets);

        /// <summary>
        /// Populate types for metadata specified by the provider
        /// </summary>
        /// <param name="userSpecifiedTypes">list of types specified by the provider</param>
        /// <param name="knownTypes">list of already known types</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <param name="entitySets">list of entity sets as specified in the data source type</param>
        protected abstract void PopulateMetadataForUserSpecifiedTypes(IEnumerable<Type> userSpecifiedTypes, IDictionary<Type, ResourceType> knownTypes, IDictionary<ResourceType, List<ResourceType>> childTypes, IEnumerable<ResourceSet> entitySets);

        /// <summary>
        /// Populate metadata for the given clr type.
        /// </summary>
        /// <param name="type">type whose metadata needs to be loaded.</param>
        /// <param name="knownTypes">list of already known resource types.</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <param name="entitySets">list of entity sets as specified in the data source.</param>
        /// <returns>resource type containing metadata for the given clr type.</returns>
        protected abstract ResourceType PopulateMetadataForType(Type type, IDictionary<Type, ResourceType> knownTypes, IDictionary<ResourceType, List<ResourceType>> childTypes, IEnumerable<ResourceSet> entitySets);

        /// <summary>
        /// Returns the resource type for the corresponding clr type.
        /// </summary>
        /// <param name="type">clrType whose corresponding resource type needs to be returned</param>
        /// <returns>Returns the resource type</returns>
        protected virtual ResourceType ResolveNonPrimitiveType(Type type)
        {
            ResourceType resourceType;
            this.TypeCache.TryGetValue(type, out resourceType);
            return resourceType;
        }

        #endregion Protected methods.

        #region Private Methods

        /// <summary>Filter callback for finding IQueryable implementations.</summary>
        /// <param name="m">Type to inspect.</param>
        /// <param name="filterCriteria">Filter criteria.</param>
        /// <returns>true if the specified type is an IQueryable of T; false otherwise.</returns>
        private static bool IQueryableTypeFilter(Type m, object filterCriteria)
        {
            Debug.Assert(m != null, "m != null");
            return m.IsGenericType && m.GetGenericTypeDefinition() == typeof(IQueryable<>);
        }

        /// <summary>Filter callback for finding IEnumerable implementations.</summary>
        /// <param name="m">Type to inspect.</param>
        /// <param name="filterCriteria">Filter criteria.</param>
        /// <returns>true if the specified type is an IEnumerable of T; false otherwise.</returns>
        private static bool IEnumerableTypeFilter(Type m, object filterCriteria)
        {
            Debug.Assert(m != null, "m != null");
            return m.IsGenericType && m.GetGenericTypeDefinition() == typeof(IEnumerable<>);
        }

        /// <summary>Updates the EDM schema version if it is currently lower than <paramref name="newVersion"/>.</summary>
        /// <param name="newVersion">New version for EDM schema.</param>
        private void UpdateEdmSchemaVersion(MetadataEdmSchemaVersion newVersion)
        {
            if (this.metadata.EdmSchemaVersion < newVersion)
            {
                this.metadata.EdmSchemaVersion = newVersion;
            }
        }

        /// <summary>
        /// Adds a new <see cref="ServiceOperation"/> based on the specified <paramref name="method"/>
        /// instance.
        /// </summary>
        /// <param name="method">Method to expose as a service operation.</param>
        /// <param name="protocolMethod">Protocol (for example HTTP) method the service operation responds to.</param>
        private void AddServiceOperation(MethodInfo method, string protocolMethod)
        {
            Debug.Assert(method != null, "method != null");
            Debug.Assert(!method.IsAbstract, "!method.IsAbstract - if method is abstract, the type is abstract - already checked");

            // This method is only called for V1 providers, since in case of custom providers,
            // they are suppose to load the metadata themselves.
            if (this.metadata.ServiceOperations.ContainsKey(method.Name))
            {
                throw new InvalidOperationException(Strings.BaseServiceProvider_OverloadingNotSupported(this.Type, method));
            }

            bool hasSingleResult = SingleResultAttribute.MethodHasSingleResult(method);
            ServiceOperationResultKind resultKind;
            ResourceType resourceType = null;
            if (method.ReturnType == typeof(void))
            {
                resultKind = ServiceOperationResultKind.Void;
                this.UpdateEdmSchemaVersion(MetadataEdmSchemaVersion.Version1Dot1);
            }
            else
            {
                // Load the metadata of the resource type on the fly.
                // For Edm provider, it might not mean anything, but for reflection service provider, we need to
                // load the metadata of the type if its used only in service operation case
                Type resultType = null;
                if (WebUtil.IsPrimitiveType(method.ReturnType))
                {
                    resultKind = ServiceOperationResultKind.DirectValue;
                    resultType = method.ReturnType;
                    resourceType = ResourceType.GetPrimitiveResourceType(resultType);
                }
                else
                {
                    Type queryableElement = GetGenericInterfaceElementType(method.ReturnType, IQueryableTypeFilter);
                    if (queryableElement != null)
                    {
                        resultKind = hasSingleResult ?
                            ServiceOperationResultKind.QueryWithSingleResult :
                            ServiceOperationResultKind.QueryWithMultipleResults;
                        resultType = queryableElement;
                    }
                    else
                    {
                        Type enumerableElement = GetIEnumerableElement(method.ReturnType);
                        if (enumerableElement != null)
                        {
                            resultKind = ServiceOperationResultKind.Enumeration;
                            resultType = enumerableElement;
                        }
                        else
                        {
                            resultType = method.ReturnType;
                            resultKind = ServiceOperationResultKind.DirectValue;
                            this.UpdateEdmSchemaVersion(MetadataEdmSchemaVersion.Version1Dot1);
                        }
                    }

                    Debug.Assert(resultType != null, "resultType != null");
                    resourceType = ResourceType.GetPrimitiveResourceType(resultType);
                    if (resourceType == null)
                    {
                        resourceType = this.PopulateMetadataForType(resultType, this.TypeCache, this.ChildTypesCache, this.EntitySets.Values);
                    }
                }

                if (resourceType == null)
                {
                    throw new InvalidOperationException(Strings.BaseServiceProvider_UnsupportedReturnType(method, method.ReturnType));
                }

                if (resultKind == ServiceOperationResultKind.Enumeration && hasSingleResult)
                {
                    throw new InvalidOperationException(Strings.BaseServiceProvider_IEnumerableAlwaysMultiple(this.Type, method));
                }

                if (hasSingleResult ||
                    (!hasSingleResult &&
                    (resourceType.ResourceTypeKind == ResourceTypeKind.ComplexType ||
                     resourceType.ResourceTypeKind == ResourceTypeKind.Primitive)))
                {
                    this.UpdateEdmSchemaVersion(MetadataEdmSchemaVersion.Version1Dot1);
                }
            }

            ParameterInfo[] parametersInfo = method.GetParameters();
            ServiceOperationParameter[] parameters = new ServiceOperationParameter[parametersInfo.Length];
            for (int i = 0; i < parameters.Length; i++)
            {
                ParameterInfo parameterInfo = parametersInfo[i];
                if (parameterInfo.IsOut || parameterInfo.IsRetval)
                {
                    throw new InvalidOperationException(Strings.BaseServiceProvider_ParameterNotIn(method, parameterInfo));
                }

                ResourceType parameterType = ResourceType.GetPrimitiveResourceType(parameterInfo.ParameterType);
                if (parameterType == null)
                {
                    throw new InvalidOperationException(
                        Strings.BaseServiceProvider_ParameterTypeNotSupported(method, parameterInfo, parameterInfo.ParameterType));
                }

                string parameterName = parameterInfo.Name ?? "p" + i.ToString(CultureInfo.InvariantCulture);
                parameters[i] = new ServiceOperationParameter(parameterName, parameterType);
            }

            ResourceSet container = null;
            if (resourceType != null && resourceType.ResourceTypeKind == ResourceTypeKind.EntityType)
            {
                if (!this.TryFindAnyContainerForType(resourceType, out container))
                {
                    throw new InvalidOperationException(
                        Strings.BaseServiceProvider_ServiceOperationMissingSingleEntitySet(method, resourceType.FullName));
                }
            }

            ServiceOperation operation = new ServiceOperation(method.Name, resultKind, resourceType, container, protocolMethod, parameters);
            operation.CustomState = method;
            MimeTypeAttribute attribute = MimeTypeAttribute.GetMimeTypeAttribute(method);
            if (attribute != null)
            {
                operation.MimeType = attribute.MimeType;
            }

            this.metadata.ServiceOperations.Add(method.Name, operation);
        }

        /// <summary>
        /// Returns the resource type for the corresponding clr type.
        /// If the given clr type is a collection, then resource type describes the element type of the collection.
        /// </summary>
        /// <param name="type">clrType whose corresponding resource type needs to be returned</param>
        /// <returns>Returns the resource type</returns>
        private ResourceType GetNonPrimitiveType(Type type)
        {
            Debug.Assert(type != null, "type != null");

            // Check for the type directly first
            ResourceType resourceType = this.ResolveNonPrimitiveType(type);
            if (resourceType == null)
            {
                // check for ienumerable types
                Type elementType = BaseServiceProvider.GetIEnumerableElement(type);
                if (elementType != null)
                {
                    resourceType = ResourceType.GetPrimitiveResourceType(elementType);
                    if (resourceType == null)
                    {
                        resourceType = this.ResolveNonPrimitiveType(elementType);
                    }
                }
            }

            return resourceType;
        }

        /// <summary>
        /// Looks for the first resource set that the specified <paramref name="type"/>
        /// could belong to.
        /// </summary>
        /// <param name="type">Type to look for.</param>
        /// <param name="container">After the method returns, the container to which the type could belong.</param>
        /// <returns>true if a container was found; false otherwise.</returns>
        private bool TryFindAnyContainerForType(ResourceType type, out ResourceSet container)
        {
            Debug.Assert(type != null, "type != null");

            foreach (ResourceSet c in this.EntitySets.Values)
            {
                if (c.ResourceType.IsAssignableFrom(type))
                {
                    container = c;
                    return true;
                }
            }

            container = default(ResourceSet);
            return false;
        }

        #endregion Private Methods

        #region QueryableOverEnumerable
        
#if DEBUG
        /// <summary>Implementation of the <see cref="IQueryable"/> interface which only support
        /// the enumerable portion of the interface.</summary>
        /// <remarks>Used for the old <see cref="IExpandProvider"/> which returns <see cref="IEnumerable"/>
        /// but we require <see cref="IQueryable"/> instead.</remarks>
        internal class QueryableOverEnumerable : IQueryable
        {
            /// <summary>The <see cref="IEnumerable"/> this object wraps.</summary>
            private readonly IEnumerable enumerable;

            /// <summary>Constructor - creates new instance.</summary>
            /// <param name="enumerable">The <see cref="IEnumerable"/> to wrap with <see cref="IQueryable"/>.</param>
            internal QueryableOverEnumerable(IEnumerable enumerable)
            {
                this.enumerable = enumerable;
            }

            /// <summary>The type of the element - not implemented as nothing should call this.</summary>
            public Type ElementType
            {
                get { throw new NotImplementedException(); }
            }

            /// <summary>The expression for the query - not implemented as nothing should call this.</summary>
            public System.Linq.Expressions.Expression Expression
            {
                get { throw new NotImplementedException(); }
            }

            /// <summary>The query provider for this query - not implemented as nothing should call this.</summary>
            public IQueryProvider Provider
            {
                get { throw new NotImplementedException(); }
            }

            /// <summary>Creates new enumerator of the results of this query.</summary>
            /// <returns>New <see cref="IEnumerator"/> to enumerate results of this query.</returns>
            public IEnumerator GetEnumerator()
            {
                return this.enumerable.GetEnumerator();
            }
        }
#endif
        #endregion
    }
}
