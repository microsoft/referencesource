//---------------------------------------------------------------------
// <copyright file="ServiceOperationWrapper.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Wrapper class for ServiceOperation so we can store service
//      specific data here.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;

    /// <summary>Use this class to represent a custom service operation.</summary>
    [DebuggerVisualizer("ServiceOperation={Name}")]
    internal sealed class ServiceOperationWrapper
    {
        #region Private Fields

        /// <summary>
        /// Wrapped instance of the service operation.
        /// </summary>
        private readonly ServiceOperation serviceOperation;

        /// <summary>Access rights to this service operation.</summary>
        private ServiceOperationRights rights;

        /// <summary>Entity set from which entities are read, if applicable.</summary>
        private ResourceSetWrapper resourceSet;

#if DEBUG
        /// <summary>Is true, if the service operation is fully initialized and validated. No more changes can be made once its set to readonly.</summary>
        private bool isReadOnly;
#endif

        #endregion Private Fields

        #region Constructor

        /// <summary>
        /// Initializes a new <see cref="ServiceOperationWrapper"/> instance.
        /// </summary>
        /// <param name="serviceOperation">ServiceOperation instance to be wrapped.</param>
        public ServiceOperationWrapper(ServiceOperation serviceOperation)
        {
            Debug.Assert(serviceOperation != null, "serviceOperation != null");
            if (!serviceOperation.IsReadOnly)
            {
                throw new DataServiceException(500, Strings.DataServiceProviderWrapper_ServiceOperationNotReadonly(serviceOperation.Name));
            }

            this.serviceOperation = serviceOperation;
        }

        #endregion Constructor

        #region Properties

        /// <summary>Protocol (for example HTTP) method the service operation responds to.</summary>
        public string Method
        {
            [DebuggerStepThrough]
            get { return this.serviceOperation.Method; }
        }

        /// <summary>MIME type specified on primitive results, possibly null.</summary>
        public string MimeType
        {
            [DebuggerStepThrough]
            get { return this.serviceOperation.MimeType; }
        }

        /// <summary>Name of the service operation.</summary>
        public string Name
        {
            [DebuggerStepThrough]
            get { return this.serviceOperation.Name; }
        }

        /// <summary>Returns all the parameters for the given service operations./// </summary>
        public ReadOnlyCollection<ServiceOperationParameter> Parameters
        {
            [DebuggerStepThrough]
            get { return this.serviceOperation.Parameters; }
        }

        /// <summary>Kind of result expected from this operation.</summary>
        public ServiceOperationResultKind ResultKind
        {
            [DebuggerStepThrough]
            get { return this.serviceOperation.ResultKind; }
        }

        /// <summary>Element of result type.</summary>
        /// <remarks>
        /// Note that if the method returns an IEnumerable&lt;string&gt;, 
        /// this property will be typeof(string).
        /// </remarks>
        public ResourceType ResultType
        {
            [DebuggerStepThrough]
            get { return this.serviceOperation.ResultType; }
        }

        /// <summary>
        /// Gets the wrapped service operation
        /// </summary>
        public ServiceOperation ServiceOperation
        {
            [DebuggerStepThrough]
            get { return this.serviceOperation; }
        }

        /// <summary>Whether the operation is visible to service consumers.</summary>
        public bool IsVisible
        {
            [DebuggerStepThrough]
            get
            {
#if DEBUG
                Debug.Assert(this.isReadOnly, "Wrapper class has not been initialized yet.");
#endif
                return (this.rights & ~ServiceOperationRights.OverrideEntitySetRights) != ServiceOperationRights.None;
            }
        }

        /// <summary>Access rights to this service operation.</summary>
        public ServiceOperationRights Rights
        {
            [DebuggerStepThrough]
            get
            {
#if DEBUG
                Debug.Assert(this.isReadOnly, "Wrapper class has not been initialized yet.");
#endif
                return this.rights;
            }
        }

        /// <summary>Entity set from which entities are read (possibly null).</summary>
        public ResourceSetWrapper ResourceSet
        {
            [DebuggerStepThrough]
            get
            {
#if DEBUG
                Debug.Assert(this.isReadOnly, "Wrapper class has not been initialized yet.");
#endif
                Debug.Assert(
                    this.serviceOperation.ResourceSet == null || this.resourceSet.ResourceSet == this.serviceOperation.ResourceSet,
                    "this.serviceOperation.ResourceSet == null || this.resourceSet.ResourceSet == this.serviceOperation.ResourceSet");
                return this.resourceSet;
            }
        }

        #endregion Properties

        /// <summary>
        /// Apply the given configuration to the resource set.
        /// </summary>
        /// <param name="configuration">data service configuration instance.</param>
        /// <param name="provider">data service provider wrapper instance for accessibility validation.</param>
        public void ApplyConfiguration(DataServiceConfiguration configuration, DataServiceProviderWrapper provider)
        {
#if DEBUG
            Debug.Assert(!this.isReadOnly, "Can only apply the configuration once.");
#endif
            this.rights = configuration.GetServiceOperationRights(this.serviceOperation);

            if ((this.rights & ~ServiceOperationRights.OverrideEntitySetRights) != ServiceOperationRights.None)
            {
                if (this.serviceOperation.ResourceSet != null)
                {
                    // If the result type is an entity type, we need to make sure its entity set is visible.
                    // If the entity set is hidden, we need to make sure that we throw an exception.
                    this.resourceSet = provider.TryResolveResourceSet(this.serviceOperation.ResourceSet.Name);
                    if (this.resourceSet == null)
                    {
                        throw new InvalidOperationException(Strings.BaseServiceProvider_ServiceOperationTypeHasNoContainer(this.serviceOperation.Name, this.serviceOperation.ResultType.FullName));
                    }
                }
            }
#if DEBUG
            this.isReadOnly = true;
#endif
        }
    }
}
