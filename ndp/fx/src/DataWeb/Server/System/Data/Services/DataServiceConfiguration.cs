//---------------------------------------------------------------------
// <copyright file="DataServiceConfiguration.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides an implementation of the IDataServiceConfiguration
//      interface.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Data.Services.Client;
    using System.Data.Services.Providers;
    using System.Data.Services.Common;
    using System.Data.Services.Configuration;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Reflection;
    using System.Net;
    #endregion Namespaces.

    /// <summary>Use this class to manage the configuration data for a data service.</summary>
    public sealed class DataServiceConfiguration : IDataServiceConfiguration
    {
        #region Private fields.

        /// <summary>Whether this configuration has been sealed.</summary>
        private bool configurationSealed;

        /// <summary>Maximum number of change sets and query operations in a batch.</summary>
        private int maxBatchCount;

        /// <summary>Maximum number of changes in a change set.</summary>
        private int maxChangeSetCount;

        /// <summary>Maximum number of segments to be expanded allowed in a request.</summary>
        private int maxExpandCount;

        /// <summary>Maximum number of segments in a single $expand path.</summary>
        private int maxExpandDepth;

        /// <summary>Maximum number of elements in each returned collection (top-level or expanded).</summary>
        private int maxResultsPerCollection;

        /// <summary>maximum number of objects that can be referred in a single insert request.</summary>
        private int maxObjectCountOnInsert;

        /// <summary>The provider for the web service.</summary>
        private IDataServiceMetadataProvider provider;

        /// <summary>Rights used for unspecified resource sets.</summary>
        private EntitySetRights rightsForUnspecifiedResourceContainer;

        /// <summary>Rights used for unspecified service operations.</summary>
        private ServiceOperationRights rightsForUnspecifiedServiceOperation;

        /// <summary>Page size for unspecified resource sets</summary>
        private int defaultPageSize;

        /// <summary>
        /// A lookup of containers to their rights.
        /// For IDSP there is no guarantee that the provider will always return the same metadata instance.  We should
        /// use the name instead of the instance as key since the configuration is cached across requests.
        /// </summary>
        private Dictionary<string, EntitySetRights> resourceRights;

        /// <summary>
        /// A lookup of service operations to their rights.
        /// For IDSP there is no guarantee that the provider will always return the same metadata instance.  We should
        /// use the name instead of the instance as key since the configuration is cached across requests.
        /// </summary>
        private Dictionary<string, ServiceOperationRights> serviceRights;

        /// <summary>
        /// A lookup of resource sets to their page sizes.
        /// For IDSP there is no guarantee that the provider will always return the same metadata instance.  We should
        /// use the name instead of the instance as key since the configuration is cached across requests.
        /// </summary>
        private Dictionary<string, int> pageSizes;

        /// <summary>A list of known types.</summary>
        private List<Type> knownTypes;

        /// <summary>Whether verbose errors should be returned by default.</summary>
        private bool useVerboseErrors;

        /// <summary>Holds configuration of service behavior</summary>
        private DataServiceBehavior dataServiceBehavior;

        /// <summary>
        /// Perform type conversion from the type specified in the payload to the actual property type.
        /// </summary>
        private bool typeConversion;

        /// <summary>
        /// A lookup of resource sets to the corresponding QueryInterceptors.
        /// For IDSP there is no guarantee that the provider will always return the same metadata instance.  We should
        /// use the name instead of the instance as key since the configuration is cached across requests.
        /// </summary>
        private Dictionary<string, List<MethodInfo>> readAuthorizationMethods;

        /// <summary>
        /// A lookup of resource sets to the corresponding ChangeInterceptors.
        /// For IDSP there is no guarantee that the provider will always return the same metadata instance.  We should
        /// use the name instead of the instance as key since the configuration is cached across requests.
        /// </summary>
        private Dictionary<string, List<MethodInfo>> writeAuthorizationMethods;

        /// <summary>This is set to true if EnableAccess("*") is called.  False otherwise.</summary>
        private bool accessEnabledForAllResourceTypes;

        /// <summary>List of fully qualified type names that were marked as visible by calling EnableAccess().</summary>
        private HashSet<string> accessEnabledResourceTypes;

        #endregion Private fields.

        #region Constructor.

        /// <summary>
        /// Initializes a new <see cref="DataServiceConfiguration"/> with
        /// the specified <paramref name="provider"/>.
        /// </summary>
        /// <param name="provider">Non-null provider for this configuration.</param>
        internal DataServiceConfiguration(IDataServiceMetadataProvider provider)
        {
            Debug.Assert(provider != null, "provider != null");
            this.provider = provider;
            this.resourceRights = new Dictionary<string, EntitySetRights>(EqualityComparer<string>.Default);
            this.serviceRights = new Dictionary<string, ServiceOperationRights>(EqualityComparer<string>.Default);
            this.pageSizes = new Dictionary<string, int>(EqualityComparer<string>.Default);
            this.rightsForUnspecifiedResourceContainer = EntitySetRights.None;
            this.rightsForUnspecifiedServiceOperation = ServiceOperationRights.None;
            this.knownTypes = new List<Type>();
            this.maxBatchCount = Int32.MaxValue;
            this.maxChangeSetCount = Int32.MaxValue;
            this.maxExpandCount = Int32.MaxValue;
            this.maxExpandDepth = Int32.MaxValue;
            this.maxResultsPerCollection = Int32.MaxValue;
            this.maxObjectCountOnInsert = Int32.MaxValue;
            this.readAuthorizationMethods = new Dictionary<string, List<MethodInfo>>(EqualityComparer<string>.Default);
            this.writeAuthorizationMethods = new Dictionary<string, List<MethodInfo>>(EqualityComparer<string>.Default);
            this.accessEnabledResourceTypes = new HashSet<string>(EqualityComparer<string>.Default);
            this.dataServiceBehavior = new DataServiceBehavior();

            // default value is true since in V1, we always did the type conversion
            // and this configuration settings was introduced in V2
            this.typeConversion = true;
        }

        #endregion Constructor.

        #region Public Properties

        /// <summary>
        /// Specifies whether the data service runtime should do type conversion from the payload type
        /// to the actual property type in POST/PUT/MERGE requests.
        /// </summary>
        public bool EnableTypeConversion
        {
            get
            {
                return this.typeConversion;
            }

            set
            {
                this.CheckNotSealed();
                this.typeConversion = value;
            }
        }

        /// <summary>Maximum number of change sets and query operations in a batch.</summary>
        public int MaxBatchCount
        {
            get { return this.maxBatchCount; }
            set { this.maxBatchCount = this.CheckNonNegativeProperty(value, "MaxBatchCount"); }
        }

        /// <summary>Maximum number of changes in a change set.</summary>
        public int MaxChangesetCount
        {
            get { return this.maxChangeSetCount; }
            set { this.maxChangeSetCount = this.CheckNonNegativeProperty(value, "MaxChangesetCount"); }
        }

        /// <summary>Maximum number of segments to be expanded allowed in a request.</summary>
        public int MaxExpandCount
        {
            get { return this.maxExpandCount; }
            set { this.maxExpandCount = this.CheckNonNegativeProperty(value, "MaxExpandCount"); }
        }

        /// <summary>Maximum number of segments in a single $expand path.</summary>
        public int MaxExpandDepth
        {
            get { return this.maxExpandDepth; }
            set { this.maxExpandDepth = this.CheckNonNegativeProperty(value, "MaxExpandDepth"); }
        }

        /// <summary>Maximum number of elements in each returned collection (top-level or expanded).</summary>
        public int MaxResultsPerCollection
        {
            get
            {
                return this.maxResultsPerCollection;
            }

            set
            {
                if (this.IsPageSizeDefined)
                {
                    throw new InvalidOperationException(Strings.DataService_SDP_PageSizeWithMaxResultsPerCollection);
                }

                this.maxResultsPerCollection = this.CheckNonNegativeProperty(value, "MaxResultsPerCollection");
            }
        }

        /// <summary>Maximum number of objects that can be referred in a single POST request.</summary>
        public int MaxObjectCountOnInsert
        {
            get { return this.maxObjectCountOnInsert; }
            set { this.maxObjectCountOnInsert = this.CheckNonNegativeProperty(value, "MaxObjectCountOnInsert"); }
        }

        /// <summary>Gets or sets whether verbose errors should be used by default.</summary>
        /// <remarks>
        /// This property sets the default for the whole service; individual responses may behave differently
        /// depending on the value of the VerboseResponse property of the arguments to the HandleException
        /// method on the <see cref="DataService&lt;T&gt;"/> class.
        /// </remarks>
        public bool UseVerboseErrors
        {
            get
            {
                return this.useVerboseErrors;
            }

            set
            {
                this.CheckNotSealed();
                this.useVerboseErrors = value;
            }
        }

        /// <summary>
        /// Gets settings that define service behavior.
        /// </summary>
        public DataServiceBehavior DataServiceBehavior
        {
            get
            {
                return this.dataServiceBehavior;
            }
        }

        #endregion Public Properties

        #region Internal Properties

        /// <summary>True if all resource types have been made visible by calling EnableAccess("*").  False otherwise.</summary>
        internal bool AccessEnabledForAllResourceTypes
        {
            [DebuggerStepThrough]
            get { return this.accessEnabledForAllResourceTypes; }
        }

        #endregion Internal Properties

        #region Private Properties

        /// <summary>
        /// Whether size of a page has been defined.
        /// </summary>
        private bool IsPageSizeDefined
        {
            get
            {
                return this.pageSizes.Count > 0 || this.defaultPageSize > 0;
            }
        }

        #endregion

        #region Public Methods

        /// <summary>Sets the access rights on the specified resource set.</summary>
        /// <param name="name">
        /// Name of resource set to set; '*' to indicate all 
        /// resource sets not otherwise specified.
        /// </param>
        /// <param name="rights">Rights to be granted to this resource.</param>
        public void SetEntitySetAccessRule(string name, EntitySetRights rights)
        {
            this.CheckNotSealed();
            if (name == null)
            {
                throw Error.ArgumentNull("name");
            }

            WebUtil.CheckResourceContainerRights(rights, "rights");
            if (name == "*")
            {
                this.rightsForUnspecifiedResourceContainer = rights;
            }
            else
            {
                ResourceSet container;
                if (!this.provider.TryResolveResourceSet(name, out container) || container == null)
                {
                    throw new ArgumentException(Strings.DataServiceConfiguration_ResourceSetNameNotFound(name), "name");
                }

                this.resourceRights[container.Name] = rights;
            }
        }

        /// <summary>Sets the access rights on the specified service operation.</summary>
        /// <param name="name">
        /// Name of service operation to set; '*' to indicate all 
        /// service operations not otherwise specified.
        /// </param>
        /// <param name="rights">Rights to be granted to this operation.</param>
        public void SetServiceOperationAccessRule(string name, ServiceOperationRights rights)
        {
            this.CheckNotSealed();
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            WebUtil.CheckServiceOperationRights(rights, "rights");
            if (name == "*")
            {
                this.rightsForUnspecifiedServiceOperation = rights;
            }
            else
            {
                ServiceOperation serviceOperation;
                if (!this.provider.TryResolveServiceOperation(name, out serviceOperation) || serviceOperation == null)
                {
                    throw new ArgumentException(Strings.DataServiceConfiguration_ServiceNameNotFound(name), "name");
                }

                this.serviceRights[serviceOperation.Name] = rights;
            }
        }

        /// <summary>
        /// Add the type to the list of known types. If there is a type that needs to be added since
        /// Astoria runtime can't detect it using the default set of rules, providers can add them using
        /// this method
        /// </summary>
        /// <param name="type">type which needs to be added to the known types collection</param>
        public void RegisterKnownType(Type type)
        {
            this.CheckNotSealed();
            this.knownTypes.Add(type);
        }

        /// <summary>Sets the page size per entity set</summary>
        /// <param name="name">Name of entity set, '*' to indicate those for which page size is not explicitly specified</param>
        /// <param name="size">Page size for the resource set(s) specified in <paramref name="name"/></param>
        public void SetEntitySetPageSize(String name, int size)
        {
            WebUtil.CheckArgumentNull(name, "name");
            if (size < 0)
            {
                throw new ArgumentOutOfRangeException("size", size, Strings.DataService_SDP_PageSizeMustbeNonNegative(size, name));
            }

            // Treat a page size of Int32.MaxValue to be the same as not setting the page size.
            if (size == Int32.MaxValue)
            {
                size = 0;
            }

            if (this.MaxResultsPerCollection != Int32.MaxValue)
            {
                throw new InvalidOperationException(Strings.DataService_SDP_PageSizeWithMaxResultsPerCollection);
            }

            this.CheckNotSealed();

            if (name == "*")
            {
                this.defaultPageSize = size;
            }
            else
            {
                ResourceSet container;
                if (!this.provider.TryResolveResourceSet(name, out container) || container == null)
                {
                    throw new ArgumentException(Strings.DataServiceConfiguration_ResourceSetNameNotFound(name), "name");
                }

                this.pageSizes[container.Name] = size;
            }
        }

        /// <summary>
        /// This method is used to register a type with the Astoria runtime which may be returned in the “open properties” of
        /// an open type such that the type is visible in $metadata output and usable with CRUD operations.
        /// 
        /// The typename parameter must be a namespace qualified type name (format: &lt;namespace&gt;.&lt;typename&gt;).  
        /// The name provided must be as it would show up in a CSDL document (ie. model types, not CLR types)
        /// 
        /// The types registered via calls to EnableAccess will be additive to those implicitly made accessible via 
        /// DSC.SetEntitySetAccessRule(…) invocations
        ///  • Note: The Astoria runtime layer won’t be able to determine if a typename specified maps to an Entity Type,
        ///    Complex Type, etc until it actually obtains type info (entity types, complex types, etc) from the underlying provider
        ///  • “*” can be used as the value of ‘typename’, which will be interpreted as matching all types
        ///  
        /// When Astoria enumerates types or needs to obtain a type (Complex Types, Entity Types) from the underlying provider
        /// it will first determine if the type should be visible (show in $metadata and accessible via operations exposed by the
        /// service) as per the standard v1 checks (ie. driven by SetEntitySetAccessRule calls). If the type is not visible via V1
        /// rules, then we consult the set of types registered via EnableAccess(&lt;typename&gt;) invocations.  If the type was
        /// included in such a call then the type is visible via $metadata and can be accessed via CRUD ops, etc.
        /// 
        /// If a type is not made visible via one of the mechanisms above, then:
        ///   • That type must not be included a response to a $metadata request
        ///   • Instances of the type must not be returned to the client as the response of a request to the data service. 
        ///     If such a type instance would be required the service MUST fail the request.  Failure semantics are covered 
        ///     in the area of the specification which covers request/response semantics with respect to open types. 
        ///
        /// Invoking this method multiple times with the same type name is allowed and considered a “NO OP”.
        /// </summary>
        /// <param name="typeName">The namespace qualified complex type name to be made visible via the data service</param>
        public void EnableTypeAccess(string typeName)
        {
            WebUtil.CheckStringArgumentNull(typeName, "typeName");
            this.CheckNotSealed();

            if (typeName == "*")
            {
                this.accessEnabledForAllResourceTypes = true;
            }
            else
            {
                ResourceType resourceType;
                if (!this.provider.TryResolveResourceType(typeName, out resourceType) || resourceType == null)
                {
                    throw new ArgumentException(Strings.DataServiceConfiguration_ResourceTypeNameNotFound(typeName), "typeName");
                }

                if (resourceType.ResourceTypeKind != ResourceTypeKind.ComplexType)
                {
                    throw new ArgumentException(Strings.DataServiceConfiguration_NotComplexType(typeName), "typeName");
                }

                Debug.Assert(resourceType.FullName == typeName, "resourceType.FullName == typeName");
                this.accessEnabledResourceTypes.Add(typeName);
            }
        }

        #endregion Public Methods

        #region Internal methods.

        /// <summary>Composes all query interceptors into a single expression.</summary>
        /// <param name="service">Web service instance.</param>
        /// <param name="container">Container for which interceptors should run.</param>
        /// <returns>An expression the filter for query interceptors, possibly null.</returns>
        internal static Expression ComposeQueryInterceptors(IDataService service, ResourceSetWrapper container)
        {
            Debug.Assert(service != null, "service != null");
            Debug.Assert(container != null, "container != null");
            MethodInfo[] methods = container.QueryInterceptors;
            if (methods == null || methods.Length == 0)
            {
                return null;
            }

            LambdaExpression filter = null;
            for (int i = 0; i < methods.Length; i++)
            {
                Expression predicate = null;
                try
                {
                    predicate = (Expression)methods[i].Invoke(service.Instance, WebUtil.EmptyObjectArray);
                }
                catch (TargetInvocationException tie)
                {
                    ErrorHandler.HandleTargetInvocationException(tie);
                    throw;
                }

                if (predicate == null)
                {
                    throw new InvalidOperationException(Strings.DataService_AuthorizationReturnedNullQuery(methods[i].Name, methods[i].DeclaringType.FullName));
                }

                Debug.Assert(predicate is LambdaExpression, "predicate is LambdaExpression -- otherwise signature check missed something.");
                if (filter == null)
                {
                    filter = (LambdaExpression)predicate;
                }
                else
                {
                    ParameterExpression parameter = filter.Parameters[0];
                    Expression adjustedPredicate = ParameterReplacerVisitor.Replace(
                        ((LambdaExpression)predicate).Body,             // expression
                        ((LambdaExpression)predicate).Parameters[0],    // oldParameter
                        parameter);                                     // newParameter
                    filter = Expression.Lambda(Expression.And(filter.Body, adjustedPredicate), parameter);
                }
            }

            return filter;
        }

        /// <summary>
        /// Composes the specified <paramref name="query"/> for the 
        /// given <paramref name="container"/> with authorization
        /// callbacks.
        /// </summary>
        /// <param name="service">Data service on which to invoke method.</param>
        /// <param name="container">resource set to compose with.</param>
        /// <param name="query">Query to compose.</param>
        /// <returns>The resulting composed query.</returns>
        internal static IQueryable ComposeResourceContainer(IDataService service, ResourceSetWrapper container, IQueryable query)
        {
            Debug.Assert(service != null, "service != null");
            Debug.Assert(container != null, "container != null");
            Debug.Assert(query != null, "query != null");

            MethodInfo[] methods = container.QueryInterceptors;
            if (methods != null)
            {
                for (int i = 0; i < methods.Length; i++)
                {
                    Expression predicate = null;
                    try
                    {
                        predicate = (Expression)methods[i].Invoke(service.Instance, WebUtil.EmptyObjectArray);
                    }
                    catch (TargetInvocationException tie)
                    {
                        ErrorHandler.HandleTargetInvocationException(tie);
                        throw;
                    }

                    if (predicate == null)
                    {
                        throw new InvalidOperationException(Strings.DataService_AuthorizationReturnedNullQuery(methods[i].Name, methods[i].DeclaringType.FullName));
                    }

                    Debug.Assert(predicate is LambdaExpression, "predicate is LambdaExpression -- otherwise signature check missed something.");
                    query = RequestUriProcessor.InvokeWhereForType(query, (LambdaExpression)predicate);
                }
            }

            return query;
        }

        /// <summary>Checks whether this request has the specified rights.</summary>
        /// <param name="container">Container to check.</param>
        /// <param name="requiredRights">Required rights.</param>
        /// <exception cref="DataServiceException">Thrown if <paramref name="requiredRights"/> aren't available.</exception>
        internal static void CheckResourceRights(ResourceSetWrapper container, EntitySetRights requiredRights)
        {
            Debug.Assert(container != null, "container != null");
            Debug.Assert(requiredRights != EntitySetRights.None, "requiredRights != EntitySetRights.None");

            if ((requiredRights & container.Rights) == 0)
            {
                throw DataServiceException.CreateForbidden();
            }
        }

        /// <summary>Checks whether this request has the specified reading rights.</summary>
        /// <param name="container">Container to check.</param>
        /// <param name="singleResult">Whether a single or multiple resources are requested.</param>
        /// <exception cref="DataServiceException">Thrown if <paramref name="singleResult"/> aren't available.</exception>
        internal static void CheckResourceRightsForRead(ResourceSetWrapper container, bool singleResult)
        {
            Debug.Assert(container != null, "container != null");
            EntitySetRights requiredRights = singleResult ? EntitySetRights.ReadSingle : EntitySetRights.ReadMultiple;
            CheckResourceRights(container, requiredRights);
        }

        /// <summary>Checks whether this request has the specified rights.</summary>
        /// <param name="operation">Operation to check.</param>
        /// <param name="requiredRights">Required rights.</param>
        /// <exception cref="DataServiceException">Thrown if <paramref name="requiredRights"/> aren't available.</exception>
        internal static void CheckServiceRights(ServiceOperationWrapper operation, ServiceOperationRights requiredRights)
        {
            Debug.Assert(operation != null, "operation != null");
            Debug.Assert(requiredRights != ServiceOperationRights.None, "requiredRights != EntitySetRights.None");

            ServiceOperationRights effectiveRights = operation.Rights;
            if ((requiredRights & effectiveRights) == 0)
            {
                throw DataServiceException.CreateForbidden();
            }
        }

        /// <summary>Checks whether this request has the specified rights.</summary>
        /// <param name="operation">Operation to check.</param>
        /// <param name="singleResult">Whether a single or multiple resources are requested.</param>
        /// <exception cref="DataServiceException">Thrown if <paramref name="singleResult"/> aren't available.</exception>
        internal static void CheckServiceRights(ServiceOperationWrapper operation, bool singleResult)
        {
            Debug.Assert(operation != null, "operation != null");

            if (operation.ResultKind != ServiceOperationResultKind.Void)
            {
                ServiceOperationRights requiredRights = singleResult ? ServiceOperationRights.ReadSingle : ServiceOperationRights.ReadMultiple;
                CheckServiceRights(operation, requiredRights);
            }
        }

        /// <summary>Gets a string with methods allowed on the target for the <paramref name="description"/>.</summary>
        /// <param name="configuration">configuration object which has the data</param>
        /// <param name="description">Description with target.</param>
        /// <returns>A string with methods allowed on the description; possibly null.</returns>
        internal static string GetAllowedMethods(DataServiceConfiguration configuration, RequestDescription description)
        {
            Debug.Assert(description != null, "description != null");
            Debug.Assert(
                description.TargetKind != RequestTargetKind.Nothing,
                "description.TargetKind != RequestTargetKind.Void - otherwise it hasn't been determined yet");
            Debug.Assert(
                description.TargetKind != RequestTargetKind.VoidServiceOperation,
                "description.TargetKind != RequestTargetKind.VoidServiceOperation - this method is only for containers");
            if (description.TargetKind == RequestTargetKind.Metadata ||
                description.TargetKind == RequestTargetKind.ServiceDirectory)
            {
                return XmlConstants.HttpMethodGet;
            }
            else if (description.TargetKind == RequestTargetKind.Batch)
            {
                return XmlConstants.HttpMethodPost;
            }
            else
            {
                int index = description.GetIndexOfTargetEntityResource();
                Debug.Assert(index >= 0 && index < description.SegmentInfos.Length, "index >=0 && index <description.SegmentInfos.Length");
                ResourceSetWrapper container = description.SegmentInfos[index].TargetContainer;
                return GetAllowedMethods(configuration, container, description);
            }
        }

        /// <summary>
        /// Gets a string representation of allowed methods on the container (with the specified target cardinality),
        /// suitable for an 'Allow' header.
        /// </summary>
        /// <param name="configuration">configuration object which has the data</param>
        /// <param name="container">Targetted container, possibly null.</param>
        /// <param name="description">Description with target.</param>
        /// <returns>A value for an 'Allow' header; null if <paramref name="container"/> is null.</returns>
        internal static string GetAllowedMethods(DataServiceConfiguration configuration, ResourceSetWrapper container, RequestDescription description)
        {
            if (container == null)
            {
                return null;
            }
            else
            {
                System.Text.StringBuilder result = new System.Text.StringBuilder();
                EntitySetRights rights = configuration.GetResourceSetRights(container.ResourceSet);
                if (description.IsSingleResult)
                {
                    AppendRight(rights, EntitySetRights.ReadSingle, XmlConstants.HttpMethodGet, result);
                    AppendRight(rights, EntitySetRights.WriteReplace, XmlConstants.HttpMethodPut, result);
                    if (description.TargetKind != RequestTargetKind.MediaResource)
                    {
                        AppendRight(rights, EntitySetRights.WriteMerge, XmlConstants.HttpMethodMerge, result);
                        AppendRight(rights, EntitySetRights.WriteDelete, XmlConstants.HttpMethodDelete, result);
                    }
                }
                else
                {
                    AppendRight(rights, EntitySetRights.ReadMultiple, XmlConstants.HttpMethodGet, result);
                    AppendRight(rights, EntitySetRights.WriteAppend, XmlConstants.HttpMethodPost, result);
                }

                return result.ToString();
            }
        }

        /// <summary>Gets the effective rights on the specified container.</summary>
        /// <param name="container">Container to get rights for.</param>
        /// <returns>The effective rights as per this configuration.</returns>
        internal EntitySetRights GetResourceSetRights(ResourceSet container)
        {
            Debug.Assert(container != null, "container != null");
            Debug.Assert(this.resourceRights != null, "this.resourceRights != null");

            EntitySetRights result;
            if (!this.resourceRights.TryGetValue(container.Name, out result))
            {
                result = this.rightsForUnspecifiedResourceContainer;
            }

            return result;
        }

        /// <summary>Gets the effective rights on the specified operation.</summary>
        /// <param name="serviceOperation">Operation to get rights for.</param>
        /// <returns>The effective rights as per this configuration.</returns>
        internal ServiceOperationRights GetServiceOperationRights(ServiceOperation serviceOperation)
        {
            Debug.Assert(serviceOperation != null, "operation != null");
            Debug.Assert(this.serviceRights != null, "this.serviceRights != null");

            ServiceOperationRights result;
            if (!this.serviceRights.TryGetValue(serviceOperation.Name, out result))
            {
                result = this.rightsForUnspecifiedServiceOperation;
            }

            return result;
        }

        /// <summary>Gets the page size per entity set</summary>
        /// <param name="container">Entity set for which to get the page size</param>
        /// <returns>Page size for the <paramref name="container"/></returns>
        internal int GetResourceSetPageSize(ResourceSet container)
        {
            Debug.Assert(container != null, "container != null");
            Debug.Assert(this.pageSizes != null, "this.pageSizes != null");

            int pageSize;
            if (!this.pageSizes.TryGetValue(container.Name, out pageSize))
            {
                pageSize = this.defaultPageSize;
            }

            return pageSize;
        }

        /// <summary>Returns the list of types registered by the data service.</summary>
        /// <returns>The list of types as registered by the data service</returns>
        internal IEnumerable<Type> GetKnownTypes()
        {
            return this.knownTypes;
        }

        /// <summary>Get the list of access enabled resourceType names.</summary>
        /// <returns>List of namespace qualified resourceType names that were marked as visible by calling EnableAccess().</returns>
        internal IEnumerable<string> GetAccessEnabledResourceTypes()
        {
            Debug.Assert(this.accessEnabledResourceTypes != null, "this.accessEnabledResourceTypes != null");
            return this.accessEnabledResourceTypes;
        }

        /// <summary>
        /// Initializes the DataServiceConfiguration instance by:
        /// 1. Invokes the static service initialization methods on the specified type family.
        /// 2. Register authorization callbacks specified on the given <paramref name="type"/>.
        /// </summary>
        /// <param name="type">Type of service to initialize for.</param>
        internal void Initialize(Type type)
        {
            Debug.Assert(type != null, "type != null");
            this.InvokeStaticInitialization(type);
            this.RegisterCallbacks(type);
            this.ReadFromConfig();
        }

        /// <summary>
        /// Returns the list of QueryInterceptors for the given resource set
        /// </summary>
        /// <param name="resourceSet">resource set instance</param>
        /// <returns>List of QueryInterceptors for the resource set, null if there is none defined for the resource set.</returns>
        internal MethodInfo[] GetReadAuthorizationMethods(ResourceSet resourceSet)
        {
            Debug.Assert(resourceSet != null, "resourceSet != null");

            List<MethodInfo> methods;
            if (this.readAuthorizationMethods.TryGetValue(resourceSet.Name, out methods))
            {
                return methods.ToArray();
            }

            return null;
        }

        /// <summary>
        /// Returns the list of ChangeInterceptors for the given resource set
        /// </summary>
        /// <param name="resourceSet">resource set instance</param>
        /// <returns>List of ChangeInterceptors for the resource set, null if there is none defined for the resource set.</returns>
        internal MethodInfo[] GetWriteAuthorizationMethods(ResourceSet resourceSet)
        {
            Debug.Assert(resourceSet != null, "resourceSet != null");

            List<MethodInfo> methods;
            if (this.writeAuthorizationMethods.TryGetValue(resourceSet.Name, out methods))
            {
                return methods.ToArray();
            }

            return null;
        }

        /// <summary>Seals this configuration instance and prevents further changes.</summary>
        /// <remarks>
        /// This method should be called after the configuration has been set up and before it's placed on the
        /// metadata cache for sharing.
        /// </remarks>
        internal void Seal()
        {
            Debug.Assert(!this.configurationSealed, "!configurationSealed - otherwise .Seal is invoked multiple times");
            this.configurationSealed = true;
            this.provider = null;
        }

        /// <summary>
        /// Validated if server options used by the service are compatible with MaxProtocolVersion
        /// </summary>
        /// <param name="friendlyFeedsV1Compatible">Whether friendly feeds are compatible with V1.</param>
        internal void ValidateServerOptions(bool friendlyFeedsV1Compatible)
        {
            Debug.Assert(this.configurationSealed, "Configuration must be sealed to validate server options");

            if (this.DataServiceBehavior.MaxProtocolVersion == DataServiceProtocolVersion.V1)
            {
                if (!friendlyFeedsV1Compatible)
                {
                    throw new InvalidOperationException(Strings.DataServiceConfiguration_FriendlyFeedsWithKeepInContentFalseNotSupportedInV1Server);
                }

                if (this.IsPageSizeDefined)
                {
                    throw new InvalidOperationException(Strings.DataServiceConfiguration_ServerPagingNotSupportedInV1Server);
                }
            }
        }

        /// <summary>
        /// Validates that the versions of features used in the request are less then or equal than the configured
        /// MaxProtocolVersion. 
        /// </summary>
        /// <param name="requestDescription">Request description</param>
        internal void ValidateMaxProtocolVersion(RequestDescription requestDescription)
        {
            Debug.Assert(
                Enum.GetValues(typeof(DataServiceProtocolVersion)).Length == 2,
                "This method has to be modified when adding/removing DataServiceProtocolVersion values.");

            Version maxProtocolVersion =
                new Version(this.DataServiceBehavior.MaxProtocolVersion == DataServiceProtocolVersion.V1 ? 1 : 2, 0);

            if (requestDescription.MaxFeatureVersion > maxProtocolVersion)
            {
                throw DataServiceException.CreateBadRequestError(Strings.DataServiceConfiguration_V2ResponseForV1Server);
            }

            // As a cross check let's verify if the version of the response is not higher than the configured max protocol version.
            Debug.Assert(maxProtocolVersion >= requestDescription.ResponseVersion, "ResponseVersion > MaxProtocolVersion");
        }

        #endregion Internal methods.

        #region Private methods.

        /// <summary>
        /// Appends the <paramref name="name"/> of a right if the <paramref name="test"/> right is enabled 
        /// on <paramref name="entitySetRights"/>.
        /// </summary>
        /// <param name="entitySetRights">Rights to be checked.</param>
        /// <param name="test">Right being looked for.</param>
        /// <param name="name">Name of right to append.</param>
        /// <param name="builder">Comma-separated list of right names to append to.</param>
        private static void AppendRight(EntitySetRights entitySetRights, EntitySetRights test, string name, System.Text.StringBuilder builder)
        {
            Debug.Assert(builder != null, "builder != null");
            if (0 != (entitySetRights & test))
            {
                if (builder.Length > 0)
                {
                    builder.Append(", ");
                }

                builder.Append(name);
            }
        }

        /// <summary>Checks that the specified <paramref name="method"/> has a correct signature.</summary>
        /// <param name="type">Service type.</param>
        /// <param name="method">Method to check.</param>
        /// <param name="container">Container associated with the interceptor.</param>
        private static void CheckQueryInterceptorSignature(Type type, MethodInfo method, ResourceSet container)
        {
            Debug.Assert(type != null, "type != null");
            Debug.Assert(method != null, "method != null");
            Debug.Assert(container != null, "container != null");

            ParameterInfo[] parameters = method.GetParameters();
            if (parameters.Length != 0)
            {
                throw new InvalidOperationException(Strings.DataService_QueryInterceptorIncorrectParameterCount(method.Name, type.FullName, parameters.Length));
            }

            Type lambdaType = typeof(Func<,>).MakeGenericType(container.ResourceType.InstanceType, typeof(bool));
            Type expectedReturnType = typeof(Expression<>).MakeGenericType(lambdaType);
            Type returnType = method.ReturnType;
            if (returnType == typeof(void))
            {
                throw new InvalidOperationException(Strings.DataService_AuthorizationMethodVoid(method.Name, type.FullName, expectedReturnType));
            }
            else if (!expectedReturnType.IsAssignableFrom(returnType))
            {
                Type nullableLambdaType = typeof(Func<,>).MakeGenericType(container.ResourceType.InstanceType, typeof(bool?));
                if (!(typeof(Expression<>).MakeGenericType(nullableLambdaType).IsAssignableFrom(returnType)))
                {
                    throw new InvalidOperationException(
                        Strings.DataService_AuthorizationReturnTypeNotAssignable(method.Name, type.FullName, returnType.FullName, expectedReturnType.FullName));
                }
            }
        }

        /// <summary>Verifies that the specified <paramref name="parameter"/> is not an [out] parameter.</summary>
        /// <param name="method">Method with parameter to check.</param>
        /// <param name="parameter">Parameter to check.</param>
        private static void CheckParameterIsNotOut(MethodInfo method, ParameterInfo parameter)
        {
            Debug.Assert(method != null, "method != null");
            Debug.Assert(parameter != null, "parameter != null");

            if (parameter.IsOut)
            {
                throw new InvalidOperationException(Strings.DataService_ParameterIsOut(method.DeclaringType.FullName, method.Name, parameter.Name));
            }
        }

        /// <summary>
        /// Invokes the static service initialization methods on the 
        /// specified type family.
        /// </summary>
        /// <param name="type">Type of service to initialize for.</param>
        private void InvokeStaticInitialization(Type type)
        {
            Debug.Assert(type != null, "type != null");

            // Build a stack going from most-specific to least-specific type.
            BindingFlags flags = BindingFlags.DeclaredOnly | BindingFlags.Static | BindingFlags.Public;
            while (type != null)
            {
                MethodInfo method = type.GetMethod(XmlConstants.ClrServiceInitializationMethodName, flags, null, new Type[] { typeof(IDataServiceConfiguration) }, null);
                if (method == null)
                {
                    method = type.GetMethod(XmlConstants.ClrServiceInitializationMethodName, flags, null, new Type[] { typeof(DataServiceConfiguration) }, null);
                }

                if (method != null && method.ReturnType == typeof(void))
                {
                    Debug.Assert(method.IsStatic, "method.IsStatic");
                    Debug.Assert(method.Name == XmlConstants.ClrServiceInitializationMethodName, "Making sure that the method name is as expected");

                    ParameterInfo[] parameters = method.GetParameters();
                    if (parameters.Length == 1 && !parameters[0].IsOut)
                    {
                        object[] initializeParameters = new object[] { this };
                        try
                        {
                            method.Invoke(null, initializeParameters);
                        }
                        catch (TargetInvocationException exception)
                        {
                            ErrorHandler.HandleTargetInvocationException(exception);
                            throw;
                        }

                        return;
                    }
                }

                type = type.BaseType;
            }
        }

        /// <summary>
        /// Register authorization callbacks specified on the given
        /// <paramref name="type"/>.
        /// </summary>
        /// <param name="type">Type of web data service to check.</param>
        private void RegisterCallbacks(Type type)
        {
            Debug.Assert(type != null, "type != null");
            Debug.Assert(this.provider != null, "this.provider != null");

            BindingFlags flags = BindingFlags.DeclaredOnly | BindingFlags.Instance | BindingFlags.Public;
            while (type != null)
            {
                MethodInfo[] methods = type.GetMethods(flags);
                for (int i = 0; i < methods.Length; i++)
                {
                    MethodInfo method = methods[i];
                    QueryInterceptorAttribute[] queryAttributes = (QueryInterceptorAttribute[])
                        method.GetCustomAttributes(typeof(QueryInterceptorAttribute), true /* inherit */);
                    foreach (QueryInterceptorAttribute attribute in queryAttributes)
                    {
                        ResourceSet container;
                        if (!this.provider.TryResolveResourceSet(attribute.EntitySetName, out container) || container == null)
                        {
                            string message = Strings.DataService_AttributeEntitySetNotFound(
                                attribute.EntitySetName,
                                method.Name,
                                type.FullName);
                            throw new InvalidOperationException(message);
                        }

                        CheckQueryInterceptorSignature(type, method, container);
                        if (!method.IsAbstract)
                        {
                            if (!this.readAuthorizationMethods.ContainsKey(container.Name))
                            {
                                this.readAuthorizationMethods[container.Name] = new List<MethodInfo>();
                            }

                            this.readAuthorizationMethods[container.Name].Add(method);
                        }
                    }

                    ChangeInterceptorAttribute[] changeAttributes = (ChangeInterceptorAttribute[])
                        method.GetCustomAttributes(typeof(ChangeInterceptorAttribute), true /* inherit */);
                    foreach (ChangeInterceptorAttribute attribute in changeAttributes)
                    {
                        ResourceSet container;
                        if (!this.provider.TryResolveResourceSet(attribute.EntitySetName, out container) || container == null)
                        {
                            string message = Strings.DataService_AttributeEntitySetNotFound(
                                attribute.EntitySetName,
                                method.Name,
                                type.FullName);
                            throw new InvalidOperationException(message);
                        }

                        // Check the signature.
                        ParameterInfo[] parameters = method.GetParameters();
                        if (parameters.Length != 2)
                        {
                            string message = Strings.DataService_ChangeInterceptorIncorrectParameterCount(
                                method.Name,
                                type.FullName,
                                parameters.Length);
                            throw new InvalidOperationException(message);
                        }

                        CheckParameterIsNotOut(method, parameters[0]);
                        CheckParameterIsNotOut(method, parameters[1]);
                        Type elementParameterType = parameters[0].ParameterType;
                        if (!elementParameterType.IsAssignableFrom(container.ResourceType.InstanceType))
                        {
                            string message = Strings.DataService_AuthorizationParameterNotAssignable(
                                parameters[0].Name,
                                method.Name,
                                type.FullName,
                                elementParameterType.FullName,
                                container.ResourceType.InstanceType);
                            throw new InvalidOperationException(message);
                        }

                        Type actionParameterType = parameters[1].ParameterType;
                        if (actionParameterType != typeof(UpdateOperations))
                        {
                            string message = Strings.DataService_AuthorizationParameterNotResourceAction(
                                parameters[1].Name,
                                method.Name,
                                type.FullName,
                                typeof(UpdateOperations).FullName);
                            throw new InvalidOperationException(message);
                        }

                        Type returnType = method.ReturnType;
                        if (returnType != typeof(void))
                        {
                            string message = Strings.DataService_AuthorizationMethodNotVoid(
                                method.Name,
                                type.FullName,
                                returnType.FullName);
                            throw new InvalidOperationException(message);
                        }

                        if (!method.IsAbstract)
                        {
                            if (!this.writeAuthorizationMethods.ContainsKey(container.Name))
                            {
                                this.writeAuthorizationMethods[container.Name] = new List<MethodInfo>();
                            }

                            this.writeAuthorizationMethods[container.Name].Add(method);
                        }
                    }
                }

                type = type.BaseType;
            }
        }

        /// <summary>
        /// Checks that the specified <paramref name="value"/> for the named property is not negative and that the
        /// configuration isn't sealed.
        /// </summary>
        /// <param name="value">Value to check.</param>
        /// <param name="propertyName">Parameter name.</param>
        /// <returns>The <paramref name="value"/> to set.</returns>
        /// <remarks>
        /// This method is typically used in properties with the following pattern:
        /// <code>public int Foo { get {... } set { this.foo = this.CheckNonNegativeProperty(value, "Foo"); } }</code>
        /// </remarks>
        private int CheckNonNegativeProperty(int value, string propertyName)
        {
            this.CheckNotSealed();
            if (value < 0)
            {
                throw new ArgumentOutOfRangeException("value", value, Strings.PropertyRequiresNonNegativeNumber(propertyName));
            }

            return value;
        }

        /// <summary>Checks that this configuration hasn't been sealed yet.</summary>
        private void CheckNotSealed()
        {
            if (this.configurationSealed)
            {
                string message = Strings.DataServiceConfiguration_NoChangesAllowed(XmlConstants.ClrServiceInitializationMethodName);
                throw new InvalidOperationException(message);
            }
        }

        /// <summary>
        /// Read the flag value from the config file
        /// </summary>
        private void ReadFromConfig()
        {
            var featuresSection = (DataServicesFeaturesSection)ConfigurationManager.GetSection("wcfDataServices/features");

            // If the element is specified in the config section, then value in the config
            // wins over the api one. Hence overriding the api value if the value is specified
            // in the config file.
            if (featuresSection != null && featuresSection.ReplaceFunction.IsPresent)
            {
                this.dataServiceBehavior.AcceptReplaceFunctionInQuery = featuresSection.ReplaceFunction.Enable;
            }
        }

        #endregion Private methods.
    }
}
