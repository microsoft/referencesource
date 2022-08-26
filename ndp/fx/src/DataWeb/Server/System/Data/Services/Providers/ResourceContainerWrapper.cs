//---------------------------------------------------------------------
// <copyright file="ResourceContainerWrapper.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Wrapper class for a resource set.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Reflection;

    #endregion Namespaces.

    /// <summary>
    /// Wrapper class for a resource set.  A resource set object can be shared across services,
    /// this wrapper class contains the resouce set information and also service specific
    /// information about that resource set.
    /// </summary>
    [DebuggerDisplay("{Name}: {ResourceType}")]
    internal class ResourceSetWrapper
    {
        #region Fields

        /// <summary>Reference to the wrapped resource set</summary>
        private readonly ResourceSet resourceSet;

        /// <summary>Reference to the wrapped resource type.</summary>
        private readonly ResourceType resourceType;

        /// <summary>Access rights to this resource set.</summary>
        private EntitySetRights rights;

        /// <summary>Page Size for this resource set.</summary>
        private int pageSize;

        /// <summary>Methods to be called when composing read queries to allow authorization.</summary>
        private MethodInfo[] readAuthorizationMethods;

        /// <summary>Methods to be called when validating write methods to allow authorization.</summary>
        private MethodInfo[] writeAuthorizationMethods;
        
        /// <summary>Whether the types contained in the set has mappings for friendly feeds are V1 compatible or not</summary>
        private bool? epmIsV1Compatible;

#if DEBUG
        /// <summary>Is true, if the resource set is fully initialized and validated. No more changes can be made once its set to readonly.</summary>
        private bool isReadOnly;
#endif

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructs a new ResourceSetWrapper instance using the ResourceSet instance to be enclosed.
        /// </summary>
        /// <param name="resourceSet">ResourceSet instance to be wrapped by the current instance</param>
        /// <param name="resourceType">Resource type (normalized to a single instance by the caller).</param>
        public ResourceSetWrapper(ResourceSet resourceSet, ResourceType resourceType)
        {
            Debug.Assert(resourceSet != null, "resourceSet != null");

            if (!resourceSet.IsReadOnly)
            {
                throw new DataServiceException(500, Strings.DataServiceProviderWrapper_ResourceContainerNotReadonly(resourceSet.Name));
            }

            this.resourceSet = resourceSet;
            this.resourceType = resourceType;
        }

        #endregion Constructors

        #region Properties

        /// <summary>Name of the resource set.</summary>
        public string Name
        {
            get { return this.resourceSet.Name; }
        }

        /// <summary> Reference to resource type that this resource set is a collection of </summary>
        public ResourceType ResourceType
        {
            get { return this.resourceType; }
        }

        /// <summary>Whether the resource set is visible to service consumers.</summary>
        public bool IsVisible
        {
            get
            {
#if DEBUG
                Debug.Assert(this.isReadOnly, "IsVisible - entity set settings not initialized.");
#endif
                return this.rights != EntitySetRights.None;
            }
        }

        /// <summary>Access rights to this resource set.</summary>
        public EntitySetRights Rights
        {
            get
            {
#if DEBUG
                Debug.Assert(this.isReadOnly, "Rights - entity set settings not initialized.");
#endif
                return this.rights;
            }
        }

        /// <summary>Page Size for this resource set.</summary>
        public int PageSize
        {
            get
            {
#if DEBUG
                Debug.Assert(this.isReadOnly, "Rights - entity set settings not initialized.");
#endif
                return this.pageSize;
            }
        }

        /// <summary>Retursn the list of query interceptors for this set (possibly null).</summary>
        public MethodInfo[] QueryInterceptors
        {
            [DebuggerStepThrough]
            get
            {
#if DEBUG
                Debug.Assert(this.isReadOnly, "QueryInterceptors - entity set settings not initialized.");
#endif
                return this.readAuthorizationMethods;
            }
        }

        /// <summary>Returns the list of change interceptors for this set (possible null).</summary>
        public MethodInfo[] ChangeInterceptors
        {
            [DebuggerStepThrough]
            get
            {
#if DEBUG
                Debug.Assert(this.isReadOnly, "ChangeInterceptors - entity set settings not initialized.");
#endif
                return this.writeAuthorizationMethods; 
            }
        }

        /// <summary>Returns the wrapped resource set instance.</summary>
        internal ResourceSet ResourceSet
        {
            [DebuggerStepThrough]
            get
            {
#if DEBUG
                Debug.Assert(this.resourceSet != null, "this.resourceSet != null");
#endif
                return this.resourceSet;
            }
        }

        #endregion Properties

        #region Methods

        /// <summary>
        /// Apply the given configuration to the resource set.
        /// </summary>
        /// <param name="configuration">data service configuration instance.</param>
        public void ApplyConfiguration(DataServiceConfiguration configuration)
        {
#if DEBUG
            Debug.Assert(!this.isReadOnly, "Can only apply the configuration once.");
#endif

            // Set entity set rights
            this.rights = configuration.GetResourceSetRights(this.resourceSet);

            // Set page size
            this.pageSize = configuration.GetResourceSetPageSize(this.resourceSet);
            if (this.pageSize < 0)
            {
                throw new DataServiceException(500, Strings.DataService_SDP_PageSizeMustbeNonNegative(this.pageSize, this.Name));
            }

            // Add QueryInterceptors
            this.readAuthorizationMethods = configuration.GetReadAuthorizationMethods(this.resourceSet);

            // Add ChangeInterceptors
            this.writeAuthorizationMethods = configuration.GetWriteAuthorizationMethods(this.resourceSet);

#if DEBUG
            this.isReadOnly = true;
#endif
        }

        /// <summary>Whether the types contained in the set has mappings for friendly feeds are V1 compatible or not</summary>
        /// <param name="provider">Data service provider instance.</param>
        /// <returns>False if there's any type in this set which has friendly feed mappings with KeepInContent=false. True otherwise.</returns>
        internal bool EpmIsV1Compatible(DataServiceProviderWrapper provider)
        {
#if DEBUG
            Debug.Assert(provider != null, "provider != null");
            Debug.Assert(this.resourceSet != null, "this.resourceSet != null");
            Debug.Assert(this.isReadOnly, "EpmIsV1Compatible - entity set settings not initialized.");
#endif
            if (!this.epmIsV1Compatible.HasValue)
            {
                // Go through all types contained in the set. If any one type is EpmIsV1Compatible == false,
                // the whole set is EpmIsV1Compatible=false.
                ResourceType baseType = this.resourceSet.ResourceType;
                bool isV1Compatible = baseType.EpmIsV1Compatible;

                // If the base type is not epm v1 compatible or it has no derived type, we need not look any further.
                if (isV1Compatible && provider.HasDerivedTypes(baseType))
                {
                    foreach (ResourceType derivedType in provider.GetDerivedTypes(baseType))
                    {
                        if (!derivedType.EpmIsV1Compatible)
                        {
                            // We can stop as soon as we find the first type that is not epm v1 compatible.
                            isV1Compatible = false;
                            break;
                        }
                    }
                }

                this.epmIsV1Compatible = isV1Compatible;
            }

            return this.epmIsV1Compatible.Value;
        }

        #endregion Methods
    }
}
