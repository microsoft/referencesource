//---------------------------------------------------------------------
// <copyright file="MetadataCacheItem.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a class to cache metadata information.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Caching
{
    using System;
    using System.Collections.Generic;
    using System.Data.Services.Common;
    using System.Data.Services.Providers;
    using System.Diagnostics;

    /// <summary>Use this class to cache metadata for providers.</summary>
    internal class MetadataCacheItem
    {
        #region Private fields.

        /// <summary> list of top level entity sets</summary>
        private readonly Dictionary<string, ResourceSet> entitySets;

        /// <summary>Collection of service operations, keyed by name.</summary>
        private readonly Dictionary<string, ServiceOperation> serviceOperations;

        /// <summary>Target type for the data provider.</summary>
        private readonly Type type;

        /// <summary>Cache of resource properties per type.</summary>
        private readonly Dictionary<Type, ResourceType> typeCache;

        /// <summary>Cache of immediate derived types per type.</summary>
        private readonly Dictionary<ResourceType, List<ResourceType>> childTypesCache;

        /// <summary>Service configuration information.</summary>
        private DataServiceConfiguration configuration;

        /// <summary>Whether all EPM properties serialize in an Astoria V1-compatible way.</summary>
        private bool epmIsV1Compatible;

        /// <summary>
        /// Keep track of the calculated visibility of resource types.
        /// </summary>
        private Dictionary<string, ResourceType> visibleTypeCache;

        /// <summary>
        /// Maps resource set names to ResourceSetWrappers.
        /// </summary>
        private Dictionary<string, ResourceSetWrapper> resourceSetWrapperCache;

        /// <summary>
        /// Maps service operation names to ServiceOperationWrappers.
        /// </summary>
        private Dictionary<string, ServiceOperationWrapper> serviceOperationWrapperCache;

        /// <summary>
        /// Maps names to ResourceAssociationSets.
        /// </summary>
        private Dictionary<string, ResourceAssociationSet> resourceAssociationSetCache;

        /// <summary>
        /// Mapes "resourceSetName_resourceTypeName" to the list of visible properties from the set.
        /// </summary>
        private Dictionary<string, List<ResourceProperty>> resourcePropertyCache;

        /// <summary>
        /// Mapes "resourceSetName_resourceTypeName" to boolean of whether resourceType is allowed for resourceSet
        /// </summary>
        private Dictionary<string, object> entityTypeDisallowedForSet;

        #endregion Private fields.

        /// <summary>Initializes a new <see cref="MetadataCacheItem"/> instance.</summary>
        /// <param name='type'>Type of data context for which metadata will be generated.</param>
        internal MetadataCacheItem(Type type)
        {
            Debug.Assert(type != null, "type != null");

            this.serviceOperations = new Dictionary<string, ServiceOperation>(EqualityComparer<string>.Default);
            this.typeCache = new Dictionary<Type, ResourceType>(EqualityComparer<Type>.Default);
            this.childTypesCache = new Dictionary<ResourceType, List<ResourceType>>(ReferenceEqualityComparer<ResourceType>.Instance);
            this.entitySets = new Dictionary<string, ResourceSet>(EqualityComparer<string>.Default);
            this.epmIsV1Compatible = true;

            this.resourceSetWrapperCache = new Dictionary<string, ResourceSetWrapper>(EqualityComparer<string>.Default);
            this.serviceOperationWrapperCache = new Dictionary<string, ServiceOperationWrapper>(EqualityComparer<string>.Default);
            this.visibleTypeCache = new Dictionary<string, ResourceType>(EqualityComparer<string>.Default);
            this.resourceAssociationSetCache = new Dictionary<string, ResourceAssociationSet>(EqualityComparer<string>.Default);
            this.resourcePropertyCache = new Dictionary<string, List<ResourceProperty>>(EqualityComparer<string>.Default);
            this.entityTypeDisallowedForSet = new Dictionary<string, object>(EqualityComparer<string>.Default);
            this.type = type;
            this.EdmSchemaVersion = MetadataEdmSchemaVersion.Version1Dot0;
        }

        #region Properties.

        /// <summary>Service configuration information.</summary>
        internal DataServiceConfiguration Configuration
        {
            [DebuggerStepThrough]
            get
            {
                return this.configuration;
            }

            set
            {
                Debug.Assert(value != null, "value != null");
                Debug.Assert(this.configuration == null, "this.configuration == null -- otherwise it's being set more than once");
                this.configuration = value;
            }
        }

        /// <summary>
        /// Keep track of the calculated visibility of resource types.
        /// </summary>
        internal Dictionary<string, ResourceType> VisibleTypeCache
        {
            [DebuggerStepThrough]
            get { return this.visibleTypeCache; }
        }

        /// <summary>
        /// Maps resource set names to ResourceSetWrappers.
        /// </summary>
        internal Dictionary<string, ResourceSetWrapper> ResourceSetWrapperCache
        {
            [DebuggerStepThrough]
            get { return this.resourceSetWrapperCache; }
        }

        /// <summary>
        /// Maps service operation names to ServiceOperationWrappers.
        /// </summary>
        internal Dictionary<string, ServiceOperationWrapper> ServiceOperationWrapperCache
        {
            [DebuggerStepThrough]
            get { return this.serviceOperationWrapperCache; }
        }

        /// <summary>
        /// Maps names to ResourceAssociationSets.
        /// </summary>
        internal Dictionary<string, ResourceAssociationSet> ResourceAssociationSetCache
        {
            [DebuggerStepThrough]
            get { return this.resourceAssociationSetCache; }
        }

        /// <summary>
        /// Mapes "resourceSetName_resourceTypeName" to the list of visible properties from the set.
        /// </summary>
        internal Dictionary<string, List<ResourceProperty>> ResourcePropertyCache
        {
            [DebuggerStepThrough]
            get { return this.resourcePropertyCache; }
        }

        /// <summary>
        /// Mapes "resourceSetName_resourceTypeName" to boolean of whether resourceType is allowed for resourceSet
        /// </summary>
        internal Dictionary<string, object> EntityTypeDisallowedForSet
        {
            [DebuggerStepThrough]
            get { return this.entityTypeDisallowedForSet; }
        }

        /// <summary>Collection of service operations, keyed by name.</summary>
        internal Dictionary<string, ServiceOperation> ServiceOperations
        {
            [DebuggerStepThrough]
            get { return this.serviceOperations; }
        }

        /// <summary>Cache of resource properties per type.</summary>
        internal Dictionary<Type, ResourceType> TypeCache
        {
            [DebuggerStepThrough]
            get { return this.typeCache; }
        }

        /// <summary>Cache of immediate derived types per type.</summary>
        internal Dictionary<ResourceType, List<ResourceType>> ChildTypesCache
        {
            [DebuggerStepThrough]
            get { return this.childTypesCache; }
        }

        /// <summary> list of top level entity sets</summary>
        internal Dictionary<string, ResourceSet> EntitySets
        {
            [DebuggerStepThrough]
            get { return this.entitySets; }
        }

        /// <summary>Target type for the data provider.</summary>
        internal Type Type
        {
            [DebuggerStepThrough]
            get { return this.type; }
        }

        /// <summary>EDM version to which metadata is compatible.</summary>
        /// <remarks>
        /// For example, a service operation of type Void is not acceptable 1.0 CSDL,
        /// so it should use 1.1 CSDL instead. Similarly, OpenTypes are supported
        /// in 1.2 and not before.
        /// </remarks>
        internal MetadataEdmSchemaVersion EdmSchemaVersion
        {
            get;
            set;
        }

        /// <summary>Whether all EPM properties serialize in an Astoria V1-compatible way.</summary>
        /// <remarks>
        /// This property is false if any property has KeepInContent set to false.
        /// </remarks>
        internal bool EpmIsV1Compatible
        {
            get { return this.epmIsV1Compatible; }
            set { this.epmIsV1Compatible = value; }
        }

        #endregion Properties.
    }
}
