//---------------------------------------------------------------------
// <copyright file="DataService.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a base class for DataWeb services.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Data.Objects;
    using System.Data.Services.Caching;
    using System.Data.Services.Providers;
    using System.Data.Services.Serializers;
    using System.Data.Services.Common;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.ServiceModel;
    using System.ServiceModel.Activation;
    using System.ServiceModel.Channels;
#if ASTORIA_FF_CALLBACKS    
    using System.ServiceModel.Syndication;
#endif    
    using System.Text;

    #endregion Namespaces.

    /// <summary>
    /// Represents a strongly typed service that can process data-oriented 
    /// resource requests.
    /// </summary>
    /// <typeparam name="T">The type of the store to provide resources.</typeparam>
    /// <remarks>
    /// <typeparamref name="T"/> will typically be a subtype of 
    /// <see cref="System.Data.Objects.ObjectContext" /> or another class that provides <see cref="IQueryable" />
    /// properties.
    /// </remarks>
    [ServiceBehavior(InstanceContextMode = InstanceContextMode.PerCall)]
    [AspNetCompatibilityRequirements(RequirementsMode = AspNetCompatibilityRequirementsMode.Allowed)]
    public class DataService<T> : IRequestHandler, IDataService
    {
        #region Private fields.

        /// <summary>A delegate used to create an instance of the data context.</summary>
        private static Func<T> cachedConstructor;

        /// <summary>Service configuration information.</summary>
        private DataServiceConfiguration configuration;

        /// <summary>Data provider for this data service.</summary>
        private DataServiceProviderWrapper provider;

        /// <summary>IUpdatable interface for this datasource's provider</summary>
        private UpdatableWrapper updatable;

        /// <summary>Custom paging provider interface exposed by the service.</summary>
        private DataServicePagingProviderWrapper pagingProvider;

        /// <summary>Context for the current operation.</summary>
        private DataServiceOperationContext operationContext;

        /// <summary>Reference to IDataServiceStreamProvider interface.</summary>
        private DataServiceStreamProviderWrapper streamProvider;

        /// <summary>Events for the data service processing pipeline.</summary>
        private DataServiceProcessingPipeline processingPipeline = new DataServiceProcessingPipeline();

#pragma warning disable 0169, 0649
        /// <summary>Test hook which gets called once a query is constructed right before its execution.</summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1823", Justification = "Test hook, nevers et by product code, only consumed by it.")]
        private Action<IQueryable> requestQueryableConstructed;
#pragma warning restore 0169, 0649

        #endregion Private fields.

        #region Properties.

        /// <summary>Events for the data service processing pipeline.</summary>
        public DataServiceProcessingPipeline ProcessingPipeline
        {
            [DebuggerStepThrough]
            get { return this.processingPipeline; }
        }

        /// <summary>Service configuration information.</summary>
        DataServiceConfiguration IDataService.Configuration
        {
            [DebuggerStepThrough]
            get { return this.configuration; }
        }

        /// <summary>Data provider for this data service</summary>
        DataServiceProviderWrapper IDataService.Provider
        {
            [DebuggerStepThrough]
            get
            {
                return this.provider;
            }
        }

        /// <summary>Paging provider for this data service.</summary>
        DataServicePagingProviderWrapper IDataService.PagingProvider
        {
            [DebuggerStepThrough]
            get
            {
                Debug.Assert(this.provider != null, "this.provider != null");
                if (this.pagingProvider == null)
                {
                    this.pagingProvider = new DataServicePagingProviderWrapper(this);
                }

                return this.pagingProvider;
            }
        }

        /// <summary>Returns the instance of data service.</summary>
        object IDataService.Instance
        {
            [DebuggerStepThrough]
            get { return this; }
        }

        /// <summary>Cached request headers.</summary>
        DataServiceOperationContext IDataService.OperationContext
        {
            [DebuggerStepThrough]
            get { return this.operationContext; }
        }

        /// <summary>Processing pipeline events</summary>
        DataServiceProcessingPipeline IDataService.ProcessingPipeline
        {
            [DebuggerStepThrough]
            get { return this.processingPipeline; }
        }

        /// <summary>IUpdatable interface for this provider</summary>
        UpdatableWrapper IDataService.Updatable
        {
            [DebuggerStepThrough]
            get
            {
                Debug.Assert(this.provider != null, "this.provider != null");
                return this.updatable;
            }
        }

        /// <summary>Reference to IDataServiceStreamProvider interface.</summary>
        DataServiceStreamProviderWrapper IDataService.StreamProvider
        {
            [DebuggerStepThrough]
            get
            {
                Debug.Assert(this.provider != null, "this.provider != null");
                return this.streamProvider ?? (this.streamProvider = new DataServiceStreamProviderWrapper(this));
            }
        }  

        /// <summary>The data source used in the current request processing.</summary>
        protected T CurrentDataSource
        {
            get { return (T)this.provider.CurrentDataSource; }
        }

        #endregion Properties.

        #region Public / interface methods.

        /// <summary>
        /// This method is called during query processing to validate and customize 
        /// paths for the $expand options are applied by the provider.
        /// </summary>
        /// <param name='queryable'>Query which will be composed.</param>
        /// <param name='expandPaths'>Collection of segment paths to be expanded.</param>
        void IDataService.InternalApplyingExpansions(IQueryable queryable, ICollection<ExpandSegmentCollection> expandPaths)
        {
            Debug.Assert(queryable != null, "queryable != null");
            Debug.Assert(expandPaths != null, "expandPaths != null");
            Debug.Assert(this.configuration != null, "this.configuration != null");

            // Check the expand depth and count.
            int actualExpandDepth = 0;
            int actualExpandCount = 0;
            foreach (ExpandSegmentCollection collection in expandPaths)
            {
                int segmentDepth = collection.Count;
                if (segmentDepth > actualExpandDepth)
                {
                    actualExpandDepth = segmentDepth;
                }

                actualExpandCount += segmentDepth;
            }

            if (this.configuration.MaxExpandDepth < actualExpandDepth)
            {
                throw DataServiceException.CreateBadRequestError(Strings.DataService_ExpandDepthExceeded(actualExpandDepth, this.configuration.MaxExpandDepth));
            }

            if (this.configuration.MaxExpandCount < actualExpandCount)
            {
                throw DataServiceException.CreateBadRequestError(Strings.DataService_ExpandCountExceeded(actualExpandCount, this.configuration.MaxExpandCount));
            }
        }

        /// <summary>Processes a catchable exception.</summary>
        /// <param name="args">The arguments describing how to handle the exception.</param>
        void IDataService.InternalHandleException(HandleExceptionArgs args)
        {
            Debug.Assert(args != null, "args != null");
            try
            {
                this.HandleException(args);
            }
            catch (Exception handlingException)
            {
#if DEBUG
                // We've hit an error, some of the processing pipeline events will not get fired. Setting this flag to skip validation.
                this.ProcessingPipeline.SkipDebugAssert = true;
#endif
                if (!WebUtil.IsCatchableExceptionType(handlingException))
                {
                    throw;
                }

                args.Exception = handlingException;
            }
        }

#if ASTORIA_FF_CALLBACKS
        /// <summary>
        /// Invoked once feed has been written to override the feed elements
        /// </summary>
        /// <param name="feed">Feed being written</param>
        void IDataService.InternalOnWriteFeed(SyndicationFeed feed)
        {
            this.OnWriteFeed(feed);
        }

        /// <summary>
        /// Invoked once an element has been written to override the element
        /// </summary>
        /// <param name="item">Item that has been written</param>
        /// <param name="obj">Object with content for the <paramref name="item"/></param>
        void IDataService.InternalOnWriteItem(SyndicationItem item, object obj)
        {
            this.OnWriteItem(item, obj);
        }
#endif
        /// <summary>
        /// Returns the segmentInfo of the resource referred by the given content Id;
        /// </summary>
        /// <param name="contentId">content id for a operation in the batch request.</param>
        /// <returns>segmentInfo for the resource referred by the given content id.</returns>
        SegmentInfo IDataService.GetSegmentForContentId(string contentId)
        {
            return null;
        }

        /// <summary>
        /// Get the resource referred by the segment in the request with the given index
        /// </summary>
        /// <param name="description">description about the request url.</param>
        /// <param name="segmentIndex">index of the segment that refers to the resource that needs to be returned.</param>
        /// <param name="typeFullName">typename of the resource.</param>
        /// <returns>the resource as returned by the provider.</returns>
        object IDataService.GetResource(RequestDescription description, int segmentIndex, string typeFullName)
        {
            Debug.Assert(description.SegmentInfos[segmentIndex].RequestEnumerable != null, "requestDescription.SegmentInfos[segmentIndex].RequestEnumerable != null");
            return Deserializer.GetResource(description.SegmentInfos[segmentIndex], typeFullName, ((IDataService)this), false /*checkForNull*/);
        }

        /// <summary>Disposes the data source of the current <see cref="provider"/> if necessary.</summary>
        /// <remarks>
        /// Because the provider has affinity with a specific data source
        /// (which is created and set by the DataService), we set
        /// the provider to null so we remember to re-create it if the
        /// service gets reused for a different request.
        /// </remarks>
        void IDataService.DisposeDataSource()
        {
#if DEBUG
            this.processingPipeline.AssertDebugStateAtDispose();
            this.processingPipeline.HasDisposedProviderInterfaces = true;
#endif
            if (this.updatable != null)
            {
                this.updatable.DisposeProvider();
                this.updatable = null;
            }

            if (this.streamProvider != null)
            {
                this.streamProvider.DisposeProvider();
                this.streamProvider = null;
            }

            if (this.pagingProvider != null)
            {
                this.pagingProvider.DisposeProvider();
                this.pagingProvider = null;
            }

            if (this.provider != null)
            {
                this.provider.DisposeDataSource();
                this.provider = null;
            }
        }

        /// <summary>
        /// This method is called before a request is processed.
        /// </summary>
        /// <param name="args">Information about the request that is going to be processed.</param>
        void IDataService.InternalOnStartProcessingRequest(ProcessRequestArgs args)
        {
#if DEBUG
            this.processingPipeline.AssertDebugStateAtOnStartProcessingRequest();
            this.processingPipeline.OnStartProcessingRequestInvokeCount++;
#endif
            this.OnStartProcessingRequest(args);
        }

        /// <summary>
        /// This method is called once the request query is constructed.
        /// </summary>
        /// <param name="query">The query which is going to be executed against the provider.</param>
        void IDataService.InternalOnRequestQueryConstructed(IQueryable query)
        {
            // Call the test hook with the query
            if (this.requestQueryableConstructed != null)
            {
                this.requestQueryableConstructed(query);
            }
        }

        /// <summary>Attaches the specified host to this service.</summary>
        /// <param name="host">Host for service to interact with.</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Maintainability", "CA1500:VariableNamesShouldNotMatchFieldNames", MessageId = "host", Justification = "Makes 1:1 argument-to-field correspondence obvious.")]
        public void AttachHost(IDataServiceHost host)
        {
            WebUtil.CheckArgumentNull(host, "host");
            this.operationContext = new DataServiceOperationContext(host);
        }

        /// <summary>Processes the specified <paramref name="messageBody"/>.</summary>
        /// <param name="messageBody"><see cref="Stream"/> with message body to process.</param>
        /// <returns>The response <see cref="Message"/>.</returns>
        public Message ProcessRequestForMessage(Stream messageBody)
        {
            WebUtil.CheckArgumentNull(messageBody, "messageBody");

            HttpContextServiceHost httpHost = new HttpContextServiceHost(messageBody);
            this.AttachHost(httpHost);

            bool shouldDispose = true;
            try
            {
                Action<Stream> writer = this.HandleRequest();
                Debug.Assert(writer != null, "writer != null");
                Message result = CreateMessage(MessageVersion.None, "", ((IDataServiceHost)httpHost).ResponseContentType, writer, this);

                // If SuppressEntityBody is false, WCF will call DelegateBodyWriter.OnWriteBodyContent(), which
                // will dispose the data source and stream provider.  Otherwise we need to dispose them in the
                // finally block below.
                if (!System.ServiceModel.Web.WebOperationContext.Current.OutgoingResponse.SuppressEntityBody)
                {
                    shouldDispose = false;
                }

                return result;
            }
#if DEBUG
            catch
            {
                this.processingPipeline.SkipDebugAssert = true;
                throw;
            }
#endif
            finally
            {
                if (shouldDispose)
                {
                    ((IDataService)this).DisposeDataSource();
                }
            }
        }

        /// <summary>Provides a host-agnostic entry point for request processing.</summary>
        public void ProcessRequest()
        {
            if (this.operationContext == null)
            {
                throw new InvalidOperationException(Strings.DataService_HostNotAttached);
            }

            try
            {
                Action<Stream> writer = this.HandleRequest();
                if (writer != null)
                {
                    writer(this.operationContext.Host.ResponseStream);
                }
            }
#if DEBUG
            catch
            {
                this.processingPipeline.SkipDebugAssert = true;
                throw;
            }
#endif
            finally
            {
                ((IDataService)this).DisposeDataSource();
#if DEBUG
                // Need to reset the states since the caller can reuse the same service instance.
                this.processingPipeline.ResetDebugState();
#endif
            }
        }

        #endregion Public / interface methods.

        #region Protected methods.

        /// <summary>Initializes a new data source instance.</summary>
        /// <returns>A new data source instance.</returns>
        /// <remarks>
        /// The default implementation uses a constructor with no parameters
        /// to create a new instance.
        /// 
        /// The instance will only be used for the duration of a single
        /// request, and will be disposed after the request has been
        /// handled.
        /// </remarks>
        protected virtual T CreateDataSource()
        {
            if (cachedConstructor == null)
            {
                Type dataContextType = typeof(T);
                if (dataContextType.IsAbstract)
                {
                    throw new InvalidOperationException(
                        Strings.DataService_ContextTypeIsAbstract(dataContextType, this.GetType()));
                }

                cachedConstructor = (Func<T>)WebUtil.CreateNewInstanceConstructor(dataContextType, null, dataContextType);
            }

            return cachedConstructor();
        }

        /// <summary>Handles an exception thrown while processing a request.</summary>
        /// <param name="args">Arguments to the exception.</param>
        protected virtual void HandleException(HandleExceptionArgs args)
        {
            WebUtil.CheckArgumentNull(args, "arg");
            Debug.Assert(args.Exception != null, "args.Exception != null -- .ctor should have checked");
#if DEBUG
            this.processingPipeline.SkipDebugAssert = true;
#endif
        }

        /// <summary>
        /// This method is called before processing each request. For batch requests
        /// it is called once for the top batch request and once for each operation
        /// in the batch.
        /// </summary>
        /// <param name="args">args containing information about the request.</param>
        protected virtual void OnStartProcessingRequest(ProcessRequestArgs args)
        {
            // Do nothing. Application writers can override this and look
            // at the request args and do some processing.
        }

#if ASTORIA_FF_CALLBACKS
        /// <summary>
        /// Invoked once feed has been written to override the feed elements
        /// </summary>
        /// <param name="feed">Feed being written</param>
        protected virtual void OnWriteFeed(SyndicationFeed feed)
        {
        }

        /// <summary>
        /// Invoked once an element has been written to override the element
        /// </summary>
        /// <param name="item">Item that has been written</param>
        /// <param name="obj">Object with content for the <paramref name="item"/></param>
        protected virtual void OnWriteItem(SyndicationItem item, object obj)
        {
        }
#endif
        #endregion Protected methods.

        #region Private methods.

        /// <summary>
        /// Checks that if etag values are specified in the header, they must be valid.
        /// </summary>
        /// <param name="host">header values.</param>
        /// <param name="description">request description.</param>
        private static void CheckETagValues(DataServiceHostWrapper host, RequestDescription description)
        {
            Debug.Assert(host != null, "host != null");

            // Media Resource ETags can be strong
            bool allowStrongEtag = description.TargetKind == RequestTargetKind.MediaResource;

            if (!WebUtil.IsETagValueValid(host.RequestIfMatch, allowStrongEtag))
            {
                throw DataServiceException.CreateBadRequestError(Strings.DataService_ETagValueNotValid(host.RequestIfMatch));
            }

            if (!WebUtil.IsETagValueValid(host.RequestIfNoneMatch, allowStrongEtag))
            {
                throw DataServiceException.CreateBadRequestError(Strings.DataService_ETagValueNotValid(host.RequestIfNoneMatch));
            }
        }

        /// <summary>
        /// Creates a <see cref="Message"/> that invokes the specified 
        /// <paramref name="writer"/> callback to write its body.
        /// </summary>
        /// <param name="version">Version for message.</param>
        /// <param name="action">Action for message.</param>
        /// <param name="contentType">MIME content type for body.</param>
        /// <param name="writer">Callback.</param>
        /// <param name="service">Service with context to dispose once the response has been written.</param>
        /// <returns>A new <see cref="Message"/>.</returns>
        private static Message CreateMessage(MessageVersion version, string action, string contentType, Action<Stream> writer, IDataService service)
        {
            Debug.Assert(version != null, "version != null");
            Debug.Assert(writer != null, "writer != null");
            Debug.Assert(service != null, "service != null");

            DelegateBodyWriter bodyWriter = new DelegateBodyWriter(writer, service);

            Message message = Message.CreateMessage(version, action, bodyWriter);
            message.Properties.Add(WebBodyFormatMessageProperty.Name, new WebBodyFormatMessageProperty(WebContentFormat.Raw));

            HttpResponseMessageProperty response = new HttpResponseMessageProperty();
            response.Headers[System.Net.HttpResponseHeader.ContentType] = contentType;
            message.Properties.Add(HttpResponseMessageProperty.Name, response);

            return message;
        }

        /// <summary>
        /// Creates a new data service configuration instance
        /// </summary>
        /// <param name="dataServiceType">data service type</param>
        /// <param name="provider">provider instance</param>
        /// <returns>data service configuration instance</returns>
        private static DataServiceConfiguration CreateConfiguration(Type dataServiceType, IDataServiceMetadataProvider provider)
        {
            Debug.Assert(dataServiceType != null, "dataServiceType != null");
            Debug.Assert(provider != null, "provider != null");

            DataServiceConfiguration configuration = new DataServiceConfiguration(provider);
            configuration.Initialize(dataServiceType);

            if (!(provider is BaseServiceProvider) && configuration.GetKnownTypes().Any())
            {
                throw new InvalidOperationException(Strings.DataService_RegisterKnownTypeNotAllowedForIDSP);
            }

            configuration.Seal();
            return configuration;
        }
        
        /// <summary>
        /// Gets the appropriate encoding specified by the request, taking 
        /// the format into consideration.
        /// </summary>
        /// <param name="responseFormat">Content format for response.</param>
        /// <param name="acceptCharset">Accept-Charset header as specified in request.</param>
        /// <returns>The requested encoding, possibly null.</returns>
        private static Encoding GetRequestAcceptEncoding(ContentFormat responseFormat, string acceptCharset)
        {
            if (responseFormat == ContentFormat.Binary)
            {
                return null;
            }
            else
            {
                return HttpProcessUtility.EncodingFromAcceptCharset(acceptCharset);
            }
        }

        /// <summary>
        /// Selects a response format for the host's request and sets the
        /// appropriate response header.
        /// </summary>
        /// <param name="host">Host with request.</param>
        /// <param name="acceptTypesText">An comma-delimited list of client-supported MIME accept types.</param>
        /// <param name="entityTarget">Whether the target is an entity.</param>
        /// <returns>The selected response format.</returns>
        private static ContentFormat SelectResponseFormat(DataServiceHostWrapper host, string acceptTypesText, bool entityTarget)
        {
            Debug.Assert(host != null, "host != null");

            string[] availableTypes;
            if (entityTarget)
            {
                availableTypes = new string[]
                { 
                    XmlConstants.MimeApplicationAtom, 
                    XmlConstants.MimeApplicationJson
                };
            }
            else
            {
                availableTypes = new string[]
                { 
                    XmlConstants.MimeApplicationXml, 
                    XmlConstants.MimeTextXml, 
                    XmlConstants.MimeApplicationJson
                };
            }

            string mime = HttpProcessUtility.SelectMimeType(acceptTypesText, availableTypes);
            if (mime == null)
            {
                return ContentFormat.Unsupported;
            }
            else
            {
                host.ResponseContentType = mime;
                return GetContentFormat(mime);
            }
        }

        /// <summary>Validate the given request.</summary>
        /// <param name="operationContext">Context for current operation.</param>
        private static void ValidateRequest(DataServiceOperationContext operationContext)
        {
            if (!String.IsNullOrEmpty(operationContext.Host.RequestIfMatch) && !String.IsNullOrEmpty(operationContext.Host.RequestIfNoneMatch))
            {
                throw DataServiceException.CreateBadRequestError(Strings.DataService_BothIfMatchAndIfNoneMatchHeaderSpecified);
            }
        }

        /// <summary>
        /// Raises the response version header if necessary for the $metadata path.
        /// WARNING!!! This property can only be called for the $metadata path because it enumerates through all resource types.
        /// Calling it from outside of the $metadata path would break our IDSP contract.
        /// </summary>
        /// <param name="description">description about the request uri</param>
        /// <param name="dataService">data service to which the request was made</param>
        private static void RaiseResponseVersionForMetadata(RequestDescription description, IDataService dataService)
        {
            Debug.Assert(description.TargetKind == RequestTargetKind.Metadata, "This method can only be called from the $metadata path because it enumerates through all resource types.");

            if (dataService.Provider.IsV1Provider)
            {
                if (!dataService.Provider.GetEpmCompatiblityForV1Provider())
                {
                    description.RaiseResponseVersion(2, 0);
                }
            }
            else
            {
                foreach (ResourceType rt in dataService.Provider.Types)
                {
                    if (!rt.EpmIsV1Compatible)
                    {
                        description.RaiseResponseVersion(2, 0);
                        break;
                    }
                }
            }
        }

        /// <summary>
        /// Processes the incoming request, without writing anything to the response body.
        /// </summary>
        /// <param name="description">description about the request uri</param>
        /// <param name="dataService">data service to which the request was made.</param>
        /// <returns>
        /// A delegate to be called to write the body; null if no body should be written out.
        /// </returns>
        private static RequestDescription ProcessIncomingRequest(
            RequestDescription description,
            IDataService dataService)
        {
            Debug.Assert(description != null, "description != null");
            Debug.Assert(dataService.OperationContext.Host != null, "dataService.OperationContext.Host != null");

            DataServiceHostWrapper host = dataService.OperationContext.Host;

            // Make a decision about metadata response version
            if (description.TargetKind == RequestTargetKind.Metadata)
            {
                RaiseResponseVersionForMetadata(description, dataService);
            }

            dataService.Configuration.ValidateMaxProtocolVersion(description);

            WebUtil.CheckVersion(dataService, description);
            CheckETagValues(host, description);

            ResourceSetWrapper lastSegmentContainer = description.LastSegmentInfo.TargetContainer;
            if (host.AstoriaHttpVerb == AstoriaVerbs.GET)
            {
                // This if expression was missing from V1.0, but is a breaking change to add it
                // without also checking for the new OverrideEntitySetRights
                if (description.LastSegmentInfo.Operation != null &&
                    (0 != (dataService.Configuration.GetServiceOperationRights(description.LastSegmentInfo.Operation.ServiceOperation) & ServiceOperationRights.OverrideEntitySetRights)))
                {
                    DataServiceConfiguration.CheckServiceRights(description.LastSegmentInfo.Operation, description.IsSingleResult);
                }
                else
                {
                    // For $count, the rights is already checked in the RequestUriProcessor and hence we don't need to check here.
                    // Also, checking for ReadSingle right is wrong, since we need to only check for ReadMultiple rights.
                    if (lastSegmentContainer != null && description.LastSegmentInfo.Identifier != XmlConstants.UriCountSegment)
                    {
                        DataServiceConfiguration.CheckResourceRightsForRead(lastSegmentContainer, description.IsSingleResult);
                    }
                }
            }
            else if (description.TargetKind == RequestTargetKind.ServiceDirectory)
            {
                throw DataServiceException.CreateMethodNotAllowed(
                    Strings.DataService_OnlyGetOperationSupportedOnServiceUrl,
                    XmlConstants.HttpMethodGet);
            }

            int statusCode = 200;
            bool shouldWriteBody = true;
            RequestDescription newDescription = description;
            if (description.TargetSource != RequestTargetSource.ServiceOperation)
            {
                if (host.AstoriaHttpVerb == AstoriaVerbs.POST)
                {
                    newDescription = HandlePostOperation(description, dataService);
                    if (description.LinkUri)
                    {
                        statusCode = 204;   // 204 - No Content
                        shouldWriteBody = false;
                    }
                    else
                    {
                        statusCode = 201;   // 201 - Created.
                    }
                }
                else if (host.AstoriaHttpVerb == AstoriaVerbs.PUT ||
                         host.AstoriaHttpVerb == AstoriaVerbs.MERGE)
                {
                    if (lastSegmentContainer != null && !description.LinkUri)
                    {
                        if (host.AstoriaHttpVerb == AstoriaVerbs.PUT)
                        {
                            DataServiceConfiguration.CheckResourceRights(lastSegmentContainer, EntitySetRights.WriteReplace);
                        }
                        else
                        {
                            DataServiceConfiguration.CheckResourceRights(lastSegmentContainer, EntitySetRights.WriteMerge);
                        }
                    }

                    // For PUT, the body itself shouldn't be written, but the etag should (unless it's just a link).
                    shouldWriteBody = !description.LinkUri;

                    newDescription = HandlePutOperation(description, dataService);
                    Debug.Assert(description.ResponseVersion == RequestDescription.DataServiceDefaultResponseVersion, "description.ResponseVersion == RequestDescription.DataServiceDefaultResponseVersion");

                    statusCode = 204;   // 204 - No Content
                }
                else if (host.AstoriaHttpVerb == AstoriaVerbs.DELETE)
                {
                    if (lastSegmentContainer != null && !description.LinkUri)
                    {
                        DataServiceConfiguration.CheckResourceRights(lastSegmentContainer, EntitySetRights.WriteDelete);
                    }

                    HandleDeleteOperation(description, dataService);
                    Debug.Assert(description.RequireMinimumVersion == new Version(1, 0), "description.RequireMinimumVersion == new Version(1, 0)");
                    Debug.Assert(description.ResponseVersion == RequestDescription.DataServiceDefaultResponseVersion, "description.ResponseVersion == RequestDescription.DataServiceDefaultResponseVersion");

                    statusCode = 204;   // 204 - No Content
                    shouldWriteBody = false;
                }
            }
            else if (description.TargetKind == RequestTargetKind.VoidServiceOperation)
            {
                statusCode = 204; // No Content
                shouldWriteBody = false;
            }

            // Set the caching policy appropriately - for the time being, we disable caching.
            host.ResponseCacheControl = XmlConstants.HttpCacheControlNoCache;

            // Always set the version when a payload will be returned, in case other
            // headers include links, which may need to be interpreted under version-specific rules.
            Debug.Assert(description.ResponseVersion == newDescription.ResponseVersion, "description.ResponseVersion == newDescription.ResponseVersion");
            host.ResponseVersion = newDescription.ResponseVersion.ToString() + ";";

            host.ResponseStatusCode = statusCode;

            if (shouldWriteBody)
            {
                // return the description, only if response or something in the response header needs to be written
                // for e.g. in PUT operations, we need to write etag to the response header, and
                // we can compute the new etag only after we have called save changes.
                return newDescription;
            }
            else
            {
                return null;
            }
        }

        /// <summary>Serializes the results for a request into the body of a response message.</summary>
        /// <param name='description'>Description of the data requested.</param>
        /// <param name="dataService">data service to which the request was made.</param>
        /// <returns>A delegate that can serialize the body into an IEnumerable.</returns>
        private static Action<Stream> SerializeResponseBody(RequestDescription description, IDataService dataService)
        {
            Debug.Assert(dataService.Provider != null, "dataService.Provider != null");
            Debug.Assert(dataService.OperationContext.Host != null, "dataService.OperationContext.Host != null");

            DataServiceHostWrapper host = dataService.OperationContext.Host;

            // Handle internal system resources.
            Action<Stream> result = HandleInternalResources(description, dataService);
            if (result != null)
            {
                return result;
            }

            // ETags are not supported if there are more than one resource expected in the response.
            if (!RequestDescription.IsETagHeaderAllowed(description))
            {
                if (!String.IsNullOrEmpty(host.RequestIfMatch) || !String.IsNullOrEmpty(host.RequestIfNoneMatch))
                {
                    throw DataServiceException.CreateBadRequestError(Strings.DataService_ETagCannotBeSpecified(host.AbsoluteRequestUri));
                }
            }

            if (host.AstoriaHttpVerb == AstoriaVerbs.PUT ||
                host.AstoriaHttpVerb == AstoriaVerbs.MERGE)
            {
                ResourceSetWrapper container;
                object actualEntity = GetContainerAndActualEntityInstance(dataService, description, out container);

                // We should only write etag in the response, if the type has one or more etag properties defined.
                // WriteETagValueInResponseHeader checks for null etag value (which means that no etag properties are defined)
                // that before calling the host.
                string etag;
                if (description.TargetKind == RequestTargetKind.MediaResource)
                {
                    etag = dataService.StreamProvider.GetStreamETag(actualEntity, dataService.OperationContext);
                }
                else
                {
                    etag = WebUtil.GetETagValue(dataService, actualEntity, container);
                }

#if DEBUG
                WebUtil.WriteETagValueInResponseHeader(description, etag, host);
#else
                WebUtil.WriteETagValueInResponseHeader(etag, host);
#endif
                return WebUtil.GetEmptyStreamWriter();
            }

            // Pick the content format to be used to serialize the body.
            Debug.Assert(description.RequestEnumerable != null, "description.RequestEnumerable != null");
            ContentFormat responseFormat = SelectResponseFormatForType(
                description.LinkUri ? RequestTargetKind.Link : description.TargetKind,
                description.TargetResourceType,
                host.RequestAccept,
                description.MimeType,
                dataService);

            // This is the code path for service operations and GET requests returning multiple results
            if (description.TargetSource == RequestTargetSource.ServiceOperation ||
                description.TargetSource == RequestTargetSource.None ||
                !description.IsSingleResult)
            {
                // For service operations returning single result, etag checks must be performed by the service operation itself.
                Debug.Assert(
                    (String.IsNullOrEmpty(host.RequestIfMatch) && String.IsNullOrEmpty(host.RequestIfNoneMatch)) || description.TargetSource == RequestTargetSource.ServiceOperation,
                    "No etag can be specified for collection or it must be a service operation");

                Encoding encoding = GetRequestAcceptEncoding(responseFormat, host.RequestAcceptCharSet);
                IEnumerator queryResults = WebUtil.GetRequestEnumerator(description.RequestEnumerable);

                try
                {
                    bool hasMoved = queryResults.MoveNext();

                    if (description.IsSingleResult)
                    {
                        if (!hasMoved || queryResults.Current == null)
                        {
                            throw DataServiceException.CreateResourceNotFound(description.LastSegmentInfo.Identifier);
                        }
                    }

                    // If we had to wait until we got a value to determine the valid contents, try that now.
                    if (responseFormat == ContentFormat.Unknown)
                    {
                        responseFormat = ResolveUnknownFormat(description, queryResults.Current, dataService);
                    }

                    Debug.Assert(responseFormat != ContentFormat.Unknown, "responseFormat != ContentFormat.Unknown");
                    host.ResponseContentType = HttpProcessUtility.BuildContentType(host.ResponseContentType, encoding);
                    return new ResponseBodyWriter(encoding, hasMoved, dataService, queryResults, description, responseFormat).Write;
                }
                catch
                {
                    WebUtil.Dispose(queryResults);
                    throw;
                }
            }
            else
            {
                return CompareETagAndWriteResponse(description, responseFormat, dataService);
            }
        }

        /// <summary>Selects the correct content format for a given resource type.</summary>
        /// <param name="targetKind">Target resource to return.</param>
        /// <param name="resourceType">resource type.</param>
        /// <param name="acceptTypesText">Accept header value.</param>
        /// <param name="mimeType">Required MIME type.</param>
        /// <param name="service">Data service.</param>
        /// <returns>
        /// The content format for the resource; Unknown if it cannot be determined statically.
        /// </returns>
        private static ContentFormat SelectResponseFormatForType(
            RequestTargetKind targetKind,
            ResourceType resourceType,
            string acceptTypesText,
            string mimeType,
            IDataService service)
        {
            ContentFormat responseFormat;
            if (targetKind == RequestTargetKind.PrimitiveValue)
            {
                responseFormat = SelectPrimitiveContentType(resourceType, acceptTypesText, mimeType, service.OperationContext.Host);
            }
            else if (targetKind == RequestTargetKind.MediaResource)
            {
                // We need the MLE instance to get the response format for the MediaResource.
                // We will resolve the response format in ResolveUnknownFormat() where we have the MLE instance.
                responseFormat = ContentFormat.Unknown;
            }
            else if (targetKind != RequestTargetKind.OpenPropertyValue)
            {
                bool entityTarget = targetKind == RequestTargetKind.Resource;
                responseFormat = SelectResponseFormat(service.OperationContext.Host, acceptTypesText, entityTarget);
                if (responseFormat == ContentFormat.Unsupported)
                {
                    throw new DataServiceException(415, Strings.DataServiceException_UnsupportedMediaType);
                }
            }
            else
            {
                // We cannot negotiate a format until we know what the value is for the object.
                responseFormat = ContentFormat.Unknown;
            }

            return responseFormat;
        }

        /// <summary>Selects the correct content format for a primitive type.</summary>
        /// <param name="targetResourceType">resource type.</param>
        /// <param name="acceptTypesText">Accept header value.</param>
        /// <param name="requiredContentType">Required MIME type, possibly null.</param>
        /// <param name="host">Host implementation for this data service.</param>
        /// <returns>The content format for the resource.</returns>
        private static ContentFormat SelectPrimitiveContentType(ResourceType targetResourceType, string acceptTypesText, string requiredContentType, DataServiceHostWrapper host)
        {
            // Debug.Assert(
            //    targetResourceType != null && 
            //    targetResourceType.ResourceTypeKind == ResourceTypeKind.Primitive,
            //    "targetElementType != null && targetResourceType.ResourceTypeKind == ResourceTypeKind.Primitive");
            string contentType;
            ContentFormat responseFormat = WebUtil.GetResponseFormatForPrimitiveValue(targetResourceType, out contentType);
            requiredContentType = requiredContentType ?? contentType;
            host.ResponseContentType = HttpProcessUtility.SelectRequiredMimeType(
                acceptTypesText,        // acceptTypesText
                new string[] { requiredContentType },    // exactContentType
                requiredContentType);   // inexactContentType
            return responseFormat;
        }

        /// <summary>Selects the correct content format for a media resource.</summary>
        /// <param name="mediaLinkEntry">The media link entry.</param>
        /// <param name="acceptTypesText">Accept header value.</param>
        /// <param name="service">Data service instance.</param>
        /// <returns>The content format for the resource.</returns>
        private static ContentFormat SelectMediaResourceContentType(object mediaLinkEntry, string acceptTypesText, IDataService service)
        {
            Debug.Assert(mediaLinkEntry != null, "mediaLinkEntry != null");
            Debug.Assert(service != null, "service != null");

            string contentType = service.StreamProvider.GetStreamContentType(mediaLinkEntry, service.OperationContext);

            service.OperationContext.Host.ResponseContentType = HttpProcessUtility.SelectRequiredMimeType(
                acceptTypesText,                 // acceptTypesText
                new string[] { contentType },    // exactContentType
                contentType);                    // inexactContentType

            return ContentFormat.Binary;
        }

        /// <summary>Handles POST requests.</summary>
        /// <param name="description">description about the target request</param>
        /// <param name="dataService">data service to which the request was made.</param>
        /// <returns>a new request description object, containing information about the response payload</returns>
        private static RequestDescription HandlePostOperation(RequestDescription description, IDataService dataService)
        {
            Debug.Assert(
                description.TargetSource != RequestTargetSource.ServiceOperation, 
                "TargetSource != ServiceOperation -- should have been handled in request URI processing");

            DataServiceHostWrapper host = dataService.OperationContext.Host;
            if (!String.IsNullOrEmpty(host.RequestIfMatch) || !String.IsNullOrEmpty(host.RequestIfNoneMatch))
            {
                throw DataServiceException.CreateBadRequestError(Strings.DataService_ETagSpecifiedForPost);
            }

            if (description.IsSingleResult)
            {
                throw DataServiceException.CreateMethodNotAllowed(
                    Strings.BadRequest_InvalidUriForPostOperation(host.AbsoluteRequestUri),
                    DataServiceConfiguration.GetAllowedMethods(dataService.Configuration, description));
            }

            Debug.Assert(
                description.TargetSource == RequestTargetSource.EntitySet ||
                description.Property.Kind == ResourcePropertyKind.ResourceSetReference,
                "Only ways to have collections of resources");

            Stream requestStream = host.RequestStream;
            Debug.Assert(requestStream != null, "requestStream != null");
            object entity = null;

            ResourceType targetResourceType = description.TargetResourceType;
            Debug.Assert(targetResourceType != null, "targetResourceType != null");
            if (!description.LinkUri && dataService.Provider.HasDerivedTypes(targetResourceType) && WebUtil.HasMediaLinkEntryInHierarchy(targetResourceType, dataService.Provider))
            {
                ResourceSetWrapper targetResourceSet = description.LastSegmentInfo.TargetContainer;
                Debug.Assert(targetResourceSet != null, "targetResourceSet != null");
                targetResourceType = dataService.StreamProvider.ResolveType(targetResourceSet.Name, dataService);
                Debug.Assert(targetResourceType != null, "targetResourceType != null");
            }

            UpdateTracker tracker = UpdateTracker.CreateUpdateTracker(dataService);
            if (!description.LinkUri && targetResourceType.IsMediaLinkEntry)
            {
                // Verify that the user has rights to add to the target container
                Debug.Assert(description.LastSegmentInfo.TargetContainer != null, "description.LastSegmentInfo.TargetContainer != null");
                DataServiceConfiguration.CheckResourceRights(description.LastSegmentInfo.TargetContainer, EntitySetRights.WriteAppend);

                entity = Deserializer.CreateMediaLinkEntry(targetResourceType.FullName, requestStream, dataService, description, tracker);
                if (description.TargetSource == RequestTargetSource.Property)
                {
                    Debug.Assert(description.Property.Kind == ResourcePropertyKind.ResourceSetReference, "Expecting POST resource set property");
                    Deserializer.HandleBindOperation(description, entity, dataService, tracker);
                }
            }
            else
            {
                using (Deserializer deserializer = Deserializer.CreateDeserializer(description, dataService, false /*update*/, tracker))
                {
                    entity = deserializer.HandlePostRequest(description);
                    Debug.Assert(entity != null, "entity != null");
                }
            }

            tracker.FireNotifications();
            return RequestDescription.CreateSingleResultRequestDescription(
                description, entity, description.LastSegmentInfo.TargetContainer);
        }

        /// <summary>Handles PUT requests.</summary>
        /// <param name="description">description about the target request</param>
        /// <param name="dataService">data service to which the request was made.</param>
        /// <returns>new request description which contains the info about the entity resource getting modified.</returns>
        private static RequestDescription HandlePutOperation(RequestDescription description, IDataService dataService)
        {
            Debug.Assert(description.TargetSource != RequestTargetSource.ServiceOperation, "description.TargetSource != RequestTargetSource.ServiceOperation");
            DataServiceHostWrapper host = dataService.OperationContext.Host;

            if (!description.IsSingleResult)
            {
                throw DataServiceException.CreateMethodNotAllowed(
                    Strings.BadRequest_InvalidUriForPutOperation(host.AbsoluteRequestUri),
                    DataServiceConfiguration.GetAllowedMethods(dataService.Configuration, description));
            }
            else if (description.LinkUri && description.Property.Kind != ResourcePropertyKind.ResourceReference)
            {
                throw DataServiceException.CreateMethodNotAllowed(Strings.DataService_CannotUpdateSetReferenceLinks, XmlConstants.HttpMethodDelete);
            }

            // Note that for Media Resources, we let the Stream Provider decide whether or not to support If-None-Match for PUT
            if (!String.IsNullOrEmpty(host.RequestIfNoneMatch) && description.TargetKind != RequestTargetKind.MediaResource)
            {
                throw DataServiceException.CreateBadRequestError(Strings.DataService_IfNoneMatchHeaderNotSupportedInPut);
            }
            else if (!RequestDescription.IsETagHeaderAllowed(description) && !String.IsNullOrEmpty(host.RequestIfMatch))
            {
                throw DataServiceException.CreateBadRequestError(Strings.DataService_ETagCannotBeSpecified(host.AbsoluteRequestUri));
            }
            else if (description.Property != null && description.Property.IsOfKind(ResourcePropertyKind.Key))
            {
                throw DataServiceException.CreateBadRequestError(Strings.DataService_CannotUpdateKeyProperties(description.Property.Name));
            }

            Stream requestStream = host.RequestStream;
            Debug.Assert(requestStream != null, "requestStream != null");

            return Deserializer.HandlePutRequest(description, dataService, requestStream);
        }

        /// <summary>Handles DELETE requests.</summary>
        /// <param name="description">description about the target request</param>
        /// <param name="dataService">data service to which the request was made.</param>
        private static void HandleDeleteOperation(RequestDescription description, IDataService dataService)
        {
            Debug.Assert(description != null, "description != null");
            Debug.Assert(description.TargetSource != RequestTargetSource.ServiceOperation, "description.TargetSource != RequestTargetSource.ServiceOperation");
            Debug.Assert(dataService != null, "dataService != null");
            Debug.Assert(dataService.Configuration != null, "dataService.Configuration != null");
            Debug.Assert(dataService.OperationContext.Host != null, "dataService.OperationContext.Host != null");

            DataServiceHostWrapper host = dataService.OperationContext.Host;

            // In general, deletes are only supported on resource referred via top level sets or collection properties.
            // If its the open property case, the key must be specified
            // or you can unbind relationships using delete
            if (description.IsSingleResult && description.LinkUri)
            {
                HandleUnbindOperation(description, dataService);
            }
            else if (description.IsSingleResult && description.TargetKind == RequestTargetKind.Resource)
            {
                Debug.Assert(description.LastSegmentInfo.TargetContainer != null, "description.LastSegmentInfo.TargetContainer != null");

                if (description.RequestEnumerable == null)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequest_ResourceCanBeCrossReferencedOnlyForBindOperation);
                }

                // 
                if (!String.IsNullOrEmpty(host.RequestIfNoneMatch))
                {
                    throw DataServiceException.CreateBadRequestError(Strings.DataService_IfNoneMatchHeaderNotSupportedInDelete);
                }

                // Get the single entity result
                // We have to query for the delete case, since we don't know the type of the resource
                object entity = Deserializer.GetResource(description.LastSegmentInfo, null, dataService, true /*checkForNull*/);
                ResourceSetWrapper container = description.LastSegmentInfo.TargetContainer;

                // Need to check etag for DELETE operation
                dataService.Updatable.SetETagValues(entity, container);

                // 

                object actualEntity = dataService.Updatable.ResolveResource(entity);

                ResourceType resourceType = dataService.Provider.GetResourceType(actualEntity);
                if (description.Property != null)
                {
                    Debug.Assert(container != null, "container != null");
                    DataServiceConfiguration.CheckResourceRights(container, EntitySetRights.WriteDelete);
                }

                dataService.Updatable.DeleteResource(entity);

                if (resourceType != null && resourceType.IsMediaLinkEntry)
                {
                    dataService.StreamProvider.DeleteStream(actualEntity, dataService.OperationContext);
                }

                UpdateTracker.FireNotification(dataService, actualEntity, container, UpdateOperations.Delete);
            }
            else if (description.TargetKind == RequestTargetKind.PrimitiveValue)
            {
                Debug.Assert(description.TargetSource == RequestTargetSource.Property, "description.TargetSource == RequestTargetSource.Property");
                Debug.Assert(description.IsSingleResult, "description.IsSingleResult");

                // 
                if (!String.IsNullOrEmpty(host.RequestIfNoneMatch))
                {
                    throw DataServiceException.CreateBadRequestError(Strings.DataService_IfNoneMatchHeaderNotSupportedInDelete);
                }

                if (description.Property != null && description.Property.IsOfKind(ResourcePropertyKind.Key))
                {
                    throw DataServiceException.CreateBadRequestError(Strings.DataService_CannotUpdateKeyProperties(description.Property.Name));
                }
                else if (description.Property.Type.IsValueType)
                {
                    // 403 - Forbidden
                    throw new DataServiceException(403, Strings.BadRequest_CannotNullifyValueTypeProperty);
                }

                // We have to issue the query to get the resource
                object securityResource;        // Resource on which security check can be made (possibly entity parent of 'resource').
                ResourceSetWrapper container;    // resource set to which the parent entity belongs to.
                object resource = Deserializer.GetResourceToModify(description, dataService, false /*allowCrossReference*/, out securityResource, out container, true /*checkETag*/);

                object actualEntity = dataService.Updatable.ResolveResource(securityResource);

                // Doesn't matter which content format we pass here, since the value we are setting to is null
                Deserializer.ModifyResource(description, resource, null, ContentFormat.Text, dataService);
                
                UpdateTracker.FireNotification(dataService, actualEntity, container, UpdateOperations.Change);
            }
            else if (description.TargetKind == RequestTargetKind.OpenProperty)
            {
                // Open navigation properties are not supported on OpenTypes.
                throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(description.LastSegmentInfo.Identifier));
            }
            else if (description.TargetKind == RequestTargetKind.OpenPropertyValue)
            {
                // 
                if (!String.IsNullOrEmpty(host.RequestIfNoneMatch))
                {
                    throw DataServiceException.CreateBadRequestError(Strings.DataService_IfNoneMatchHeaderNotSupportedInDelete);
                }

                object securityResource;
                ResourceSetWrapper container;
                object resource = Deserializer.GetResourceToModify(description, dataService, false /*allowCrossReference*/, out securityResource, out container, true /*checkETag*/);

                object actualEntity = dataService.Updatable.ResolveResource(securityResource);

                // Doesn't matter which content format we pass here, since the value we are setting to is null
                Deserializer.ModifyResource(description, resource, null, ContentFormat.Text, dataService);

                UpdateTracker.FireNotification(dataService, actualEntity, container, UpdateOperations.Change);
            }
            else
            {
                throw DataServiceException.CreateMethodNotAllowed(
                    Strings.BadRequest_InvalidUriForDeleteOperation(host.AbsoluteRequestUri),
                    DataServiceConfiguration.GetAllowedMethods(dataService.Configuration, description));
            }
        }

        /// <summary>Handles a request for an internal resource if applicable.</summary>
        /// <param name="description">Request description.</param>
        /// <param name="dataService">data service to which the request was made.</param>
        /// <returns>
        /// An action that produces the resulting stream; null if the description isn't for an internal resource.
        /// </returns>
        private static Action<Stream> HandleInternalResources(RequestDescription description, IDataService dataService)
        {
            string[] exactContentType = null;
            ContentFormat format = ContentFormat.Unknown;
            string mime = null;
            DataServiceHostWrapper host = dataService.OperationContext.Host;

            if (description.TargetKind == RequestTargetKind.Metadata)
            {
                exactContentType = new string[] { XmlConstants.MimeMetadata };
                format = ContentFormat.MetadataDocument;
                mime = HttpProcessUtility.SelectRequiredMimeType(
                    host.RequestAccept,   // acceptTypesText
                    exactContentType,                   // exactContentType
                    XmlConstants.MimeApplicationXml);   // inexactContentType
            }
            else if (description.TargetKind == RequestTargetKind.ServiceDirectory)
            {
                exactContentType = new string[] { XmlConstants.MimeApplicationAtomService, XmlConstants.MimeApplicationJson, XmlConstants.MimeApplicationXml };
                mime = HttpProcessUtility.SelectRequiredMimeType(
                    host.RequestAccept,   // acceptTypesText
                    exactContentType,                   // exactContentType
                    XmlConstants.MimeApplicationXml);   // inexactContentType;
                format = GetContentFormat(mime);
            }

            if (exactContentType != null)
            {
                Debug.Assert(
                    format != ContentFormat.Unknown,
                    "format(" + format + ") != ContentFormat.Unknown -- otherwise exactContentType should be null");
                Encoding encoding = HttpProcessUtility.EncodingFromAcceptCharset(host.RequestAcceptCharSet);
                host.ResponseContentType = HttpProcessUtility.BuildContentType(mime, encoding);
                return new ResponseBodyWriter(
                    encoding,
                    false,                  // hasMoved
                    dataService,
                    null,                   // queryResults
                    description,
                    format).Write;
            }

            return null;
        }

        /// <summary>
        /// Compare the ETag value and then serialize the value if required
        /// </summary>
        /// <param name="description">Description of the uri requested.</param>
        /// <param name="responseFormat">Content format for response.</param>
        /// <param name="dataService">Data service to which the request was made.</param>
        /// <returns>A delegate that can serialize the result.</returns>
        private static Action<Stream> CompareETagAndWriteResponse(
            RequestDescription description,
            ContentFormat responseFormat,
            IDataService dataService)
        {
            Debug.Assert(description != null, "description != null");
            Debug.Assert(dataService != null, "dataService != null");
            Debug.Assert(dataService.OperationContext != null && dataService.OperationContext.Host != null, "dataService.OperationContext != null && dataService.OperationContext.Host != null");
            DataServiceHostWrapper host = dataService.OperationContext.Host;
            Debug.Assert(
                String.IsNullOrEmpty(host.RequestIfMatch) || String.IsNullOrEmpty(host.RequestIfNoneMatch), 
                "Both If-Match and If-None-Match header cannot be specified");
            IEnumerator queryResults = null;
            try
            {
                if (host.AstoriaHttpVerb == AstoriaVerbs.GET)
                {
                    bool writeResponse = true;

                    // Get the index of the last resource in the request uri
                    int parentResourceIndex = description.GetIndexOfTargetEntityResource();
                    Debug.Assert(parentResourceIndex >= 0 && parentResourceIndex < description.SegmentInfos.Length, "parentResourceIndex >= 0 && parentResourceIndex < description.SegmentInfos.Length");

                    SegmentInfo parentEntitySegment = description.SegmentInfos[parentResourceIndex];
                    queryResults = RequestDescription.GetSingleResultFromEnumerable(parentEntitySegment);
                    object resource = queryResults.Current;
                    string etagValue = null;

                    if (description.LinkUri)
                    {
                        // This must be already checked in SerializeResponseBody method.
                        Debug.Assert(String.IsNullOrEmpty(host.RequestIfMatch) && String.IsNullOrEmpty(host.RequestIfNoneMatch), "ETag cannot be specified for $link requests");
                        if (resource == null)
                        {
                            throw DataServiceException.CreateResourceNotFound(description.LastSegmentInfo.Identifier);
                        }
                    }
                    else if (RequestDescription.IsETagHeaderAllowed(description) && description.TargetKind != RequestTargetKind.MediaResource)
                    {
                        // Media Resources have their own ETags, we let the Stream Provider handle it. No need to compare the MLE ETag here.
                        ResourceSetWrapper container = parentEntitySegment.TargetContainer;
                        Debug.Assert(container != null, "container != null");

                        etagValue = WebUtil.CompareAndGetETag(resource, resource, container, dataService, out writeResponse);
                    }

                    if (resource == null && description.TargetKind == RequestTargetKind.Resource)
                    {
                        Debug.Assert(description.Property != null, "non-open type property");

                        WebUtil.Dispose(queryResults);
                        queryResults = null;

                        // If you are querying reference nav property and the value is null, 
                        // return 204 - No Content e.g. /Customers(1)/BestFriend
                        host.ResponseStatusCode = 204; // No Content
                        return WebUtil.GetEmptyStreamWriter();
                    }

                    if (writeResponse)
                    {
                        return WriteSingleElementResponse(description, responseFormat, queryResults, parentResourceIndex, etagValue, dataService);
                    }
                    else
                    {
                        WebUtil.Dispose(queryResults);
                        queryResults = null;
#if DEBUG
                        WebUtil.WriteETagValueInResponseHeader(description, etagValue, host);
#else
                        WebUtil.WriteETagValueInResponseHeader(etagValue, host);
#endif
                        host.ResponseStatusCode = 304; // Not Modified
                        return WebUtil.GetEmptyStreamWriter();
                    }
                }
                else
                {
                    Debug.Assert(host.AstoriaHttpVerb == AstoriaVerbs.POST, "Must be POST method");
                    ResourceSetWrapper container;
                    object actualEntity = GetContainerAndActualEntityInstance(dataService, description, out container);
                    host.ResponseLocation = Serializer.GetUri(actualEntity, dataService.Provider, container, host.AbsoluteServiceUri).AbsoluteUri;
                    string etagValue = WebUtil.GetETagValue(dataService, actualEntity, container);
                    queryResults = RequestDescription.GetSingleResultFromEnumerable(description.LastSegmentInfo);
                    return WriteSingleElementResponse(description, responseFormat, queryResults, description.SegmentInfos.Length - 1, etagValue, dataService);
                }
            }
            catch
            {
                WebUtil.Dispose(queryResults);
                throw;
            }
        }

        /// <summary>Resolves the content format required when it is statically unknown.</summary>
        /// <param name='description'>Request description.</param>
        /// <param name='element'>Result target.</param>
        /// <param name="dataService">data service to which the request was made.</param>
        /// <returns>The format for the specified element.</returns>
        private static ContentFormat ResolveUnknownFormat(RequestDescription description, object element, IDataService dataService)
        {
            Debug.Assert(
                description.TargetKind == RequestTargetKind.OpenProperty ||
                description.TargetKind == RequestTargetKind.OpenPropertyValue ||
                description.TargetKind == RequestTargetKind.MediaResource,
                description.TargetKind + " is open property, open property value, or MediaResource.");
            WebUtil.CheckResourceExists(element != null, description.LastSegmentInfo.Identifier);
            ResourceType resourceType = WebUtil.GetResourceType(dataService.Provider, element);
            Debug.Assert(resourceType != null, "resourceType != null, WebUtil.GetResourceType() should throw if it fails to resolve the resource type.");
            DataServiceHostWrapper host = dataService.OperationContext.Host;

            // Determine the appropriate target type based on the kind of resource.
            bool rawValue = description.TargetKind == RequestTargetKind.OpenPropertyValue || description.TargetKind == RequestTargetKind.MediaResource;
            RequestTargetKind targetKind;
            switch (resourceType.ResourceTypeKind)
            {
                case ResourceTypeKind.ComplexType:
                    if (rawValue)
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequest_ValuesCanBeReturnedForPrimitiveTypesOnly);
                    }
                    else
                    {
                        targetKind = RequestTargetKind.ComplexObject;
                    }

                    break;
                case ResourceTypeKind.Primitive:
                    if (rawValue)
                    {
                        targetKind = RequestTargetKind.PrimitiveValue;
                    }
                    else
                    {
                        targetKind = RequestTargetKind.Primitive;
                    }

                    break;
                default:
                    Debug.Assert(ResourceTypeKind.EntityType == resourceType.ResourceTypeKind, "ResourceTypeKind.EntityType == " + resourceType.ResourceTypeKind);
                    if (rawValue)
                    {
                        if (resourceType.IsMediaLinkEntry)
                        {
                            return SelectMediaResourceContentType(element, host.RequestAccept, dataService);
                        }
                        else
                        {
                            throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidUriForMediaResource(host.AbsoluteRequestUri));
                        }
                    }
                    else
                    {
                        targetKind = RequestTargetKind.Resource;
                    }

                    break;
            }

            if (description.LinkUri)
            {
                targetKind = RequestTargetKind.Link;
            }

            return SelectResponseFormatForType(targetKind, resourceType, host.RequestAccept, null, dataService);
        }

        /// <summary>
        /// Compare the ETag value and then serialize the value if required
        /// </summary>
        /// <param name="description">Description of the uri requested.</param>
        /// <param name="responseFormat">format of the response</param>
        /// <param name="queryResults">Enumerator whose current resource points to the resource which needs to be written</param>
        /// <param name="parentResourceIndex">index of the segment info that represents the last resource</param>
        /// <param name="etagValue">etag value for the resource specified in parent resource parameter</param>
        /// <param name="dataService">data service to which the request was made.</param>
        /// <returns>A delegate that can serialize the result.</returns>
        private static Action<Stream> WriteSingleElementResponse(
            RequestDescription description,
            ContentFormat responseFormat,
            IEnumerator queryResults,
            int parentResourceIndex,
            string etagValue,
            IDataService dataService)
        {
            try
            {
                // The queryResults parameter contains the enumerator of the parent resource. If the parent resource's RequestEnumerable is not
                // the same instance as that of the last segment, we need to get the enumerator for the last segment.
                // Take MediaResource for example, the MLE is its parent resource, which is what we want to write out and we don't want to
                // query for another instance of the enumerator.
                if (description.SegmentInfos[parentResourceIndex].RequestEnumerable != description.LastSegmentInfo.RequestEnumerable)
                {
                    object resource = queryResults.Current;

                    for (int segmentIdx = parentResourceIndex + 1; segmentIdx < description.SegmentInfos.Length; segmentIdx++)
                    {
                        SegmentInfo parentSegment = description.SegmentInfos[segmentIdx - 1];
                        SegmentInfo currentSegment = description.SegmentInfos[segmentIdx];

                        WebUtil.CheckResourceExists(resource != null, parentSegment.Identifier);

                        // $value has the same query as the preceding segment.
                        if (currentSegment.TargetKind == RequestTargetKind.PrimitiveValue || currentSegment.TargetKind == RequestTargetKind.OpenPropertyValue)
                        {
                            Debug.Assert(segmentIdx == description.SegmentInfos.Length - 1, "$value has to be the last segment.");
                            break;
                        }

                        if (currentSegment.TargetKind == RequestTargetKind.OpenProperty)
                        {
                            ResourceType openTypeParentResourceType = WebUtil.GetResourceType(dataService.Provider, resource);

                            if (openTypeParentResourceType.ResourceTypeKind == ResourceTypeKind.ComplexType)
                            {
                                ResourceProperty resProperty = openTypeParentResourceType.Properties.First(p => p.Name == currentSegment.Identifier);
                                resource = WebUtil.GetPropertyValue(dataService.Provider, resource, openTypeParentResourceType, resProperty, null);
                            }
                            else
                            {
                                Debug.Assert(openTypeParentResourceType.ResourceTypeKind == ResourceTypeKind.EntityType, "Entity Type expected");
                                resource = WebUtil.GetPropertyValue(dataService.Provider, resource, openTypeParentResourceType, null, currentSegment.Identifier);
                            }
                        }
                        else
                        {
                            resource = WebUtil.GetPropertyValue(dataService.Provider, resource, parentSegment.TargetResourceType, currentSegment.ProjectedProperty, null);
                        }
                    }

                    RequestDescription.CheckQueryResult(resource, description.LastSegmentInfo);

                    queryResults = new QueryResultsWrapper((new object[] { resource }).GetEnumerator(), queryResults);
                    queryResults.MoveNext();
                }

                // If we had to wait until we got a value to determine the valid contents, try that now.
                if (responseFormat == ContentFormat.Unknown)
                {
                    responseFormat = ResolveUnknownFormat(description, queryResults.Current, dataService);
                }

                Debug.Assert(responseFormat != ContentFormat.Unknown, "responseFormat != ContentFormat.Unknown");
                DataServiceHostWrapper host = dataService.OperationContext.Host;

                // Write the etag header
#if DEBUG
                WebUtil.WriteETagValueInResponseHeader(description, etagValue, host);
#else
                WebUtil.WriteETagValueInResponseHeader(etagValue, host);
#endif

                Encoding encoding = GetRequestAcceptEncoding(responseFormat, host.RequestAcceptCharSet);
                host.ResponseContentType = HttpProcessUtility.BuildContentType(host.ResponseContentType, encoding);
                return new ResponseBodyWriter(
                    encoding,
                    true /* hasMoved */,
                    dataService,
                    queryResults,
                    description,
                    responseFormat).Write;
            }
            catch
            {
                WebUtil.Dispose(queryResults);
                throw;
            }
        }

        /// <summary>
        /// Returns the actual entity instance and its containers for the resource in the description results.
        /// </summary>
        /// <param name="service">Data service</param>
        /// <param name="description">description about the request made.</param>
        /// <param name="container">returns the container to which the result resource belongs to.</param>
        /// <returns>returns the actual entity instance for the given resource.</returns>
        private static object GetContainerAndActualEntityInstance(
            IDataService service, RequestDescription description, out ResourceSetWrapper container)
        {
            // For POST operations, we need to resolve the entity only after save changes. Hence we need to do this at the serialization
            // to make sure save changes has been called
            object[] results = (object[])description.RequestEnumerable;
            Debug.Assert(results != null && results.Length == 1, "results != null && results.Length == 1");

            // Make a call to the provider to get the exact resource instance back
            results[0] = service.Updatable.ResolveResource(results[0]);
            container = description.LastSegmentInfo.TargetContainer;
            if (container == null)
            {
                // Open navigation properties are not supported on OpenTypes.
                throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(description.LastSegmentInfo.Identifier));
            }

            Debug.Assert(container != null, "description.LastSegmentInfo.TargetContainer != null");
            return results[0];
        }

        /// <summary>
        /// Handles the unbind operations
        /// </summary>
        /// <param name="description">description about the request made.</param>
        /// <param name="dataService">data service to which the request was made.</param>
        private static void HandleUnbindOperation(RequestDescription description, IDataService dataService)
        {
            Debug.Assert(description.LinkUri, "This method must be called for link operations");
            Debug.Assert(description.IsSingleResult, "Expecting this method to be called on single resource uris");

            if (!String.IsNullOrEmpty(dataService.OperationContext.Host.RequestIfMatch) || !String.IsNullOrEmpty(dataService.OperationContext.Host.RequestIfNoneMatch))
            {
                throw DataServiceException.CreateBadRequestError(Strings.DataService_ETagNotSupportedInUnbind);
            }

            object parentEntity;
            ResourceSetWrapper parentEntityResourceSet;
            Deserializer.GetResourceToModify(description, dataService, out parentEntity, out parentEntityResourceSet);
            Debug.Assert(description.Property != null, "description.Property != null");
            if (description.Property.Kind == ResourcePropertyKind.ResourceReference)
            {
                dataService.Updatable.SetReference(parentEntity, description.Property.Name, null);
            }
            else
            {
                Debug.Assert(description.Property.Kind == ResourcePropertyKind.ResourceSetReference, "expecting collection nav properties");
                Debug.Assert(description.LastSegmentInfo.HasKeyValues, "expecting properties to have key value specified");
                object childEntity = Deserializer.GetResource(description.LastSegmentInfo, null, dataService, true /*checkForNull*/);
                dataService.Updatable.RemoveReferenceFromCollection(parentEntity, description.Property.Name, childEntity);
            }

            if (dataService.Configuration.DataServiceBehavior.InvokeInterceptorsOnLinkDelete)
            {
                object actualParentEntity = dataService.Updatable.ResolveResource(parentEntity);
                UpdateTracker.FireNotification(dataService, actualParentEntity, parentEntityResourceSet, UpdateOperations.Change);
            }   
        }

        /// <summary>
        /// Get the content format corresponding to the given mime type.
        /// </summary>
        /// <param name="mime">mime type for the request.</param>
        /// <returns>content format mapping to the given mime type.</returns>
        private static ContentFormat GetContentFormat(string mime)
        {
            if (WebUtil.CompareMimeType(mime, XmlConstants.MimeApplicationJson))
            {
                return ContentFormat.Json;
            }
            else if (WebUtil.CompareMimeType(mime, XmlConstants.MimeApplicationAtom))
            {
                return ContentFormat.Atom;
            }
            else
            {
                Debug.Assert(
                    WebUtil.CompareMimeType(mime, XmlConstants.MimeApplicationXml) ||
                    WebUtil.CompareMimeType(mime, XmlConstants.MimeApplicationAtomService) ||
                    WebUtil.CompareMimeType(mime, XmlConstants.MimeTextXml),
                    "expecting application/xml, application/atomsvc+xml or plain/xml, got " + mime);
                return ContentFormat.PlainXml;
            }
        }

        /// <summary>
        /// Handle the request - whether its a batch request or a non-batch request
        /// </summary>
        /// <returns>Returns the delegate for writing the response</returns>
        private Action<Stream> HandleRequest()
        {
            Debug.Assert(this.operationContext != null, "this.operationContext != null");

            // Need to cache the request headers for every request. Note that the while the underlying
            // host instance may stay the same across requests, the request headers can change between
            // requests. We have to refresh the cache for every request.
            this.operationContext.InitializeAndCacheHeaders();
            Action<Stream> writer = null;
            
            try
            {
                this.EnsureProviderAndConfigForRequest();
            }
            catch (Exception ex)
            {
#if DEBUG
                // We've hit an error, some of the processing pipeline events will not get fired. Setting this flag to skip validation.
                this.ProcessingPipeline.SkipDebugAssert = true;
#endif
                int responseStatusCode = 500;
                if (!WebUtil.IsCatchableExceptionType(ex))
                {
                    throw;
                }

                // if Exception been thrown is DSE, we keep the exception's status code
                // otherwise, the status code is 500.
                DataServiceException dse = ex as DataServiceException;
                if (dse != null)
                {
                    responseStatusCode = dse.StatusCode;
                }

                // safe handling of initialization time error
                DataServiceHostWrapper host = this.operationContext.Host;
                host.ResponseStatusCode = responseStatusCode;
                host.ResponseVersion = XmlConstants.DataServiceVersion1Dot0 + ";";
                throw;
            }

            try
            {
                RequestDescription description = this.ProcessIncomingRequestUri();
                if (description.TargetKind != RequestTargetKind.Batch)
                {
                    writer = this.HandleNonBatchRequest(description);
                    
                    // Query Processing Pipeline - Request end event
                    // Note 1 we only invoke the event handler for ALL operations
                    // Note 2 we invoke this event before serialization is complete
                    // Note 3 we invoke this event before any provider interface held by the data service runtime is released/disposed
                    DataServiceProcessingPipelineEventArgs eventArg = new DataServiceProcessingPipelineEventArgs(this.operationContext);
                    this.processingPipeline.InvokeProcessedRequest(this, eventArg);
                }
                else
                {
                    writer = this.HandleBatchRequest();
                }
            }
            catch (Exception exception)
            {
#if DEBUG
                // We've hit an error, some of the processing pipeline events will not get fired. Setting this flag to skip validation.
                this.ProcessingPipeline.SkipDebugAssert = true;
#endif
                // Exception should be re-thrown if not handled.
                if (!WebUtil.IsCatchableExceptionType(exception))
                {
                    throw;
                }

                string accept = (this.operationContext != null) ? this.operationContext.Host.RequestAccept : null;
                string acceptCharset = (this.operationContext != null) ? this.operationContext.Host.RequestAcceptCharSet : null;
                writer = ErrorHandler.HandleBeforeWritingException(exception, this, accept, acceptCharset);
            }

            Debug.Assert(writer != null, "writer != null");
            return writer;
        }

        /// <summary>
        /// Handle non-batch requests
        /// </summary>
        /// <param name="description">description about the request uri.</param>
        /// <returns>Returns the delegate which takes the response stream for writing the response.</returns>
        private Action<Stream> HandleNonBatchRequest(RequestDescription description)
        {
            Debug.Assert(description.TargetKind != RequestTargetKind.Batch, "description.TargetKind != RequestTargetKind.Batch");
            bool serviceOperationRequest = (description.TargetSource == RequestTargetSource.ServiceOperation);

            // The reason to create UpdatableWrapper here is to make sure that the right data service instance is
            // passed to the UpdatableWrapper. Earlier this line used to live in EnsureProviderAndConfigForRequest
            // method, which means that same data service instance was passed to the UpdatableWrapper, irrespective
            // of whether this was a batch request or not. The issue with that was in UpdatableWrapper.SetConcurrencyValues
            // method, if someone tried to access the service.RequestParams, this will give you the headers for the
            // top level batch request, not the part of the batch request we are processing.
            this.updatable = new UpdatableWrapper(this);
            description = ProcessIncomingRequest(description, this);

            if (this.operationContext.Host.AstoriaHttpVerb != AstoriaVerbs.GET)
            {
                // Bug 470090: Since we used to call SaveChanges() for service operations in V1, we need to
                // keep doing that for V1 providers that implement IUpdatable. In other words, for ObjectContextServiceProvider
                // we will always do this, and for reflection service provider, we will have to check.
                if (serviceOperationRequest)
                {
                    if (this.provider.IsV1ProviderAndImplementsUpdatable())
                    {
                        this.updatable.SaveChanges();
                    }
                }
                else
                {
                   this.updatable.SaveChanges();
                }

                // Query Processing Pipeline - Changeset end event
                // Note 1 we only invoke the event handler for CUD operations
                // Note 2 we invoke this event immediately after SaveChanges()
                // Note 3 we invoke this event before serialization happens
                this.processingPipeline.InvokeProcessedChangeset(this, new EventArgs());
            }

            return (description == null) ? WebUtil.GetEmptyStreamWriter() : SerializeResponseBody(description, this);
        }

        /// <summary>Handle the batch request.</summary>
        /// <returns>Returns the delegate which takes the response stream for writing the response.</returns>
        private Action<Stream> HandleBatchRequest()
        {
            Debug.Assert(this.operationContext != null && this.operationContext.Host != null, "this.operationContext != null && this.operationContext.Host != null");
            DataServiceHostWrapper host = this.operationContext.Host;

            // Verify the HTTP method.
            if (host.AstoriaHttpVerb != AstoriaVerbs.POST)
            {
                throw DataServiceException.CreateMethodNotAllowed(
                    Strings.DataService_BatchResourceOnlySupportsPost,
                    XmlConstants.HttpMethodPost);
            }

            WebUtil.CheckVersion(this, null);

            // Verify the content type and get the boundary string
            Encoding encoding;
            string boundary;

            if (!BatchStream.GetBoundaryAndEncodingFromMultipartMixedContentType(host.RequestContentType, out boundary, out encoding) ||
                String.IsNullOrEmpty(boundary))
            {
                throw DataServiceException.CreateBadRequestError(Strings.DataService_InvalidContentTypeForBatchRequest);
            }

            // Write the response headers
            host.ResponseStatusCode = 202; // OK
            host.ResponseCacheControl = XmlConstants.HttpCacheControlNoCache;

            string batchBoundary = XmlConstants.HttpMultipartBoundaryBatchResponse + '_' + Guid.NewGuid().ToString();
            host.ResponseContentType = String.Format(
                System.Globalization.CultureInfo.InvariantCulture,
                "{0}; {1}={2}",
                XmlConstants.MimeMultiPartMixed,
                XmlConstants.HttpMultipartBoundary,
                batchBoundary);

            // DEVNOTE(Microsoft):
            // Added for V2+ services
            // The batch response format should be 1.0 until we update the format itself
            // Each individual batch response will set its version to the batch host.
            host.ResponseVersion = XmlConstants.DataServiceVersion1Dot0 + ";";

            BatchStream batchStream = new BatchStream(host.RequestStream, boundary, encoding, true);
            BatchDataService batchDataService = new BatchDataService(this, batchStream, batchBoundary);
            return batchDataService.HandleBatchContent;
        }

        /// <summary>Creates the provider and configuration as necessary to be used for this request.</summary>
        private void EnsureProviderAndConfigForRequest()
        {
            if (this.provider == null)
            {
                this.CreateProvider();
            }
            else
            {
                Debug.Assert(this.configuration != null, "this.configuration != null -- otherwise this.provider was ----signed with no configuration");
            }

#if DEBUG
            // No event should be fired before this point.
            // No provider interfaces except IDSP should be created before this point.
            this.processingPipeline.AssertInitialDebugState();
#endif
        }

        /// <summary>
        /// Creates a provider implementation that wraps the T type.
        /// </summary>
        private void CreateProvider()
        {
            // From the IDSP Spec:
            // If the class derived from DataService<T> implements IServiceProvider then:
            // a. Invoke 
            //    IDataServiceMetadataProvider provider = IServiceProvider.GetService(TypeOf(IDataMetadataServiceProvider))
            //
            // b. If provider != null, then the service is using a Custom provider (ie. custom implementation of IDSP)
            //    Note: DataService<T>.CreateDataSource is NOT invoked for custom data service providers
            //
            // c. If provider == null, then:
            //    i.   Create an instance of T by invoking DataService<T>.CreateDataSource (this method may be overridden by a service author)
            //    ii.  If T implements IDataServiceMetadataProvider then the service is using a custom data service provider (skip step iii. & iv.)
            //    iii. If typeof(T) == typeof(System.Data.Objects.ObjectContext) then the service will use the built-in IDSP implementation for EF (ie. the “EF provider”)
            //    iv.  If typeof(T) != typeof(System.Data.Objects.ObjectContext) then the service will use the built-in reflection for arbitrary .NET classes (ie. the “reflection provider”)
            Type dataServiceType = this.GetType();
            Type dataContextType = typeof(T);

            bool friendlyFeedsV1Compatible;

            // If the GetService call returns a provider, that means there is a custom implementation of the provider
            IDataServiceMetadataProvider metadataProviderInstance = WebUtil.GetService<IDataServiceMetadataProvider>(this);
            IDataServiceQueryProvider queryProviderInstance = null;
            object dataSourceInstance = null;
            if (metadataProviderInstance != null)
            {
                queryProviderInstance = WebUtil.GetService<IDataServiceQueryProvider>(this);
                if (queryProviderInstance == null)
                {
                    throw new InvalidOperationException(Strings.DataService_IDataServiceQueryProviderNull);
                }

                // For custom providers, we will first query the queryProvider to check if the provider returns a data source instance.
                // If it doesn't, then we will create a instance of data source and pass it to the query provider.
                dataSourceInstance = queryProviderInstance.CurrentDataSource;
                if (dataSourceInstance == null)
                {
                    dataSourceInstance = this.CreateDataSourceInstance();
                    queryProviderInstance.CurrentDataSource = dataSourceInstance;
                }

                if (!dataContextType.IsAssignableFrom(dataSourceInstance.GetType()))
                {
                    throw new InvalidOperationException(Strings.DataServiceProviderWrapper_DataSourceTypeMustBeAssignableToContextType);
                }
            }
            else
            {
                // Create the data source from the service by calling DataService<T>.CreateDataSource
                dataSourceInstance = this.CreateDataSourceInstance();

                // Try if the data source implements IDSMP
                metadataProviderInstance = dataSourceInstance as IDataServiceMetadataProvider;
                if (metadataProviderInstance != null)
                {
                    queryProviderInstance = dataSourceInstance as IDataServiceQueryProvider;
                    if (queryProviderInstance == null)
                    {
                        throw new InvalidOperationException(Strings.DataService_IDataServiceQueryProviderNull);
                    }

                    // For customer providers if we already have the data source instance, we will pass it to the query provider.
                    queryProviderInstance.CurrentDataSource = dataSourceInstance;
                }
            }

            // If we found IDSMP by now - it means we will use custom provider, otherwise we will use one of our built-in providers
            if (metadataProviderInstance != null)
            {
                Debug.Assert(queryProviderInstance != null, "If we have IDSMP we should also have IDSQP.");

                // For IDSMP, we cache the configuration object and we must NOT cache any of the metadata objects.
                // This means we call InitializeService() once per service instead of once per request.
                MetadataCacheItem metadata = MetadataCache.TryLookup(dataServiceType, dataSourceInstance);
                bool metadataRequiresInitialization = metadata == null;
                if (metadataRequiresInitialization)
                {
                    metadata = new MetadataCacheItem(dataContextType);
                    metadata.Configuration = CreateConfiguration(dataServiceType, metadataProviderInstance);
                    MetadataCache.AddCacheItem(dataServiceType, dataSourceInstance, metadata);
                }

                Debug.Assert(metadata != null, "Metadata item should have been found or created within this function");
                this.configuration = metadata.Configuration;

                // For IDSMP, we cannot cache anything except for the configuration.
                // We need to pass in a new MetadataCacheItem instance for each request.
                metadata = new MetadataCacheItem(dataContextType) { Configuration = this.configuration };
                this.provider = new DataServiceProviderWrapper(metadata, metadataProviderInstance, queryProviderInstance);

                // For IDSMP we have to assume that friendly feeds are V1 compatible. We will validate this assumption
                // when processing requests and throw an exception if this happens to be false. 
                friendlyFeedsV1Compatible = true;
            }
            else
            {
                MetadataCacheItem metadata = MetadataCache.TryLookup(dataServiceType, dataSourceInstance);
                bool metadataRequiresInitialization = metadata == null;
                if (metadataRequiresInitialization)
                {
                    metadata = new MetadataCacheItem(dataContextType);
                }

                BaseServiceProvider dataProviderInstance;

                // use our built-in providers and policy layer
                if (typeof(ObjectContext).IsAssignableFrom(dataContextType))
                {
                    dataProviderInstance = new ObjectContextServiceProvider(metadata, this);
                }
                else
                {
                    dataProviderInstance = new ReflectionServiceProvider(metadata, this);
                }

                dataProviderInstance.CurrentDataSource = dataSourceInstance;
                this.provider = new DataServiceProviderWrapper(metadata, dataProviderInstance, dataProviderInstance);
                dataProviderInstance.ProviderWrapper = this.provider;

                if (metadataRequiresInitialization)
                {
                    // Populate metadata in provider.
                    dataProviderInstance.PopulateMetadata();
                    dataProviderInstance.AddOperationsFromType(dataServiceType);

                    // Create and cache configuration, which goes hand-in-hand with metadata.
                    metadata.Configuration = CreateConfiguration(dataServiceType, dataProviderInstance);

                    // Apply the access rights info from the configuration.
                    dataProviderInstance.ApplyConfiguration(metadata.Configuration);

                    // After all the operations are done, make metadata readonly.
                    dataProviderInstance.MakeMetadataReadonly();

                    // Populate and cache the metadata.
                    this.provider.PopulateMetadataCacheItemForV1Provider();
                    MetadataCache.AddCacheItem(dataServiceType, dataSourceInstance, metadata);
                }

                this.configuration = metadata.Configuration;

                friendlyFeedsV1Compatible = metadata.EpmIsV1Compatible;
            }

            this.configuration.ValidateServerOptions(friendlyFeedsV1Compatible);

            Debug.Assert(this.configuration != null, "configuration != null");
            Debug.Assert(this.provider != null, "wrapper != null");
        }

        /// <summary>
        /// Processes the incoming request and cache all the request headers
        /// </summary>
        /// <returns>description about the request uri.</returns>
        private RequestDescription ProcessIncomingRequestUri()
        {
            Debug.Assert(
                this.operationContext != null && this.operationContext.Host != null,
                "this.operationContext != null && this.operationContext.Host != null");
            DataServiceHostWrapper host = this.operationContext.Host;

            // Validation of query parameters must happen only after the request parameters have been cached,
            // otherwise we might not serialize the errors in the correct serialization format.
            host.VerifyQueryParameters();

            ValidateRequest(this.operationContext);

            // Query Processing Pipeline - Request start event
            DataServiceProcessingPipelineEventArgs eventArg = new DataServiceProcessingPipelineEventArgs(this.operationContext);
            this.processingPipeline.InvokeProcessingRequest(this, eventArg);

            // V1 OnStartProcessingRequest().
            ((IDataService)this).InternalOnStartProcessingRequest(new ProcessRequestArgs(host.AbsoluteRequestUri, false /*isBatchOperation*/, this.operationContext));

            // Query Processing Pipeline - Changeset start event
            // Note 1 we only invoke the event handler for CUD operations
            // Note 2 for a batch request this event will be invoked when we process the changeset boundary
            if (host.AstoriaHttpVerb != AstoriaVerbs.GET && !this.operationContext.IsBatchRequest)
            {
                this.processingPipeline.InvokeProcessingChangeset(this, new EventArgs());
            }

            return RequestUriProcessor.ProcessRequestUri(host.AbsoluteRequestUri, this);
        }

        /// <summary>
        /// Create the data source instance by calling the CreateDataSource virtual method
        /// </summary>
        /// <returns>returns the instance of the data source.</returns>
        private object CreateDataSourceInstance()
        {
            object dataSourceInstance = this.CreateDataSource();
            if (dataSourceInstance == null)
            {
                throw new InvalidOperationException(Strings.DataService_CreateDataSourceNull);
            }

            return dataSourceInstance;
        }

        #endregion Private methods.

        /// <summary>
        /// Dummy data service for batch requests
        /// </summary>
        private class BatchDataService : IDataService
        {
            #region Private fields.

            /// <summary>Original data service instance.</summary>
            private readonly IDataService dataService;

            /// <summary>batch stream which reads the content of the batch from the underlying request stream.</summary>
            private readonly BatchStream batchRequestStream;

            /// <summary>batch response seperator string.</summary>
            private readonly string batchBoundary;

            /// <summary>Hashset to make sure that the content ids specified in the batch are all unique.</summary>
            private readonly HashSet<int> contentIds = new HashSet<int>(new Int32EqualityComparer());

            /// <summary>Dictionary to track objects represented by each content id within a changeset.</summary>
            private readonly Dictionary<string, SegmentInfo> contentIdsToSegmentInfoMapping = new Dictionary<string, SegmentInfo>(StringComparer.Ordinal);

            /// <summary>Number of changset/query operations encountered in the current batch.</summary>
            private int batchElementCount;

            /// <summary>Whether the batch limit has been exceeded (implies no further processing should take place).</summary>
            private bool batchLimitExceeded;

            /// <summary>List of the all request description within a changeset.</summary>
            private List<RequestDescription> batchRequestDescription = new List<RequestDescription>();

            /// <summary>List of the all response headers and results of each operation within a changeset.</summary>
            private List<DataServiceOperationContext> batchOperationContexts = new List<DataServiceOperationContext>();

            /// <summary>Number of CUD operations encountered in the current changeset.</summary>
            private int changeSetElementCount;

            /// <summary>The context of the current batch operation.</summary>
            private DataServiceOperationContext operationContext;

            /// <summary>Instance which implements IUpdatable interface.</summary>
            private UpdatableWrapper updatable;

            /// <summary>Instance which implements the IDataServicePagingProvider interface.</summary>
            private DataServicePagingProviderWrapper pagingProvider;

            /// <summary>Instance which implements IDataServiceStreamProvider interface.</summary>
            private DataServiceStreamProviderWrapper streamProvider;

            #endregion Private fields.

            /// <summary>
            /// Creates an instance of the batch data service which keeps track of the 
            /// request and response headers per operation in the batch
            /// </summary>
            /// <param name="dataService">original data service to which the batch request was made</param>
            /// <param name="batchRequestStream">batch stream which read batch content from the request stream</param>
            /// <param name="batchBoundary">batch response seperator string.</param>
            internal BatchDataService(IDataService dataService, BatchStream batchRequestStream, string batchBoundary)
            {
                Debug.Assert(dataService != null, "dataService != null");
                Debug.Assert(batchRequestStream != null, "batchRequestStream != null");
                Debug.Assert(batchBoundary != null, "batchBoundary != null");
                this.dataService = dataService;
                this.batchRequestStream = batchRequestStream;
                this.batchBoundary = batchBoundary;
            }

            #region IDataService Members

            /// <summary>Service configuration information.</summary>
            public DataServiceConfiguration Configuration
            {
                get { return this.dataService.Configuration; }
            }

            /// <summary>Data provider for this data service.</summary>
            public DataServiceProviderWrapper Provider
            {
                get { return this.dataService.Provider; }
            }

            /// <summary>IUpdatable interface for this provider</summary>
            public UpdatableWrapper Updatable
            {
                get { return this.updatable ?? (this.updatable = new UpdatableWrapper(this)); }
            }

            /// <summary>IDataServicePagingProvider wrapper object.</summary>
            public DataServicePagingProviderWrapper PagingProvider
            {
                get { return this.pagingProvider ?? (this.pagingProvider = new DataServicePagingProviderWrapper(this)); }
            }

            /// <summary>Instance which implements IDataServiceStreamProvider interface.</summary>
            public DataServiceStreamProviderWrapper StreamProvider
            {
                get { return this.streamProvider ?? (this.streamProvider = new DataServiceStreamProviderWrapper(this)); }
            }

            /// <summary>Instance of the data provider.</summary>
            public object Instance
            {
                get { return this.dataService.Instance; }
            }

            /// <summary>Gets the context of the current batch operation.</summary>
            public DataServiceOperationContext OperationContext
            {
                get { return this.operationContext; }
            }

            /// <summary>Processing pipeline events</summary>
            public DataServiceProcessingPipeline ProcessingPipeline
            {
                get { return this.dataService.ProcessingPipeline; }
            }

            /// <summary>
            /// This method is called during query processing to validate and customize 
            /// paths for the $expand options are applied by the provider.
            /// </summary>
            /// <param name='queryable'>Query which will be composed.</param>
            /// <param name='expandPaths'>Collection of segment paths to be expanded.</param>
            public void InternalApplyingExpansions(IQueryable queryable, ICollection<ExpandSegmentCollection> expandPaths)
            {
                this.dataService.InternalApplyingExpansions(queryable, expandPaths);
            }

            /// <summary>Processes a catchable exception.</summary>
            /// <param name="args">The arguments describing how to handle the exception.</param>
            public void InternalHandleException(HandleExceptionArgs args)
            {
                this.dataService.InternalHandleException(args);
            }

            /// <summary>
            /// This method is called once the request query is constructed.
            /// </summary>
            /// <param name="query">The query which is going to be executed against the provider.</param>
            public void InternalOnRequestQueryConstructed(IQueryable query)
            {
                // Do nothing - batch service doesn't support the test hook
            }

#if ASTORIA_FF_CALLBACKS
            /// <summary>
            /// Invoked once feed has been written to override the feed elements
            /// </summary>
            /// <param name="feed">Feed being written</param>
            void IDataService.InternalOnWriteFeed(SyndicationFeed feed)
            {
                this.dataService.InternalOnWriteFeed(feed);
            }

            /// <summary>
            /// Invoked once an element has been written to override the element
            /// </summary>
            /// <param name="item">Item that has been written</param>
            /// <param name="obj">Object with content for the <paramref name="item"/></param>
            void IDataService.InternalOnWriteItem(SyndicationItem item, object obj)
            {
                this.dataService.InternalOnWriteItem(item, obj);
            }
#endif
            /// <summary>
            /// Returns the segmentInfo of the resource referred by the given content Id;
            /// </summary>
            /// <param name="contentId">content id for a operation in the batch request.</param>
            /// <returns>segmentInfo for the resource referred by the given content id.</returns>
            public SegmentInfo GetSegmentForContentId(string contentId)
            {
                if (contentId.StartsWith("$", StringComparison.Ordinal))
                {
                    SegmentInfo segmentInfo;
                    this.contentIdsToSegmentInfoMapping.TryGetValue(contentId.Substring(1), out segmentInfo);
                    return segmentInfo;
                }

                return null;
            }

            /// <summary>
            /// Get the resource referred by the segment in the request with the given index
            /// </summary>
            /// <param name="description">description about the request url.</param>
            /// <param name="segmentIndex">index of the segment that refers to the resource that needs to be returned.</param>
            /// <param name="typeFullName">typename of the resource.</param>
            /// <returns>the resource as returned by the provider.</returns>
            public object GetResource(RequestDescription description, int segmentIndex, string typeFullName)
            {
                if (Deserializer.IsCrossReferencedSegment(description.SegmentInfos[0], this))
                {
                    Debug.Assert(segmentIndex >= 0 && segmentIndex < description.SegmentInfos.Length, "segment index must be a valid one");
                    if (description.SegmentInfos[segmentIndex].RequestEnumerable == null)
                    {
                        object resource = Deserializer.GetCrossReferencedResource(description.SegmentInfos[0]);
                        for (int i = 1; i <= segmentIndex; i++)
                        {
                            resource = this.Updatable.GetValue(resource, description.SegmentInfos[i].Identifier);
                            if (resource == null)
                            {
                                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_DereferencingNullPropertyValue(description.SegmentInfos[i].Identifier));
                            }

                            description.SegmentInfos[i].RequestEnumerable = new object[] { resource };
                        }

                        return resource;
                    }
                    else
                    {
                        return Deserializer.GetCrossReferencedResource(description.SegmentInfos[segmentIndex]);
                    }
                }

                return Deserializer.GetResource(description.SegmentInfos[segmentIndex], typeFullName, this, false /*checkForNull*/);
            }

            /// <summary>
            /// Dispose the data source instance
            /// </summary>
            public void DisposeDataSource()
            {
#if DEBUG
                this.dataService.ProcessingPipeline.AssertDebugStateAtDispose();
                this.dataService.ProcessingPipeline.HasDisposedProviderInterfaces = true;
#endif
                if (this.updatable != null)
                {
                    this.updatable.DisposeProvider();
                    this.updatable = null;
                }

                if (this.pagingProvider != null)
                {
                    this.pagingProvider.DisposeProvider();
                    this.pagingProvider = null;
                }

                if (this.streamProvider != null)
                {
                    this.streamProvider.DisposeProvider();
                    this.streamProvider = null;
                }

                this.dataService.DisposeDataSource();
            }

            /// <summary>
            /// This method is called before a request is processed.
            /// </summary>
            /// <param name="args">Information about the request that is going to be processed.</param>
            public void InternalOnStartProcessingRequest(ProcessRequestArgs args)
            {
                this.dataService.InternalOnStartProcessingRequest(args);
            }

            #endregion

            /// <summary>
            /// Handle the batch content
            /// </summary>
            /// <param name="responseStream">response stream for writing batch response</param>
            internal void HandleBatchContent(Stream responseStream)
            {
                DataServiceOperationContext currentOperationContext = null;
                RequestDescription description;
                string changesetBoundary = null;
                Exception exceptionEncountered = null;
                bool serviceOperationRequests = true;

                try
                {
                    // After we have completely read the request, we should not close 
                    // the request stream, since its owned by the underlying host.
                    StreamWriter writer = new StreamWriter(responseStream, HttpProcessUtility.FallbackEncoding);
                    while (!this.batchLimitExceeded && this.batchRequestStream.State != BatchStreamState.EndBatch)
                    {
                        // clear the context from the last operation
                        this.operationContext = null;

                        // If we encounter any error while reading the batch request,
                        // we write out the exception message and return. We do not try
                        // and read the request further.
                        try
                        {
                            this.batchRequestStream.MoveNext();
                        }
                        catch (Exception exception)
                        {
                            if (!WebUtil.IsCatchableExceptionType(exception))
                            {
                                throw;
                            }

                            ErrorHandler.HandleBatchRequestException(this, exception, writer);
                            break;
                        }

                        try
                        {
                            switch (this.batchRequestStream.State)
                            {
                                case BatchStreamState.BeginChangeSet:
                                    this.IncreaseBatchCount();
                                    changesetBoundary = XmlConstants.HttpMultipartBoundaryChangesetResponse + '_' + Guid.NewGuid().ToString();
                                    BatchWriter.WriteStartBatchBoundary(writer, this.batchBoundary, changesetBoundary);

                                    // Query Processing Pipeline - Changeset start event
                                    // Note we only invoke the event handler for CUD operations
                                    this.dataService.ProcessingPipeline.InvokeProcessingChangeset(this.dataService, new EventArgs());
                                    break;

                                case BatchStreamState.EndChangeSet:
                                    #region EndChangeSet
                                    this.changeSetElementCount = 0;
                                    this.contentIdsToSegmentInfoMapping.Clear();

                                    // In case of exception, the changeset boundary will be set to null.
                                    // for that case, just write the end boundary and continue
                                    if (exceptionEncountered == null && this.batchRequestDescription.Count > 0)
                                    {
                                        Debug.Assert(!String.IsNullOrEmpty(changesetBoundary), "!String.IsNullOrEmpty(changesetBoundary)");

                                        // Bug 470090: We don't need to call SaveChanges if all requests in the changesets are requests to
                                        // service operations. But in V1, we used to call SaveChanges, we need to keep calling it, if its
                                        // implemented.
                                        if (serviceOperationRequests)
                                        {
                                            if (this.Provider.IsV1ProviderAndImplementsUpdatable())
                                            {
                                                this.Updatable.SaveChanges();
                                            }
                                        }
                                        else
                                        {
                                            // Save all the changes and write the response
                                            this.Updatable.SaveChanges();
                                        }
                                    }

                                    if (exceptionEncountered == null)
                                    {
                                        // Query Processing Pipeline - Changeset end event
                                        // Note 1 we only invoke the event handler for CUD operations
                                        // Note 2 we invoke this event immediately after SaveChanges()
                                        // Note 3 we invoke this event before serialization happens
                                        this.dataService.ProcessingPipeline.InvokeProcessedChangeset(this.dataService, new EventArgs());

                                        Debug.Assert(this.batchOperationContexts.Count == this.batchRequestDescription.Count, "counts must be the same");
                                        for (int i = 0; i < this.batchRequestDescription.Count; i++)
                                        {
                                            this.operationContext = this.batchOperationContexts[i];
                                            this.WriteRequest(this.batchRequestDescription[i], this.batchOperationContexts[i].Host.BatchServiceHost);
                                        }

                                        BatchWriter.WriteEndBoundary(writer, changesetBoundary);
                                    }
                                    else
                                    {
                                        this.HandleChangesetException(exceptionEncountered, this.batchOperationContexts, changesetBoundary, writer);
                                    }

                                    break;
                                    #endregion //EndChangeSet
                                case BatchStreamState.Get:
                                    #region GET Operation
                                    this.IncreaseBatchCount();
                                    currentOperationContext = CreateOperationContextFromBatchStream(
                                        this.dataService.OperationContext.AbsoluteServiceUri,
                                        this.batchRequestStream,
                                        this.contentIds,
                                        this.batchBoundary,
                                        writer);
                                    this.operationContext = currentOperationContext;

                                    // it must be GET operation
                                    Debug.Assert(this.operationContext.Host.AstoriaHttpVerb == AstoriaVerbs.GET, "this.operationContext.Host.AstoriaHttpVerb == AstoriaVerbs.GET");
                                    Debug.Assert(this.batchRequestDescription.Count == 0, "this.batchRequestDescription.Count == 0");
                                    Debug.Assert(this.batchOperationContexts.Count == 0, "this.batchRequestHost.Count == 0");

                                    this.dataService.InternalOnStartProcessingRequest(new ProcessRequestArgs(this.operationContext.AbsoluteRequestUri, true /*isBatchOperation*/, this.operationContext));
                                    description = RequestUriProcessor.ProcessRequestUri(this.operationContext.AbsoluteRequestUri, this);
                                    description = ProcessIncomingRequest(description, this);
                                    this.WriteRequest(description, currentOperationContext.Host.BatchServiceHost);
                                    break;
                                    #endregion // GET Operation
                                case BatchStreamState.Post:
                                case BatchStreamState.Put:
                                case BatchStreamState.Delete:
                                case BatchStreamState.Merge:
                                    #region CUD Operation
                                    // if we encounter an error, we ignore rest of the operations
                                    // within a changeset.
                                    this.IncreaseChangeSetCount();
                                    currentOperationContext = CreateOperationContextFromBatchStream(this.dataService.OperationContext.AbsoluteServiceUri, this.batchRequestStream, this.contentIds, changesetBoundary, writer);
                                    if (exceptionEncountered == null)
                                    {
                                        this.batchOperationContexts.Add(currentOperationContext);
                                        this.operationContext = currentOperationContext;

                                        this.dataService.InternalOnStartProcessingRequest(new ProcessRequestArgs(this.operationContext.AbsoluteRequestUri, true /*isBatchOperation*/, this.operationContext));
                                        description = RequestUriProcessor.ProcessRequestUri(this.operationContext.AbsoluteRequestUri, this);

                                        // If there are all batch requests in the changeset, then we don't need to call SaveChanges()
                                        serviceOperationRequests &= (description.TargetSource == RequestTargetSource.ServiceOperation);

                                        description = ProcessIncomingRequest(description, this);
                                        this.batchRequestDescription.Add(description);

                                        // In Link case, we do not write any response out. hence the description will be null
                                        if (description != null)
                                        {
                                            if (this.batchRequestStream.State == BatchStreamState.Post)
                                            {
                                                Debug.Assert(
                                                        description.TargetKind == RequestTargetKind.Resource || description.TargetSource == RequestTargetSource.ServiceOperation,
                                                        "The target must be a resource or source should be a service operation, since otherwise cross-referencing doesn't make sense");

                                                // if the content id is specified, only then add it to the collection
                                                string contentId = currentOperationContext.Host.BatchServiceHost.ContentId;
                                                if (contentId != null)
                                                {
                                                    this.contentIdsToSegmentInfoMapping.Add(contentId, description.LastSegmentInfo);
                                                }
                                            }
                                            else if (this.batchRequestStream.State == BatchStreamState.Put)
                                            {
                                                // If this is a cross-referencing a previous POST resource, then we need to
                                                // replace the resource in the previous POST request with the new resource
                                                // that the provider returned for this request so that while serializing out,
                                                // we will have the same instance for POST/PUT
                                                this.UpdateRequestEnumerableForPut(description);
                                            }
                                        }
                                    }

                                    break;
                                    #endregion // CUD Operation
                                default:
                                    Debug.Assert(this.batchRequestStream.State == BatchStreamState.EndBatch, "expecting end batch state");

                                    // Query Processing Pipeline - Request end event
                                    // Note 1 we only invoke the event handler for ALL operations
                                    // Note 2 we invoke this event before serialization is complete
                                    // Note 3 we invoke this event before any provider interface held by the data service runtime is released/disposed
                                    DataServiceProcessingPipelineEventArgs eventArg = new DataServiceProcessingPipelineEventArgs(this.dataService.OperationContext);
                                    this.dataService.ProcessingPipeline.InvokeProcessedRequest(this.dataService, eventArg);
                                    break;
                            }
                        }
                        catch (Exception exception)
                        {
                            if (!WebUtil.IsCatchableExceptionType(exception))
                            {
                                throw;
                            }

                            if (this.batchRequestStream.State == BatchStreamState.EndChangeSet)
                            {
                                this.HandleChangesetException(exception, this.batchOperationContexts, changesetBoundary, writer);
                            }
                            else if (this.batchRequestStream.State == BatchStreamState.Post ||
                                     this.batchRequestStream.State == BatchStreamState.Put ||
                                     this.batchRequestStream.State == BatchStreamState.Delete ||
                                     this.batchRequestStream.State == BatchStreamState.Merge)
                            {
                                // Store the exception if its in the middle of the changeset,
                                // we need to write the same exception for every
                                exceptionEncountered = exception;
                            }
                            else
                            {
                                DataServiceHostWrapper currentHost = this.operationContext == null ? null : this.operationContext.Host;
                                if (currentHost == null)
                                {
                                    // For error cases (like we encounter an error while parsing request headers
                                    // and were not able to create the host), we need to create a dummy host
                                    currentHost = new DataServiceHostWrapper(new BatchServiceHost(this.batchBoundary, writer));
                                }

                                ErrorHandler.HandleBatchProcessException(this, currentHost, exception, writer);
                            }
                        }
                        finally
                        {
                            // Once the end of the changeset is reached, clear the error state
                            if (this.batchRequestStream.State == BatchStreamState.EndChangeSet)
                            {
                                exceptionEncountered = null;
                                changesetBoundary = null;
                                this.batchRequestDescription.Clear();
                                this.batchOperationContexts.Clear();
                            }
                        }
                    }

                    BatchWriter.WriteEndBoundary(writer, this.batchBoundary);
                    writer.Flush();

                    Exception ex = this.batchRequestStream.ValidateNoDataBeyondEndOfBatch();
                    if (ex != null)
                    {
                        ErrorHandler.HandleBatchRequestException(this, ex, writer);
                        writer.Flush();
                    }
                }
#if DEBUG
                catch
                {
                    // We've hit an error, some of the processing pipeline events will not get fired. Setting this flag to skip validation.
                    this.ProcessingPipeline.SkipDebugAssert = true;
                    throw;
                }
#endif
                finally
                {
                    this.DisposeDataSource();
                }
            }

            #region Private methods.

            /// <summary>
            /// Creates an operation context for the current batch operation
            /// </summary>
            /// <param name="absoluteServiceUri">Absolute service uri</param>
            /// <param name="batchStream">batch stream which contains the header information.</param>
            /// <param name="contentIds">content ids that are defined in the batch.</param>
            /// <param name="boundary">Part separator for host.</param>
            /// <param name="writer">Output writer.</param>
            /// <returns>instance of the operation context which represents the current operation.</returns>
            private static DataServiceOperationContext CreateOperationContextFromBatchStream(Uri absoluteServiceUri, BatchStream batchStream, HashSet<int> contentIds, string boundary, StreamWriter writer)
            {
                Debug.Assert(absoluteServiceUri != null && absoluteServiceUri.IsAbsoluteUri, "absoluteServiceUri != null && absoluteServiceUri.IsAbsoluteUri");
                Debug.Assert(batchStream != null, "batchStream != null");
                Debug.Assert(boundary != null, "boundary != null");

                // If the Content-ID header is defined, it should be unique.
                string contentIdValue;
                if (batchStream.ContentHeaders.TryGetValue(XmlConstants.HttpContentID, out contentIdValue) && !String.IsNullOrEmpty(contentIdValue))
                {
                    int contentId;
                    if (!Int32.TryParse(contentIdValue, System.Globalization.NumberStyles.Integer, System.Globalization.NumberFormatInfo.InvariantInfo, out contentId))
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.DataService_ContentIdMustBeAnInteger(contentIdValue));
                    }

                    if (!contentIds.Add(contentId))
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.DataService_ContentIdMustBeUniqueInBatch(contentId));
                    }
                }

                BatchServiceHost host = new BatchServiceHost(absoluteServiceUri, batchStream, contentIdValue, boundary, writer);
                DataServiceOperationContext operationContext = new DataServiceOperationContext(true /*isBatchRequest*/, host);
                operationContext.InitializeAndCacheHeaders();
                return operationContext;
            }

            /// <summary>
            /// Write the exception encountered in the middle of the changeset to the response
            /// </summary>
            /// <param name="exception">exception encountered</param>
            /// <param name="changesetOperationContexts">list of operation contexts in the changeset</param>
            /// <param name="changesetBoundary">changeset boundary for the current processing changeset</param>
            /// <param name="writer">writer to which the response needs to be written</param>
            private void HandleChangesetException(
                Exception exception, 
                List<DataServiceOperationContext> changesetOperationContexts, 
                string changesetBoundary, 
                StreamWriter writer)
            {
                Debug.Assert(exception != null, "exception != null");
                Debug.Assert(changesetOperationContexts != null, "changesetOperationContexts != null");
                Debug.Assert(WebUtil.IsCatchableExceptionType(exception), "WebUtil.IsCatchableExceptionType(exception)");

                // For a changeset, we need to write the exception only once. Since we ignore all the changesets
                // after we encounter an error, its the last changeset which had error. For cases, which we don't
                // know, (like something in save changes, etc), we will still write the last operation information.
                // If there are no host, then just pass null.
                DataServiceHostWrapper currentHost = null;
                DataServiceOperationContext currentContext = changesetOperationContexts.Count == 0 ? null : changesetOperationContexts[changesetOperationContexts.Count - 1];
                if (currentContext == null || currentContext.Host == null)
                {
                    currentHost = new DataServiceHostWrapper(new BatchServiceHost(changesetBoundary, writer));
                }
                else
                {
                    currentHost = currentContext.Host;
                }

                ErrorHandler.HandleBatchProcessException(this, currentHost, exception, writer);

                // Write end boundary for the changeset
                BatchWriter.WriteEndBoundary(writer, changesetBoundary);
                this.Updatable.ClearChanges();
            }

            /// <summary>Increases the count of batch changsets/queries found, and checks it is within limits.</summary>
            private void IncreaseBatchCount()
            {
                checked
                {
                    this.batchElementCount++;
                }
                
                if (this.batchElementCount > this.dataService.Configuration.MaxBatchCount)
                {
                    this.batchLimitExceeded = true;
                    throw new DataServiceException(400, Strings.DataService_BatchExceedMaxBatchCount(this.dataService.Configuration.MaxBatchCount));
                }
            }

            /// <summary>Increases the count of changeset CUD operations found, and checks it is within limits.</summary>
            private void IncreaseChangeSetCount()
            {
                checked
                {
                    this.changeSetElementCount++;
                }
                
                if (this.changeSetElementCount > this.dataService.Configuration.MaxChangesetCount)
                {
                    throw new DataServiceException(400, Strings.DataService_BatchExceedMaxChangeSetCount(this.dataService.Configuration.MaxChangesetCount));
                }
            }

            /// <summary>
            ///  For POST operations, the RequestEnumerable could be out of date
            ///  when a PUT is referring to the POST within the changeset.
            ///  We need to update the RequestEnumerable to reflect what actually
            ///  happened to the database.
            /// </summary>
            /// <param name="requestDescription">description for the current request.</param>
            private void UpdateRequestEnumerableForPut(RequestDescription requestDescription)
            {
                Debug.Assert(this.batchRequestStream.State == BatchStreamState.Put, "This method must be called only for PUT requests");
                Debug.Assert(this.batchRequestDescription[this.batchRequestDescription.Count - 1] == requestDescription, "The current request description must be the last one");
                Debug.Assert(this.batchRequestDescription.Count == this.batchOperationContexts.Count, "Host and request description count must be the same");

                // If this PUT request is cross referencing some resource
                string identifier = requestDescription.SegmentInfos[0].Identifier;
                if (identifier.StartsWith("$", StringComparison.Ordinal))
                {
                    // Get the content id of the POST request that is being cross-referenced
                    string contentId = identifier.Substring(1);

                    // Now we need to scan all the previous request to find the 
                    // POST request resource which is cross-referenced by the current request
                    // and replace the resource in the POST request by the current one.

                    // Note: since today we do not return payloads in the PUT request, this is fine.
                    // When we support that, we need to find all the PUT requests that also refers
                    // to the same resource and replace it with the current version.

                    // Ignore the last one, since the parameters to the method are the last ones.
                    for (int i = 0; i < this.batchOperationContexts.Count - 1; i++)
                    {
                        DataServiceOperationContext previousContext = this.batchOperationContexts[i];
                        BatchServiceHost previousHost = previousContext.Host.BatchServiceHost;
                        RequestDescription previousRequest = this.batchRequestDescription[i];

                        if (previousContext.Host.AstoriaHttpVerb == AstoriaVerbs.POST && previousHost.ContentId == contentId)
                        {
                            object resource = Deserializer.GetCrossReferencedResource(requestDescription.LastSegmentInfo);
                            previousRequest.LastSegmentInfo.RequestEnumerable = new object[] { resource };
                            break;
                        }
                    }
                }
            }

            /// <summary>
            /// Write the response for the given request, if required.
            /// </summary>
            /// <param name="description">description of the request uri. If this is null, means that no response needs to be written</param>
            /// <param name="batchHost">Batch host for which the request should be written.</param>
            private void WriteRequest(RequestDescription description, BatchServiceHost batchHost)
            {
                Debug.Assert(batchHost != null, "host != null");

                // For DELETE operations, description will be null
                if (description == null)
                {
                    BatchWriter.WriteBoundaryAndHeaders(batchHost.Writer, batchHost, batchHost.ContentId, batchHost.BoundaryString);
                }
                else
                {
                    Action<Stream> responseWriter = DataService<T>.SerializeResponseBody(description, this);
                    if (responseWriter != null)
                    {
                        BatchWriter.WriteBoundaryAndHeaders(batchHost.Writer, batchHost, batchHost.ContentId, batchHost.BoundaryString);
                        batchHost.Writer.Flush();
                        responseWriter(batchHost.Writer.BaseStream);
                        batchHost.Writer.WriteLine();
                    }
                    else
                    {
                        BatchWriter.WriteBoundaryAndHeaders(batchHost.Writer, batchHost, batchHost.ContentId, batchHost.BoundaryString);
                    }
                }
            }

            #endregion Private methods.
        }

        /// <summary>
        /// For performance reasons we reuse results from existing query to read a projected value. We create an enumerator
        /// containing the projected value but must not dispose the original query until later. This wrapper allows us to 
        /// pass the created enumerator and dispose the query at the right time. 
        /// </summary>
        private class QueryResultsWrapper : IEnumerator, IDisposable
        {
            /// <summary>
            /// Query that needs to be disposed.
            /// </summary>
            private IEnumerator query;

            /// <summary>
            /// Enumerator containing the projected property.
            /// </summary>
            private IEnumerator enumerator;

            /// <summary>
            /// QueryResultsWrapper constructor
            /// </summary>
            /// <param name="enumerator">Enumerator containing the projected value.</param>
            /// <param name="query">Query that needs to be disposed.</param>
            public QueryResultsWrapper(IEnumerator enumerator, IEnumerator query)
            {
                Debug.Assert(enumerator != null, "enumerator != null");
                this.enumerator = enumerator;
                this.query = query;
            }

            #region IEnumerator Members

            /// <summary>
            /// Gets the current element from enumerator.
            /// </summary>
            object IEnumerator.Current
            {
                get { return this.enumerator.Current; }
            }

            /// <summary>
            /// Moves the enumerator to the next element. 
            /// </summary>
            /// <returns>true if the enumerator moved;false if the enumerator reached the end of the collection.</returns>
            bool IEnumerator.MoveNext()
            {
                return this.enumerator.MoveNext();
            }

            /// <summary>
            /// Resets the enumerator to the initial position.
            /// </summary>
            void IEnumerator.Reset()
            {
                this.enumerator.Reset();
            }

            #endregion

            #region IDisposable Members
            /// <summary>
            /// Disposes the cached query.
            /// </summary>
            void IDisposable.Dispose()
            {
                WebUtil.Dispose(this.query);
                GC.SuppressFinalize(this);
            }

            #endregion
        }
    }
}
