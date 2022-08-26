//---------------------------------------------------------------------
// <copyright file="DataServiceContext.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// context
// </summary>
//---------------------------------------------------------------------

// #define TESTUNIXNEWLINE

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Data.Services.Common;
#if !ASTORIA_LIGHT
    using System.Net;
#else // Data.Services http stack
    using System.Data.Services.Http;
#endif
    using System.Text;
    using System.Xml;
    using System.Xml.Linq;

    #endregion Namespaces.

    /// <summary>
    /// context
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Maintainability", "CA1506", Justification = "Central class of the API, likely to have many cross-references")]
    public class DataServiceContext
    {
#if !TESTUNIXNEWLINE
        /// <summary>cache NewLine string</summary>
        private static readonly string NewLine = System.Environment.NewLine;
#else
        /// <summary>cache NewLine string</summary>
        private const string NewLine = "\n";
#endif

        /// <summary>base uri prepended to relative uri</summary>
        private readonly System.Uri baseUri;

        /// <summary>base uri with guranteed trailing slash</summary>
        private readonly System.Uri baseUriWithSlash;

#if !ASTORIA_LIGHT  // Credentials not available
        /// <summary>Authentication interface for retrieving credentials for Web client authentication.</summary>
        private System.Net.ICredentials credentials;
#endif

        /// <summary>Override the namespace used for the data parts of the ATOM entries</summary>
        private string dataNamespace;

        /// <summary>resolve type from a typename</summary>
        private Func<Type, string> resolveName;

        /// <summary>resolve typename from a type</summary>
        private Func<string, Type> resolveType;

#if !ASTORIA_LIGHT  // Timeout not available
        /// <summary>time-out value in seconds, 0 for default</summary>
        private int timeout;
#endif

        /// <summary>whether to use post-tunneling for PUT/DELETE</summary>
        private bool postTunneling;

        /// <summary>Options when deserializing properties to the target type.</summary>
        private bool ignoreMissingProperties;

        /// <summary>Used to specify a value synchronization strategy.</summary>
        private MergeOption mergeOption;

        /// <summary>Default options to be used while doing savechanges.</summary>
        private SaveChangesOptions saveChangesDefaultOptions;

        /// <summary>Override the namespace used for the scheme in the category for ATOM entries.</summary>
        private Uri typeScheme;

        /// <summary>Client will ignore 404 resource not found exception and return an empty set when this is set to true</summary>
        private bool ignoreResourceNotFoundException;

#if ASTORIA_LIGHT // Multiple HTTP stacks for Silverlight
        /// <summary>The HTTP stack to use for requests.</summary>
        private HttpStack httpStack;
#endif

        #region Resource state management

        /// <summary>change order</summary>
        private uint nextChange;

        /// <summary>Set of tracked resources</summary>
        private Dictionary<object, EntityDescriptor> entityDescriptors = new Dictionary<object, EntityDescriptor>(EqualityComparer<object>.Default);

        /// <summary>Set of tracked resources by Identity</summary>
        private Dictionary<String, EntityDescriptor> identityToDescriptor;

        /// <summary>Set of tracked bindings</summary>
        private Dictionary<LinkDescriptor, LinkDescriptor> bindings = new Dictionary<LinkDescriptor, LinkDescriptor>(LinkDescriptor.EquivalenceComparer);

        /// <summary>
        /// A flag indicating if the data service context is applying changes 
        /// </summary>
        private bool applyingChanges;

        #endregion

        #region ctor

        /// <summary>
        /// Instantiates a new context with the specified <paramref name="serviceRoot"/> Uri.
        /// The library expects the Uri to point to the root of a data service,
        /// but does not issue a request to validate it does indeed identify the root of a service.
        /// If the Uri does not identify the root of the service, the behavior of the client library is undefined.    
        /// </summary>
        /// <param name="serviceRoot">
        /// An absolute, well formed http or https URI without a query or fragment which identifies the root of a data service.
        /// A Uri provided with a trailing slash is equivalent to one without such a trailing character
        /// </param>
        /// <exception cref="ArgumentException">if the <paramref name="serviceRoot"/> is not an absolute, well formed http or https URI without a query or fragment</exception>
        /// <exception cref="ArgumentNullException">when the <paramref name="serviceRoot"/> is null</exception>
        /// <remarks>
        /// With Silverlight, the <paramref name="serviceRoot"/> can be a relative Uri
        /// that will be combined with System.Windows.Browser.HtmlPage.Document.DocumentUri.
        /// </remarks>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1062:Validate arguments of public methods", MessageId = "0", Justification = "parameters are validated against null via CheckArgumentNull")]
        public DataServiceContext(Uri serviceRoot)
        {
            Util.CheckArgumentNull(serviceRoot, "serviceRoot");

#if ASTORIA_LIGHT
            if (!serviceRoot.IsAbsoluteUri)
            {
                // If we can use XHR, we will and thus we should use the old (V1) way of determining our
                //   base URI. That is, use the uri of the HTML page
                if (XHRHttpWebRequest.IsAvailable())
                {
                    serviceRoot = new Uri(System.Windows.Browser.HtmlPage.Document.DocumentUri, serviceRoot);
                }
                else
                {
                    // Otherwise we are going to use the new Client stack. In this case there might not be any
                    //   HTML page hosting us, or it might not be accessible, so the only base uri we can use
                    //   is the base uri of the xap we're running in.
                    System.Net.WebClient webClient = new System.Net.WebClient();
                    serviceRoot = new Uri(new Uri(webClient.BaseAddress), serviceRoot);
                }
            }
#endif
            if (!serviceRoot.IsAbsoluteUri ||
                !Uri.IsWellFormedUriString(CommonUtil.UriToString(serviceRoot), UriKind.Absolute) ||
                !String.IsNullOrEmpty(serviceRoot.Query) ||
                !string.IsNullOrEmpty(serviceRoot.Fragment) ||
                ((serviceRoot.Scheme != "http") && (serviceRoot.Scheme != "https")))
            {
                throw Error.Argument(Strings.Context_BaseUri, "serviceRoot");
            }

            this.baseUri = serviceRoot;
            this.baseUriWithSlash = serviceRoot;
            string serviceRootString = CommonUtil.UriToString(serviceRoot);
            if (!serviceRootString.EndsWith("/", StringComparison.Ordinal))
            {
                this.baseUriWithSlash = Util.CreateUri(serviceRootString + "/", UriKind.Absolute);
            }

            this.mergeOption = MergeOption.AppendOnly;
            this.DataNamespace = XmlConstants.DataWebNamespace;
#if ASTORIA_LIGHT
            this.UsePostTunneling = true;
#else
            this.UsePostTunneling = false;
#endif
            this.typeScheme = new Uri(XmlConstants.DataWebSchemeNamespace);
#if ASTORIA_LIGHT
            this.httpStack = HttpStack.Auto;
#endif
        }

        #endregion

        /// <summary>
        /// This event is fired before a request it sent to the server, giving
        /// the handler the opportunity to inspect, adjust and/or replace the
        /// WebRequest object used to perform the request.
        /// </summary>
        /// <remarks>
        /// When calling BeginSaveChanges and not using SaveChangesOptions.Batch,
        /// this event may be raised from a different thread.
        /// </remarks>
        public event EventHandler<SendingRequestEventArgs> SendingRequest;

        /// <summary>
        /// This event fires once an entry has been read into a .NET object
        /// but before the serializer returns to the caller, giving handlers
        /// an opporunity to read further information from the incoming ATOM
        /// entry and updating the object
        /// </summary>
        /// <remarks>
        /// This event should only be raised from the thread that was used to
        /// invoke Execute, EndExecute, SaveChanges, EndSaveChanges.
        /// </remarks>
        public event EventHandler<ReadingWritingEntityEventArgs> ReadingEntity;

        /// <summary>
        /// This event fires once an ATOM entry is ready to be written to
        /// the network for a request, giving handlers an opportunity to
        /// customize the entry with information from the corresponding
        /// .NET object or the environment.
        /// </summary>
        /// <remarks>
        /// When calling BeginSaveChanges and not using SaveChangesOptions.Batch,
        /// this event may be raised from a different thread.
        /// </remarks>
        public event EventHandler<ReadingWritingEntityEventArgs> WritingEntity;

        /// <summary>
        /// This event fires when SaveChanges or EndSaveChanges is called
        /// </summary>
        internal event EventHandler<SaveChangesEventArgs> ChangesSaved;

        #region BaseUri, Credentials, MergeOption, Timeout, Links, Entities
        /// <summary>
        /// Absolute Uri identifying the root of the target data service.
        /// A Uri provided with a trailing slash is equivalent to one without such a trailing character.
        /// </summary>
        /// <remarks>
        /// Example: http://server/host/myservice.svc
        /// </remarks>
        public Uri BaseUri
        {
            get { return this.baseUri; }
        }

#if !ASTORIA_LIGHT  // Credentials not available
        /// <summary>
        /// Gets and sets the authentication information used by each query created using the context object.
        /// </summary>
        public System.Net.ICredentials Credentials
        {
            get { return this.credentials; }
            set { this.credentials = value; }
        }
#endif

        /// <summary>
        /// Used to specify a synchronization strategy when sending/receiving entities to/from a data service.
        /// This value is read by the deserialization component of the client prior to materializing objects.
        /// As such, it is recommended to set this property to the appropriate materialization strategy
        /// before executing any queries/updates to the data service.
        /// </summary>
        /// <remarks>
        /// The default value is <see cref="MergeOption"/>.AppendOnly.
        /// </remarks>
        public MergeOption MergeOption
        {
            get { return this.mergeOption; }
            set { this.mergeOption = Util.CheckEnumerationValue(value, "MergeOption"); }
        }

        /// <summary>
        /// A flag indicating if the data service context is applying changes 
        /// </summary>
        public bool ApplyingChanges
        {
            get { return this.applyingChanges; }
            internal set { this.applyingChanges = value; }
        }

        /// <summary>
        /// Are properties missing from target type ignored? 
        /// </summary>
        /// <remarks>
        /// This also affects responses during SaveChanges.
        /// </remarks>
        public bool IgnoreMissingProperties
        {
            get { return this.ignoreMissingProperties; }
            set { this.ignoreMissingProperties = value; }
        }

        /// <summary>Override the namespace used for the data parts of the ATOM entries</summary>
        public string DataNamespace
        {
            get
            {
                return this.dataNamespace;
            }

            set
            {
                Util.CheckArgumentNull(value, "value");
                this.dataNamespace = value;
            }
        }

        /// <summary>
        /// Enables one to override the default type resolution strategy used by the client library.
        /// Set this property to a delegate which identifies a function that resolves
        /// a type within the client application to a namespace-qualified type name.
        /// This enables the client to perform custom mapping between the type name
        /// provided in a response from the server and a type on the client.
        /// </summary>
        /// <remarks>
        /// This method enables one to override the entity name that is serialized
        /// to the target representation (ATOM,JSON, etc) for the specified type.
        /// </remarks>
        public Func<Type, string> ResolveName
        {
            get { return this.resolveName; }
            set { this.resolveName = value; }
        }

        /// <summary>
        /// Enables one to override the default type resolution strategy used by the client library.
        /// Set this property to a delegate which identifies a function that resolves a 
        /// namespace-qualified type name to type within the client application.
        /// This enables the client to perform custom mapping between the type name
        /// provided in a response from the server and a type on the client.
        /// </summary>
        /// <remarks>
        /// Overriding type resolution enables inserting a custom type name to type mapping strategy.
        /// It does not enable one to affect how a response is materialized into the identified type.
        /// </remarks>
        public Func<string, Type> ResolveType
        {
            get { return this.resolveType; }
            set { this.resolveType = value; }
        }

#if !ASTORIA_LIGHT  // Timeout not available
        /// <summary>
        /// Get and sets the timeout span in seconds to use for the underlying HTTP request to the data service.
        /// </summary>
        /// <remarks>
        /// A value of 0 will use the default timeout of the underlying HTTP request.
        /// This value must be set before executing any query or update operations against
        /// the target data service for it to have effect on the on the request.
        /// The value may be changed between requests to a data service and the new value
        /// will be picked up by the next data service request.  
        /// </remarks>
        public int Timeout
        {
            get
            {
                return this.timeout;
            }

            set
            {
                if (value < 0)
                {
                    throw Error.ArgumentOutOfRange("Timeout");
                }

                this.timeout = value;
            }
        }
#endif

        /// <summary>Gets or sets the URI used to indicate what type scheme is used by the service.</summary>
        public Uri TypeScheme
        {
            get
            {
                return this.typeScheme;
            }

            set
            {
                Util.CheckArgumentNull(value, "value");
                this.typeScheme = value;
            }
        }

        /// <summary>whether to use post-tunneling for PUT/DELETE</summary>
        public bool UsePostTunneling
        {
            get { return this.postTunneling; }
            set { this.postTunneling = value; }
        }

        /// <summary>
        /// Returns a collection of all the links (ie. associations) currently being tracked by the context.
        /// If no links are being tracked, a collection with 0 elements is returned.
        /// </summary>
        public ReadOnlyCollection<LinkDescriptor> Links
        {
            get
            {
                return this.bindings.Values.OrderBy(l => l.ChangeOrder).ToList().AsReadOnly();
            }
        }

        /// <summary>
        /// Returns a collection of all the resources currently being tracked by the context.
        /// If no resources are being tracked, a collection with 0 elements is returned.
        /// </summary>
        public ReadOnlyCollection<EntityDescriptor> Entities
        {
            get
            {
                return this.entityDescriptors.Values.OrderBy(d => d.ChangeOrder).ToList().AsReadOnly();
            }
        }

        /// <summary>
        /// Default SaveChangesOptions that needs to be used when doing SaveChanges.
        /// </summary>
        public SaveChangesOptions SaveChangesDefaultOptions
        {
            get
            {
                return this.saveChangesDefaultOptions;
            }

            set
            {
                ValidateSaveChangesOptions(value);
                this.saveChangesDefaultOptions = value;
            }
        }

        #endregion

        /// <summary>
        /// When set to true, client will return an empty set instead of throwing when the
        /// server generates a HTTP 404: Resource Not Found exception
        /// </summary>
        public bool IgnoreResourceNotFoundException
        {
            get { return this.ignoreResourceNotFoundException; }
            set { this.ignoreResourceNotFoundException = value; }
        }

#if ASTORIA_LIGHT // Multiple HTTP stacks in Silverlight
        /// <summary>
        /// The HTTP stack to use in Silverlight.
        /// Default value is HttpStack.Auto.
        /// </summary>
        public HttpStack HttpStack
        {
            get { return this.httpStack; }
            set { this.httpStack = Util.CheckEnumerationValue(value, "HttpStack"); }
        }
#endif

        /// <summary>base uri with guranteed trailing slash</summary>
        internal Uri BaseUriWithSlash
        {
            get { return this.baseUriWithSlash; }
        }

        /// <summary>Indicates if there are subscribers for the ReadingEntity event</summary>
        internal bool HasReadingEntityHandlers
        {
            [DebuggerStepThrough]
            get { return this.ReadingEntity != null; }
        }

        #region Entity and Link Tracking
        
        /// <summary>Gets the entity descriptor corresponding to a particular entity</summary>
        /// <param name="entity">Entity for which to find the entity descriptor</param>
        /// <returns>EntityDescriptor for the <paramref name="entity"/> or null if not found</returns>
        public EntityDescriptor GetEntityDescriptor(object entity)
        {
            Util.CheckArgumentNull(entity, "entity");

            EntityDescriptor descriptor;
            if (this.entityDescriptors.TryGetValue(entity, out descriptor))
            {
                return descriptor;
            }
            else
            {
                return null;
            }
        }
        
        /// <summary>
        /// Gets the link descriptor corresponding to a particular link b/w source and target objects
        /// </summary>
        /// <param name="source">Source entity</param>
        /// <param name="sourceProperty">Property of <paramref name="source"/></param>
        /// <param name="target">Target entity</param>
        /// <returns>LinkDescriptor for the relationship b/w source and target entities or null if not found</returns>
        public LinkDescriptor GetLinkDescriptor(object source, string sourceProperty, object target)
        {
            Util.CheckArgumentNull(source, "source");
            Util.CheckArgumentNotEmpty(sourceProperty, "sourceProperty");
            Util.CheckArgumentNull(target, "target");
            
            LinkDescriptor link;
            
            if (this.bindings.TryGetValue(new LinkDescriptor(source, sourceProperty, target), out link))
            {
                return link;
            }
            else
            {
                return null;
            }
        }
        
        #endregion

        #region CancelRequest
        /// <summary>Best effort to abort the outstand request</summary>
        /// <param name="asyncResult">the current async request to cancel</param>
        /// <remarks>DataServiceContext is not safe to use until asyncResult.IsCompleted is true.</remarks>
        public void CancelRequest(IAsyncResult asyncResult)
        {
            Util.CheckArgumentNull(asyncResult, "asyncResult");
            BaseAsyncResult result = asyncResult as BaseAsyncResult;

            // verify this asyncResult orginated from this context or via query from this context
            if ((null == result) || (this != result.Source))
            {
                object context = null;
                DataServiceQuery query = null;
                if (null != result)
                {
                    query = result.Source as DataServiceQuery;

                    if (null != query)
                    {
                        DataServiceQueryProvider provider = query.Provider as DataServiceQueryProvider;
                        if (null != provider)
                        {
                            context = provider.Context;
                        }
                    }
                }

                if (this != context)
                {
                    throw Error.Argument(Strings.Context_DidNotOriginateAsync, "asyncResult");
                }
            }

            // at this point the result originated from this context or a query from this context
            if (!result.IsCompletedInternally)
            {
                result.SetAborted();

                WebRequest request = result.Abortable;
                if (null != request)
                {
                    // with Silverlight we can't wait around to check if the request was aborted
                    // because that would block callbacks for the abort from actually running.
                    request.Abort();
                }
            }
        }
        #endregion

        #region CreateQuery
        /// <summary>
        /// create a query based on (BaseUri + relativeUri)
        /// </summary>
        /// <typeparam name="T">type of object to materialize</typeparam>
        /// <param name="entitySetName">entitySetName</param>
        /// <returns>composible, enumerable query object</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter", Justification = "required for this feature")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1057:StringUriOverloadsCallSystemUriOverloads", Justification = "required for this feature")]
        public DataServiceQuery<T> CreateQuery<T>(string entitySetName)
        {
            Util.CheckArgumentNotEmpty(entitySetName, "entitySetName");
            this.ValidateEntitySetName(ref entitySetName);

            ResourceSetExpression rse = new ResourceSetExpression(typeof(IOrderedQueryable<T>), null, Expression.Constant(entitySetName), typeof(T), null, CountOption.None, null, null);
            return new DataServiceQuery<T>.DataServiceOrderedQuery(rse, new DataServiceQueryProvider(this));
        }
        #endregion

        #region GetMetadataUri
        /// <summary>
        /// Given the base URI, resolves the location of the metadata endpoint for the service by using an HTTP OPTIONS request or falling back to convention ($metadata)
        /// </summary>
        /// <returns>Uri to retrieve metadata from</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1024:UsePropertiesWhereAppropriate", Justification = "required for this feature")]
        public Uri GetMetadataUri()
        {
            // 
            Uri metadataUri = Util.CreateUri(this.baseUriWithSlash.OriginalString + XmlConstants.UriMetadataSegment, UriKind.Absolute);
            return metadataUri;
        }
        #endregion

        #region LoadProperty

        /// <summary>
        /// Begin getting response to load a collection or reference property.
        /// </summary>
        /// <remarks>actually doesn't modify the property until EndLoadProperty is called.</remarks>
        /// <param name="entity">entity</param>
        /// <param name="propertyName">name of collection or reference property to load</param>
        /// <param name="callback">The AsyncCallback delegate.</param>
        /// <param name="state">The state object for this request.</param>
        /// <returns>An IAsyncResult that references the asynchronous request for a response.</returns>
        public IAsyncResult BeginLoadProperty(object entity, string propertyName, AsyncCallback callback, object state)
        {
            return this.BeginLoadProperty(entity, propertyName, (Uri)null, callback, state);
        }

        /// <summary>
        /// Begin getting response to load a page for a collection.
        /// </summary>
        /// <param name="entity">The entity</param>
        /// <param name="propertyName">name of collection or reference property to load</param>
        /// <param name="nextLinkUri">load the page from this URI</param>
        /// <param name="callback">The AsyncCallback delegate.</param>
        /// <param name="state">The state object for this request.</param>
        /// <returns>An IAsyncResult that references the asynchronous request for a response.</returns>
        public IAsyncResult BeginLoadProperty(object entity, string propertyName, Uri nextLinkUri, AsyncCallback callback, object state)
        {
            LoadPropertyResult result = this.CreateLoadPropertyRequest(entity, propertyName, callback, state, nextLinkUri, null);
            result.BeginExecute();
            return result;
        }

        /// <summary>
        /// Begin getting response to load a page for a collection.
        /// </summary>
        /// <param name="entity">The entity</param>
        /// <param name="propertyName">name of collection or reference property to load</param>
        /// <param name="continuation">Continuation from which the property should be loaded.</param>
        /// <param name="callback">The AsyncCallback delegate.</param>
        /// <param name="state">The state object for this request.</param>
        /// <returns>An IAsyncResult that references the asynchronous request for a response.</returns>
        public IAsyncResult BeginLoadProperty(object entity, string propertyName, DataServiceQueryContinuation continuation, AsyncCallback callback, object state)
        {
            Util.CheckArgumentNull(continuation, "continuation");
            LoadPropertyResult result = this.CreateLoadPropertyRequest(entity, propertyName, callback, state, null, continuation);
            result.BeginExecute();
            return result;
        }

        /// <summary>
        /// Load a collection or reference property from a async result.
        /// </summary>
        /// <param name="asyncResult">async result generated by BeginLoadProperty</param>
        /// <returns>QueryOperationResponse instance containing information about the response.</returns>
        public QueryOperationResponse EndLoadProperty(IAsyncResult asyncResult)
        {
            LoadPropertyResult response = QueryResult.EndExecute<LoadPropertyResult>(this, "LoadProperty", asyncResult);
            return response.LoadProperty();
        }

#if !ASTORIA_LIGHT // Synchronous methods not available
        /// <summary>
        /// Load a collection or reference property.
        /// </summary>
        /// <remarks>
        /// An entity in detached or added state will throw an InvalidOperationException
        /// since there is nothing it can load from the server.
        /// 
        /// An entity in unchanged or modified state will load its collection or
        /// reference elements as unchanged with unchanged bindings.
        ///
        /// An entity in deleted state will loads its collection or reference elements
        /// in the unchanged state with bindings in the deleted state.
        /// </remarks>
        /// <param name="entity">entity</param>
        /// <param name="propertyName">name of collection or reference property to load</param>
        /// <returns>QueryOperationResponse instance containing information about the response.</returns>
        public QueryOperationResponse LoadProperty(object entity, string propertyName)
        {
            return this.LoadProperty(entity, propertyName, (Uri)null);
        }

        /// <summary>
        /// Load a page for collection from an uri
        /// </summary>
        /// <remarks>
        /// An entity in detached or added state will throw an InvalidOperationException
        /// since there is nothing it can load from the server.
        /// 
        /// An entity in unchanged or modified state will load its collection or
        /// reference elements as unchanged with unchanged bindings.
        ///
        /// An entity in deleted state will loads its collection or reference elements
        /// in the unchanged state with bindings in the deleted state.
        /// </remarks>
        /// <param name="entity">entity</param>
        /// <param name="propertyName">name of collection or reference property to load</param>
        /// <param name="nextLinkUri">The uri to load the page from; possibly null.</param>
        /// <returns>QueryOperationResponse instance containing information about the response.</returns>
        public QueryOperationResponse LoadProperty(object entity, string propertyName, Uri nextLinkUri)
        {
            LoadPropertyResult result = this.CreateLoadPropertyRequest(entity, propertyName, null, null, nextLinkUri, null);
            result.Execute();
            return result.LoadProperty();
        }

        /// <summary>
        /// Load a page for collection from an uri
        /// </summary>
        /// <remarks>
        /// An entity in detached or added state will throw an InvalidOperationException
        /// since there is nothing it can load from the server.
        /// 
        /// An entity in unchanged or modified state will load its collection or
        /// reference elements as unchanged with unchanged bindings.
        ///
        /// An entity in deleted state will loads its collection or reference elements
        /// in the unchanged state with bindings in the deleted state.
        /// </remarks>
        /// <param name="entity">entity</param>
        /// <param name="propertyName">Name of collection or reference property to load.</param>
        /// <param name="continuation">The continuation object from a previous response.</param>
        /// <returns>QueryOperationResponse instance containing information about the response.</returns>
        public QueryOperationResponse LoadProperty(object entity, string propertyName, DataServiceQueryContinuation continuation)
        {
            LoadPropertyResult result = this.CreateLoadPropertyRequest(entity, propertyName, null, null, null, continuation);
            result.Execute();
            return result.LoadProperty();
        }

        /// <summary>Loads a page for a collection from a continuation object..</summary>
        /// <remarks>
        /// An entity in detached or added state will throw an InvalidOperationException
        /// since there is nothing it can load from the server.
        /// 
        /// An entity in unchanged or modified state will load its collection or
        /// reference elements as unchanged with unchanged bindings.
        ///
        /// An entity in deleted state will loads its collection or reference elements
        /// in the unchanged state with bindings in the deleted state.
        /// </remarks>
        /// <typeparam name='T'>Element type of collection to load.</typeparam>
        /// <param name="entity">entity</param>
        /// <param name="propertyName">name of collection or reference property to load</param>
        /// <param name="continuation">The continuation object from a previous response.</param>
        /// <returns>QueryOperationResponse instance containing information about the response.</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1011", Justification = "allows compiler to infer 'T'")]
        public QueryOperationResponse<T> LoadProperty<T>(object entity, string propertyName, DataServiceQueryContinuation<T> continuation)
        {
            LoadPropertyResult result = this.CreateLoadPropertyRequest(entity, propertyName, null, null, null, continuation);
            result.Execute();
            return (QueryOperationResponse<T>)result.LoadProperty();
        }

#endif
        #endregion

        #region GetReadStreamUri
        /// <summary>
        /// If the specified entity is a Media Link Entry, this method will return
        /// an URI which can be used to access the content of the Media Resource.
        /// This URI should only be used to GET/Read the content of the MR. It may not respond to POST/PUT/DELETE requests.
        /// </summary>
        /// <param name="entity">The entity to lookup the MR URI for.</param>
        /// <returns>The read URI of the MR.</returns>
        /// <exception cref="ArgumentNullException">If the entity specified is null.</exception>
        /// <exception cref="ArgumentException">If the entity specified is not being tracked.</exception>
        public Uri GetReadStreamUri(object entity) 
        {
            EntityDescriptor box = this.EnsureContained(entity, "entity");
            return box.GetMediaResourceUri(this.baseUriWithSlash);
        }
        #endregion

        #region GetReadStream, BeginGetReadStream, EndGetReadStream

        /// <summary>
        /// This method begins a request to get the read stream for a Media Resource
        /// associated with the media link entry represented by the entity object.
        /// </summary>
        /// <param name="entity">The entity which is the Media Link Entry for the requested Media Resource. Thist must specify
        /// a tracked entity in a non-added state.</param>
        /// <param name="args">Instance of <see cref="DataServiceRequestArgs"/> class with additional metadata for the request.
        /// Must not be null.</param>
        /// <param name="callback">User defined callback to be called when results are available. Can be null.</param>
        /// <param name="state">User state in IAsyncResult. Can be null.</param>
        /// <returns>The async result object to track the request.</returns>
        /// <exception cref="ArgumentNullException">Either entity or args parameters are null.</exception>
        /// <exception cref="ArgumentException">The specified entity is either not tracked, is in the added state or it's not an MLE.</exception>
        public IAsyncResult BeginGetReadStream(object entity, DataServiceRequestArgs args, AsyncCallback callback, object state)
        {
            GetReadStreamResult result;
            result = this.CreateGetReadStreamResult(entity, args, callback, state);
            result.Begin();
            return result;
        }

        /// <summary>
        /// Call this method when the results from the request for the Media Resource are required. 
        /// The method will block if the request have not finished yet.
        /// </summary>
        /// <param name="asyncResult">Async result object returned from BeginGetReadStream.</param>
        /// <returns>An instance of <see cref="DataServiceStreamResponse"/> which contains the response stream
        /// as well as its metadata.</returns>
        public DataServiceStreamResponse EndGetReadStream(IAsyncResult asyncResult)
        {
            GetReadStreamResult result = BaseAsyncResult.EndExecute<GetReadStreamResult>(this, "GetReadStream", asyncResult);
            return result.End();
        }

#if !ASTORIA_LIGHT
        /// <summary>
        /// This method executes synchronously a request to get the read stream for a Media Resource
        /// associated with the Media Link Entry represented by the entity object.
        /// </summary>
        /// <param name="entity">The entity which is the Media Link Entry for the requested Media Resource. Thist must specify
        /// a tracked entity in a non-added state.</param>
        /// <returns>An instance of <see cref="DataServiceStreamResponse"/> which represents the response.</returns>
        /// <exception cref="ArgumentNullException">entity parameter are null.</exception>
        /// <exception cref="ArgumentException">The specified entity is either not tracked, is in the added state or it's not an MLE.</exception>
        public DataServiceStreamResponse GetReadStream(object entity)
        {
            DataServiceRequestArgs args = new DataServiceRequestArgs();
            return this.GetReadStream(entity, args);
        }

        /// <summary>
        /// This method executes synchronously a request to get the read stream for a Media Resource
        /// associated with the Media Link Entry represented by the entity object.
        /// </summary>
        /// <param name="entity">The entity which is the Media Link Entry for the requested Media Resource. Thist must specify
        /// a tracked entity in a non-added state.</param>
        /// <param name="acceptContentType">The content type which should be requested from the server.</param>
        /// <returns>An instance of <see cref="DataServiceStreamResponse"/> which represents the response.</returns>
        /// <exception cref="ArgumentNullException">Either entity or contentType parameters are null.</exception>
        /// <exception cref="ArgumentException">The specified entity is either not tracked, is in the added state or it's not an MLE.
        /// Or the contentType is an empty string.</exception>
        public DataServiceStreamResponse GetReadStream(object entity, string acceptContentType)
        {
            Util.CheckArgumentNotEmpty(acceptContentType, "acceptContentType");
            DataServiceRequestArgs args = new DataServiceRequestArgs();
            args.AcceptContentType = acceptContentType;
            return this.GetReadStream(entity, args);
        }

        /// <summary>
        /// This method executes synchronously a request to get the read stream for a Media Resource
        /// associated with the Media Link Entry represented by the entity object.
        /// </summary>
        /// <param name="entity">The entity which is the Media Link Entry for the requested Media Resource. Thist must specify
        /// a tracked entity in a non-added state.</param>
        /// <param name="args">Instance of <see cref="DataServiceRequestArgs"/> class with additional metadata for the request.
        /// Must not be null.</param>
        /// <returns>An instance of <see cref="DataServiceStreamResponse"/> which represents the response.</returns>
        /// <exception cref="ArgumentNullException">Either entity or args parameters are null.</exception>
        /// <exception cref="ArgumentException">The specified entity is either not tracked, is in the added state or it's not an MLE.</exception>
        public DataServiceStreamResponse GetReadStream(object entity, DataServiceRequestArgs args)
        {
            GetReadStreamResult result = this.CreateGetReadStreamResult(entity, args, null, null);
            return result.Execute();
        }

#endif
        #endregion

        #region SetSaveStream

        /// <summary>
        /// Sets a new content for a Media Resource associated with the specified Media Link Entry entity.
        /// </summary>
        /// <param name="entity">The entity (MLE) for which to set the MR content.</param>
        /// <param name="stream">The stream from which to read the content. The stream will be read to its end.
        /// No Seek operation will be tried on the stream.</param>
        /// <param name="closeStream">If set to true SaveChanges will close the stream before it returns. It will close the stream
        /// even if it failed (throws) and even if it didn't get to use the stream.</param>
        /// <param name="contentType">The Content-Type header value to set for the MR request. The value is not validated
        /// in any way and it's the responsibility of the user to make sure it's a valid value for Content-Type header.</param>
        /// <param name="slug">The Slug header value to set for the MR request. The value is not validated in any way 
        /// and it's the responsibility of the user to make usre it's a valid value for Slug header.</param>
        /// <remarks>Calling this method marks the entity as media link resource (MLE). It also marks the entity as modified
        /// so that it will participate in the next call to SaveChanges.</remarks>
        /// <exception cref="ArgumentException">The entity is not being tracked or the contentType is an empty string. 
        /// The entity has the MediaEntry attribute marking it to use the older way of handling MRs.</exception>
        /// <exception cref="ArgumentNullException">Any of the arguments is null.</exception>
        public void SetSaveStream(object entity, Stream stream, bool closeStream, string contentType, string slug)
        {
            Util.CheckArgumentNull(contentType, "contentType");
            Util.CheckArgumentNull(slug, "slug");

            DataServiceRequestArgs args = new DataServiceRequestArgs();
            args.ContentType = contentType;
            args.Slug = slug;
            this.SetSaveStream(entity, stream, closeStream, args);
        }

        /// <summary>
        /// Sets a new content for a Media Resource associated with the specified Media Link Entry entity.
        /// </summary>
        /// <param name="entity">The entity (MLE) for which to set the MR content.</param>
        /// <param name="stream">The stream from which to read the content. The stream will be read to its end.
        /// No Seek operation will be tried on the stream.</param>
        /// <param name="closeStream">If set to true SaveChanges will close the stream before it returns. It will close the stream
        /// even if it failed (throws) and even if it didn't get to use the stream.</param>
        /// <param name="args">Instance of <see cref="DataServiceRequestArgs"/> class with additional metadata for the MR request.
        /// Must not be null.</param>
        /// <remarks>Calling this method marks the entity as media link resource (MLE). It also marks the entity as modified
        /// so that it will participate in the next call to SaveChanges.</remarks>
        /// <exception cref="ArgumentException">The entity is not being tracked. The entity has the MediaEntry attribute
        /// marking it to use the older way of handling MRs.</exception>
        /// <exception cref="ArgumentNullException">Any of the arguments is null.</exception>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1062:Validate arguments of public methods", MessageId = "0", Justification = "parameters are validated against null via CheckArgumentNull")]
        public void SetSaveStream(object entity, Stream stream, bool closeStream, DataServiceRequestArgs args)
        {
            EntityDescriptor box = this.EnsureContained(entity, "entity");
            Util.CheckArgumentNull(stream, "stream");
            Util.CheckArgumentNull(args, "args");

            ClientType clientType = ClientType.Create(entity.GetType());
            if (clientType.MediaDataMember != null)
            { 
                throw new ArgumentException(
                    Strings.Context_SetSaveStreamOnMediaEntryProperty(clientType.ElementTypeName), 
                    "entity");
            }

            box.SaveStream = new DataServiceSaveStream(stream, closeStream, args);

            Debug.Assert(box.State != EntityStates.Detached, "We should never have a detached entity in the entityDescriptor dictionary.");
            switch (box.State)
            {
                case EntityStates.Added:
                    box.StreamState = StreamStates.Added;
                    break;

                case EntityStates.Modified:
                case EntityStates.Unchanged:
                    box.StreamState = StreamStates.Modified;
                    break;

                case EntityStates.Deleted:
                default:
                    // 

                    throw new DataServiceClientException(Strings.DataServiceException_GeneralError);
            }

            // Note that there's no need to mark the entity as updated because we consider the presense
            // of the save stream as the mark that the MR for this MLE has been updated.
        }

        #endregion

        #region ExecuteBatch, BeginExecuteBatch, EndExecuteBatch

        /// <summary>
        /// Batch multiple queries into a single async request.
        /// </summary>
        /// <param name="callback">User callback when results from batch are available.</param>
        /// <param name="state">user state in IAsyncResult</param>
        /// <param name="queries">queries to batch</param>
        /// <returns>async result object</returns>
        public IAsyncResult BeginExecuteBatch(AsyncCallback callback, object state, params DataServiceRequest[] queries)
        {
            Util.CheckArgumentNotEmpty(queries, "queries");

            SaveResult result = new SaveResult(this, "ExecuteBatch", queries, SaveChangesOptions.Batch, callback, state, true);
            result.BatchBeginRequest(false /*replaceOnUpdate*/);
            return result;
        }

        /// <summary>
        /// Call when results from batch are desired.
        /// </summary>
        /// <param name="asyncResult">async result object returned from BeginExecuteBatch</param>
        /// <returns>batch response from which query results can be enumerated.</returns>
        public DataServiceResponse EndExecuteBatch(IAsyncResult asyncResult)
        {
            SaveResult result = BaseAsyncResult.EndExecute<SaveResult>(this, "ExecuteBatch", asyncResult);
            return result.EndRequest();
        }

#if !ASTORIA_LIGHT // Synchronous methods not available
        /// <summary>
        /// Batch multiple queries into a single request.
        /// </summary>
        /// <param name="queries">queries to batch</param>
        /// <returns>batch response from which query results can be enumerated.</returns>
        public DataServiceResponse ExecuteBatch(params DataServiceRequest[] queries)
        {
            Util.CheckArgumentNotEmpty(queries, "queries");

            SaveResult result = new SaveResult(this, "ExecuteBatch", queries, SaveChangesOptions.Batch, null, null, false);
            result.BatchRequest(false /*replaceOnUpdate*/);
            return result.EndRequest();
        }
#endif

        #endregion

        #region Execute(Uri), BeginExecute(Uri), EndExecute(Uri)

        /// <summary>Begins the execution of the request uri.</summary>
        /// <typeparam name="TElement">element type of the result</typeparam>
        /// <param name="requestUri">request to execute</param>
        /// <param name="callback">User callback when results from execution are available.</param>
        /// <param name="state">user state in IAsyncResult</param>
        /// <returns>async result object</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter", Justification = "Type is used to infer result")]
        public IAsyncResult BeginExecute<TElement>(Uri requestUri, AsyncCallback callback, object state)
        {
            requestUri = Util.CreateUri(this.baseUriWithSlash, requestUri);
            QueryComponents qc = new QueryComponents(requestUri, Util.DataServiceVersionEmpty, typeof(TElement), null, null);
            return (new DataServiceRequest<TElement>(qc, null)).BeginExecute(this, this, callback, state);
        }

        /// <summary>Begins the execution of spscified continuation.</summary>
        /// <typeparam name="T">Element type of the result</typeparam>
        /// <param name="continuation">Conti----ation to execute.</param>
        /// <param name="callback">User callback when results from execution are available.</param>
        /// <param name="state">user state in IAsyncResult</param>
        /// <returns>async result object</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter", Justification = "Type is used to infer result")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1062:Validate arguments of public methods", MessageId = "0", Justification = "parameters are validated against null via CheckArgumentNull")]
        public IAsyncResult BeginExecute<T>(DataServiceQueryContinuation<T> continuation, AsyncCallback callback, object state)
        {
            Util.CheckArgumentNull(continuation, "continuation");
            QueryComponents qc = continuation.CreateQueryComponents();
            return (new DataServiceRequest<T>(qc, continuation.Plan)).BeginExecute(this, this, callback, state);
        }

        /// <summary>
        /// Call when results from batch are desired.
        /// </summary>
        /// <typeparam name="TElement">element type of the result</typeparam>
        /// <param name="asyncResult">async result object returned from BeginExecuteBatch</param>
        /// <returns>batch response from which query results can be enumerated.</returns>
        /// <exception cref="ArgumentNullException">asyncResult is null</exception>
        /// <exception cref="ArgumentException">asyncResult did not originate from this instance or End was previously called</exception>
        /// <exception cref="InvalidOperationException">problem in request or materializing results of query into objects</exception>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter", Justification = "Type is used to infer result")]
        public IEnumerable<TElement> EndExecute<TElement>(IAsyncResult asyncResult)
        {
            Util.CheckArgumentNull(asyncResult, "asyncResult");
            return DataServiceRequest.EndExecute<TElement>(this, this, asyncResult);
        }

#if !ASTORIA_LIGHT // Synchronous methods not available
        /// <summary>
        /// Execute the requestUri
        /// </summary>
        /// <typeparam name="TElement">element type of the result</typeparam>
        /// <param name="requestUri">request uri to execute</param>
        /// <returns>batch response from which query results can be enumerated.</returns>
        /// <exception cref="ArgumentNullException">null requestUri</exception>
        /// <exception cref="ArgumentException">!BaseUri.IsBaseOf(requestUri)</exception>
        /// <exception cref="InvalidOperationException">problem materializing results of query into objects</exception>
        /// <exception cref="WebException">failure to get response for requestUri</exception>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter", Justification = "Type is used to infer result")]
        public IEnumerable<TElement> Execute<TElement>(Uri requestUri)
        {
            requestUri = Util.CreateUri(this.baseUriWithSlash, requestUri);
            QueryComponents qc = new QueryComponents(requestUri, Util.DataServiceVersionEmpty, typeof(TElement), null, null);
            DataServiceRequest request = new DataServiceRequest<TElement>(qc, null);
            return request.Execute<TElement>(this, qc);
        }

        /// <summary>Executes the specified <paramref name="continuation"/>.</summary>
        /// <typeparam name="T">Element type for response.</typeparam>
        /// <param name="continuation">Continuation for query to execute.</param>
        /// <returns>The response for the specified <paramref name="continuation"/>.</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1062:Validate arguments of public methods", MessageId = "0", Justification = "parameters are validated against null via CheckArgumentNull")]
        public QueryOperationResponse<T> Execute<T>(DataServiceQueryContinuation<T> continuation)
        {
            Util.CheckArgumentNull(continuation, "continuation");
            QueryComponents qc = continuation.CreateQueryComponents();
            DataServiceRequest request = new DataServiceRequest<T>(qc, continuation.Plan);
            return request.Execute<T>(this, qc);
        }
#endif
        #endregion

        #region SaveChanges, BeginSaveChanges, EndSaveChanges

        /// <summary>
        /// submit changes to the server in a single change set
        /// </summary>
        /// <param name="callback">callback</param>
        /// <param name="state">state</param>
        /// <returns>async result</returns>
        public IAsyncResult BeginSaveChanges(AsyncCallback callback, object state)
        {
            return this.BeginSaveChanges(this.SaveChangesDefaultOptions, callback, state);
        }

        /// <summary>
        /// begin submitting changes to the server
        /// </summary>
        /// <param name="options">options on how to save changes</param>
        /// <param name="callback">The AsyncCallback delegate.</param>
        /// <param name="state">The state object for this request.</param>
        /// <returns>An IAsyncResult that references the asynchronous request for a response.</returns>
        /// <remarks>
        /// BeginSaveChanges will asynchronously attach identity Uri returned by server to sucessfully added entites.
        /// EndSaveChanges will apply updated values to entities, raise ReadingEntity events and change entity states.
        /// </remarks>
        public IAsyncResult BeginSaveChanges(SaveChangesOptions options, AsyncCallback callback, object state)
        {
            ValidateSaveChangesOptions(options);
            SaveResult result = new SaveResult(this, "SaveChanges", null, options, callback, state, true);
            bool replaceOnUpdate = IsFlagSet(options, SaveChangesOptions.ReplaceOnUpdate);
            if (IsFlagSet(options, SaveChangesOptions.Batch))
            {
                result.BatchBeginRequest(replaceOnUpdate);
            }
            else
            {
                result.BeginNextChange(replaceOnUpdate); // may invoke callback before returning
            }

            return result;
        }

        /// <summary>
        /// done submitting changes to the server
        /// </summary>
        /// <param name="asyncResult">The pending request for a response. </param>
        /// <returns>changeset response</returns>
        public DataServiceResponse EndSaveChanges(IAsyncResult asyncResult)
        {
            SaveResult result = BaseAsyncResult.EndExecute<SaveResult>(this, "SaveChanges", asyncResult);
            
            DataServiceResponse errors = result.EndRequest();

            if (this.ChangesSaved != null)
            {
                this.ChangesSaved(this, new SaveChangesEventArgs(errors));
            }

            return errors;
        }

#if !ASTORIA_LIGHT // Synchronous methods not available
        /// <summary>
        /// submit changes to the server in a single change set
        /// </summary>
        /// <returns>changeset response</returns>
        public DataServiceResponse SaveChanges()
        {
            return this.SaveChanges(this.SaveChangesDefaultOptions);
        }

        /// <summary>
        /// submit changes to the server
        /// </summary>
        /// <param name="options">options on how to save changes</param>
        /// <returns>changeset response</returns>
        /// <remarks>
        /// MergeOption.NoTracking is tricky but supported because to insert a relationship we need the identity
        /// of both ends and if one end was an inserted object then its identity is attached, but may not match its value
        /// 
        /// This initial implementation does not do batching.
        /// Things are sent to the server in the following order
        /// 1) delete relationships
        /// 2) delete objects
        /// 3) update objects
        /// 4) insert objects
        /// 5) insert relationship
        /// </remarks>
        public DataServiceResponse SaveChanges(SaveChangesOptions options)
        {
            DataServiceResponse errors = null;
            ValidateSaveChangesOptions(options);

            SaveResult result = new SaveResult(this, "SaveChanges", null, options, null, null, false);
            bool replaceOnUpdate = IsFlagSet(options, SaveChangesOptions.ReplaceOnUpdate);
            if (IsFlagSet(options, SaveChangesOptions.Batch))
            {
                result.BatchRequest(replaceOnUpdate);
            }
            else
            {
                result.BeginNextChange(replaceOnUpdate);
            }

            errors = result.EndRequest();

            Debug.Assert(null != errors, "null errors");

            if (this.ChangesSaved != null)
            {
                this.ChangesSaved(this, new SaveChangesEventArgs(errors));
            }

            return errors;
        }
#endif
        #endregion

        #region Add, Attach, Delete, Detach, Update, TryGetEntity, TryGetUri

        /// <summary>
        /// Notifies the context that a new link exists between the <paramref name="source"/> and <paramref name="target"/> objects
        /// and that the link is represented via the source.<paramref name="sourceProperty"/> which is a collection.
        /// The context adds this link to the set of newly created links to be sent to
        /// the data service on the next call to SaveChanges().
        /// </summary>
        /// <remarks>
        /// Links are one way relationships.  If a back pointer exists (ie. two way association),
        /// this method should be called a second time to notify the context object of the second link.
        /// </remarks>
        /// <param name="source">Source object participating in the link.</param>
        /// <param name="sourceProperty">The name of the property on the source object which represents a link from the source to the target object.</param>
        /// <param name="target">The target object involved in the link which is bound to the source object also specified in this call.</param>
        /// <exception cref="ArgumentNullException">If <paramref name="source"/>, <paramref name="sourceProperty"/> or <paramref name="target"/> are null.</exception>
        /// <exception cref="InvalidOperationException">if link already exists</exception>
        /// <exception cref="InvalidOperationException">if source or target are detached</exception>
        /// <exception cref="InvalidOperationException">if source or target are in deleted state</exception>
        /// <exception cref="InvalidOperationException">if sourceProperty is not a collection</exception>
        public void AddLink(object source, string sourceProperty, object target)
        {
            this.EnsureRelatable(source, sourceProperty, target, EntityStates.Added);

            LinkDescriptor relation = new LinkDescriptor(source, sourceProperty, target);
            if (this.bindings.ContainsKey(relation))
            {
                throw Error.InvalidOperation(Strings.Context_RelationAlreadyContained);
            }

            relation.State = EntityStates.Added;
            this.bindings.Add(relation, relation);
            this.IncrementChange(relation);
        }

        /// <summary>
        /// Notifies the context to start tracking the specified link between source and the specified target entity.
        /// </summary>
        /// <param name="source">Source object participating in the link.</param>
        /// <param name="sourceProperty">The name of the property on the source object which represents a link from the source to the target object.</param>
        /// <param name="target">The target object involved in the link which is bound to the source object also specified in this call.</param>
        /// <exception cref="ArgumentNullException">If <paramref name="source"/>, <paramref name="sourceProperty"/> or <paramref name="target"/> are null.</exception>
        /// <exception cref="InvalidOperationException">if binding already exists</exception>
        /// <exception cref="InvalidOperationException">if source or target are in added state</exception>
        /// <exception cref="InvalidOperationException">if source or target are in deleted state</exception>
        public void AttachLink(object source, string sourceProperty, object target)
        {
            this.AttachLink(source, sourceProperty, target, MergeOption.NoTracking);
        }

        /// <summary>
        /// Removes the specified link from the list of links being tracked by the context.
        /// Any link being tracked by the context, regardless of its current state, can be detached.   
        /// </summary>
        /// <param name="source">Source object participating in the link.</param>
        /// <param name="sourceProperty">The name of the property on the source object which represents a link from the source to the target object.</param>
        /// <param name="target">The target object involved in the link which is bound to the source object also specified in this call.</param>
        /// <exception cref="ArgumentNullException">If <paramref name="source"/> or <paramref name="sourceProperty"/> are null.</exception>
        /// <exception cref="ArgumentException">if sourceProperty is empty</exception>
        /// <returns>true if binding was previously being tracked, false if not</returns>
        public bool DetachLink(object source, string sourceProperty, object target)
        {
            Util.CheckArgumentNull(source, "source");
            Util.CheckArgumentNotEmpty(sourceProperty, "sourceProperty");

            LinkDescriptor existing;
            LinkDescriptor relation = new LinkDescriptor(source, sourceProperty, target);
            if (!this.bindings.TryGetValue(relation, out existing))
            {
                return false;
            }

            this.DetachExistingLink(existing, false);
            return true;
        }

        /// <summary>
        /// Notifies the context that a link exists between the <paramref name="source"/> and <paramref name="target"/> object
        /// and that the link is represented via the source.<paramref name="sourceProperty"/> which is a collection.
        /// The context adds this link to the set of deleted links to be sent to
        /// the data service on the next call to SaveChanges().
        /// If the specified link exists in the "Added" state, then the link is detached (see DetachLink method) instead.
        /// </summary>
        /// <param name="source">Source object participating in the link.</param>
        /// <param name="sourceProperty">The name of the property on the source object which represents a link from the source to the target object.</param>
        /// <param name="target">The target object involved in the link which is bound to the source object also specified in this call.</param>
        /// <exception cref="ArgumentNullException">If <paramref name="source"/>, <paramref name="sourceProperty"/> or <paramref name="target"/> are null.</exception>
        /// <exception cref="InvalidOperationException">if source or target are detached</exception>
        /// <exception cref="InvalidOperationException">if source or target are in added state</exception>
        /// <exception cref="InvalidOperationException">if sourceProperty is not a collection</exception>
        public void DeleteLink(object source, string sourceProperty, object target)
        {
            bool delay = this.EnsureRelatable(source, sourceProperty, target, EntityStates.Deleted);

            LinkDescriptor existing = null;
            LinkDescriptor relation = new LinkDescriptor(source, sourceProperty, target);
            if (this.bindings.TryGetValue(relation, out existing) && (EntityStates.Added == existing.State))
            {   // Added -> Detached
                this.DetachExistingLink(existing, false);
            }
            else
            {
                if (delay)
                {   // can't have non-added relationship when source or target is in added state
                    throw Error.InvalidOperation(Strings.Context_NoRelationWithInsertEnd);
                }

                if (null == existing)
                {   // detached -> deleted
                    this.bindings.Add(relation, relation);
                    existing = relation;
                }

                if (EntityStates.Deleted != existing.State)
                {
                    existing.State = EntityStates.Deleted;

                    // It is the users responsibility to delete the link
                    // before deleting the entity when required.
                    this.IncrementChange(existing);
                }
            }
        }

        /// <summary>
        /// Notifies the context that a modified link exists between the <paramref name="source"/> and <paramref name="target"/> objects
        /// and that the link is represented via the source.<paramref name="sourceProperty"/> which is a reference.
        /// The context adds this link to the set of modified created links to be sent to
        /// the data service on the next call to SaveChanges().
        /// </summary>
        /// <remarks>
        /// Links are one way relationships.  If a back pointer exists (ie. two way association),
        /// this method should be called a second time to notify the context object of the second link.
        /// </remarks>
        /// <param name="source">Source object participating in the link.</param>
        /// <param name="sourceProperty">The name of the property on the source object which represents a link from the source to the target object.</param>
        /// <param name="target">The target object involved in the link which is bound to the source object also specified in this call.</param>
        /// <exception cref="ArgumentNullException">If <paramref name="source"/>, <paramref name="sourceProperty"/> or <paramref name="target"/> are null.</exception>
        /// <exception cref="InvalidOperationException">if link already exists</exception>
        /// <exception cref="InvalidOperationException">if source or target are detached</exception>
        /// <exception cref="InvalidOperationException">if source or target are in deleted state</exception>
        /// <exception cref="InvalidOperationException">if sourceProperty is not a reference property</exception>
        public void SetLink(object source, string sourceProperty, object target)
        {
            this.EnsureRelatable(source, sourceProperty, target, EntityStates.Modified);

            LinkDescriptor relation = this.DetachReferenceLink(source, sourceProperty, target, MergeOption.NoTracking);
            if (null == relation)
            {
                relation = new LinkDescriptor(source, sourceProperty, target);
                this.bindings.Add(relation, relation);
            }

            Debug.Assert(
                0 == relation.State ||
                IncludeLinkState(relation.State),
                "set link entity state");

            if (EntityStates.Modified != relation.State)
            {
                relation.State = EntityStates.Modified;
                this.IncrementChange(relation);
            }
        }

        #endregion

        #region AddObject, AttachTo, DeleteObject, Detach, TryGetEntity, TryGetUri
        /// <summary>
        /// Add entity into the context in the Added state for tracking.
        /// It does not follow the object graph and add related objects.
        /// </summary>
        /// <param name="entitySetName">EntitySet for the object to be added.</param>
        /// <param name="entity">entity graph to add</param>
        /// <exception cref="ArgumentNullException">if entitySetName is null</exception>
        /// <exception cref="ArgumentException">if entitySetName is empty</exception>
        /// <exception cref="ArgumentNullException">if entity is null</exception>
        /// <exception cref="ArgumentException">if entity does not have a key property</exception>
        /// <exception cref="InvalidOperationException">if entity is already being tracked by the context</exception>
        /// <remarks>
        /// Any leading or trailing forward slashes will automatically be trimmed from entitySetName.
        /// </remarks>
        public void AddObject(string entitySetName, object entity)
        {
            this.ValidateEntitySetName(ref entitySetName);
            ValidateEntityType(entity);

            EntityDescriptor resource = new EntityDescriptor(null, null /*selfLink*/, null /*editLink*/, entity, null, null, entitySetName, null, EntityStates.Added);

            try
            {
                this.entityDescriptors.Add(entity, resource);
            }
            catch (ArgumentException)
            {
                throw Error.InvalidOperation(Strings.Context_EntityAlreadyContained);
            }

            this.IncrementChange(resource);
        }

        /// <summary>
        /// This API enables adding the target object and setting the link between the source object and target object in one request.
        /// </summary>
        /// <param name="source">the parent object which is already tracked by the context.</param>
        /// <param name="sourceProperty">The name of the navigation property which forms the association between the source and target.</param>
        /// <param name="target">the target object which needs to be added.</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1062:Validate arguments of public methods", MessageId = "0", Justification = "parameters are validated against null via CheckArgumentNull")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1062:Validate arguments of public methods", MessageId = "2", Justification = "parameters are validated against null via CheckArgumentNull")]
        public void AddRelatedObject(object source, string sourceProperty, object target)
        {
            Util.CheckArgumentNull(source, "source");
            Util.CheckArgumentNotEmpty(sourceProperty, "propertyName");
            Util.CheckArgumentNull(target, "target");

            // Validate that the source is an entity and is already tracked by the context.
            ValidateEntityType(source);

            EntityDescriptor sourceResource = this.EnsureContained(source, "source");

            // Check for deleted source entity
            if (sourceResource.State == EntityStates.Deleted)
            {
                throw Error.InvalidOperation(Strings.Context_AddRelatedObjectSourceDeleted);
            }

            // Validate that the property is valid and exists on the source
            ClientType parentType = ClientType.Create(source.GetType());
            ClientType.ClientProperty property = parentType.GetProperty(sourceProperty, false);
            if (property.IsKnownType || property.CollectionType == null)
            {
                throw Error.InvalidOperation(Strings.Context_AddRelatedObjectCollectionOnly);
            }

            // Validate that the target is an entity
            ClientType childType = ClientType.Create(target.GetType());
            ValidateEntityType(target);

            // Validate that the property type matches with the target type
            ClientType propertyElementType = ClientType.Create(property.CollectionType);
            if (!propertyElementType.ElementType.IsAssignableFrom(childType.ElementType))
            {
                // target is not of the correct type
                throw Error.Argument(Strings.Context_RelationNotRefOrCollection, "target");
            }

            EntityDescriptor targetResource = new EntityDescriptor(null, null, null, target, sourceResource, sourceProperty, null /*entitySetName*/, null, EntityStates.Added);

            try
            {
                this.entityDescriptors.Add(target, targetResource);
            }
            catch (ArgumentException)
            {
                throw Error.InvalidOperation(Strings.Context_EntityAlreadyContained);
            }

            // Add the link in the added state.
            LinkDescriptor end = targetResource.GetRelatedEnd();
            end.State = EntityStates.Added;
            this.bindings.Add(end, end);

            this.IncrementChange(targetResource);
        }

        /// <summary>
        /// Attach entity into the context in the Unchanged state for tracking.
        /// It does not follow the object graph and attach related objects.
        /// </summary>
        /// <param name="entitySetName">EntitySet for the object to be attached.</param>        
        /// <param name="entity">entity graph to attach</param>
        /// <exception cref="ArgumentNullException">if entitySetName is null</exception>
        /// <exception cref="ArgumentException">if entitySetName is empty</exception>
        /// <exception cref="ArgumentNullException">if entity is null</exception>
        /// <exception cref="ArgumentException">if entity does not have a key property</exception>
        /// <exception cref="InvalidOperationException">if entity is already being tracked by the context</exception>
        public void AttachTo(string entitySetName, object entity)
        {
            this.AttachTo(entitySetName, entity, null);
        }

        /// <summary>
        /// Attach entity into the context in the Unchanged state for tracking.
        /// It does not follow the object graph and attach related objects.
        /// </summary>
        /// <param name="entitySetName">EntitySet for the object to be attached.</param>        
        /// <param name="entity">entity graph to attach</param>
        /// <param name="etag">etag</param>
        /// <exception cref="ArgumentNullException">if entitySetName is null</exception>
        /// <exception cref="ArgumentException">if entitySetName is empty</exception>
        /// <exception cref="ArgumentNullException">if entity is null</exception>
        /// <exception cref="ArgumentException">if entity does not have a key property</exception>
        /// <exception cref="InvalidOperationException">if entity is already being tracked by the context</exception>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704", MessageId = "etag", Justification = "represents ETag in request")]
        public void AttachTo(string entitySetName, object entity, string etag)
        {
            this.ValidateEntitySetName(ref entitySetName);
            Uri editLink = GenerateEditLinkUri(this.baseUriWithSlash, entitySetName, entity);

            // we fake the identity by using the generated edit link
            // ReferenceIdentity is a test hook to help verify we dont' use identity instead of editLink
            String identity = Util.ReferenceIdentity(editLink.AbsoluteUri);

            // EntitySetName need to be cached only when the entity is in added state. For all other states, entities self/edit link must be used.
            EntityDescriptor descriptor = new EntityDescriptor(identity, null /* selfLink */, editLink, entity, null /* parent */, null /* parent property */, null /*entitySetName*/, etag, EntityStates.Unchanged);
            this.InternalAttachEntityDescriptor(descriptor, true);
        }

        /// <summary>
        /// Mark an existing object being tracked by the context for deletion.
        /// </summary>
        /// <param name="entity">entity to be mark deleted</param>
        /// <exception cref="ArgumentNullException">if entity is null</exception>
        /// <exception cref="InvalidOperationException">if entity is not being tracked by the context</exception>
        /// <remarks>
        /// Existings objects in the Added state become detached.
        /// </remarks>
        public void DeleteObject(object entity)
        {
            Util.CheckArgumentNull(entity, "entity");

            EntityDescriptor resource = null;
            if (!this.entityDescriptors.TryGetValue(entity, out resource))
            {   // detached object
                throw Error.InvalidOperation(Strings.Context_EntityNotContained);
            }

            EntityStates state = resource.State;
            if (EntityStates.Added == state)
            {   // added -> detach
                this.DetachResource(resource);
            }
            else if (EntityStates.Deleted != state)
            {
                Debug.Assert(
                    IncludeLinkState(state),
                    "bad state transition to deleted");

                // Leave related links alone which means we can have a link in the Added
                // or Modified state referencing a source/target entity in the Deleted state.
                resource.State = EntityStates.Deleted;
                this.IncrementChange(resource);
            }
        }

        /// <summary>
        /// Detach entity from the context.
        /// </summary>
        /// <param name="entity">entity to detach.</param>
        /// <returns>true if object was detached</returns>
        /// <exception cref="ArgumentNullException">if entity is null</exception>
        public bool Detach(object entity)
        {
            Util.CheckArgumentNull(entity, "entity");

            EntityDescriptor resource = null;
            if (this.entityDescriptors.TryGetValue(entity, out resource))
            {
                return this.DetachResource(resource);
            }

            return false;
        }

        /// <summary>
        /// Mark an existing object for update in the context.
        /// </summary>
        /// <param name="entity">entity to be mark for update</param>
        /// <exception cref="ArgumentNullException">if entity is null</exception>
        /// <exception cref="ArgumentException">if entity is detached</exception>
        public void UpdateObject(object entity)
        {
            Util.CheckArgumentNull(entity, "entity");

            EntityDescriptor resource = null;
            if (!this.entityDescriptors.TryGetValue(entity, out resource))
            {
                throw Error.Argument(Strings.Context_EntityNotContained, "entity");
            }

            if (EntityStates.Unchanged == resource.State)
            {
                resource.State = EntityStates.Modified;
                this.IncrementChange(resource);
            }
        }

        /// <summary>
        /// Find tracked entity by its identity.
        /// </summary>
        /// <remarks>entities in added state are not likely to have a identity</remarks>
        /// <typeparam name="TEntity">entity type</typeparam>
        /// <param name="identity">identity</param>
        /// <param name="entity">entity being tracked by context</param>
        /// <returns>true if entity was found</returns>
        /// <exception cref="ArgumentNullException">identity is null</exception>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1062:Validate arguments of public methods", MessageId = "0", Justification = "parameters are validated against null via CheckArgumentNull")]
        public bool TryGetEntity<TEntity>(Uri identity, out TEntity entity) where TEntity : class
        {
            entity = null;
            Util.CheckArgumentNull(identity, "relativeUri");

            EntityStates state;

            // ReferenceIdentity is a test hook to help verify we dont' use identity instead of editLink
            entity = (TEntity)this.TryGetEntity(Util.ReferenceIdentity(CommonUtil.UriToString(identity)), null, MergeOption.AppendOnly, out state);
            return (null != entity);
        }

        /// <summary>
        /// Identity uri for tracked entity.
        /// Though the identity might use a dereferencable scheme, you MUST NOT assume it can be dereferenced.
        /// </summary>
        /// <remarks>Entities in added state are not likely to have an identity.</remarks>
        /// <param name="entity">entity being tracked by context</param>
        /// <param name="identity">identity</param>
        /// <returns>true if entity is being tracked and has a identity</returns>
        /// <exception cref="ArgumentNullException">entity is null</exception>
        public bool TryGetUri(object entity, out Uri identity)
        {
            identity = null;
            Util.CheckArgumentNull(entity, "entity");

            // if the entity's identity does not map back to the entity, don't return it
            EntityDescriptor resource = null;
            if ((null != this.identityToDescriptor) &&
                this.entityDescriptors.TryGetValue(entity, out resource) &&
                (null != resource.Identity) &&
                Object.ReferenceEquals(resource, this.identityToDescriptor[resource.Identity]))
            {
                // DereferenceIdentity is a test hook to help verify we dont' use identity instead of editLink
                string identityUri = Util.DereferenceIdentity(resource.Identity);
                identity = Util.CreateUri(identityUri, UriKind.Absolute);
            }

            return (null != identity);
        }

        /// <summary>
        /// Handle response by looking at status and possibly throwing an exception.
        /// </summary>
        /// <param name="statusCode">response status code</param>
        /// <param name="responseVersion">Version string on the response header; possibly null.</param>
        /// <param name="getResponseStream">delegate to get response stream</param>
        /// <param name="throwOnFailure">throw or return on failure</param>
        /// <returns>exception on failure</returns>
        internal static Exception HandleResponse(
            HttpStatusCode statusCode,
            string responseVersion,
            Func<Stream> getResponseStream,
            bool throwOnFailure)
        {
            InvalidOperationException failure = null;
            if (!CanHandleResponseVersion(responseVersion))
            {
                string description = Strings.Context_VersionNotSupported(
                    responseVersion,
                    SerializeSupportedVersions());

                failure = Error.InvalidOperation(description);
            }

            if (failure == null && !WebUtil.SuccessStatusCode(statusCode))
            {
                failure = GetResponseText(getResponseStream, statusCode);
            }

            if (failure != null && throwOnFailure)
            {
                throw failure;
            }

            return failure;
        }

        /// <summary>
        /// get the response text into a string
        /// </summary>
        /// <param name="getResponseStream">method to get response stream</param>
        /// <param name="statusCode">status code</param>
        /// <returns>text</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031", Justification = "Cache exception so user can examine it later")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "underlying stream is disposed so wrapping StreamReader doesn't need to be disposed")]
        internal static DataServiceClientException GetResponseText(Func<Stream> getResponseStream, HttpStatusCode statusCode)
        {
            string message = null;
            using (System.IO.Stream stream = getResponseStream())
            {
                if ((null != stream) && stream.CanRead)
                {
                    // this StreamReader can go out of scope without dispose because the underly stream is disposed of
                    message = new StreamReader(stream).ReadToEnd();
                }
            }

            if (String.IsNullOrEmpty(message))
            {
                message = statusCode.ToString();
            }

            return new DataServiceClientException(message, (int)statusCode);
        }

        /// <summary>response materialization has an identity to attach to the inserted object</summary>
        /// <param name="identity">identity of entity</param>
        /// <param name="selfLink">query link of entity</param>
        /// <param name="editLink">edit link of entity</param>
        /// <param name="entity">inserted object</param>
        /// <param name="etag">etag of attached object</param>
        internal void AttachIdentity(String identity, Uri selfLink, Uri editLink, object entity, string etag)
        {   // insert->unchanged
            Debug.Assert(null != identity, "must have identity");

            this.EnsureIdentityToResource();

            // resource.State == EntityState.Added or Unchanged for second pass of media link
            EntityDescriptor resource = this.entityDescriptors[entity];

            this.DetachResourceIdentity(resource);

            // While processing the response, we need to find out if the given resource was inserted deep
            // If it was, then we need to change the link state from added to unchanged
            if (resource.IsDeepInsert)
            {
                LinkDescriptor end = this.bindings[resource.GetRelatedEnd()];
                end.State = EntityStates.Unchanged;
            }

            resource.ETag = etag;
            resource.Identity = identity; // always attach the identity
            resource.SelfLink = selfLink;
            resource.EditLink = editLink;

            resource.State = EntityStates.Unchanged;

            // scenario: sucessfully (1) delete an existing entity and (2) add a new entity where the new entity has the same identity as deleted entity
            // where the SaveChanges pass1 will now associate existing identity with new entity
            // but pass2 for the deleted entity will not blindly remove the identity that is now associated with the new identity
            this.identityToDescriptor[identity] = resource;
        }

        /// <summary>use location from header to generate initial edit and identity</summary>
        /// <param name="entity">entity in added state</param>
        /// <param name="location">location from post header</param>
        internal void AttachLocation(object entity, string location)
        {
            Debug.Assert(null != entity, "null != entity");
            Uri editLink = new Uri(location, UriKind.Absolute);
            String identity = Util.ReferenceIdentity(CommonUtil.UriToString(editLink));

            this.EnsureIdentityToResource();

            // resource.State == EntityState.Added or Unchanged for second pass of media link
            EntityDescriptor resource = this.entityDescriptors[entity];
            this.DetachResourceIdentity(resource);

            // While processing the response, we need to find out if the given resource was inserted deep
            // If it was, then we need to change the link state from added to unchanged
            if (resource.IsDeepInsert)
            {
                LinkDescriptor end = this.bindings[resource.GetRelatedEnd()];
                end.State = EntityStates.Unchanged;
            }

            resource.Identity = identity; // always attach the identity
            resource.EditLink = editLink;

            // scenario: sucessfully batch (1) add a new entity and (2) delete an existing entity where the new entity has the same identity as deleted entity
            // where the SaveChanges pass1 will now associate existing identity with new entity
            // but pass2 for the deleted entity will not blindly remove the identity that is now associated with the new identity
            this.identityToDescriptor[identity] = resource;
        }

        /// <summary>
        /// Track a binding.
        /// </summary>
        /// <param name="source">Source resource.</param>
        /// <param name="sourceProperty">Property on the source resource that relates to the target resource.</param>
        /// <param name="target">Target resource.</param>
        /// <param name="linkMerge">merge operation</param>
        internal void AttachLink(object source, string sourceProperty, object target, MergeOption linkMerge)
        {
            this.EnsureRelatable(source, sourceProperty, target, EntityStates.Unchanged);

            LinkDescriptor existing = null;
            LinkDescriptor relation = new LinkDescriptor(source, sourceProperty, target);
            if (this.bindings.TryGetValue(relation, out existing))
            {
                switch (linkMerge)
                {
                    case MergeOption.AppendOnly:
                        break;

                    case MergeOption.OverwriteChanges:
                        relation = existing;
                        break;

                    case MergeOption.PreserveChanges:
                        if ((EntityStates.Added == existing.State) ||
                            (EntityStates.Unchanged == existing.State) ||
                            (EntityStates.Modified == existing.State && null != existing.Target))
                        {
                            relation = existing;
                        }

                        break;

                    case MergeOption.NoTracking: // public API point should throw if link exists
                        throw Error.InvalidOperation(Strings.Context_RelationAlreadyContained);
                }
            }
            else
            {
                bool collectionProperty = (null != ClientType.Create(source.GetType()).GetProperty(sourceProperty, false).CollectionType);
                if (collectionProperty || (null == (existing = this.DetachReferenceLink(source, sourceProperty, target, linkMerge))))
                {
                    this.bindings.Add(relation, relation);
                    this.IncrementChange(relation);
                }
                else if (!((MergeOption.AppendOnly == linkMerge) ||
                           (MergeOption.PreserveChanges == linkMerge && EntityStates.Modified == existing.State)))
                {
                    // AppendOnly doesn't change state or target
                    // OverWriteChanges changes target and state
                    // PreserveChanges changes target if unchanged, leaves modified target and state alone
                    relation = existing;
                }
            }

            relation.State = EntityStates.Unchanged;
        }

        /// <summary>
        /// Attach entity into the context in the Unchanged state.
        /// </summary>
        /// <param name="descriptor">the descriptor to add</param>
        /// <param name="failIfDuplicated">fail for public api else change existing relationship to unchanged</param>
        /// <remarks>Caller should validate descriptor instance.</remarks>       
        /// <returns>The attached descriptor, if one already exists in the context and failIfDuplicated is set to false, then the existing instance is returned</returns>
        /// <exception cref="InvalidOperationException">if entity is already being tracked by the context</exception>
        /// <exception cref="InvalidOperationException">if identity is pointing to another entity</exception>
        internal EntityDescriptor InternalAttachEntityDescriptor(EntityDescriptor descriptor, bool failIfDuplicated)
        {
            Debug.Assert((null != descriptor.Identity), "must have identity");
            Debug.Assert(null != descriptor.Entity && ClientType.Create(descriptor.Entity.GetType()).IsEntityType, "must be entity type to attach");

            this.EnsureIdentityToResource();

            EntityDescriptor resource;
            this.entityDescriptors.TryGetValue(descriptor.Entity, out resource);

            EntityDescriptor existing;
            this.identityToDescriptor.TryGetValue(descriptor.Identity, out existing);

            // identity existing & pointing to something else
            if (failIfDuplicated && (null != resource))
            {
                throw Error.InvalidOperation(Strings.Context_EntityAlreadyContained);
            }
            else if (resource != existing)
            {
                throw Error.InvalidOperation(Strings.Context_DifferentEntityAlreadyContained);
            }
            else if (null == resource)
            {
                resource = descriptor;
                
                // if resource doesn't exist...
                this.IncrementChange(descriptor);
                this.entityDescriptors.Add(descriptor.Entity, descriptor);
                this.identityToDescriptor.Add(descriptor.Identity, descriptor);
            }

            // DEVNOTE(Microsoft):
            // we used to mark the descriptor as Unchanged
            // but it's now up to the caller to do that
            return resource;
        }

        #endregion

#if ASTORIA_LIGHT
        /// <summary>
        /// create the request object
        /// </summary>
        /// <param name="requestUri">requestUri</param>
        /// <param name="method">updating</param>
        /// <param name="allowAnyType">Whether the request/response should request/assume ATOM or any MIME type</param>
        /// <param name="contentType">content type for the request</param>
        /// <param name="requestVersion">The version associated with this request</param>
        /// <param name="sendChunked">Set to true if the request body should be sent using chunked encoding.</param>
        /// <returns>a request ready to get a response</returns>
        internal HttpWebRequest CreateRequest(Uri requestUri, string method, bool allowAnyType, string contentType, Version requestVersion, bool sendChunked)
        {
            return CreateRequest(requestUri, method, allowAnyType, contentType, requestVersion, sendChunked, HttpStack.Auto);
        }
#endif

#if !ASTORIA_LIGHT
        /// <summary>
        /// create the request object
        /// </summary>
        /// <param name="requestUri">requestUri</param>
        /// <param name="method">updating</param>
        /// <param name="allowAnyType">Whether the request/response should request/assume ATOM or any MIME type</param>
        /// <param name="contentType">content type for the request. Can be null which means don't set the header.</param>
        /// <param name="requestVersion">The version associated with this request</param>
        /// <param name="sendChunked">Set to true if the request body should be sent using chunked encoding.</param>
        /// <returns>a request ready to get a response</returns>
        internal HttpWebRequest CreateRequest(Uri requestUri, string method, bool allowAnyType, string contentType, Version requestVersion, bool sendChunked)
#else
        /// <summary>
        /// create the request object
        /// </summary>
        /// <param name="requestUri">requestUri</param>
        /// <param name="method">updating</param>
        /// <param name="allowAnyType">Whether the request/response should request/assume ATOM or any MIME type</param>
        /// <param name="contentType">content type for the request</param>
        /// <param name="requestVersion">The version associated with this request</param>
        /// <param name="sendChunked">Set to true if the request body should be sent using chunked encoding.</param>
        /// <param name="httpStackArg">If set to non-Auto it forces usage of that http stack in SL regardless of the context settings.</param>
        /// <returns>a request ready to get a response</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", MessageId = "sendChunked", Justification = "common parameter not used in silverlight")]
        internal HttpWebRequest CreateRequest(Uri requestUri, string method, bool allowAnyType, string contentType, Version requestVersion, bool sendChunked, HttpStack httpStackArg)
#endif
        {
            Debug.Assert(null != requestUri, "request uri is null");
            Debug.Assert(requestUri.IsAbsoluteUri, "request uri is not absolute uri");
            Debug.Assert(
                requestUri.Scheme.Equals("http", StringComparison.OrdinalIgnoreCase) ||
                    requestUri.Scheme.Equals("https", StringComparison.OrdinalIgnoreCase), 
                "request uri is not for HTTP");

            Debug.Assert(
                Object.ReferenceEquals(XmlConstants.HttpMethodDelete, method) ||
                Object.ReferenceEquals(XmlConstants.HttpMethodGet, method) ||
                Object.ReferenceEquals(XmlConstants.HttpMethodPost, method) ||
                Object.ReferenceEquals(XmlConstants.HttpMethodPut, method) ||
                Object.ReferenceEquals(XmlConstants.HttpMethodMerge, method),
                "unexpected http method string reference");

#if !ASTORIA_LIGHT
            HttpWebRequest request = (HttpWebRequest)WebRequest.Create(requestUri);
#else
            if (httpStackArg == HttpStack.Auto)
            {
                httpStackArg = this.httpStack;
            }
            HttpWebRequest request = (HttpWebRequest)WebRequest.Create(requestUri, httpStackArg);
#endif

#if !ASTORIA_LIGHT  // Credentials not available
            if (null != this.Credentials)
            {
                request.Credentials = this.Credentials;
            }
#endif

#if !ASTORIA_LIGHT  // Timeout not available
            if (0 != this.timeout)
            {
                request.Timeout = (int)Math.Min(Int32.MaxValue, new TimeSpan(0, 0, this.timeout).TotalMilliseconds);
            }
#endif

#if !ASTORIA_LIGHT // KeepAlive not available
            request.KeepAlive = true;
#endif

#if !ASTORIA_LIGHT // UserAgent not available
            request.UserAgent = "Microsoft ADO.NET Data Services";
#endif

            if (this.UsePostTunneling &&
                (!Object.ReferenceEquals(XmlConstants.HttpMethodPost, method)) &&
                (!Object.ReferenceEquals(XmlConstants.HttpMethodGet, method)))
            {
                request.Headers[XmlConstants.HttpXMethod] = method;
                request.Method = XmlConstants.HttpMethodPost;
            }
            else
            {
                request.Method = method;
            }

            // Always sending the version along allows the server to fail before processing.
            // devnote(Microsoft): this needs to be set before SendingRequest is fired, so client has a chance to modify them
            if (requestVersion != null && requestVersion.Major > 0)
            {
                // if request version is 0.x, then we don't put a DSV header 
                // in this case it's up to the server to decide what version this request is
                request.Headers[XmlConstants.HttpDataServiceVersion] = requestVersion.ToString() + Util.VersionSuffix;
            }

            request.Headers[XmlConstants.HttpMaxDataServiceVersion] = Util.MaxResponseVersion.ToString() + Util.VersionSuffix;

#if !ASTORIA_LIGHT // Silverlight doesn't support chunked encoding yet - so just ignore it
            if (sendChunked)
            {
                request.SendChunked = true;
            }
#endif

            // Fires whenever a new HttpWebRequest has been created
            // The event fires early - before the client library sets many of its required property values.
            // This ensures the client library has the last say on the value of mandated properties
            // such as the HTTP verb  being used for the request.
            if (this.SendingRequest != null)
            {
                System.Net.WebHeaderCollection requestHeaders;
#if !ASTORIA_LIGHT
                requestHeaders = request.Headers;
                SendingRequestEventArgs args = new SendingRequestEventArgs(request, requestHeaders);
#else
                requestHeaders = request.CreateEmptyWebHeaderCollection();
                SendingRequestEventArgs args = new SendingRequestEventArgs(null, requestHeaders);
#endif
                this.SendingRequest(this, args);

#if !ASTORIA_LIGHT
                if (!Object.ReferenceEquals(args.Request, request))
                {
                    request = (System.Net.HttpWebRequest)args.Request;
                }
#else
                // apply all headers to the request
                foreach (string key in requestHeaders.AllKeys)
                {
                    request.Headers[key] = requestHeaders[key];
                }
#endif
            }

            request.Accept = allowAnyType ?
                    XmlConstants.MimeAny :
                    (XmlConstants.MimeApplicationAtom + "," + XmlConstants.MimeApplicationXml);

            request.Headers[HttpRequestHeader.AcceptCharset] = XmlConstants.Utf8Encoding;

#if !ASTORIA_LIGHT // AllowWriteStreamBuffering not available
            bool allowStreamBuffering = false;
            bool removeXMethod = true;
#endif

            if (!Object.ReferenceEquals(XmlConstants.HttpMethodGet, method))
            {
                Debug.Assert(!String.IsNullOrEmpty(contentType), "Content-Type must be specified for non get operation");
                request.ContentType = contentType;
                if (Object.ReferenceEquals(XmlConstants.HttpMethodDelete, method))
                {
                    request.ContentLength = 0;
                }
#if !ASTORIA_LIGHT // AllowWriteStreamBuffering not available
                // else
                {   // always set to workaround NullReferenceException in HttpWebRequest.GetResponse when ContentLength = 0
                    allowStreamBuffering = true;
                }
#endif

                if (this.UsePostTunneling && (!Object.ReferenceEquals(XmlConstants.HttpMethodPost, method)))
                {
                    request.Headers[XmlConstants.HttpXMethod] = method;
                    method = XmlConstants.HttpMethodPost;
#if !ASTORIA_LIGHT
                    removeXMethod = false;
#endif
                }
            }
            else
            {
                Debug.Assert(contentType == null, "Content-Type for get methods should be null");
            }

#if !ASTORIA_LIGHT // AllowWriteStreamBuffering not available
            // When AllowWriteStreamBuffering is true, the data is buffered in memory so it is ready to be resent
            // in the event of redirections or authentication requests.
            request.AllowWriteStreamBuffering = allowStreamBuffering;
#endif

            ICollection<string> headers;
            headers = request.Headers.AllKeys;

#if !ASTORIA_LIGHT  // alternate IfMatch header doesn't work
            if (headers.Contains(XmlConstants.HttpRequestIfMatch))
            {
                request.Headers.Remove(HttpRequestHeader.IfMatch);
            }
#endif

#if !ASTORIA_LIGHT  // alternate HttpXMethod header doesn't work
            if (removeXMethod && headers.Contains(XmlConstants.HttpXMethod))
            {
                request.Headers.Remove(XmlConstants.HttpXMethod);
            }
#endif

            request.Method = method;
            return request;
        }

        /// <summary>
        /// Find tracked entity by its resourceUri and update its etag.
        /// </summary>
        /// <param name="resourceUri">resource id</param>
        /// <param name="etag">updated etag</param>
        /// <param name="merger">merge option</param>
        /// <param name="state">state of entity</param>
        /// <returns>entity if found else null</returns>
        internal object TryGetEntity(String resourceUri, string etag, MergeOption merger, out EntityStates state)
        {
            Debug.Assert(null != resourceUri, "null uri");
            state = EntityStates.Detached;

            EntityDescriptor resource = null;
            if ((null != this.identityToDescriptor) &&
                 this.identityToDescriptor.TryGetValue(resourceUri, out resource))
            {
                state = resource.State;
                if ((null != etag) && (MergeOption.AppendOnly != merger))
                {   // don't update the etag if AppendOnly
                    resource.ETag = etag;
                }

                Debug.Assert(null != resource.Entity, "null entity");
                return resource.Entity;
            }

            return null;
        }

        /// <summary>
        /// get the related links ignoring target entity
        /// </summary>
        /// <param name="source">source entity</param>
        /// <param name="sourceProperty">source entity's property</param>
        /// <returns>enumerable of related ends</returns>
        internal IEnumerable<LinkDescriptor> GetLinks(object source, string sourceProperty)
        {
            return this.bindings.Values.Where(o => (o.Source == source) && (o.SourceProperty == sourceProperty));
        }

        /// <summary>
        /// user hook to resolve name into a type
        /// </summary>
        /// <param name="wireName">name to resolve</param>
        /// <param name="userType">base type associated with name</param>
        /// <param name="checkAssignable">Whether to check that the resulting type is assignable.</param>
        /// <returns>null to skip node</returns>
        /// <exception cref="InvalidOperationException">if ResolveType function returns a type not assignable to the userType</exception>
        internal Type ResolveTypeFromName(string wireName, Type userType, bool checkAssignable)
        {
            Debug.Assert(null != userType, "null != baseType");

            if (String.IsNullOrEmpty(wireName))
            {
                return userType;
            }

            Type payloadType;
            if (!ClientConvert.ToNamedType(wireName, out payloadType))
            {
                payloadType = null;

                Func<string, Type> resolve = this.ResolveType;
                if (null != resolve)
                {
                    // if the ResolveType property is set, call the provided type resultion method
                    payloadType = resolve(wireName);
                }

                if (null == payloadType)
                {
                    // if the type resolution method returns null or the ResolveType property was not set
#if !ASTORIA_LIGHT
                    payloadType = ClientType.ResolveFromName(wireName, userType);
#else
                    payloadType = ClientType.ResolveFromName(wireName, userType, this.GetType());
#endif
                }

                if (checkAssignable && (null != payloadType) && (!userType.IsAssignableFrom(payloadType)))
                {
                    // throw an exception if the type from the resolver is not assignable to the expected type
                    throw Error.InvalidOperation(Strings.Deserialize_Current(userType, payloadType));
                }
            }

            return payloadType ?? userType;
        }

        /// <summary>
        /// The reverse of ResolveType, use for complex types and LINQ query expression building
        /// </summary>
        /// <param name="type">client type</param>
        /// <returns>type for the server</returns>
        internal string ResolveNameFromType(Type type)
        {
            Debug.Assert(null != type, "null type");
            Func<Type, string> resolve = this.ResolveName;
            return ((null != resolve) ? resolve(type) : (String)null);
        }

        /// <summary>
        /// Get from entity descriptor or resolve the server type name for the entitydescriptor
        /// </summary>
        /// <param name="descriptor">The entity descriptor</param>
        /// <returns>The server type name for the entity</returns>
        internal string GetServerTypeName(EntityDescriptor descriptor)
        {
            Debug.Assert(descriptor != null && descriptor.Entity != null, "Null descriptor or no entity in descriptor");

            if (this.resolveName != null)
            {
                // is resolveName the code-gen resolver?
                Type entityType = descriptor.Entity.GetType();
                var codegenAttr = this.resolveName.Method.GetCustomAttributes(false).OfType<System.CodeDom.Compiler.GeneratedCodeAttribute>().FirstOrDefault();
                if (codegenAttr == null || codegenAttr.Tool != Util.CodeGeneratorToolName)
                {
                    // User-supplied resolver, must call first
                    return this.resolveName(entityType) ?? descriptor.ServerTypeName;
                }
                else
                {
                    // V2+ codegen resolver, called last
                    return descriptor.ServerTypeName ?? this.resolveName(entityType);
                }
            }
            else
            {
                return descriptor.ServerTypeName;
            }
        }

        /// <summary>
        /// Fires the ReadingEntity event
        /// </summary>
        /// <param name="entity">Entity being (de)serialized</param>
        /// <param name="data">XML data of the ATOM entry</param>
        internal void FireReadingEntityEvent(object entity, XElement data)
        {
            Debug.Assert(entity != null, "entity != null");
            Debug.Assert(data != null, "data != null");

            ReadingWritingEntityEventArgs args = new ReadingWritingEntityEventArgs(entity, data);
            this.ReadingEntity(this, args);
        }

        #region Ensure

        /// <summary>modified or unchanged</summary>
        /// <param name="x">state to test</param>
        /// <returns>true if modified or unchanged</returns>
        private static bool IncludeLinkState(EntityStates x)
        {
            return ((EntityStates.Modified == x) || (EntityStates.Unchanged == x));
        }

        #endregion

        /// <summary>Checks whether a WCF Data Service version string can be handled.</summary>
        /// <param name="responseVersion">Version string on the response header; possibly null.</param>
        /// <returns>true if the version can be handled; false otherwise.</returns>
        private static bool CanHandleResponseVersion(string responseVersion)
        {
            if (!String.IsNullOrEmpty(responseVersion))
            {
                KeyValuePair<Version, string> version;
                if (!HttpProcessUtility.TryReadVersion(responseVersion, out version))
                {
                    return false;
                }

                if (!Util.SupportedResponseVersions.Contains(version.Key))
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// Serialize supported data service versions to a string that will be used in the exception message.
        /// The string contains versions in single quotes separated by comma followed by a single space (e.g. "'1.0', '2.0'").
        /// </summary>
        /// <returns>Supported data service versions in single quotes separated by comma followed by a space.</returns>
        private static string SerializeSupportedVersions()
        {
            Debug.Assert(Util.SupportedResponseVersions.Length > 0, "At least one supported version must exist.");

            StringBuilder supportedVersions = new StringBuilder("'").Append(Util.SupportedResponseVersions[0].ToString());
            for (int versionIdx = 1; versionIdx < Util.SupportedResponseVersions.Length; versionIdx++)
            {
                supportedVersions.Append("', '");
                supportedVersions.Append(Util.SupportedResponseVersions[versionIdx].ToString());
            }

            supportedVersions.Append("'");

            return supportedVersions.ToString();
        }

        /// <summary>generate a Uri based on key properties of the entity</summary>
        /// <param name="baseUriWithSlash">baseUri</param>
        /// <param name="entitySetName">entitySetName</param>
        /// <param name="entity">entity</param>
        /// <returns>absolute uri</returns>
        private static Uri GenerateEditLinkUri(Uri baseUriWithSlash, string entitySetName, object entity)
        {
            Debug.Assert(null != baseUriWithSlash && baseUriWithSlash.IsAbsoluteUri && CommonUtil.UriToString(baseUriWithSlash).EndsWith("/", StringComparison.Ordinal), "baseUriWithSlash");
            Debug.Assert(!String.IsNullOrEmpty(entitySetName) && !entitySetName.StartsWith("/", StringComparison.Ordinal), "entitySetName");
                        
            // to generate the edit link uri, we need keys
            // this also checks for null
            ValidateEntityTypeHasKeys(entity);

            StringBuilder builder = new StringBuilder();
            builder.Append(baseUriWithSlash.AbsoluteUri);
            builder.Append(entitySetName);
            builder.Append("(");

            string prefix = String.Empty;
            ClientType clientType = ClientType.Create(entity.GetType());

            ClientType.ClientProperty[] keys = clientType.Properties.Where<ClientType.ClientProperty>(ClientType.ClientProperty.GetKeyProperty).ToArray();
            foreach (ClientType.ClientProperty property in keys)
            {
#if ASTORIA_OPEN_OBJECT
                Debug.Assert(!property.OpenObjectProperty, "key property values can't be OpenProperties");
#endif

                builder.Append(prefix);
                if (1 < keys.Length)
                {
                    builder.Append(property.PropertyName).Append("=");
                }

                object value = property.GetValue(entity);
                if (null == value)
                {
                    throw Error.InvalidOperation(Strings.Serializer_NullKeysAreNotSupported(property.PropertyName));
                }

                string converted;
                if (!ClientConvert.TryKeyPrimitiveToString(value, out converted))
                {
                    throw Error.InvalidOperation(Strings.Context_CannotConvertKey(value));
                }

                builder.Append(converted);
                prefix = ",";
            }

            builder.Append(")");

            return Util.CreateUri(builder.ToString(), UriKind.Absolute);
        }

        /// <summary>Get http method string from entity resource state</summary>
        /// <param name="state">resource state</param>
        /// <param name="replaceOnUpdate">whether we need to update MERGE or PUT method for update.</param>
        /// <returns>http method string delete, put or post</returns>
        private static string GetEntityHttpMethod(EntityStates state, bool replaceOnUpdate)
        {
            switch (state)
            {
                case EntityStates.Deleted:
                    return XmlConstants.HttpMethodDelete;
                case EntityStates.Modified:
                    if (replaceOnUpdate)
                    {
                        return XmlConstants.HttpMethodPut;
                    }
                    else
                    {
                        return XmlConstants.HttpMethodMerge;
                    }

                case EntityStates.Added:
                    return XmlConstants.HttpMethodPost;
                default:
                    throw Error.InternalError(InternalError.UnvalidatedEntityState);
            }
        }

        /// <summary>Get http method string from link resource state</summary>
        /// <param name="link">resource</param>
        /// <returns>http method string put or post</returns>
        private static string GetLinkHttpMethod(LinkDescriptor link)
        {
            bool collection = (null != ClientType.Create(link.Source.GetType()).GetProperty(link.SourceProperty, false).CollectionType);
            if (!collection)
            {
                Debug.Assert(EntityStates.Modified == link.State, "not Modified state");
                if (null == link.Target)
                {   // REMOVE/DELETE a reference
                    return XmlConstants.HttpMethodDelete;
                }
                else
                {   // UPDATE/PUT a reference
                    return XmlConstants.HttpMethodPut;
                }
            }
            else if (EntityStates.Deleted == link.State)
            {   // you call DELETE on $links
                return XmlConstants.HttpMethodDelete;
            }
            else
            {   // you INSERT/POST into a collection
                Debug.Assert(EntityStates.Added == link.State, "not Added state");
                return XmlConstants.HttpMethodPost;
            }
        }

        /// <summary>Handle changeset response.</summary>
        /// <param name="entry">headers of changeset response</param>
        private static void HandleResponsePost(LinkDescriptor entry)
        {
            if (!((EntityStates.Added == entry.State) || (EntityStates.Modified == entry.State && null != entry.Target)))
            {
                Error.ThrowBatchUnexpectedContent(InternalError.LinkNotAddedState);
            }

            entry.State = EntityStates.Unchanged;
        }

        /// <summary>Handle changeset response.</summary>
        /// <param name="entry">updated entity or link</param>
        /// <param name="etag">updated etag</param>
        private static void HandleResponsePut(Descriptor entry, string etag)
        {
            if (entry.IsResource)
            {
                EntityDescriptor descriptor = (EntityDescriptor)entry;
                if (EntityStates.Modified != descriptor.State && StreamStates.Modified != descriptor.StreamState)
                {
                    Error.ThrowBatchUnexpectedContent(InternalError.EntryNotModified);
                }

                // We MUST process the MR before the MLE since we always issue the requests in that order.
                if (descriptor.StreamState == StreamStates.Modified)
                {
                    descriptor.StreamETag = etag;
                    descriptor.StreamState = StreamStates.NoStream;
                }
                else
                {
                    Debug.Assert(descriptor.State == EntityStates.Modified, "descriptor.State == EntityStates.Modified");
                    descriptor.ETag = etag;
                    descriptor.State = EntityStates.Unchanged;
                }
            }
            else
            {
                LinkDescriptor link = (LinkDescriptor)entry;
                if ((EntityStates.Added == entry.State) || (EntityStates.Modified == entry.State))
                {
                    link.State = EntityStates.Unchanged;
                }
                else if (EntityStates.Detached != entry.State)
                {   // this link may have been previously detached by a detaching entity
                    Error.ThrowBatchUnexpectedContent(InternalError.LinkBadState);
                }
            }
        }

        /// <summary>
        /// write out an individual property value which can be a primitive or link
        /// </summary>
        /// <param name="writer">writer</param>
        /// <param name="namespaceName">namespaceName in which we need to write the property element.</param>
        /// <param name="property">property which contains name, type, is key (if false and null value, will throw)</param>
        /// <param name="propertyValue">property value</param>
        private static void WriteContentProperty(XmlWriter writer, string namespaceName, ClientType.ClientProperty property, object propertyValue)
        {
            writer.WriteStartElement(property.PropertyName, namespaceName);

            string typename = ClientConvert.GetEdmType(property.PropertyType);
            if (null != typename)
            {
                writer.WriteAttributeString(XmlConstants.AtomTypeAttributeName, XmlConstants.DataWebMetadataNamespace, typename);
            }

            if (null == propertyValue)
            {   // <d:property adsm:null="true" />
                writer.WriteAttributeString(XmlConstants.AtomNullAttributeName, XmlConstants.DataWebMetadataNamespace, XmlConstants.XmlTrueLiteral);

                if (property.KeyProperty)
                {
                    throw Error.InvalidOperation(Strings.Serializer_NullKeysAreNotSupported(property.PropertyName));
                }
            }
            else
            {
                string convertedValue = ClientConvert.ToString(propertyValue, false /* atomDateConstruct */);
                if (0 == convertedValue.Length)
                {   // <d:property m:null="false" />
                    writer.WriteAttributeString(XmlConstants.AtomNullAttributeName, XmlConstants.DataWebMetadataNamespace, XmlConstants.XmlFalseLiteral);
                }
                else
                {   // <d:property>value</property>
                    if (Char.IsWhiteSpace(convertedValue[0]) ||
                        Char.IsWhiteSpace(convertedValue[convertedValue.Length - 1]))
                    {   // xml:space="preserve"
                        writer.WriteAttributeString(XmlConstants.XmlSpaceAttributeName, XmlConstants.XmlNamespacesNamespace, XmlConstants.XmlSpacePreserveValue);
                    }

                    writer.WriteValue(convertedValue);
                }
            }

            writer.WriteEndElement();
        }

        /// <summary>validate <paramref name="entity"/> is entity type</summary>
        /// <param name="entity">entity to validate</param>
        /// <exception cref="ArgumentNullException">if entity was null</exception>
        /// <exception cref="ArgumentException">if entity does not have a key property</exception>
        private static void ValidateEntityType(object entity)
        {
            Util.CheckArgumentNull(entity, "entity");

            if (!ClientType.Create(entity.GetType()).IsEntityType)
            {
                throw Error.Argument(Strings.Content_EntityIsNotEntityType, "entity");
            }
        }

        /// <summary>validate <paramref name="entity"/> has key properties</summary>
        /// <param name="entity">entity to validate</param>
        /// <exception cref="ArgumentNullException">if entity was null</exception>
        /// <exception cref="ArgumentException">if entity does not have a key property</exception>
        private static void ValidateEntityTypeHasKeys(object entity)
        {
            Util.CheckArgumentNull(entity, "entity");

            if (ClientType.Create(entity.GetType()).KeyCount <= 0)
            {
                throw Error.Argument(Strings.Content_EntityWithoutKey, "entity");
            }
        }

        /// <summary>
        /// Validate the SaveChanges Option
        /// </summary>
        /// <param name="options">options as specified by the user.</param>
        private static void ValidateSaveChangesOptions(SaveChangesOptions options)
        {
            const SaveChangesOptions All =
                SaveChangesOptions.ContinueOnError |
                SaveChangesOptions.Batch |
                SaveChangesOptions.ReplaceOnUpdate;

            // Make sure no higher order bits are set.
            if ((options | All) != All)
            {
                throw Error.ArgumentOutOfRange("options");
            }

            // Both batch and continueOnError can't be set together
            if (IsFlagSet(options, SaveChangesOptions.Batch | SaveChangesOptions.ContinueOnError))
            {
                throw Error.ArgumentOutOfRange("options");
            }
        }

        /// <summary>
        /// checks whether the given flag is set on the options
        /// </summary>
        /// <param name="options">options as specified by the user.</param>
        /// <param name="flag">whether the given flag is set on the options</param>
        /// <returns>true if the given flag is set, otherwise false.</returns>
        private static bool IsFlagSet(SaveChangesOptions options, SaveChangesOptions flag)
        {
            return ((options & flag) == flag);
        }

        /// <summary>
        /// Write the batch headers along with the first http header for the batch operation.
        /// </summary>
        /// <param name="writer">Stream writer which writes to the underlying stream.</param>
        /// <param name="methodName">HTTP method name for the operation.</param>
        /// <param name="uri">uri for the operation.</param>
        /// <param name="requestVersion">version for the request</param>
        private static void WriteOperationRequestHeaders(StreamWriter writer, string methodName, string uri, Version requestVersion)
        {
            writer.WriteLine("{0}: {1}", XmlConstants.HttpContentType, XmlConstants.MimeApplicationHttp);
            writer.WriteLine("{0}: {1}", XmlConstants.HttpContentTransferEncoding, XmlConstants.BatchRequestContentTransferEncoding);
            writer.WriteLine();

            writer.WriteLine("{0} {1} {2}", methodName, uri, XmlConstants.HttpVersionInBatching);
            if (requestVersion != Util.DataServiceVersion1 && requestVersion != Util.DataServiceVersionEmpty)
            {
                writer.WriteLine("{0}: {1}{2}", XmlConstants.HttpDataServiceVersion, requestVersion, Util.VersionSuffix);
            }
        }

        /// <summary>
        /// Write the batch headers along with the first http header for the batch operation.
        /// </summary>
        /// <param name="writer">Stream writer which writes to the underlying stream.</param>
        /// <param name="statusCode">status code for the response.</param>
        private static void WriteOperationResponseHeaders(StreamWriter writer, int statusCode)
        {
            writer.WriteLine("{0}: {1}", XmlConstants.HttpContentType, XmlConstants.MimeApplicationHttp);
            writer.WriteLine("{0}: {1}", XmlConstants.HttpContentTransferEncoding, XmlConstants.BatchRequestContentTransferEncoding);
            writer.WriteLine();

            writer.WriteLine("{0} {1} {2}", XmlConstants.HttpVersionInBatching, statusCode, (HttpStatusCode)statusCode);
        }

        /// <summary>the work to detach a resource</summary>
        /// <param name="resource">resource to detach</param>
        /// <returns>true if detached</returns>
        private bool DetachResource(EntityDescriptor resource)
        {
            // Since we are changing the list on the fly, we need to convert it into a list first
            // so that enumeration won't get effected.
            foreach (LinkDescriptor end in this.bindings.Values.Where(resource.IsRelatedEntity).ToList())
            {
                this.DetachExistingLink(
                        end, 
                        end.Target == resource.Entity && resource.State == EntityStates.Added);
            }

            resource.ChangeOrder = UInt32.MaxValue;
            resource.State = EntityStates.Detached;
            bool flag = this.entityDescriptors.Remove(resource.Entity);
            Debug.Assert(flag, "should have removed existing entity");
            this.DetachResourceIdentity(resource);

            return true;
        }

        /// <summary>remove the identity attached to the resource</summary>
        /// <param name="resource">resource with an identity to detach to detach</param>
        private void DetachResourceIdentity(EntityDescriptor resource)
        {
            EntityDescriptor existing = null;
            if ((null != resource.Identity) &&
                this.identityToDescriptor.TryGetValue(resource.Identity, out existing) &&
                Object.ReferenceEquals(existing, resource))
            {
                bool removed = this.identityToDescriptor.Remove(resource.Identity);
                Debug.Assert(removed, "should have removed existing identity");
            }
        }

        /// <summary>
        /// write out binding payload using POST with http method override for PUT
        /// </summary>
        /// <param name="binding">binding</param>
        /// <returns>for non-batching its a request object ready to get a response from else null when batching</returns>
        private HttpWebRequest CreateRequest(LinkDescriptor binding)
        {
            Debug.Assert(null != binding, "null binding");
            if (binding.ContentGeneratedForSave)
            {
                return null;
            }

            EntityDescriptor sourceResource = this.entityDescriptors[binding.Source];
            EntityDescriptor targetResource = (null != binding.Target) ? this.entityDescriptors[binding.Target] : null;

            // these failures should only with SaveChangesOptions.ContinueOnError
            if (null == sourceResource.Identity)
            {
                Debug.Assert(!binding.ContentGeneratedForSave, "already saved link");
                binding.ContentGeneratedForSave = true;
                Debug.Assert(EntityStates.Added == sourceResource.State, "expected added state");
                throw Error.InvalidOperation(Strings.Context_LinkResourceInsertFailure, sourceResource.SaveError);
            }
            else if ((null != targetResource) && (null == targetResource.Identity))
            {
                Debug.Assert(!binding.ContentGeneratedForSave, "already saved link");
                binding.ContentGeneratedForSave = true;
                Debug.Assert(EntityStates.Added == targetResource.State, "expected added state");
                throw Error.InvalidOperation(Strings.Context_LinkResourceInsertFailure, targetResource.SaveError);
            }

            Debug.Assert(null != sourceResource.Identity, "missing sourceResource.Identity");
            return this.CreateRequest(this.CreateRequestUri(sourceResource, binding), GetLinkHttpMethod(binding), false, XmlConstants.MimeApplicationXml, Util.DataServiceVersion1, false);
        }

        /// <summary>create the uri for a link</summary>
        /// <param name="sourceResource">edit link of source</param>
        /// <param name="binding">link</param>
        /// <returns>appropriate uri for link state</returns>
        private Uri CreateRequestUri(EntityDescriptor sourceResource, LinkDescriptor binding)
        {
            Uri requestUri = Util.CreateUri(sourceResource.GetResourceUri(this.baseUriWithSlash, false /*queryLink*/), this.CreateRequestRelativeUri(binding));
            return requestUri;
        }

        /// <summary>
        /// create the uri for the link relative to its source entity
        /// </summary>
        /// <param name="binding">link</param>
        /// <returns>uri</returns>
        private Uri CreateRequestRelativeUri(LinkDescriptor binding)
        {
            Uri relative;
            bool collection = (null != ClientType.Create(binding.Source.GetType()).GetProperty(binding.SourceProperty, false).CollectionType);
            if (collection && (EntityStates.Added != binding.State))
            {   // you DELETE(PUT NULL) from a collection
                Debug.Assert(null != binding.Target, "null target in collection");
                EntityDescriptor targetResource = this.entityDescriptors[binding.Target];

                // For collections, we need to generate the uri with the property name followed by the keys.
                // GenerateEditLinkUri generates an absolute uri
                //      First parameters is the base service uri
                //      Second parameter is the segment name (in this case, navigation property name)
                //      Third parameter is the resource whose key values need to be appended after the segment
                // For e.g. If the navigation property name is "Purchases" and the resource type is Order with key '1', then this method will generate 'baseuri/Purchases(1)'
                Uri navigationPropertyUri = this.BaseUriWithSlash.MakeRelativeUri(DataServiceContext.GenerateEditLinkUri(this.BaseUriWithSlash, binding.SourceProperty, targetResource.Entity));

                // Get the relative uri and appends links segment at the start.
                relative = Util.CreateUri(XmlConstants.UriLinkSegment + "/" + CommonUtil.UriToString(navigationPropertyUri), UriKind.Relative);
            }
            else
            {   // UPDATE(PUT ID) a reference && INSERT(POST ID) into a collection
                relative = Util.CreateUri(XmlConstants.UriLinkSegment + "/" + binding.SourceProperty, UriKind.Relative);
            }

            Debug.Assert(!relative.IsAbsoluteUri, "should be relative uri");
            return relative;
        }

        /// <summary>
        /// write content to batch text stream
        /// </summary>
        /// <param name="binding">link</param>
        /// <param name="text">batch text stream</param>
        private void CreateRequestBatch(LinkDescriptor binding, StreamWriter text)
        {
            EntityDescriptor sourceResource = this.entityDescriptors[binding.Source];
            string requestString;
            if (null != sourceResource.Identity)
            {
                requestString = this.CreateRequestUri(sourceResource, binding).AbsoluteUri;
            }
            else
            {
                Uri relative = this.CreateRequestRelativeUri(binding);
                requestString = "$" + sourceResource.ChangeOrder.ToString(CultureInfo.InvariantCulture) + "/" + CommonUtil.UriToString(relative);
            }

            WriteOperationRequestHeaders(text, GetLinkHttpMethod(binding), requestString, Util.DataServiceVersion1);
            text.WriteLine("{0}: {1}", XmlConstants.HttpContentID, binding.ChangeOrder);

            // if (EntityStates.Deleted || (EntityState.Modifed && null == TargetResource))
            // then the server will fail the batch section if content type exists
            if ((EntityStates.Added == binding.State) || (EntityStates.Modified == binding.State && (null != binding.Target)))
            {
                text.WriteLine("{0}: {1}", XmlConstants.HttpContentType, XmlConstants.MimeApplicationXml);
            }
        }

        /// <summary>
        /// create content memory stream for link
        /// </summary>
        /// <param name="binding">link</param>
        /// <param name="newline">should newline be written</param>
        /// <returns>memory stream</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "Returning MemoryStream which doesn't require disposal")]
        private MemoryStream CreateRequestData(LinkDescriptor binding, bool newline)
        {
            Debug.Assert(
                (binding.State == EntityStates.Added) ||
                (binding.State == EntityStates.Modified && null != binding.Target),
                "This method must be called only when a binding is added or put");
            MemoryStream stream = new MemoryStream();
            XmlWriter writer = XmlUtil.CreateXmlWriterAndWriteProcessingInstruction(stream, HttpProcessUtility.EncodingUtf8NoPreamble);
            EntityDescriptor targetResource = this.entityDescriptors[binding.Target];

            #region <uri xmlns="metadata">
            writer.WriteStartElement(XmlConstants.UriElementName, XmlConstants.DataWebMetadataNamespace);

            string id;
            if (null != targetResource.Identity)
            {
                // When we write the uri in the payload, we need to make sure that we write the edit 
                // link in the payload, since the request uri is the edit link of the parent entity.
                // Think of a read/write service - since the uri is the target link to the parent entity
                // its better than we write the edit link of the child entity in the payload.
                id = CommonUtil.UriToString(targetResource.GetResourceUri(this.baseUriWithSlash, false /*queryLink*/));
            }
            else
            {
                id = "$" + targetResource.ChangeOrder.ToString(CultureInfo.InvariantCulture);
            }

            writer.WriteValue(id);
            writer.WriteEndElement(); // </uri>
            #endregion

            writer.Flush();

            if (newline)
            {
                // end the xml content stream with a newline
                for (int kk = 0; kk < NewLine.Length; ++kk)
                {
                    stream.WriteByte((byte)NewLine[kk]);
                }
            }

            // strip the preamble.
            stream.Position = 0;
            return stream;
        }

        /// <summary>
        /// Create HttpWebRequest from a resource
        /// </summary>
        /// <param name="box">resource</param>
        /// <param name="state">resource state</param>
        /// <param name="replaceOnUpdate">whether we need to update MERGE or PUT method for update.</param>
        /// <returns>web request</returns>
        private HttpWebRequest CreateRequest(EntityDescriptor box, EntityStates state, bool replaceOnUpdate)
        {
            Debug.Assert(null != box && ((EntityStates.Added == state) || (EntityStates.Modified == state) || (EntityStates.Deleted == state)), "unexpected entity ResourceState");

            string httpMethod = GetEntityHttpMethod(state, replaceOnUpdate);
            Uri requestUri = box.GetResourceUri(this.baseUriWithSlash, false /*queryLink*/);

            Version requestVersion = ClientType.Create(box.Entity.GetType()).EpmIsV1Compatible ? Util.DataServiceVersion1 : Util.DataServiceVersion2;
            HttpWebRequest request = this.CreateRequest(requestUri, httpMethod, false, XmlConstants.MimeApplicationAtom, requestVersion, false);
            if ((null != box.ETag) && ((EntityStates.Deleted == state) || (EntityStates.Modified == state)))
            {
                request.Headers.Set(HttpRequestHeader.IfMatch, box.ETag);
            }

            return request;
        }

        /// <summary>
        /// generate batch request for entity
        /// </summary>
        /// <param name="box">entity</param>
        /// <param name="text">batch stream to write to</param>
        /// <param name="replaceOnUpdate">whether we need to update MERGE or PUT method for update.</param>
        private void CreateRequestBatch(EntityDescriptor box, StreamWriter text, bool replaceOnUpdate)
        {
            Debug.Assert(null != box, "null box");
            Debug.Assert(null != text, "null text");
            Debug.Assert(box.State == EntityStates.Added || box.State == EntityStates.Deleted || box.State == EntityStates.Modified, "the entity must be in one of the 3 possible states");

            Uri requestUri = box.GetResourceUri(this.baseUriWithSlash, false /*queryLink*/);

            Debug.Assert(null != requestUri, "request uri is null");
            Debug.Assert(requestUri.IsAbsoluteUri, "request uri is not absolute uri");

            Version requestVersion = ClientType.Create(box.Entity.GetType()).EpmIsV1Compatible ? Util.DataServiceVersion1 : Util.DataServiceVersion2;
            WriteOperationRequestHeaders(text, GetEntityHttpMethod(box.State, replaceOnUpdate), requestUri.AbsoluteUri, requestVersion);
            text.WriteLine("{0}: {1}", XmlConstants.HttpContentID, box.ChangeOrder);
            if (EntityStates.Deleted != box.State)
            {
                text.WriteLine("{0}: {1}", XmlConstants.HttpContentType, XmlConstants.LinkMimeTypeEntry);
            }

            if ((null != box.ETag) && (EntityStates.Deleted == box.State || EntityStates.Modified == box.State))
            {
                text.WriteLine("{0}: {1}", XmlConstants.HttpRequestIfMatch, box.ETag);
            }
        }

        /// <summary>
        /// create memory stream with entity data
        /// </summary>
        /// <param name="box">entity</param>
        /// <param name="newline">should newline be written</param>
        /// <returns>memory stream containing data</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "Returning MemoryStream which doesn't require disposal")]
        private MemoryStream CreateRequestData(EntityDescriptor box, bool newline)
        {
            Debug.Assert(null != box, "null box");
            MemoryStream stream = null;
            switch (box.State)
            {
                case EntityStates.Deleted:
                    break;
                case EntityStates.Modified:
                case EntityStates.Added:
                    stream = new MemoryStream();
                    break;
                default:
                    Error.ThrowInternalError(InternalError.UnvalidatedEntityState);
                    break;
            }

            if (null != stream)
            {
                XmlWriter writer;
                XDocument node = null;
                if (this.WritingEntity != null)
                {
                    // if we have to fire the WritingEntity event, buffer the content
                    // in an XElement so we can present the handler with the data
                    node = new XDocument();
                    writer = node.CreateWriter();
                }
                else
                {
                    writer = XmlUtil.CreateXmlWriterAndWriteProcessingInstruction(stream, HttpProcessUtility.EncodingUtf8NoPreamble);
                }

                ClientType type = ClientType.Create(box.Entity.GetType());

                string typeName = this.GetServerTypeName(box);

                #region <entry xmlns="Atom" xmlns:d="DataWeb", xmlns:m="DataWebMetadata">
                writer.WriteStartElement(XmlConstants.AtomEntryElementName, XmlConstants.AtomNamespace);
                writer.WriteAttributeString(XmlConstants.DataWebNamespacePrefix, XmlConstants.XmlNamespacesNamespace, this.DataNamespace);
                writer.WriteAttributeString(XmlConstants.DataWebMetadataNamespacePrefix, XmlConstants.XmlNamespacesNamespace, XmlConstants.DataWebMetadataNamespace);

                // <category scheme='http://scheme/' term='typeName' />
                if (!String.IsNullOrEmpty(typeName))
                {
                    writer.WriteStartElement(XmlConstants.AtomCategoryElementName, XmlConstants.AtomNamespace);
                    writer.WriteAttributeString(XmlConstants.AtomCategorySchemeAttributeName, this.typeScheme.OriginalString);
                    writer.WriteAttributeString(XmlConstants.AtomCategoryTermAttributeName, typeName);
                    writer.WriteEndElement();
                }

                if (type.HasEntityPropertyMappings)
                {
                    using (EpmSyndicationContentSerializer s = new EpmSyndicationContentSerializer(type.EpmTargetTree, box.Entity, writer))
                    {
                        s.Serialize();
                    }
                }
                else
                {
                    // <title />
                    // <updated>2008-05-05T21:44:55Z</updated>
                    // <author><name /></author>
                    writer.WriteElementString(XmlConstants.AtomTitleElementName, XmlConstants.AtomNamespace, String.Empty);
                    writer.WriteStartElement(XmlConstants.AtomAuthorElementName, XmlConstants.AtomNamespace);
                    writer.WriteElementString(XmlConstants.AtomNameElementName, XmlConstants.AtomNamespace, String.Empty);
                    writer.WriteEndElement();

                    writer.WriteElementString(XmlConstants.AtomUpdatedElementName, XmlConstants.AtomNamespace, XmlConvert.ToString(DateTime.UtcNow, XmlDateTimeSerializationMode.RoundtripKind));
                }

                if (EntityStates.Modified == box.State)
                {
                    // <id>http://host/service/entityset(key)</id>
                    writer.WriteElementString(XmlConstants.AtomIdElementName, Util.DereferenceIdentity(box.Identity));
                }
                else
                {
                    writer.WriteElementString(XmlConstants.AtomIdElementName, XmlConstants.AtomNamespace, String.Empty);
                }

                #region <link href=”%EditLink%” rel=”%DataWebRelatedNamespace%%AssociationName%” type=”application/atom+xml;feed” />
                if (EntityStates.Added == box.State)
                {
                    this.CreateRequestDataLinks(box, writer);
                }
                #endregion

                #region <content type="application/xml"><m:Properites> or <m:Properties>
                // For MLE we need to write the properties to the entry level, we also omit the content element completely
                //   as the server will ignore it anyway.
                if (!type.IsMediaLinkEntry && !box.IsMediaLinkEntry)
                {
                    writer.WriteStartElement(XmlConstants.AtomContentElementName, XmlConstants.AtomNamespace); // <atom:content>
                    writer.WriteAttributeString(XmlConstants.AtomTypeAttributeName, XmlConstants.MimeApplicationXml); // empty namespace
                }

                writer.WriteStartElement(XmlConstants.AtomPropertiesElementName, XmlConstants.DataWebMetadataNamespace); // <m:Properties>

                bool propertiesWritten;
                this.WriteContentProperties(writer, type, box.Entity, type.HasEntityPropertyMappings ? type.EpmSourceTree.Root : null, out propertiesWritten);

                writer.WriteEndElement(); // </m:Properties>

                if (!type.IsMediaLinkEntry && !box.IsMediaLinkEntry)
                {
                    writer.WriteEndElement(); // </atom:content>
                }

                if (type.HasEntityPropertyMappings)
                {
                    using (EpmCustomContentSerializer s = new EpmCustomContentSerializer(type.EpmTargetTree, box.Entity, writer))
                    {
                        s.Serialize();
                    }
                }

                writer.WriteEndElement(); // </atom:entry>
                writer.Flush();
                writer.Close();
                #endregion
                #endregion

                if (this.WritingEntity != null)
                {
                    ReadingWritingEntityEventArgs args = new ReadingWritingEntityEventArgs(box.Entity, node.Root);
                    this.WritingEntity(this, args);

                    // copy the buffered XDocument to the memory stream. no easy way of avoiding
                    // the copy given that we need to know the length before scanning the stream
                    XmlWriterSettings settings = XmlUtil.CreateXmlWriterSettings(HttpProcessUtility.EncodingUtf8NoPreamble);
                    settings.ConformanceLevel = ConformanceLevel.Auto;
                    using (XmlWriter streamWriter = XmlWriter.Create(stream, settings))
                    {
                        node.Save(streamWriter);
                    }
                }

                if (newline)
                {
                    // end the xml content stream with a newline
                    for (int kk = 0; kk < NewLine.Length; ++kk)
                    {
                        stream.WriteByte((byte)NewLine[kk]);
                    }
                }

                stream.Position = 0;
            }

            return stream;
        }

        /// <summary>
        /// add the related links for new entites to non-new entites
        /// </summary>
        /// <param name="box">entity in added state</param>
        /// <param name="writer">writer to add links to</param>
        private void CreateRequestDataLinks(EntityDescriptor box, XmlWriter writer)
        {
            Debug.Assert(EntityStates.Added == box.State, "entity not added state");

            ClientType clientType = null;
            foreach (LinkDescriptor end in this.RelatedLinks(box))
            {
                Debug.Assert(!end.ContentGeneratedForSave, "already saved link");
                end.ContentGeneratedForSave = true;

                if (null == clientType)
                {
                    clientType = ClientType.Create(box.Entity.GetType());
                }

                string typeAttributeValue;
                if (null != clientType.GetProperty(end.SourceProperty, false).CollectionType)
                {
                    typeAttributeValue = XmlConstants.LinkMimeTypeFeed;
                }
                else
                {
                    typeAttributeValue = XmlConstants.LinkMimeTypeEntry;
                }

                Debug.Assert(null != end.Target, "null is DELETE");
                String targetEditLink = CommonUtil.UriToString(this.entityDescriptors[end.Target].EditLink);

                writer.WriteStartElement(XmlConstants.AtomLinkElementName, XmlConstants.AtomNamespace);
                writer.WriteAttributeString(XmlConstants.AtomHRefAttributeName, targetEditLink);
                writer.WriteAttributeString(XmlConstants.AtomLinkRelationAttributeName, XmlConstants.DataWebRelatedNamespace + end.SourceProperty);
                writer.WriteAttributeString(XmlConstants.AtomTypeAttributeName, typeAttributeValue);
                writer.WriteEndElement();
            }
        }

        /// <summary>Handle response to deleted entity.</summary>
        /// <param name="entry">deleted entity</param>
        private void HandleResponseDelete(Descriptor entry)
        {
            if (EntityStates.Deleted != entry.State)
            {
                Error.ThrowBatchUnexpectedContent(InternalError.EntityNotDeleted);
            }

            if (entry.IsResource)
            {
                EntityDescriptor resource = (EntityDescriptor)entry;
                this.DetachResource(resource);
            }
            else
            {
                this.DetachExistingLink((LinkDescriptor)entry, false);
            }
        }

        /// <summary>Handle changeset response.</summary>
        /// <param name="entry">headers of changeset response</param>
        /// <param name="materializer">changeset response stream</param>
        /// <param name="editLink">editLink of the newly created item (non-null if materialize is null)</param>
        /// <param name="etag">ETag header value from the server response (or null if no etag or if there is an actual response)</param>
        private void HandleResponsePost(EntityDescriptor entry, MaterializeAtom materializer, Uri editLink, string etag)
        {
            Debug.Assert(editLink != null, "location header must be specified in POST responses.");

            if (EntityStates.Added != entry.State && StreamStates.Added != entry.StreamState)
            {
                Error.ThrowBatchUnexpectedContent(InternalError.EntityNotAddedState);
            }

            if (materializer == null)
            {
                // If the materializer is null, that means the POST request didn't send a response back. Hence we will use
                // the location header as the self and edit links.
                String identity = Util.ReferenceIdentity(CommonUtil.UriToString(editLink));
                this.AttachIdentity(identity, null /*queryLink*/, editLink, entry.Entity, etag);
            }
            else
            {
                materializer.SetInsertingObject(entry.Entity);

                foreach (object x in materializer)
                {
                    Debug.Assert(null != entry.Identity, "updated inserted should always gain an identity");
                    Debug.Assert(x == entry.Entity, "x == box.Entity, should have same object generated by response");
                    Debug.Assert(EntityStates.Unchanged == entry.State, "should have moved out of insert");
                    Debug.Assert((null != this.identityToDescriptor) && this.identityToDescriptor.ContainsKey(entry.Identity), "should have identity tracked");

                    // If there was no edit link specified in the payload, then we need to set the location header as the edit link
                    if (entry.EditLink == null)
                    {
                        entry.EditLink = editLink;
                    }

                    // If there was no etag specified in the payload, then we need to set the etag from the header
                    if (entry.ETag == null)
                    {
                        entry.ETag = etag;
                    }
                }
            }

            foreach (LinkDescriptor end in this.RelatedLinks(entry))
            {
                Debug.Assert(0 != end.SaveResultWasProcessed, "link should have been saved with the enty");

                // Since we allow link folding on collection properties also, we need to check if the link
                // was in added state also, and make sure we put that link in unchanged state.
                if (IncludeLinkState(end.SaveResultWasProcessed) || end.SaveResultWasProcessed == EntityStates.Added)
                {
                    HandleResponsePost(end);
                }
            }
        }

        /// <summary>flag results as being processed</summary>
        /// <param name="entry">result entry being processed</param>
        /// <returns>count of related links that were also processed</returns>
        private int SaveResultProcessed(Descriptor entry)
        {
            // media links will be processed twice
            entry.SaveResultWasProcessed = entry.State;

            int count = 0;
            if (entry.IsResource && (EntityStates.Added == entry.State))
            {
                foreach (LinkDescriptor end in this.RelatedLinks((EntityDescriptor)entry))
                {
                    Debug.Assert(end.ContentGeneratedForSave, "link should have been saved with the enty");
                    if (end.ContentGeneratedForSave)
                    {
                        Debug.Assert(0 == end.SaveResultWasProcessed, "this link already had a result");
                        end.SaveResultWasProcessed = end.State;
                        count++;
                    }
                }
            }

            return count;
        }

        /// <summary>
        /// enumerate the related Modified/Unchanged links for an added item
        /// </summary>
        /// <param name="box">entity</param>
        /// <returns>related links</returns>
        /// <remarks>
        /// During a non-batch SaveChanges, an Added entity can become an Unchanged entity
        /// and should be included in the set of related links for the second Added entity.
        /// </remarks>
        private IEnumerable<LinkDescriptor> RelatedLinks(EntityDescriptor box)
        {
            foreach (LinkDescriptor end in this.bindings.Values)
            {
                if (end.Source == box.Entity)
                {
                    if (null != end.Target)
                    {   // null TargetResource is equivalent to Deleted
                        EntityDescriptor target = this.entityDescriptors[end.Target];

                        // assumption: the source entity started in the Added state
                        // note: SaveChanges operates with two passes
                        //      a) first send the request and then attach identity and append the result into a batch response  (Example: BeginSaveChanges)
                        //      b) process the batch response (shared code with SaveChanges(Batch))  (Example: EndSaveChanges)
                        // note: SaveResultWasProcessed is set when to the pre-save state when the save result is sucessfully processed

                        // scenario #1 when target entity started in modified or unchanged state
                        // 1) the link target entity was modified and now implicitly assumed to be unchanged (this is true in second pass)
                        // 2) or link target entity has not been saved is in the modified or unchanged state (this is true in first pass)

                        // scenario #2 when target entity started in added state
                        // 1) target entity has an identity (true in first pass for non-batch)
                        // 2) target entity is processed before source to qualify (1) better during the second pass
                        // 3) the link target has not been saved and is in the added state
                        // 4) or the link target has been saved and was in the added state
                        if (IncludeLinkState(target.SaveResultWasProcessed) || ((0 == target.SaveResultWasProcessed) && IncludeLinkState(target.State)) ||
                            ((null != target.Identity) && (target.ChangeOrder < box.ChangeOrder) &&
                             ((0 == target.SaveResultWasProcessed && EntityStates.Added == target.State) ||
                              (EntityStates.Added == target.SaveResultWasProcessed))))
                        {
                            Debug.Assert(box.ChangeOrder < end.ChangeOrder, "saving is out of order");
                            yield return end;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// create the load property request
        /// </summary>
        /// <param name="entity">entity</param>
        /// <param name="propertyName">name of collection or reference property to load</param>
        /// <param name="callback">The AsyncCallback delegate.</param>
        /// <param name="state">user state</param>
        /// <param name="requestUri">The request uri, or null if one is to be constructed</param>
        /// <param name="continuation">Continuation, if one is available.</param>
        /// <returns>a aync result that you can get a response from</returns>
        private LoadPropertyResult CreateLoadPropertyRequest(object entity, string propertyName, AsyncCallback callback, object state, Uri requestUri, DataServiceQueryContinuation continuation)
        {
            Debug.Assert(continuation == null || requestUri == null, "continuation == null || requestUri == null -- only one or the either (or neither) may be passed in");
            EntityDescriptor box = this.EnsureContained(entity, "entity");
            Util.CheckArgumentNotEmpty(propertyName, "propertyName");

            ClientType type = ClientType.Create(entity.GetType());
            Debug.Assert(type.IsEntityType, "must be entity type to be contained");

            if (EntityStates.Added == box.State)
            {
                throw Error.InvalidOperation(Strings.Context_NoLoadWithInsertEnd);
            }

            ClientType.ClientProperty property = type.GetProperty(propertyName, false);
            Debug.Assert(null != property, "should have thrown if propertyName didn't exist");

            ProjectionPlan plan;
            if (continuation == null)
            {
                plan = null;
            }
            else
            {
                plan = continuation.Plan;
                requestUri = continuation.NextLinkUri;
            }

            bool mediaLink = (type.MediaDataMember != null && propertyName == type.MediaDataMember.PropertyName);
            Version requestVersion;
            if (requestUri == null)
            {
                Uri relativeUri;
                if (mediaLink)
                {
                    // special case for requesting the "media" value of an ATOM media link entry
                    relativeUri = Util.CreateUri(XmlConstants.UriValueSegment, UriKind.Relative);
                }
                else
                {
                    relativeUri = Util.CreateUri(propertyName + (null != property.CollectionType ? "()" : String.Empty), UriKind.Relative);
                }

                requestUri = Util.CreateUri(box.GetResourceUri(this.baseUriWithSlash, true /*queryLink*/), relativeUri);
                requestVersion = Util.DataServiceVersion1;
            }
            else
            {
                // if we specify an URI, we need to keep the Execute<T> behaviour
                requestVersion = Util.DataServiceVersionEmpty;
            }

            HttpWebRequest request = this.CreateRequest(requestUri, XmlConstants.HttpMethodGet, mediaLink, null, requestVersion, false);
            DataServiceRequest dataServiceRequest = DataServiceRequest.GetInstance(property.PropertyType, requestUri);
            return new LoadPropertyResult(entity, propertyName, this, request, callback, state, dataServiceRequest, plan);
        }

        /// <summary>
        /// write the content section of the atom payload
        /// </summary>
        /// <param name="writer">writer</param>
        /// <param name="type">resource type</param>
        /// <param name="resource">resource value</param>
        /// <param name="currentSegment">Source EPM tree segment corresponding to <paramref name="type"/></param>
        /// <param name="propertiesWritten">True if we have written any property to the writer.</param>
        private void WriteContentProperties(XmlWriter writer, ClientType type, object resource, EpmSourcePathSegment currentSegment, out bool propertiesWritten)
        {
            #region <d:property>value</property>
            propertiesWritten = false;
            foreach (ClientType.ClientProperty property in type.Properties)
            {
                // don't write mime data member or the mime type member for it
                if (property == type.MediaDataMember ||
                    (type.MediaDataMember != null &&
                     type.MediaDataMember.MimeTypeProperty == property))
                {
                    continue;
                }

                object propertyValue = property.GetValue(resource);

                EpmSourcePathSegment matchedSegment = currentSegment != null ? currentSegment.SubProperties.SingleOrDefault(s => s.PropertyName == property.PropertyName) : null;

                if (property.IsKnownType)
                {
                    if (propertyValue == null || matchedSegment == null || matchedSegment.EpmInfo.Attribute.KeepInContent)
                    {
                        WriteContentProperty(writer, this.DataNamespace, property, propertyValue);
                        propertiesWritten = true;
                    }
                }
#if ASTORIA_OPEN_OBJECT
                else if (property.OpenObjectProperty)
                {
                    foreach (KeyValuePair<string, object> pair in (IDictionary<string, object>)propertyValue)
                    {
                        if ((null == pair.Value) || ClientConvert.IsKnownType(pair.Value.GetType()))
                        {
                            Type valueType = pair.Value != null ? pair.Value.GetType() : typeof(string);
                            ClientType.ClientProperty openProperty = new ClientType.ClientProperty(null, valueType, false, true);
                            WriteContentProperty(writer, this.DataNamespace, openProperty, pair.Value);
                            propertiesWritten = true;
                        }
                    }
                }
#endif
                else if (null == property.CollectionType)
                {
                    ClientType nested = ClientType.Create(property.PropertyType);
                    if (!nested.IsEntityType)
                    {
                        #region complex type
                        XElement complexProperty = new XElement(((XNamespace)this.DataNamespace) + property.PropertyName);
                        bool shouldWriteComplexProperty = false;
                        string typeName = this.ResolveNameFromType(nested.ElementType);
                        if (!String.IsNullOrEmpty(typeName))
                        {
                            complexProperty.Add(new XAttribute(((XNamespace)XmlConstants.DataWebMetadataNamespace) + XmlConstants.AtomTypeAttributeName, typeName));
                        }
                        
                        // Handle null values for complex types by putting m:null="true"
                        if (null == propertyValue)
                        {
                            complexProperty.Add(new XAttribute(((XNamespace)XmlConstants.DataWebMetadataNamespace) + XmlConstants.AtomNullAttributeName, XmlConstants.XmlTrueLiteral));
                            shouldWriteComplexProperty = true;
                        }
                        else
                        {
                            using (XmlWriter complexPropertyWriter = complexProperty.CreateWriter())
                            {
                                this.WriteContentProperties(complexPropertyWriter, nested, propertyValue, matchedSegment, out shouldWriteComplexProperty);
                            }
                        }

                        if (shouldWriteComplexProperty)
                        {
                            complexProperty.WriteTo(writer);
                            propertiesWritten = true;
                        }
                        #endregion
                    }
                }
            }
            #endregion
        }

        /// <summary>Detach existing link</summary>
        /// <param name="existingLink">link to detach</param>
        /// <param name="targetDelete">true if target is being deleted, false otherwise</param>
        private void DetachExistingLink(LinkDescriptor existingLink, bool targetDelete)
        {
            // The target can be null in which case we don't need this check
            if (existingLink.Target != null)
            {
                // Identify the target resource for the link
                EntityDescriptor targetResource = this.entityDescriptors[existingLink.Target];
                
                // Check if there is a dependency relationship b/w the source and target objects i.e. target can not exist without source link
                // Deep insert requires this check to be made but skip the check if the target object is being deleted
                if (targetResource.IsDeepInsert && !targetDelete)
                {
                    EntityDescriptor parentOfTarget = targetResource.ParentForInsert;
                    if (Object.ReferenceEquals(targetResource.ParentEntity, existingLink.Source) && 
                       (parentOfTarget.State != EntityStates.Deleted || 
                        parentOfTarget.State != EntityStates.Detached))
                    {
                        throw new InvalidOperationException(Strings.Context_ChildResourceExists);
                    }
                }
            }
        
            if (this.bindings.Remove(existingLink))
            {   // this link may have been previously detached by a detaching entity
                existingLink.State = EntityStates.Detached;
            }
        }

        /// <summary>
        /// find and detach link for reference property
        /// </summary>
        /// <param name="source">source entity</param>
        /// <param name="sourceProperty">source entity property name for target entity</param>
        /// <param name="target">target entity</param>
        /// <param name="linkMerge">link merge option</param>
        /// <returns>true if found and not removed</returns>
        private LinkDescriptor DetachReferenceLink(object source, string sourceProperty, object target, MergeOption linkMerge)
        {
            LinkDescriptor existing = this.GetLinks(source, sourceProperty).FirstOrDefault();
            if (null != existing)
            {
                if ((target == existing.Target) ||
                    (MergeOption.AppendOnly == linkMerge) ||
                    (MergeOption.PreserveChanges == linkMerge && EntityStates.Modified == existing.State))
                {
                    return existing;
                }

                // Since we don't support deep insert on reference property, no need to check for deep insert.
                this.DetachExistingLink(existing, false);
                Debug.Assert(!this.bindings.Values.Any(o => (o.Source == source) && (o.SourceProperty == sourceProperty)), "only expecting one");
            }

            return null;
        }

        /// <summary>
        /// verify the resource being tracked by context
        /// </summary>
        /// <param name="resource">resource</param>
        /// <param name="parameterName">parameter name to include in ArgumentException</param>
        /// <returns>The given resource.</returns>
        /// <exception cref="ArgumentException">if resource is not contained</exception>
        private EntityDescriptor EnsureContained(object resource, string parameterName)
        {
            Util.CheckArgumentNull(resource, parameterName);

            EntityDescriptor box = null;
            if (!this.entityDescriptors.TryGetValue(resource, out box))
            {
                throw Error.InvalidOperation(Strings.Context_EntityNotContained);
            }

            return box;
        }

        /// <summary>
        /// verify the source and target are relatable
        /// </summary>
        /// <param name="source">source Resource</param>
        /// <param name="sourceProperty">source Property</param>
        /// <param name="target">target Resource</param>
        /// <param name="state">destination state of relationship to evaluate for</param>
        /// <returns>true if DeletedState and one of the ends is in the added state</returns>
        /// <exception cref="ArgumentNullException">if source or target are null</exception>
        /// <exception cref="ArgumentException">if source or target are not contained</exception>
        /// <exception cref="ArgumentNullException">if source property is null</exception>
        /// <exception cref="ArgumentException">if source property empty</exception>
        /// <exception cref="InvalidOperationException">Can only relate ends with keys.</exception>
        /// <exception cref="ArgumentException">If target doesn't match property type.</exception>
        /// <exception cref="InvalidOperationException">If adding relationship where one of the ends is in the deleted state.</exception>
        /// <exception cref="InvalidOperationException">If attaching relationship where one of the ends is in the added or deleted state.</exception>
        private bool EnsureRelatable(object source, string sourceProperty, object target, EntityStates state)
        {
            EntityDescriptor sourceResource = this.EnsureContained(source, "source");
            EntityDescriptor targetResource = null;
            if ((null != target) || ((EntityStates.Modified != state) && (EntityStates.Unchanged != state)))
            {
                targetResource = this.EnsureContained(target, "target");
            }

            Util.CheckArgumentNotEmpty(sourceProperty, "sourceProperty");

            ClientType type = ClientType.Create(source.GetType());
            Debug.Assert(type.IsEntityType, "should be enforced by just adding an object");

            // will throw InvalidOperationException if property doesn't exist
            ClientType.ClientProperty property = type.GetProperty(sourceProperty, false);

            if (property.IsKnownType)
            {
                throw Error.InvalidOperation(Strings.Context_RelationNotRefOrCollection);
            }

            if ((EntityStates.Unchanged == state) && (null == target) && (null != property.CollectionType))
            {
                targetResource = this.EnsureContained(target, "target");
            }

            if (((EntityStates.Added == state) || (EntityStates.Deleted == state)) && (null == property.CollectionType))
            {
                throw Error.InvalidOperation(Strings.Context_AddLinkCollectionOnly);
            }
            else if ((EntityStates.Modified == state) && (null != property.CollectionType))
            {
                throw Error.InvalidOperation(Strings.Context_SetLinkReferenceOnly);
            }

            // if (property.IsCollection) then property.PropertyType is the collection elementType
            // either way you can only have a relation ship between keyed objects
            type = ClientType.Create(property.CollectionType ?? property.PropertyType);
            Debug.Assert(type.IsEntityType, "should be enforced by just adding an object");

            if ((null != target) && !type.ElementType.IsInstanceOfType(target))
            {
                // target is not of the correct type
                throw Error.Argument(Strings.Context_RelationNotRefOrCollection, "target");
            }

            if ((EntityStates.Added == state) || (EntityStates.Unchanged == state))
            {
                if ((sourceResource.State == EntityStates.Deleted) ||
                    ((targetResource != null) && (targetResource.State == EntityStates.Deleted)))
                {
                    // can't add/attach new relationship when source or target in deleted state
                    throw Error.InvalidOperation(Strings.Context_NoRelationWithDeleteEnd);
                }
            }

            if ((EntityStates.Deleted == state) || (EntityStates.Unchanged == state))
            {
                if ((sourceResource.State == EntityStates.Added) ||
                    ((targetResource != null) && (targetResource.State == EntityStates.Added)))
                {
                    // can't have non-added relationship when source or target is in added state
                    if (EntityStates.Deleted == state)
                    {
                        return true;
                    }

                    throw Error.InvalidOperation(Strings.Context_NoRelationWithInsertEnd);
                }
            }

            return false;
        }

        /// <summary>validate <paramref name="entitySetName"/> and trim leading and trailing forward slashes</summary>
        /// <param name="entitySetName">resource name to validate</param>
        /// <exception cref="ArgumentNullException">if entitySetName was null</exception>
        /// <exception cref="ArgumentException">if entitySetName was empty or contained only forward slash</exception>
        private void ValidateEntitySetName(ref string entitySetName)
        {
            Util.CheckArgumentNotEmpty(entitySetName, "entitySetName");
            entitySetName = entitySetName.Trim(Util.ForwardSlash);

            Util.CheckArgumentNotEmpty(entitySetName, "entitySetName");

            Uri tmp = Util.CreateUri(entitySetName, UriKind.RelativeOrAbsolute);
            if (tmp.IsAbsoluteUri ||
                !String.IsNullOrEmpty(Util.CreateUri(this.baseUriWithSlash, tmp)
                                     .GetComponents(UriComponents.Query | UriComponents.Fragment, UriFormat.SafeUnescaped)))
            {
                throw Error.Argument(Strings.Context_EntitySetName, "entitySetName");
            }
        }

        /// <summary>create this.identityToResource when necessary</summary>
        private void EnsureIdentityToResource()
        {
            if (null == this.identityToDescriptor)
            {
                System.Threading.Interlocked.CompareExchange(ref this.identityToDescriptor, new Dictionary<String, EntityDescriptor>(EqualityComparer<String>.Default), null);
            }
        }

        /// <summary>
        /// increment the resource change for sorting during submit changes
        /// </summary>
        /// <param name="descriptor">the resource to update the change order</param>
        private void IncrementChange(Descriptor descriptor)
        {
            descriptor.ChangeOrder = ++this.nextChange;
        }

        /// <summary>
        /// This method creates an async result object around a request to get the read stream for a Media Resource
        /// associated with the Media Link Entry represented by the entity object.
        /// </summary>
        /// <param name="entity">The entity which is the Media Link Entry for the requested Media Resource. Thist must specify
        /// a tracked entity in a non-added state.</param>
        /// <param name="args">Instance of <see cref="DataServiceRequestArgs"/> class with additional metadata for the request.
        /// Must not be null.</param>
        /// <param name="callback">User defined callback to be called when results are available. Can be null.</param>
        /// <param name="state">User state in IAsyncResult. Can be null.</param>
        /// <returns>The async result object for the request, the request hasn't been started yet.</returns>
        /// <exception cref="ArgumentNullException">Either entity or args parameters are null.</exception>
        /// <exception cref="ArgumentException">The specified entity is either not tracked, 
        /// is in the added state or it's not an MLE.</exception>
        private GetReadStreamResult CreateGetReadStreamResult(
            object entity, 
            DataServiceRequestArgs args,
            AsyncCallback callback, 
            object state)
        {
            EntityDescriptor box = this.EnsureContained(entity, "entity");
            Util.CheckArgumentNull(args, "args");

            Uri requestUri = box.GetMediaResourceUri(this.baseUriWithSlash);
            if (requestUri == null)
            {
                throw new ArgumentException(Strings.Context_EntityNotMediaLinkEntry, "entity");
            }

#if ASTORIA_LIGHT
            // For MR requests always use the ClientHtpp stack as the XHR doesn't support binary content
            HttpWebRequest request = this.CreateRequest(requestUri, XmlConstants.HttpMethodGet, true, null, null, false, HttpStack.ClientHttp);
#else
            HttpWebRequest request = this.CreateRequest(requestUri, XmlConstants.HttpMethodGet, true, null, null, false);
#endif

            WebUtil.ApplyHeadersToRequest(args.Headers, request, false);

            return new GetReadStreamResult(this, "GetReadStream", request, callback, state);
        }

        /// <summary>Stream wrapper for MR POST/PUT which also holds the information if the stream should be closed or not.</summary>
        internal class DataServiceSaveStream
        {
            /// <summary>The stream we are wrapping.
            /// Can be null in which case we didn't open it yet.</summary>
            private readonly Stream stream;

            // This class would one day hold a Func<Stream> to open the stream when needed
            //   instead of specifying the stream up front.

            /// <summary>Set to true if the stream should be closed once we're done with it.</summary>
            private readonly bool close;

            /// <summary>Arguments for the request when POST/PUT of the stream is issued.</summary>
            private readonly DataServiceRequestArgs args;

            /// <summary>
            /// Constructor
            /// </summary>
            /// <param name="stream">The stream to use.</param>
            /// <param name="close">Should the stream be closed before SaveChanges returns.</param>
            /// <param name="args">Additional arguments to apply to the request before sending it.</param>
            internal DataServiceSaveStream(Stream stream, bool close, DataServiceRequestArgs args)
            {
                Debug.Assert(stream != null, "stream must not be null.");

                this.stream = stream;
                this.close = close;
                this.args = args;
            }

            /// <summary>The stream to use.</summary>
            internal Stream Stream
            {
                get 
                {
                    return this.stream;
                }
            }

            /// <summary>
            /// Arguments to be used for creation of the HTTP request when POST/PUT for the MR is issued.
            /// </summary>
            internal DataServiceRequestArgs Args
            {
                get { return this.args; }
            }

            /// <summary>
            /// Close the stream if required.
            /// This is so that callers can simply call this method and don't have to care about the settings.
            /// </summary>
            internal void Close()
            {
                if (this.stream != null && this.close)
                {
                    this.stream.Close();
                }
            }
        }

        /// <summary>wrapper around loading a property from a response</summary>
        private class LoadPropertyResult : QueryResult
        {
            #region Private fields.

            /// <summary>entity whose property is being loaded</summary>
            private readonly object entity;

            /// <summary>Projection plan for loading results; possibly null.</summary>
            private readonly ProjectionPlan plan;

            /// <summary>name of the property on the entity that is being loaded</summary>
            private readonly string propertyName;

            #endregion Private fields.

            /// <summary>constructor</summary>
            /// <param name="entity">entity</param>
            /// <param name="propertyName">name of collection or reference property to load</param>
            /// <param name="context">Originating context</param>
            /// <param name="request">Originating WebRequest</param>
            /// <param name="callback">user callback</param>
            /// <param name="state">user state</param>
            /// <param name="dataServiceRequest">request object.</param>
            /// <param name="plan">Projection plan for materialization; possibly null.</param>
            internal LoadPropertyResult(object entity, string propertyName, DataServiceContext context, HttpWebRequest request, AsyncCallback callback, object state, DataServiceRequest dataServiceRequest, ProjectionPlan plan)
                : base(context, "LoadProperty", dataServiceRequest, request, callback, state)
            {
                this.entity = entity;
                this.propertyName = propertyName;
                this.plan = plan;
            }

            /// <summary>
            /// loading a property from a response
            /// </summary>
            /// <returns>QueryOperationResponse instance containing information about the response.</returns>
            internal QueryOperationResponse LoadProperty()
            {
                MaterializeAtom results = null;

                DataServiceContext context = (DataServiceContext)this.Source;

                ClientType type = ClientType.Create(this.entity.GetType());
                Debug.Assert(type.IsEntityType, "must be entity type to be contained");

                EntityDescriptor box = context.EnsureContained(this.entity, "entity");

                if (EntityStates.Added == box.State)
                {
                    throw Error.InvalidOperation(Strings.Context_NoLoadWithInsertEnd);
                }

                ClientType.ClientProperty property = type.GetProperty(this.propertyName, false);
                Type elementType = property.CollectionType ?? property.NullablePropertyType;
                try
                {
                    if (type.MediaDataMember == property)
                    {
                        results = this.ReadPropertyFromRawData(property);
                    }
                    else
                    {
                        results = this.ReadPropertyFromAtom(box, property);
                    }
                    
                    return this.GetResponseWithType(results, elementType);
                }
                catch (InvalidOperationException ex)
                {
                    QueryOperationResponse response = this.GetResponseWithType(results, elementType);
                    if (response != null)
                    {
                        response.Error = ex;
                        throw new DataServiceQueryException(Strings.DataServiceException_GeneralError, ex, response);
                    }

                    throw;
                }
            }

            /// <summary>
            /// Reads the data from the response stream into a buffer using the content length.
            /// </summary>
            /// <param name="responseStream">Response stream.</param>
            /// <param name="totalLength">Length of data to read.</param>
            /// <returns>byte array containing read data.</returns>
            private static byte[] ReadByteArrayWithContentLength(Stream responseStream, int totalLength)
            {
                byte[] buffer = new byte[totalLength];
                int read = 0;
                while (read < totalLength)
                {
                    int r = responseStream.Read(buffer, read, totalLength - read);
                    if (r <= 0)
                    {
                        throw Error.InvalidOperation(Strings.Context_UnexpectedZeroRawRead);
                    }

                    read += r;
                }

                return buffer;
            }

            /// <summary>Reads the data from the response stream in chunks.</summary>
            /// <param name="responseStream">Response stream.</param>
            /// <returns>byte array containing read data.</returns>
            private static byte[] ReadByteArrayChunked(Stream responseStream)
            {
                byte[] completeBuffer = null;
                using (MemoryStream m = new MemoryStream())
                {
                    byte[] buffer = new byte[4096];
                    int numRead = 0;
                    int totalRead = 0;
                    while (true)
                    {
                        numRead = responseStream.Read(buffer, 0, buffer.Length);
                        if (numRead <= 0)
                        {
                            break;
                        }

                        m.Write(buffer, 0, numRead);
                        totalRead += numRead;
                    }

                    completeBuffer = new byte[totalRead];
                    m.Position = 0;
                    numRead = m.Read(completeBuffer, 0, completeBuffer.Length);
                }

                return completeBuffer;
            }

            /// <summary>
            /// Load property data from an ATOM response
            /// </summary>
            /// <param name="box">Box pointing to the entity to load this to</param>
            /// <param name="property">The property being loaded</param>
            /// <returns>property values as IEnumerable.</returns>
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "Returning MaterializeAtom, caller will dispose")]
            private MaterializeAtom ReadPropertyFromAtom(EntityDescriptor box, ClientType.ClientProperty property)
            {
                DataServiceContext context = (DataServiceContext)this.Source;

                bool merging = context.ApplyingChanges;

                try
                {
                    context.ApplyingChanges = true;

                    bool deletedState = (EntityStates.Deleted == box.State);

                    Type nestedType;
#if ASTORIA_OPEN_OBJECT
                if (property.OpenObjectProperty)
                {
                    nestedType = typeof(OpenObject);
                }
                else
#endif
                    {
                        nestedType = property.CollectionType ?? property.NullablePropertyType;
                    }

                    ClientType clientType = ClientType.Create(nestedType);

                    // when setting a reference, use the entity
                    // when adding an item to a collection, use the collection object referenced by the entity
                    bool setNestedValue = false;
                    object collection = this.entity;
                    if (null != property.CollectionType)
                    {   // get the collection that we actually add nested
                        collection = property.GetValue(this.entity);
                        if (null == collection)
                        {
                            setNestedValue = true;
                            if (BindingEntityInfo.IsDataServiceCollection(property.PropertyType))
                            {
                                Debug.Assert(WebUtil.GetDataServiceCollectionOfT(nestedType) != null, "DataServiceCollection<> must be available here.");

                                // new DataServiceCollection<nestedType>(null, TrackingMode.None)
                                collection = Activator.CreateInstance(
                                    WebUtil.GetDataServiceCollectionOfT(nestedType), 
                                    null,
                                    TrackingMode.None);
                            }
                            else
                            {
                                collection = Activator.CreateInstance(typeof(List<>).MakeGenericType(nestedType));
                            }
                        }
                    }

                    // store the results so that they can be there in the response body.
                    Type elementType = property.CollectionType ?? property.NullablePropertyType;
                    IList results = (IList)Activator.CreateInstance(typeof(List<>).MakeGenericType(elementType));

                    DataServiceQueryContinuation continuation = null;

                    // elementType.ElementType has Nullable stripped away, use nestedType for materializer
                    using (MaterializeAtom materializer = this.GetMaterializer(context, this.plan))
                    {
                        Debug.Assert(materializer != null, "materializer != null -- otherwise GetMaterializer() returned null rather than empty");
                        int count = 0;
#if ASTORIA_OPEN_OBJECT
                        object openProperties = null;
#endif
                        foreach (object child in materializer)
                        {
                            results.Add(child);
                            count++;
#if ASTORIA_OPEN_OBJECT
                            property.SetValue(collection, child, this.propertyName, ref openProperties, true);
#else
                            property.SetValue(collection, child, this.propertyName, true);
#endif

                            // via LoadProperty, you can have a property with <id> and null value
                            if ((null != child) && (MergeOption.NoTracking != materializer.MergeOptionValue) && clientType.IsEntityType)
                            {
                                if (deletedState)
                                {
                                    context.DeleteLink(this.entity, this.propertyName, child);
                                }
                                else
                                {   // put link into unchanged state
                                    context.AttachLink(this.entity, this.propertyName, child, materializer.MergeOptionValue);
                                }
                            }
                        }

                        // LoadProperty should only deal with single level collections
                        continuation = materializer.GetContinuation(null);
                        Util.SetNextLinkForCollection(collection, continuation);

                        // we don't do this because we are loading, not refreshing
                        // if ((0 == count) && (MergeOption.OverwriteChanges == this.mergeOption))
                        // { property.Clear(entity); }
                    }

                    if (setNestedValue)
                    {
#if ASTORIA_OPEN_OBJECT
                    object openProperties = null;
                    property.SetValue(this.entity, collection, this.propertyName, ref openProperties, false);
#else
                        property.SetValue(this.entity, collection, this.propertyName, false);
#endif
                    }

                    return MaterializeAtom.CreateWrapper(results, continuation);
                }
                finally
                {
                    context.ApplyingChanges = merging;
                }
            }

            /// <summary>
            /// Load property data form a raw response
            /// </summary>
            /// <param name="property">The property being loaded</param>
            /// <returns>property values as IEnumerable.</returns>
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "Returning MaterializeAtom, caller will dispose")]
            private MaterializeAtom ReadPropertyFromRawData(ClientType.ClientProperty property)
            {
                DataServiceContext context = (DataServiceContext)this.Source;

                bool merging = context.ApplyingChanges;

                try
                {
                    context.ApplyingChanges = true;

                    // if this is the data property for a media entry, what comes back
                    // is the raw value (no markup)
#if ASTORIA_OPEN_OBJECT
                object openProps = null;
#endif
                    string mimeType = null;
                    Encoding encoding = null;
                    Type elementType = property.CollectionType ?? property.NullablePropertyType;
                    IList results = (IList)Activator.CreateInstance(typeof(List<>).MakeGenericType(elementType));
                    HttpProcessUtility.ReadContentType(this.ContentType, out mimeType, out encoding);

                    using (Stream responseStream = this.GetResponseStream())
                    {
                        // special case byte[], and for everything else let std conversion kick-in
                        if (property.PropertyType == typeof(byte[]))
                        {
                            int total = checked((int)this.ContentLength);
                            byte[] buffer = null;
                            if (total >= 0)
                            {
                                buffer = LoadPropertyResult.ReadByteArrayWithContentLength(responseStream, total);
                            }
                            else
                            {
                                buffer = LoadPropertyResult.ReadByteArrayChunked(responseStream);
                            }

                            results.Add(buffer);
#if ASTORIA_OPEN_OBJECT
                            property.SetValue(this.entity, buffer, this.propertyName, ref openProps, false);
#else
                            property.SetValue(this.entity, buffer, this.propertyName, false);
#endif
                        }
                        else
                        {
                            // responseStream will disposed, StreamReader doesn't need to dispose of it.
                            StreamReader reader = new StreamReader(responseStream, encoding);
                            object convertedValue = property.PropertyType == typeof(string) ?
                                                        reader.ReadToEnd() :
                                                        ClientConvert.ChangeType(reader.ReadToEnd(), property.PropertyType);
                            results.Add(convertedValue);
#if ASTORIA_OPEN_OBJECT
                            property.SetValue(this.entity, convertedValue, this.propertyName, ref openProps, false);
#else
                            property.SetValue(this.entity, convertedValue, this.propertyName, false);
#endif
                        }
                    }

#if ASTORIA_OPEN_OBJECT
                Debug.Assert(openProps == null, "These should not be set in this path");
#endif
                    if (property.MimeTypeProperty != null)
                    {
                        // an implication of this 3rd-arg-null is that mime type properties cannot be open props
#if ASTORIA_OPEN_OBJECT
                    property.MimeTypeProperty.SetValue(this.entity, mimeType, null, ref openProps, false);
                    Debug.Assert(openProps == null, "These should not be set in this path");
#else
                        property.MimeTypeProperty.SetValue(this.entity, mimeType, null, false);
#endif
                    }

                    return MaterializeAtom.CreateWrapper(results);
                }
                finally
                {
                    context.ApplyingChanges = merging;
                }
            }
        }

        /// <summary>
        /// implementation of IAsyncResult for SaveChanges
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1001:TypesThatOwnDisposableFieldsShouldBeDisposable", Justification = "Pending")]
        private class SaveResult : BaseAsyncResult
        {
            /// <summary>where to pull the changes from</summary>
            private readonly DataServiceContext Context;

            /// <summary>sorted list of entries by change order</summary>
            private readonly List<Descriptor> ChangedEntries;

            /// <summary>array of queries being executed</summary>
            private readonly DataServiceRequest[] Queries;

            /// <summary>operations</summary>
            private readonly List<OperationResponse> Responses;

            /// <summary>boundary used when generating batch boundary</summary>
            private readonly string batchBoundary;

            /// <summary>option in use for SaveChanges</summary>
            private readonly SaveChangesOptions options;

            /// <summary>if true then async, else sync</summary>
            private readonly bool executeAsync;

            /// <summary>debugging trick to track number of completed requests</summary>
            private int changesCompleted;

            /// <summary>wrapped request</summary>
            private PerRequest request;

            /// <summary>batch web response</summary>
            private HttpWebResponse batchResponse;

            /// <summary>response stream for the batch</summary>
            private Stream httpWebResponseStream;

            /// <summary>service response</summary>
            private DataServiceResponse service;

            /// <summary>The ResourceBox or RelatedEnd currently in flight</summary>
            private int entryIndex = -1;

            /// <summary>
            /// True if the current in-flight request is an MR POST or PUT
            /// that might need to be followed by a PUT for the MLE
            /// </summary>
            private bool processingMediaLinkEntry;

            /// <summary>
            /// True if the current in-flight request is an MR PUT
            /// that might need to be followed by a PUT for the MLE
            /// </summary>
            private bool processingMediaLinkEntryPut;

            /// <summary>
            /// If the <see cref="processingMediaLinkEntry"/> is set to true
            /// this field holds a stream which contains the body of the MR POST request
            /// to be sent.
            /// This can be null in the case where the content of MR is empty. (In which case
            /// we will not try to open the request stream and thus avoid additional async call).
            /// </summary>
            private Stream mediaResourceRequestStream;

            /// <summary>response stream</summary>
            private BatchStream responseBatchStream;

            /// <summary>temporary buffer when cache results from CUD op in non-batching save changes</summary>
            private byte[] buildBatchBuffer;

            /// <summary>temporary writer when cache results from CUD op in non-batching save changes</summary>
            private StreamWriter buildBatchWriter;

            /// <summary>count of data actually copied</summary>
            private long copiedContentLength;

            /// <summary>what is the changset boundary</summary>
            private string changesetBoundary;

            /// <summary>is a change set being cached</summary>
            private bool changesetStarted;

            #region constructors
            /// <summary>
            /// constructor for operations
            /// </summary>
            /// <param name="context">context</param>
            /// <param name="method">method</param>
            /// <param name="queries">queries</param>
            /// <param name="options">options</param>
            /// <param name="callback">user callback</param>
            /// <param name="state">user state object</param>
            /// <param name="async">async or sync</param>
            internal SaveResult(DataServiceContext context, string method, DataServiceRequest[] queries, SaveChangesOptions options, AsyncCallback callback, object state, bool async)
                : base(context, method, callback, state)
            {
                this.executeAsync = async;
                this.Context = context;
                this.Queries = queries;
                this.options = options;

                this.Responses = new List<OperationResponse>();

                if (null == queries)
                {
                    #region changed entries
                    this.ChangedEntries = context.entityDescriptors.Values.Cast<Descriptor>()
                                          .Union(context.bindings.Values.Cast<Descriptor>())
                                          .Where(o => o.IsModified && o.ChangeOrder != UInt32.MaxValue)
                                          .OrderBy(o => o.ChangeOrder)
                                          .ToList();

                    foreach (Descriptor e in this.ChangedEntries)
                    {
                        e.ContentGeneratedForSave = false;
                        e.SaveResultWasProcessed = 0;
                        e.SaveError = null;

                        if (!e.IsResource)
                        {
                            object target = ((LinkDescriptor)e).Target;
                            if (null != target)
                            {
                                Descriptor f = context.entityDescriptors[target];
                                if (EntityStates.Unchanged == f.State)
                                {
                                    f.ContentGeneratedForSave = false;
                                    f.SaveResultWasProcessed = 0;
                                    f.SaveError = null;
                                }
                            }
                        }
                    }
                    #endregion
                }
                else
                {
                    this.ChangedEntries = new List<Descriptor>();
                }

                if (IsFlagSet(options, SaveChangesOptions.Batch))
                {
                    this.batchBoundary = XmlConstants.HttpMultipartBoundaryBatch + "_" + Guid.NewGuid().ToString();
                }
                else
                {
                    this.batchBoundary = XmlConstants.HttpMultipartBoundaryBatchResponse + "_" + Guid.NewGuid().ToString();
                    this.DataServiceResponse = new DataServiceResponse(null, -1, this.Responses, false /*batchResponse*/);
                }
            }
            #endregion constructor

            #region end

            /// <summary>generate the batch request of all changes to save</summary>
            internal DataServiceResponse DataServiceResponse
            {
                get
                {
                    return this.service;
                }

                set
                {
                    this.service = value;
                }
            }

            /// <summary>process the batch</summary>
            /// <returns>data service response</returns>
            internal DataServiceResponse EndRequest()
            {
                // Close all Save streams before we return (and do this before we throw for any errors below)
                foreach (EntityDescriptor box in this.ChangedEntries.Where(e => e.IsResource).Cast<EntityDescriptor>())
                {
                    box.CloseSaveStream();
                }

                // This will process the responses and throw if failure was detected
                if ((null != this.responseBatchStream) || (null != this.httpWebResponseStream))
                {
                    this.HandleBatchResponse();
                }

                return this.DataServiceResponse;
            }

            #endregion

            #region start a batch

            /// <summary>initial the async batch save changeset</summary>
            /// <param name="replaceOnUpdate">whether we need to update MERGE or PUT method for update.</param>
            internal void BatchBeginRequest(bool replaceOnUpdate)
            {
                PerRequest pereq = null;
                try
                {
                    MemoryStream memory = this.GenerateBatchRequest(replaceOnUpdate);
                    if (null != memory)
                    {
                        HttpWebRequest httpWebRequest = this.CreateBatchRequest(memory);
                        this.Abortable = httpWebRequest;

                        this.request = pereq = new PerRequest();
                        pereq.Request = httpWebRequest;
                        pereq.RequestContentStream = new PerRequest.ContentStream(memory, true);

                        this.httpWebResponseStream = new MemoryStream();

                        IAsyncResult asyncResult = BaseAsyncResult.InvokeAsync(httpWebRequest.BeginGetRequestStream, this.AsyncEndGetRequestStream, pereq);
                        pereq.RequestCompletedSynchronously &= asyncResult.CompletedSynchronously;
                    }
                    else
                    {
                        Debug.Assert(this.CompletedSynchronously, "completedSynchronously");
                        Debug.Assert(this.IsCompletedInternally, "completed");
                    }
                }
                catch (Exception e)
                {
                    this.HandleFailure(pereq, e);
                    throw; // to user on BeginSaveChangeSet, will still invoke Callback
                }
                finally
                {
                    this.HandleCompleted(pereq); // will invoke user callback
                }

                Debug.Assert((this.CompletedSynchronously && this.IsCompleted) || !this.CompletedSynchronously, "sync without complete");
            }

#if !ASTORIA_LIGHT // Synchronous methods not available
            /// <summary>
            /// Synchronous batch request
            /// </summary>
            /// <param name="replaceOnUpdate">whether we need to update MERGE or PUT method for update.</param>
            internal void BatchRequest(bool replaceOnUpdate)
            {
                MemoryStream memory = this.GenerateBatchRequest(replaceOnUpdate);
                if ((null != memory) && (0 < memory.Length))
                {
                    HttpWebRequest httpWebRequest = this.CreateBatchRequest(memory);
                    using (System.IO.Stream requestStream = httpWebRequest.GetRequestStream())
                    {
                        byte[] buffer = memory.GetBuffer();
                        int bufferOffset = checked((int)memory.Position);
                        int bufferLength = checked((int)memory.Length) - bufferOffset;

                        // the following is useful in the debugging Immediate Window
                        // string x = System.Text.Encoding.UTF8.GetString(buffer, bufferOffset, bufferLength);
                        requestStream.Write(buffer, bufferOffset, bufferLength);
                    }

                    HttpWebResponse httpWebResponse = (HttpWebResponse)httpWebRequest.GetResponse();
                    this.batchResponse = httpWebResponse;

                    if (null != httpWebResponse)
                    {
                        this.httpWebResponseStream = httpWebResponse.GetResponseStream();
                    }
                }
            }
#endif
            #endregion

            #region start a non-batch requests
            /// <summary>
            /// This starts the next change
            /// </summary>
            /// <param name="replaceOnUpdate">whether we need to update MERGE or PUT method for update.</param>
            internal void BeginNextChange(bool replaceOnUpdate)
            {
                Debug.Assert(!this.IsCompletedInternally, "why being called if already completed?");

                // SaveCallback can't chain synchronously completed responses, caller will loop the to next change
                PerRequest pereq = null;
                IAsyncResult asyncResult = null;
                do
                {
                    HttpWebRequest httpWebRequest = null;
                    HttpWebResponse response = null;
                    try
                    {
                        if (null != this.request)
                        {
                            this.SetCompleted();
                            Error.ThrowInternalError(InternalError.InvalidBeginNextChange);
                        }

                        this.Abortable = httpWebRequest = this.CreateNextRequest(replaceOnUpdate);
                        if ((null != httpWebRequest) || (this.entryIndex < this.ChangedEntries.Count))
                        {
                            if (this.ChangedEntries[this.entryIndex].ContentGeneratedForSave)
                            {
                                Debug.Assert(this.ChangedEntries[this.entryIndex] is LinkDescriptor, "only expected RelatedEnd to presave");
                                Debug.Assert(
                                    this.ChangedEntries[this.entryIndex].State == EntityStates.Added ||
                                    this.ChangedEntries[this.entryIndex].State == EntityStates.Modified,
                                    "only expected added to presave");
                                continue;
                            }

                            PerRequest.ContentStream contentStream = this.CreateChangeData(this.entryIndex, false);
                            if (this.executeAsync)
                            {
                                #region async
                                this.request = pereq = new PerRequest();
                                pereq.Request = httpWebRequest;

                                if (null == contentStream || null == contentStream.Stream)
                                {
                                    asyncResult = BaseAsyncResult.InvokeAsync(httpWebRequest.BeginGetResponse, this.AsyncEndGetResponse, pereq);
                                }
                                else
                                {
                                    if (contentStream.IsKnownMemoryStream)
                                    {
                                        httpWebRequest.ContentLength = contentStream.Stream.Length - contentStream.Stream.Position;
                                    }

                                    pereq.RequestContentStream = contentStream;
                                    asyncResult = BaseAsyncResult.InvokeAsync(httpWebRequest.BeginGetRequestStream, this.AsyncEndGetRequestStream, pereq);
                                }

                                pereq.RequestCompletedSynchronously &= asyncResult.CompletedSynchronously;
                                this.CompletedSynchronously &= asyncResult.CompletedSynchronously;
                                #endregion
                            }
#if !ASTORIA_LIGHT // Synchronous methods not available
                            else
                            {
                                #region sync
                                if (null != contentStream && null != contentStream.Stream)
                                {
                                    if (contentStream.IsKnownMemoryStream)
                                    {
                                        httpWebRequest.ContentLength = contentStream.Stream.Length - contentStream.Stream.Position;
                                    }

                                    using (Stream stream = httpWebRequest.GetRequestStream())
                                    {
                                        byte[] buffer = new byte[64 * 1024];
                                        int read;
                                        do
                                        {
                                            read = contentStream.Stream.Read(buffer, 0, buffer.Length);
                                            if (read > 0)
                                            {
                                                stream.Write(buffer, 0, read);
                                            }
                                        }
                                        while (read > 0);
                                    }
                                }

                                response = (HttpWebResponse)httpWebRequest.GetResponse();
                                if (!this.processingMediaLinkEntry)
                                {
                                    this.changesCompleted++;
                                }

                                this.HandleOperationResponse(response);
                                this.HandleOperationResponseData(response);
                                this.HandleOperationEnd();
                                this.request = null;
                                #endregion
                            }
#endif
                        }
                        else
                        {
                            this.SetCompleted();

                            if (this.CompletedSynchronously)
                            {
                                this.HandleCompleted(pereq);
                            }
                        }
                    }
                    catch (InvalidOperationException e)
                    {
                        WebUtil.GetHttpWebResponse(e, ref response);
                        this.HandleOperationException(e, response);
                        this.HandleCompleted(pereq);
                    }
                    finally
                    {
                        if (null != response)
                        {
                            response.Close();
                        }
                    }

                    // either everything completed synchronously until a change is saved and its state changed
                    // and we don't return to this loop until then or something was asynchronous
                    // and we won't continue in this loop, instead letting the inner most loop start the next request
                }
                while (((null == pereq) || (pereq.RequestCompleted && asyncResult != null && asyncResult.CompletedSynchronously)) && !this.IsCompletedInternally);

                Debug.Assert(this.executeAsync || this.CompletedSynchronously, "sync !CompletedSynchronously");
                Debug.Assert((this.CompletedSynchronously && this.IsCompleted) || !this.CompletedSynchronously, "sync without complete");
                Debug.Assert(this.entryIndex < this.ChangedEntries.Count || this.ChangedEntries.All(o => o.ContentGeneratedForSave), "didn't generate content for all entities/links");
            }

            /// <summary>cleanup work to do once the batch / savechanges is complete</summary>
            protected override void CompletedRequest()
            {
                this.buildBatchBuffer = null;
                if (null != this.buildBatchWriter)
                {
                    Debug.Assert(!IsFlagSet(this.options, SaveChangesOptions.Batch), "should be non-batch");
                    this.HandleOperationEnd();
                    this.buildBatchWriter.WriteLine("--{0}--", this.batchBoundary);

                    this.buildBatchWriter.Flush();
                    Debug.Assert(Object.ReferenceEquals(this.httpWebResponseStream, this.buildBatchWriter.BaseStream), "expected different stream");
                    this.httpWebResponseStream.Position = 0;

                    this.buildBatchWriter = null;

                    // the following is useful in the debugging Immediate Window
                    // string x = System.Text.Encoding.UTF8.GetString(memoryStream.GetBuffer(), 0, (int)memoryStream.Length);
                    this.responseBatchStream = new BatchStream(this.httpWebResponseStream, this.batchBoundary, HttpProcessUtility.EncodingUtf8NoPreamble, false);
                }
            }

            /// <summary>verify non-null and not completed</summary>
            /// <param name="value">the request in progress</param>
            /// <param name="errorcode">error code if null or completed</param>
            private static void CompleteCheck(PerRequest value, InternalError errorcode)
            {
                if ((null == value) || value.RequestCompleted)
                {
                    // since PerRequest is nested, it won't get set true during Abort unlike BaseAsyncResult
                    // but like QueryAsyncResult, when the request is aborted it it lets the request throw on next operation
                    Error.ThrowInternalError(errorcode);
                }
            }

            /// <summary>verify they have the same reference</summary>
            /// <param name="actual">the actual thing</param>
            /// <param name="expected">the expected thing</param>
            /// <param name="errorcode">error code if they are not</param>
            private static void EqualRefCheck(PerRequest actual, PerRequest expected, InternalError errorcode)
            {
                if (!Object.ReferenceEquals(actual, expected))
                {
                    Error.ThrowInternalError(errorcode);
                }
            }

            /// <summary>Set the AsyncWait and invoke the user callback.</summary>
            /// <param name="pereq">the request object</param>
            private void HandleCompleted(PerRequest pereq)
            {
                if (null != pereq)
                {
                    this.CompletedSynchronously &= pereq.RequestCompletedSynchronously;

                    if (pereq.RequestCompleted)
                    {
                        System.Threading.Interlocked.CompareExchange(ref this.request, null, pereq);
                        if (IsFlagSet(this.options, SaveChangesOptions.Batch))
                        {   // all competing thread must complete this before user calback is invoked
                            System.Threading.Interlocked.CompareExchange(ref this.batchResponse, pereq.HttpWebResponse, null);
                            pereq.HttpWebResponse = null;
                        }

                        pereq.Dispose();
                    }
                }

                this.HandleCompleted();
            }

            /// <summary>Cache the exception that happened on the background thread for the caller of EndSaveChanges.</summary>
            /// <param name="pereq">the request object</param>
            /// <param name="e">exception object from background thread</param>
            /// <returns>true if the exception should be rethrown</returns>
            private bool HandleFailure(PerRequest pereq, Exception e)
            {
                if (null != pereq)
                {
                    if (IsAborted)
                    {
                        pereq.SetAborted();
                    }
                    else
                    {
                        pereq.SetComplete();
                    }
                }

                return this.HandleFailure(e);
            }

            /// <summary>
            /// Create HttpWebRequest from the next availabe resource
            /// </summary>
            /// <param name="replaceOnUpdate">whether we need to update MERGE or PUT method for update.</param>
            /// <returns>web request</returns>
            private HttpWebRequest CreateNextRequest(bool replaceOnUpdate)
            {
                if (!this.processingMediaLinkEntry)
                {
                    this.entryIndex++;
                }
                else
                {
                    // If the previous request was an MR request the next one might be a PUT for the MLE
                    //   but if the entity was not changed (just the MR changed) no PUT for MLE should be sent
                    Debug.Assert(this.ChangedEntries[this.entryIndex].IsResource, "Only resources can have MR's.");
                    EntityDescriptor box = (EntityDescriptor)this.ChangedEntries[this.entryIndex];
                    if (this.processingMediaLinkEntryPut && EntityStates.Unchanged == box.State)
                    {
                        // Only the MR changed. In this case we also need to mark the entry as processed to notify 
                        //   that the content for save has been generated as there's not going to be another request for it.
                        box.ContentGeneratedForSave = true;
                        this.entryIndex++;
                    }

                    this.processingMediaLinkEntry = false;
                    this.processingMediaLinkEntryPut = false;

                    // In any case also close the save stream if there's any and forget about it
                    // for POST this is just a good practice to do so as soon as possible
                    // for PUT it's actually required for us to recognize that we already processed the MR part of the change
                    box.CloseSaveStream();
                }

                if (unchecked((uint)this.entryIndex < (uint)this.ChangedEntries.Count))
                {
                    Descriptor entry = this.ChangedEntries[this.entryIndex];
                    if (entry.IsResource)
                    {
                        EntityDescriptor box = (EntityDescriptor)entry;

                        HttpWebRequest req;
                        if (((EntityStates.Unchanged == entry.State) || (EntityStates.Modified == entry.State)) &&
                            (null != (req = this.CheckAndProcessMediaEntryPut(box))))
                        {
                            this.processingMediaLinkEntry = true;
                            this.processingMediaLinkEntryPut = true;
                        }
                        else if ((EntityStates.Added == entry.State) && (null != (req = this.CheckAndProcessMediaEntryPost(box))))
                        {
                            this.processingMediaLinkEntry = true;
                            this.processingMediaLinkEntryPut = false;
                        }
                        else
                        {
                            Debug.Assert(!this.processingMediaLinkEntry || entry.State == EntityStates.Modified, "!this.processingMediaLinkEntry || entry.State == EntityStates.Modified");
                            req = this.Context.CreateRequest(box, entry.State, replaceOnUpdate);
                        }

                        return req;
                    }

                    return this.Context.CreateRequest((LinkDescriptor)entry);
                }

                return null;
            }

            /// <summary>
            /// Check to see if the resource to be inserted is a media entry, and if so
            /// setup a POST request for the media content first and turn the rest of 
            /// the operation into a PUT to update the rest of the properties.
            /// </summary>
            /// <param name="entityDescriptor">The resource to check/process</param>
            /// <returns>A web request setup to do POST the media resource</returns>
            private HttpWebRequest CheckAndProcessMediaEntryPost(EntityDescriptor entityDescriptor)
            {
                // 
                ClientType type = ClientType.Create(entityDescriptor.Entity.GetType());

                if (!type.IsMediaLinkEntry && !entityDescriptor.IsMediaLinkEntry)
                {
                    // this is not a media link entry, process normally
                    return null;
                }

                if (type.MediaDataMember == null && entityDescriptor.SaveStream == null)
                {
                    // The entity is marked as MLE but we don't have the content property
                    //   and the user didn't set the save stream.
                    throw Error.InvalidOperation(Strings.Context_MLEWithoutSaveStream(type.ElementTypeName));
                }

                Debug.Assert(
                    (type.MediaDataMember != null && entityDescriptor.SaveStream == null) ||
                    (type.MediaDataMember == null && entityDescriptor.SaveStream != null),
                    "Only one way of specifying the MR content is allowed.");

                HttpWebRequest mediaRequest = this.CreateMediaResourceRequest(
                    entityDescriptor.GetResourceUri(this.Context.baseUriWithSlash, false /*queryLink*/),
                    XmlConstants.HttpMethodPost,
                    type.MediaDataMember == null);

                if (type.MediaDataMember != null)
                {
                    if (type.MediaDataMember.MimeTypeProperty == null)
                    {
                        mediaRequest.ContentType = XmlConstants.MimeApplicationOctetStream;
                    }
                    else
                    {
                        object mimeTypeValue = type.MediaDataMember.MimeTypeProperty.GetValue(entityDescriptor.Entity);
                        String mimeType = mimeTypeValue != null ? mimeTypeValue.ToString() : null;

                        if (String.IsNullOrEmpty(mimeType))
                        {
                            throw Error.InvalidOperation(
                                Strings.Context_NoContentTypeForMediaLink(
                                    type.ElementTypeName,
                                    type.MediaDataMember.MimeTypeProperty.PropertyName));
                        }

                        mediaRequest.ContentType = mimeType;
                    }

                    object value = type.MediaDataMember.GetValue(entityDescriptor.Entity);
                    if (value == null)
                    {
                        mediaRequest.ContentLength = 0;
                        this.mediaResourceRequestStream = null;
                    }
                    else
                    {
                        byte[] buffer = value as byte[];
                        if (buffer == null)
                        {
                            string mime;
                            Encoding encoding;
                            HttpProcessUtility.ReadContentType(mediaRequest.ContentType, out mime, out encoding);

                            if (encoding == null)
                            {
                                encoding = Encoding.UTF8;
                                mediaRequest.ContentType += XmlConstants.MimeTypeUtf8Encoding;
                            }

                            buffer = encoding.GetBytes(ClientConvert.ToString(value, false /* atomDateConstruct */));
                        }

                        mediaRequest.ContentLength = buffer.Length;

                        // Need to specify that the buffer is publicly visible as we need to access it later on
                        this.mediaResourceRequestStream = new MemoryStream(buffer, 0, buffer.Length, false, true);
                    }
                }
                else
                {
                    this.SetupMediaResourceRequest(mediaRequest, entityDescriptor);
                }

                // Convert the insert into an update for the media link entry we just created
                // (note that the identity still needs to be fixed up on the resbox once
                // the response comes with the 'location' header; that happens during processing
                // of the response in SavedResource())
                entityDescriptor.State = EntityStates.Modified;

                return mediaRequest;
            }

            /// <summary>
            /// Checks if the resource box represents an MLE with modified MR and if so creates a PUT request
            ///   to update the MR.
            /// </summary>
            /// <param name="box">The resource box for the entity to be checked.</param>
            /// <returns>Newly created MR PUT request or null if the entity is not MLE or its MR hasn't changed.</returns>
            private HttpWebRequest CheckAndProcessMediaEntryPut(EntityDescriptor box)
            {
                // If there's no save stream associated with the entity it's not MLE or its MR hasn't changed
                //  (which for purposes of PUT is the same anyway)
                if (box.SaveStream == null)
                {
                    return null;
                }

                Uri requestUri = box.GetEditMediaResourceUri(this.Context.baseUriWithSlash);
                if (requestUri == null)
                {
                    throw Error.InvalidOperation(
                        Strings.Context_SetSaveStreamWithoutEditMediaLink);
                }

                HttpWebRequest mediaResourceRequest = this.CreateMediaResourceRequest(requestUri, XmlConstants.HttpMethodPut, true);
                this.SetupMediaResourceRequest(mediaResourceRequest, box);

                if (box.StreamETag != null)
                {
                    mediaResourceRequest.Headers.Set(HttpRequestHeader.IfMatch, box.StreamETag);
                }

                return mediaResourceRequest;
            }

            /// <summary>
            /// Creates HTTP request for the media resource (MR)
            /// </summary>
            /// <param name="requestUri">The URI to request</param>
            /// <param name="method">The HTTP method to use (POST or PUT)</param>
            /// <param name="sendChunked">Send the request using chunked encoding to avoid buffering.</param>
            /// <returns>The newly created HTTP request object.</returns>
            private HttpWebRequest CreateMediaResourceRequest(Uri requestUri, string method, bool sendChunked)
            {
#if ASTORIA_LIGHT
                // For MR requests always use the ClientHtpp stack as the XHR doesn't support binary content
                HttpWebRequest mediaResourceRequest = this.Context.CreateRequest(
                    requestUri,
                    method,
                    false,
                    XmlConstants.MimeAny,
                    Util.DataServiceVersion1,
                    sendChunked,
                    HttpStack.ClientHttp);
#else
                HttpWebRequest mediaResourceRequest = this.Context.CreateRequest(
                    requestUri,
                    method,
                    false,
                    XmlConstants.MimeAny,
                    Util.DataServiceVersion1,
                    sendChunked);
#endif
                return mediaResourceRequest;
            }

            /// <summary>
            /// Sets the content and the headers of the media resource request
            /// </summary>
            /// <param name="mediaResourceRequest">The request to setup</param>
            /// <param name="box">The resource box for the entity with the MT</param>
            /// <remarks>This only works with the V2 MR support (SetSaveStream), this will not setup
            /// the request for V1 property based MRs.</remarks>
            private void SetupMediaResourceRequest(HttpWebRequest mediaResourceRequest, EntityDescriptor box)
            {
                // Get the write stream for this MR
                this.mediaResourceRequestStream = box.SaveStream.Stream;

                // Apply the arguments for the request
                WebUtil.ApplyHeadersToRequest(box.SaveStream.Args.Headers, mediaResourceRequest, true);

                // Do NOT set the ContentLength since we don't know if the stream even supports reporting its length
            }

            /// <summary>
            /// create memory stream for entry (entity or link)
            /// </summary>
            /// <param name="index">index into changed entries</param>
            /// <param name="newline">include newline in output</param>
            /// <returns>stream of data for entry</returns>
            private PerRequest.ContentStream CreateChangeData(int index, bool newline)
            {
                Descriptor entry = this.ChangedEntries[index];
                Debug.Assert(!entry.ContentGeneratedForSave, "already saved entity/link");

                if (entry.IsResource)
                {
                    EntityDescriptor box = (EntityDescriptor)entry;
                    if (this.processingMediaLinkEntry)
                    {
                        Debug.Assert(
                            this.processingMediaLinkEntryPut || entry.State == EntityStates.Modified, 
                            "We should have modified the MLE state to Modified when we've created the MR POST request.");
                        Debug.Assert(
                            !this.processingMediaLinkEntryPut || (entry.State == EntityStates.Unchanged || entry.State == EntityStates.Modified),
                            "If we're processing MR PUT the entity must be either in Unchanged or Modified state.");

                        // media resource request - we already precreated the body of the request
                        // in the CheckAndProcessMediaEntryPost or CheckAndProcessMediaEntryPut method.
                        Debug.Assert(this.mediaResourceRequestStream != null, "We should have precreated the MR stream already.");
                        return new PerRequest.ContentStream(this.mediaResourceRequestStream, false);
                    }
                    else
                    {
                        // either normal entity or second call for media link entity, generate content payload
                        // else first call of media link entry where we only send the default value
                        entry.ContentGeneratedForSave = true;
                        return new PerRequest.ContentStream(this.Context.CreateRequestData(box, newline), true);
                    }
                }
                else
                {
                    entry.ContentGeneratedForSave = true;
                    LinkDescriptor link = (LinkDescriptor)entry;
                    if ((EntityStates.Added == link.State) ||
                        ((EntityStates.Modified == link.State) && (null != link.Target)))
                    {
                        return new PerRequest.ContentStream(this.Context.CreateRequestData(link, newline), true);
                    }
                }

                return null;
            }
            #endregion

            #region generate batch response from non-batch

            /// <summary>basic separator between response</summary>
            private void HandleOperationStart()
            {
                this.HandleOperationEnd();

                if (null == this.httpWebResponseStream)
                {
                    this.httpWebResponseStream = new MemoryStream();
                }

                if (null == this.buildBatchWriter)
                {
                    this.buildBatchWriter = new StreamWriter(this.httpWebResponseStream);     // defaults to UTF8 w/o preamble
#if TESTUNIXNEWLINE
                    this.buildBatchWriter.NewLine = NewLine;
#endif
                }

                if (null == this.changesetBoundary)
                {
                    this.changesetBoundary = XmlConstants.HttpMultipartBoundaryChangesetResponse + "_" + Guid.NewGuid().ToString();
                }

                this.changesetStarted = true;
                this.buildBatchWriter.WriteLine("--{0}", this.batchBoundary);
                this.buildBatchWriter.WriteLine("{0}: {1}; boundary={2}", XmlConstants.HttpContentType, XmlConstants.MimeMultiPartMixed, this.changesetBoundary);
                this.buildBatchWriter.WriteLine();
                this.buildBatchWriter.WriteLine("--{0}", this.changesetBoundary);
            }

            /// <summary>write the trailing --changesetboundary--</summary>
            private void HandleOperationEnd()
            {
                if (this.changesetStarted)
                {
                    Debug.Assert(null != this.buildBatchWriter, "buildBatchWriter");
                    Debug.Assert(null != this.changesetBoundary, "changesetBoundary");
                    this.buildBatchWriter.WriteLine();
                    this.buildBatchWriter.WriteLine("--{0}--", this.changesetBoundary);
                    this.changesetStarted = false;
                }
            }

            /// <summary>operation with exception</summary>
            /// <param name="e">exception object</param>
            /// <param name="response">response object</param>
            private void HandleOperationException(Exception e, HttpWebResponse response)
            {
                if (null != response)
                {
                    this.HandleOperationResponse(response);
                    this.HandleOperationResponseData(response);
                    this.HandleOperationEnd();
                }
                else
                {
                    this.HandleOperationStart();
                    WriteOperationResponseHeaders(this.buildBatchWriter, 500);
                    this.buildBatchWriter.WriteLine("{0}: {1}", XmlConstants.HttpContentType, XmlConstants.MimeTextPlain);
                    this.buildBatchWriter.WriteLine("{0}: {1}", XmlConstants.HttpContentID, this.ChangedEntries[this.entryIndex].ChangeOrder);
                    this.buildBatchWriter.WriteLine();
                    this.buildBatchWriter.WriteLine(e.ToString());
                    this.HandleOperationEnd();
                }

                this.request = null;
                if (!IsFlagSet(this.options, SaveChangesOptions.ContinueOnError))
                {
                    this.SetCompleted();

                    // if it was a media link entry don't even try to do a PUT if the POST didn't succeed
                    this.processingMediaLinkEntry = false;

                    // Need to set this to true since we check this even on error cases, but we're here
                    //   because exception was thrown during preparation of the request, so we might not have a chance
                    //   to generate the content for save yet.
                    this.ChangedEntries[this.entryIndex].ContentGeneratedForSave = true;
                }
            }

            /// <summary>operation with HttpWebResponse</summary>
            /// <param name="response">response object</param>
            private void HandleOperationResponse(HttpWebResponse response)
            {
                this.HandleOperationStart();

                Descriptor entry = this.ChangedEntries[this.entryIndex];

                // in the first pass, the http response is packaged into a batch response (which is then processed in second pass).
                // in this first pass, (all added entities and first call of modified media link entities) update their edit location
                // added entities - so entities that have not sent content yet w/ reference links can inline those reference links in their payload
                // media entities - because they can change edit location which is then necessary for second call that includes property content
                if (entry.IsResource)
                {
                    EntityDescriptor entityDescriptor = (EntityDescriptor)entry;

                    if (entry.State == EntityStates.Added ||
                         (entry.State == EntityStates.Modified &&
                          this.processingMediaLinkEntry && !this.processingMediaLinkEntryPut))
                    {
                        string location = response.Headers[XmlConstants.HttpResponseLocation];

                        if (WebUtil.SuccessStatusCode(response.StatusCode))
                        {
                            if (null != location)
                            {
                                this.Context.AttachLocation(entityDescriptor.Entity, location);
                            }
                            else
                            {
                                throw Error.NotSupported(Strings.Deserialize_NoLocationHeader);
                            }
                        }
                    }

                    if (this.processingMediaLinkEntry)
                    {
                        if (!WebUtil.SuccessStatusCode(response.StatusCode))
                        {
                            // If the request failed and it was the MR request we should not try to send the PUT MLE after it
                            //   for one we don't have the location to send it to (if it was POST MR)

                            // Just reset the processMLE flag - that means that we will not try to PUT the MLE and instead skip over
                            //   to the next change (if we are to ignore errors that is)
                            this.processingMediaLinkEntry = false;

                            if (!this.processingMediaLinkEntryPut)
                            {
                                // If this was the POST MR it means we tried to add the entity. Now its state is Modified but we need
                                //   to revert back to Added so that user can retry by calling SaveChanges again.
                                Debug.Assert(entry.State == EntityStates.Modified, "Entity state should be set to Modified once we've sent the POST MR");
                                entry.State = EntityStates.Added;
                                this.processingMediaLinkEntryPut = false;
                            }

                            // And we also need to mark it such that we generated the save content (which we did before the POST request in fact)
                            // to workaround the fact that we use the same entry object to track two requests.
                            entry.ContentGeneratedForSave = true;
                        }
                        else if (response.StatusCode == HttpStatusCode.Created)
                        {
                            // We just finished a POST MR request and the PUT MLE coming immediately after it will
                            // need the new etag value from the server to succeed.
                            entityDescriptor.ETag = response.Headers[XmlConstants.HttpResponseETag];

                            // else is not interesting and we intentionally do nothing.
                        }
                    }
                }

                WriteOperationResponseHeaders(this.buildBatchWriter, (int)response.StatusCode);
                foreach (string name in response.Headers.AllKeys)
                {
                    if (XmlConstants.HttpContentLength != name)
                    {
                        this.buildBatchWriter.WriteLine("{0}: {1}", name, response.Headers[name]);
                    }
                }

                this.buildBatchWriter.WriteLine("{0}: {1}", XmlConstants.HttpContentID, entry.ChangeOrder);
                this.buildBatchWriter.WriteLine();
            }

            /// <summary>
            /// copy the response data
            /// </summary>
            /// <param name="response">response object</param>
            private void HandleOperationResponseData(HttpWebResponse response)
            {
                using (Stream stream = response.GetResponseStream())
                {
                    if (null != stream)
                    {
                        this.buildBatchWriter.Flush();
                        if (0 == WebUtil.CopyStream(stream, this.buildBatchWriter.BaseStream, ref this.buildBatchBuffer))
                        {
                            this.HandleOperationResponseNoData();
                        }
                    }
                }
            }

            /// <summary>only call when no data was written to added "Content-Length: 0"</summary>
            private void HandleOperationResponseNoData()
            {
                Debug.Assert(null != this.buildBatchWriter, "null buildBatchWriter");
                this.buildBatchWriter.Flush();
#if DEBUG
                MemoryStream memory = this.buildBatchWriter.BaseStream as MemoryStream;
                Debug.Assert(null != memory, "expected MemoryStream");
                Debug.Assert(this.buildBatchWriter.NewLine == NewLine, "mismatch NewLine");
                for (int kk = 0; kk < NewLine.Length; ++kk)
                {
                    Debug.Assert((char)memory.GetBuffer()[memory.Length - (NewLine.Length - kk)] == NewLine[kk], "didn't end with newline");
                }
#endif
                this.buildBatchWriter.BaseStream.Position -= NewLine.Length;
                this.buildBatchWriter.WriteLine("{0}: {1}", XmlConstants.HttpContentLength, 0);
                this.buildBatchWriter.WriteLine();
            }

            #endregion

            /// <summary>
            /// create the web request for a batch
            /// </summary>
            /// <param name="memory">memory stream for length</param>
            /// <returns>httpweb request</returns>
            private HttpWebRequest CreateBatchRequest(MemoryStream memory)
            {
                Uri requestUri = Util.CreateUri(this.Context.baseUriWithSlash, Util.CreateUri("$batch", UriKind.Relative));
                string contentType = XmlConstants.MimeMultiPartMixed + "; " + XmlConstants.HttpMultipartBoundary + "=" + this.batchBoundary;
                HttpWebRequest httpWebRequest = this.Context.CreateRequest(requestUri, XmlConstants.HttpMethodPost, false, contentType, Util.DataServiceVersion1, false);
                httpWebRequest.ContentLength = memory.Length - memory.Position;
                return httpWebRequest;
            }

            /// <summary>generate the batch request of all changes to save</summary>
            /// <param name="replaceOnUpdate">whether we need to update MERGE or PUT method for update.</param>
            /// <returns>buffer containing data for request stream</returns>
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "Disposal not required for MemoryStream which is being returned")]
            private MemoryStream GenerateBatchRequest(bool replaceOnUpdate)
            {
                this.changesetBoundary = null;
                if (null != this.Queries)
                {
                }
                else if (0 == this.ChangedEntries.Count)
                {
                    this.DataServiceResponse = new DataServiceResponse(null, (int)WebExceptionStatus.Success, this.Responses, true /*batchResponse*/);
                    this.SetCompleted();
                    return null;
                }
                else
                {
                    this.changesetBoundary = XmlConstants.HttpMultipartBoundaryChangeSet + "_" + Guid.NewGuid().ToString();
                }

                MemoryStream memory = new MemoryStream();
                StreamWriter text = new StreamWriter(memory);     // defaults to UTF8 w/o preamble
#if TESTUNIXNEWLINE
                text.NewLine = NewLine;
#endif

                if (null != this.Queries)
                {
                    for (int i = 0; i < this.Queries.Length; ++i)
                    {
                        Uri requestUri = Util.CreateUri(this.Context.baseUriWithSlash, this.Queries[i].QueryComponents.Uri);

                        Debug.Assert(null != requestUri, "request uri is null");
                        Debug.Assert(requestUri.IsAbsoluteUri, "request uri is not absolute uri");

                        text.WriteLine("--{0}", this.batchBoundary);
                        WriteOperationRequestHeaders(text, XmlConstants.HttpMethodGet, requestUri.AbsoluteUri, this.Queries[i].QueryComponents.Version);
                        text.WriteLine();
                    }
                }
                else if (0 < this.ChangedEntries.Count)
                {
                    text.WriteLine("--{0}", this.batchBoundary);
                    text.WriteLine("{0}: {1}; boundary={2}", XmlConstants.HttpContentType, XmlConstants.MimeMultiPartMixed, this.changesetBoundary);
                    text.WriteLine();

                    for (int i = 0; i < this.ChangedEntries.Count; ++i)
                    {
                        #region validate changeset boundary starts on newline
#if DEBUG
                        {
                            text.Flush();
                            for (int kk = 0; kk < NewLine.Length; ++kk)
                            {
                                Debug.Assert((char)memory.GetBuffer()[memory.Length - (NewLine.Length - kk)] == NewLine[kk], "boundary didn't start with newline");
                            }
                        }
#endif
                        #endregion

                        Descriptor entry = this.ChangedEntries[i];
                        if (entry.ContentGeneratedForSave)
                        {
                            continue;
                        }

                        text.WriteLine("--{0}", this.changesetBoundary);

                        EntityDescriptor entityDescriptor = entry as EntityDescriptor;
                        if (entry.IsResource)
                        {
                            if (entityDescriptor.State == EntityStates.Added)
                            {
                                // We don't support adding MLE/MR in batch mode
                                ClientType type = ClientType.Create(entityDescriptor.Entity.GetType());
                                if (type.IsMediaLinkEntry || entityDescriptor.IsMediaLinkEntry)
                                {
                                    throw Error.NotSupported(Strings.Context_BatchNotSupportedForMediaLink);
                                }
                            }
                            else if (entityDescriptor.State == EntityStates.Unchanged || entityDescriptor.State == EntityStates.Modified)
                            {
                                // We don't support PUT for the MR in batch mode
                                // It's OK to PUT the MLE alone inside a batch mode though
                                if (entityDescriptor.SaveStream != null)
                                {
                                    throw Error.NotSupported(Strings.Context_BatchNotSupportedForMediaLink);
                                }
                            }
                        }

                        PerRequest.ContentStream contentStream = this.CreateChangeData(i, true);
                        MemoryStream stream = null;
                        if (null != contentStream)
                        {
                            Debug.Assert(contentStream.IsKnownMemoryStream, "Batch requests don't support MRs yet");
                            stream = contentStream.Stream as MemoryStream;
                        }

                        if (entry.IsResource)
                        {
                            this.Context.CreateRequestBatch(entityDescriptor, text, replaceOnUpdate);
                        }
                        else
                        {
                            this.Context.CreateRequestBatch((LinkDescriptor)entry, text);
                        }

                        byte[] buffer = null;
                        int bufferOffset = 0, bufferLength = 0;
                        if (null != stream)
                        {
                            buffer = stream.GetBuffer();
                            bufferOffset = checked((int)stream.Position);
                            bufferLength = checked((int)stream.Length) - bufferOffset;
                        }

                        if (0 < bufferLength)
                        {
                            text.WriteLine("{0}: {1}", XmlConstants.HttpContentLength, bufferLength);
                        }

                        text.WriteLine(); // NewLine separates header from message

                        if (0 < bufferLength)
                        {
                            text.Flush();
                            text.BaseStream.Write(buffer, bufferOffset, bufferLength);
                        }
                    }

                    #region validate changeset boundary ended with newline
#if DEBUG
                    {
                        text.Flush();

                        for (int kk = 0; kk < NewLine.Length; ++kk)
                        {
                            Debug.Assert((char)memory.GetBuffer()[memory.Length - (NewLine.Length - kk)] == NewLine[kk], "post CreateRequest boundary didn't start with newline");
                        }
                    }
#endif
                    #endregion

                    // The boundary delimiter line following the last body part
                    // has two more hyphens after the boundary parameter value.
                    text.WriteLine("--{0}--", this.changesetBoundary);
                }

                text.WriteLine("--{0}--", this.batchBoundary);

                text.Flush();
                Debug.Assert(Object.ReferenceEquals(text.BaseStream, memory), "should be same");
                Debug.Assert(this.ChangedEntries.All(o => o.ContentGeneratedForSave), "didn't generated content for all entities/links");

                #region Validate batch format
#if DEBUG
                int testGetCount = 0;
                int testOpCount = 0;
                int testBeginSetCount = 0;
                int testEndSetCount = 0;
                memory.Position = 0;
                BatchStream testBatch = new BatchStream(memory, this.batchBoundary, HttpProcessUtility.EncodingUtf8NoPreamble, true);
                while (testBatch.MoveNext())
                {
                    switch (testBatch.State)
                    {
                        case BatchStreamState.StartBatch:
                        case BatchStreamState.EndBatch:
                        default:
                            Debug.Assert(false, "shouldn't happen");
                            break;

                        case BatchStreamState.Get:
                            testGetCount++;
                            break;

                        case BatchStreamState.BeginChangeSet:
                            testBeginSetCount++;
                            break;
                        case BatchStreamState.EndChangeSet:
                            testEndSetCount++;
                            break;
                        case BatchStreamState.Post:
                        case BatchStreamState.Put:
                        case BatchStreamState.Delete:
                        case BatchStreamState.Merge:
                            testOpCount++;
                            break;
                    }
                }

                Debug.Assert((null == this.Queries && 1 == testBeginSetCount) || (0 == testBeginSetCount), "more than one BeginChangeSet");
                Debug.Assert(testBeginSetCount == testEndSetCount, "more than one EndChangeSet");
                Debug.Assert((null == this.Queries && testGetCount == 0) || this.Queries.Length == testGetCount, "too many get count");
                // Debug.Assert(this.ChangedEntries.Count == testOpCount, "too many op count");
                Debug.Assert(BatchStreamState.EndBatch == testBatch.State, "should have ended propertly");
#endif
                #endregion

                this.changesetBoundary = null;

                memory.Position = 0;
                return memory;
            }

            #region handle batch response

            /// <summary>
            /// process the batch changeset response
            /// </summary>
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "BatchStream will be disposed after user iterates through final response enumerator")]
            private void HandleBatchResponse()
            {
                string boundary = this.batchBoundary;
                Encoding encoding = Encoding.UTF8;
                Dictionary<string, string> headers = null;
                Exception exception = null;

                try
                {
                    if (IsFlagSet(this.options, SaveChangesOptions.Batch))
                    {
                        if ((null == this.batchResponse) || (HttpStatusCode.NoContent == this.batchResponse.StatusCode))
                        {   // we always expect a response to our batch POST request
                            throw Error.InvalidOperation(Strings.Batch_ExpectedResponse(1));
                        }

                        headers = WebUtil.WrapResponseHeaders(this.batchResponse);
                        HandleResponse(
                            this.batchResponse.StatusCode,                                      // statusCode
                            this.batchResponse.Headers[XmlConstants.HttpDataServiceVersion],    // responseVersion
                            delegate() { return this.httpWebResponseStream; },                  // getResponseStream
                            true);                                                              // throwOnFailure

                        if (!BatchStream.GetBoundaryAndEncodingFromMultipartMixedContentType(this.batchResponse.ContentType, out boundary, out encoding))
                        {
                            string mime;
                            Exception inner = null;
                            HttpProcessUtility.ReadContentType(this.batchResponse.ContentType, out mime, out encoding);
                            if (String.Equals(XmlConstants.MimeTextPlain, mime))
                            {
                                inner = GetResponseText(this.batchResponse.GetResponseStream, this.batchResponse.StatusCode);
                            }

                            throw Error.InvalidOperation(Strings.Batch_ExpectedContentType(this.batchResponse.ContentType), inner);
                        }

                        if (null == this.httpWebResponseStream)
                        {
                            Error.ThrowBatchExpectedResponse(InternalError.NullResponseStream);
                        }

                        this.DataServiceResponse = new DataServiceResponse(headers, (int)this.batchResponse.StatusCode, this.Responses, true /*batchResponse*/);
                    }

                    bool close = true;
                    BatchStream batchStream = null;
                    try
                    {
                        // BatchStream takes ownership of the httpWebResponseStream (which may be a network stream)
                        // BatchStream will be disposed after user iterates through enumerator
                        batchStream = this.responseBatchStream ?? new BatchStream(this.httpWebResponseStream, boundary, encoding, false);
                        this.httpWebResponseStream = null;
                        this.responseBatchStream = null;

                        IEnumerable<OperationResponse> responses = this.HandleBatchResponse(batchStream);
                        if (IsFlagSet(this.options, SaveChangesOptions.Batch) && (null != this.Queries))
                        {
                            // ExecuteBatch, EndExecuteBatch
                            close = false;
                            this.responseBatchStream = batchStream;

                            this.DataServiceResponse = new DataServiceResponse(
                                (Dictionary<string, string>)this.DataServiceResponse.BatchHeaders,
                                this.DataServiceResponse.BatchStatusCode,
                                responses,
                                true /*batchResponse*/);
                        }
                        else
                        {   // SaveChanges, EndSaveChanges
                            // enumerate the entire response
                            foreach (ChangeOperationResponse response in responses)
                            {
                                if (exception == null && response.Error != null)
                                {
                                    exception = response.Error;
                                }
                            }
                        }
                    }
                    finally
                    {
                        if (close && (null != batchStream))
                        {
                            batchStream.Close();
                        }
                    }
                }
                catch (InvalidOperationException ex)
                {
                    exception = ex;
                }

                if (exception != null)
                {
                    if (this.DataServiceResponse == null)
                    {
                        int statusCode = this.batchResponse == null ? (int)HttpStatusCode.InternalServerError : (int)this.batchResponse.StatusCode;
                        this.DataServiceResponse = new DataServiceResponse(headers, statusCode, null, IsFlagSet(this.options, SaveChangesOptions.Batch));
                    }

                    throw new DataServiceRequestException(Strings.DataServiceException_GeneralError, exception, this.DataServiceResponse);
                }
            }

            /// <summary>
            /// process the batch changeset response
            /// </summary>
            /// <param name="batch">batch stream</param>
            /// <returns>enumerable of QueryResponse or null</returns>
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Maintainability", "CA1506", Justification = "Central method of the API, likely to have many cross-references")]
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031", Justification = "Cache exception so user can examine it later")]
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "MaterializeAtom is responsible for closing XmlReader which will close the stream")]
            private IEnumerable<OperationResponse> HandleBatchResponse(BatchStream batch)
            {
                if (!batch.CanRead)
                {
                    yield break;
                }

                string contentType;
                string location;
                string etag;

                Uri editLink = null;

                HttpStatusCode status;
                int changesetIndex = 0;
                int queryCount = 0;
                int operationCount = 0;
                this.entryIndex = 0;
                while (batch.MoveNext())
                {
                    var contentHeaders = batch.ContentHeaders; // get the headers before materialize clears them

                    Descriptor entry;
                    switch (batch.State)
                    {
                        #region BeginChangeSet
                        case BatchStreamState.BeginChangeSet:
                            if ((IsFlagSet(this.options, SaveChangesOptions.Batch) && (0 != changesetIndex)) ||
                                (0 != operationCount))
                            {   // for now, we only send a single batch, single changeset
                                Error.ThrowBatchUnexpectedContent(InternalError.UnexpectedBeginChangeSet);
                            }

                            break;
                        #endregion

                        #region EndChangeSet
                        case BatchStreamState.EndChangeSet:
                            // move forward to next expected changelist
                            changesetIndex++;
                            operationCount = 0;
                            break;
                        #endregion

                        #region GetResponse
                        case BatchStreamState.GetResponse:
                            Debug.Assert(0 == operationCount, "missing an EndChangeSet 2");

                            contentHeaders.TryGetValue(XmlConstants.HttpContentType, out contentType);
                            status = (HttpStatusCode)(-1);

                            Exception ex = null;
                            QueryOperationResponse qresponse = null;
                            try
                            {
                                status = batch.GetStatusCode();

                                ex = HandleResponse(status, batch.GetResponseVersion(), batch.GetContentStream, false);
                                if (null == ex)
                                {
                                    DataServiceRequest query = this.Queries[queryCount];
                                    MaterializeAtom materializer = DataServiceRequest.Materialize(this.Context, query.QueryComponents, null, contentType, batch.GetContentStream());
                                    qresponse = QueryOperationResponse.GetInstance(query.ElementType, contentHeaders, query, materializer);
                                }
                            }
                            catch (ArgumentException e)
                            {
                                ex = e;
                            }
                            catch (FormatException e)
                            {
                                ex = e;
                            }
                            catch (InvalidOperationException e)
                            {
                                ex = e;
                            }

                            if (null == qresponse)
                            {
                                if (null != this.Queries)
                                {
                                    // this is the normal ExecuteBatch response
                                    DataServiceRequest query = this.Queries[queryCount];

                                    if (this.Context.ignoreResourceNotFoundException && status == HttpStatusCode.NotFound)
                                    {
                                        qresponse = QueryOperationResponse.GetInstance(query.ElementType, contentHeaders, query, MaterializeAtom.EmptyResults);
                                    }
                                    else
                                    {
                                        qresponse = QueryOperationResponse.GetInstance(query.ElementType, contentHeaders, query, MaterializeAtom.EmptyResults);
                                        qresponse.Error = ex;
                                    }
                                }
                                else
                                {
                                    // This is top-level failure SaveChanges(SaveChangesOptions.Batch) response
                                    // example: server doesn't support batching or number of batch objects exceeded an allowed limit.
                                    // ex could be null if the server responded to SaveChanges with an unexpected success with
                                    // response of batched GETS that did not correspond the original POST/MERGE/PUT/DELETE requests.
                                    // we expect non-null since server should have failed with a non-success code
                                    // and HandleResponse(status, ...) should generate the exception object
                                    throw ex;
                                }
                            }

                            qresponse.StatusCode = (int)status;
                            queryCount++;
                            yield return qresponse;
                            break;
                        #endregion

                        #region ChangeResponse
                        case BatchStreamState.ChangeResponse:

                            HttpStatusCode statusCode = batch.GetStatusCode();
                            Exception error = HandleResponse(statusCode, batch.GetResponseVersion(), batch.GetContentStream, false);
                            int index = this.ValidateContentID(contentHeaders);

                            try
                            {
                                entry = this.ChangedEntries[index];
                                operationCount += this.Context.SaveResultProcessed(entry);

                                if (null != error)
                                {
                                    throw error;
                                }

                                StreamStates streamState = StreamStates.NoStream;
                                if (entry.IsResource)
                                {
                                    EntityDescriptor descriptor = (EntityDescriptor)entry;
                                    streamState = descriptor.StreamState;
#if DEBUG
                                    if (descriptor.StreamState == StreamStates.Added)
                                    {
                                        Debug.Assert(
                                            statusCode == HttpStatusCode.Created && entry.State == EntityStates.Modified && descriptor.IsMediaLinkEntry,
                                            "statusCode == HttpStatusCode.Created && entry.State == EntityStates.Modified && descriptor.IsMediaLinkEntry -- Processing Post MR");
                                    }
                                    else if (descriptor.StreamState == StreamStates.Modified)
                                    {
                                        Debug.Assert(
                                            statusCode == HttpStatusCode.NoContent && descriptor.IsMediaLinkEntry,
                                            "statusCode == HttpStatusCode.NoContent && descriptor.IsMediaLinkEntry -- Processing Put MR");
                                    }
#endif
                                }

                                if (streamState == StreamStates.Added || entry.State == EntityStates.Added)
                                {
                                    #region Post
                                    if (entry.IsResource)
                                    {
                                        string mime = null;
                                        Encoding postEncoding = null;
                                        contentHeaders.TryGetValue(XmlConstants.HttpContentType, out contentType);
                                        contentHeaders.TryGetValue(XmlConstants.HttpResponseLocation, out location);
                                        contentHeaders.TryGetValue(XmlConstants.HttpResponseETag, out etag);
                                        EntityDescriptor entityDescriptor = (EntityDescriptor)entry;

                                        // If the location header is specified, we need to set the edit link
                                        // for the entity descriptor to that value.
                                        if (location != null)
                                        {
                                            editLink = Util.CreateUri(location, UriKind.Absolute);
                                        }
                                        else
                                        {
                                            throw Error.NotSupported(Strings.Deserialize_NoLocationHeader);
                                        }

                                        Stream stream = batch.GetContentStream();
                                        if (null != stream)
                                        {
                                            HttpProcessUtility.ReadContentType(contentType, out mime, out postEncoding);
                                            if (!String.Equals(XmlConstants.MimeApplicationAtom, mime, StringComparison.OrdinalIgnoreCase))
                                            {
                                                throw Error.InvalidOperation(Strings.Deserialize_UnknownMimeTypeSpecified(mime));
                                            }

                                            XmlReader reader = XmlUtil.CreateXmlReader(stream, postEncoding);
                                            QueryComponents qc = new QueryComponents(null, Util.DataServiceVersionEmpty, entityDescriptor.Entity.GetType(), null, null);
                                            EntityDescriptor descriptor = (EntityDescriptor)entry;
                                            MergeOption mergeOption = MergeOption.OverwriteChanges;

                                            // If we are processing a POST MR, we want to materialize the payload to get the metadata for the stream.
                                            // However we must not modify the MLE properties with the server initialized properties.  The next request
                                            // will be a Put MLE operation and we will set the server properties with values from the client entity.
                                            if (descriptor.StreamState == StreamStates.Added)
                                            {
                                                mergeOption = MergeOption.PreserveChanges;
                                                Debug.Assert(descriptor.State == EntityStates.Modified, "The MLE state must be Modified.");
                                            }

                                            try
                                            {
                                                using (MaterializeAtom atom = new MaterializeAtom(this.Context, reader, qc, null, mergeOption))
                                                {
                                                    this.Context.HandleResponsePost(entityDescriptor, atom, editLink, etag);
                                                }
                                            }
                                            finally
                                            {
                                                if (descriptor.StreamState == StreamStates.Added)
                                                {
                                                    // The materializer will always set the entity state to Unchanged.  We just processed Post MR, we
                                                    // need to restore the entity state to Modified to process the Put MLE.
                                                    Debug.Assert(descriptor.State == EntityStates.Unchanged, "The materializer should always set the entity state to Unchanged.");
                                                    descriptor.State = EntityStates.Modified;

                                                    // Need to clear the stream state so the next iteration we will always process the Put MLE operation.
                                                    descriptor.StreamState = StreamStates.NoStream;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            this.Context.HandleResponsePost(entityDescriptor, null /*materializer*/, editLink, etag);
                                        }
                                    }
                                    else
                                    {
                                        HandleResponsePost((LinkDescriptor)entry);
                                    }
                                    #endregion
                                }
                                else if (streamState == StreamStates.Modified || entry.State == EntityStates.Modified)
                                {
                                    #region Put, Merge
                                    contentHeaders.TryGetValue(XmlConstants.HttpResponseETag, out etag);
                                    HandleResponsePut(entry, etag);
                                    #endregion
                                }
                                else if (entry.State == EntityStates.Deleted)
                                {
                                    #region Delete
                                    this.Context.HandleResponseDelete(entry);
                                    #endregion
                                }

                                // else condition is not interesting here and we intentionally do nothing.
                            }
                            catch (Exception e)
                            {
                                this.ChangedEntries[index].SaveError = e;
                                error = e;
                            }

                            ChangeOperationResponse changeOperationResponse = 
                                new ChangeOperationResponse(contentHeaders, this.ChangedEntries[index]);
                            changeOperationResponse.StatusCode = (int)statusCode;
                            if (error != null)
                            {
                                changeOperationResponse.Error = error;
                            }

                            this.Responses.Add(changeOperationResponse);
                            operationCount++;
                            this.entryIndex++;
                            yield return changeOperationResponse;
                            break;
                        #endregion

                        default:
                            Error.ThrowBatchExpectedResponse(InternalError.UnexpectedBatchState);
                            break;
                    }
                }

                Debug.Assert(batch.State == BatchStreamState.EndBatch, "unexpected batch state");

                // Check for a changeset without response (first line) or GET request without response (second line).
                // either all saved entries must be processed or it was a batch and one of the entries has the error
                if ((null == this.Queries && 
                    (0 == changesetIndex || 
                     0 < queryCount || 
                     this.ChangedEntries.Any(o => o.ContentGeneratedForSave && 0 == o.SaveResultWasProcessed) &&
                     (!IsFlagSet(this.options, SaveChangesOptions.Batch) || null == this.ChangedEntries.FirstOrDefault(o => null != o.SaveError)))) ||
                    (null != this.Queries && queryCount != this.Queries.Length))
                {
                    throw Error.InvalidOperation(Strings.Batch_IncompleteResponseCount);
                }

                batch.Dispose();
            }

            /// <summary>
            /// validate the content-id
            /// </summary>
            /// <param name="contentHeaders">headers</param>
            /// <returns>return the correct ChangedEntries index</returns>
            private int ValidateContentID(Dictionary<string, string> contentHeaders)
            {
                int contentID = 0;
                string contentValueID;

                if (!contentHeaders.TryGetValue(XmlConstants.HttpContentID, out contentValueID) ||
                    !Int32.TryParse(contentValueID, NumberStyles.Integer, NumberFormatInfo.InvariantInfo, out contentID))
                {
                    Error.ThrowBatchUnexpectedContent(InternalError.ChangeResponseMissingContentID);
                }

                for (int i = 0; i < this.ChangedEntries.Count; ++i)
                {
                    if (this.ChangedEntries[i].ChangeOrder == contentID)
                    {
                        return i;
                    }
                }

                Error.ThrowBatchUnexpectedContent(InternalError.ChangeResponseUnknownContentID);
                return -1;
            }

            #endregion Batch

            #region callback handlers

            /// <summary>handle request.BeginGetRequestStream with request.EndGetRquestStream and then write out request stream</summary>
            /// <param name="asyncResult">async result</param>
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes", Justification = "required for this feature")]
            private void AsyncEndGetRequestStream(IAsyncResult asyncResult)
            {
                Debug.Assert(asyncResult != null && asyncResult.IsCompleted, "asyncResult.IsCompleted");
                PerRequest pereq = asyncResult == null ? null : asyncResult.AsyncState as PerRequest;
                try
                {
                    CompleteCheck(pereq, InternalError.InvalidEndGetRequestCompleted);
                    pereq.RequestCompletedSynchronously &= asyncResult.CompletedSynchronously; // BeginGetRequestStream

                    EqualRefCheck(this.request, pereq, InternalError.InvalidEndGetRequestStream);
                    HttpWebRequest httpWebRequest = Util.NullCheck(pereq.Request, InternalError.InvalidEndGetRequestStreamRequest);

                    Stream stream = Util.NullCheck(httpWebRequest.EndGetRequestStream(asyncResult), InternalError.InvalidEndGetRequestStreamStream);
                    pereq.RequestStream = stream;

                    PerRequest.ContentStream contentStream = pereq.RequestContentStream;
                    Util.NullCheck(contentStream, InternalError.InvalidEndGetRequestStreamContent);
                    Util.NullCheck(contentStream.Stream, InternalError.InvalidEndGetRequestStreamContent);
                    if (contentStream.IsKnownMemoryStream)
                    {
                        MemoryStream memoryStream = contentStream.Stream as MemoryStream;
                        byte[] buffer = memoryStream.GetBuffer();
                        int bufferOffset = checked((int)memoryStream.Position);
                        int bufferLength = checked((int)memoryStream.Length) - bufferOffset;
                        if ((null == buffer) || (0 == bufferLength))
                        {
                            Error.ThrowInternalError(InternalError.InvalidEndGetRequestStreamContentLength);
                        }
                    }

                    // Start the Read on the request content stream.
                    // Note that we don't deal with synchronous results here.
                    // If the read finishes synchronously the AsyncRequestContentEndRead will be called from inside the BeginRead
                    //   call below. In there we will call BeginWrite. If that completes synchronously we will loop
                    //   and call BeginRead again. If that completes synchronously as well we will call BeginWrite and so on.
                    //   AsyncEndWrite will return immedially if it finished synchronously (otherwise it calls BeginRead).
                    // So in the worst case we will have a stack like this:
                    //   AsyncEndGetRequestStream
                    //     AsyncRequestContentEndRead
                    //       AsyncRequestContentEndRead or AsyncEndWrite

                    // We just need to differentiate between the first AsyncRequestContentEndRead and the others (the first one
                    //   must not return even if it completed synchronously, otherwise we would have to do the loop here as well).
                    //   We'll use the RequestContentBufferValidLength as the notification. It will start with -1 which means
                    //   we didn't read anything at all and thus it's the first read ending.
                    pereq.RequestContentBufferValidLength = -1;

                    Util.DebugInjectFault("SaveAsyncResult::AsyncEndGetRequestStream_BeforeBeginRead");
                    asyncResult = BaseAsyncResult.InvokeAsync(contentStream.Stream.BeginRead, pereq.RequestContentBuffer, 0, pereq.RequestContentBuffer.Length, this.AsyncRequestContentEndRead, pereq);
                    pereq.RequestCompletedSynchronously &= asyncResult.CompletedSynchronously;
                }
                catch (Exception e)
                {
                    if (this.HandleFailure(pereq, e))
                    {
                        throw;
                    }
                }
                finally
                {
                    this.HandleCompleted(pereq);
                }
            }

            /// <summary>
            /// Callback for Stream.BeginRead on the request content input stream. Calls request content output stream BeginWrite
            /// and in case of synchronous also the next BeginRead.
            /// </summary>
            /// <param name="asyncResult">The asynchronous result associated with the completed operation.</param>
            private void AsyncRequestContentEndRead(IAsyncResult asyncResult)
            {
                Debug.Assert(asyncResult != null && asyncResult.IsCompleted, "asyncResult.IsCompleted");
                PerRequest pereq = asyncResult == null ? null : asyncResult.AsyncState as PerRequest;
                try
                {
                    CompleteCheck(pereq, InternalError.InvalidEndReadCompleted);
                    pereq.RequestCompletedSynchronously &= asyncResult.CompletedSynchronously; // BeginRead

                    EqualRefCheck(this.request, pereq, InternalError.InvalidEndRead);
                    PerRequest.ContentStream contentStream = pereq.RequestContentStream;
                    Util.NullCheck(contentStream, InternalError.InvalidEndReadStream);
                    Util.NullCheck(contentStream.Stream, InternalError.InvalidEndReadStream);
                    Stream stream = Util.NullCheck(pereq.RequestStream, InternalError.InvalidEndReadStream);

                    Util.DebugInjectFault("SaveAsyncResult::AsyncRequestContentEndRead_BeforeEndRead");
                    int count = contentStream.Stream.EndRead(asyncResult);
                    if (0 < count)
                    {
                        bool firstEndRead = (pereq.RequestContentBufferValidLength == -1);
                        pereq.RequestContentBufferValidLength = count;

                        // If we completed synchronously then just return. Our caller will take care of processing the results.
                        // First EndRead must not return even if completed synchronously.
                        if (!asyncResult.CompletedSynchronously || firstEndRead)
                        {
                            do
                            {
                                // Write the data we've read to the request stream
                                Util.DebugInjectFault("SaveAsyncResult::AsyncRequestContentEndRead_BeforeBeginWrite");
                                asyncResult = BaseAsyncResult.InvokeAsync(stream.BeginWrite, pereq.RequestContentBuffer, 0, pereq.RequestContentBufferValidLength, this.AsyncEndWrite, pereq);
                                pereq.RequestCompletedSynchronously &= asyncResult.CompletedSynchronously;

                                // If the write above completed synchronously
                                //   immediately start the next read so that we loop instead of recursion.
                                // If it completed asynchronously we just return as we will deal with the results in the EndWrite
                                if (asyncResult.CompletedSynchronously && !pereq.RequestCompleted && !this.IsCompletedInternally)
                                {
                                    Util.DebugInjectFault("SaveAsyncResult::AsyncRequestContentEndRead_BeforeBeginRead");
                                    asyncResult = BaseAsyncResult.InvokeAsync(contentStream.Stream.BeginRead, pereq.RequestContentBuffer, 0, pereq.RequestContentBuffer.Length, this.AsyncRequestContentEndRead, pereq);
                                    pereq.RequestCompletedSynchronously &= asyncResult.CompletedSynchronously;
                                }

                                // If the read completed synchronously as well we loop to write the data to the request stream without recursion.
                                // Only loop if there's actually some data to be processed. If there's no more data then return.
                                // The request will continue inside the inner call to AsyncRequestContentEndRead (which will get 0 data
                                //   and will end up in the else branch of the big if).
                            }
                            while (asyncResult.CompletedSynchronously && !pereq.RequestCompleted && !this.IsCompletedInternally &&
                                pereq.RequestContentBufferValidLength > 0);
                        }
                    }
                    else
                    {
                        // Done reading data (and writing them)
                        pereq.RequestContentBufferValidLength = 0;
                        pereq.RequestStream = null;
                        stream.Close();

                        HttpWebRequest httpWebRequest = Util.NullCheck(pereq.Request, InternalError.InvalidEndWriteRequest);
                        asyncResult = BaseAsyncResult.InvokeAsync(httpWebRequest.BeginGetResponse, this.AsyncEndGetResponse, pereq);
                        pereq.RequestCompletedSynchronously &= asyncResult.CompletedSynchronously; // BeginGetResponse
                    }
                }
                catch (Exception e)
                {
                    if (this.HandleFailure(pereq, e))
                    {
                        throw;
                    }
                }
                finally
                {
                    this.HandleCompleted(pereq);
                }
            }
                        
            /// <summary>handle reqestStream.BeginWrite with requestStream.EndWrite then BeginGetResponse</summary>
            /// <param name="asyncResult">async result</param>
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes", Justification = "required for this feature")]
            private void AsyncEndWrite(IAsyncResult asyncResult)
            {
                Debug.Assert(asyncResult != null && asyncResult.IsCompleted, "asyncResult.IsCompleted");
                PerRequest pereq = asyncResult == null ? null : asyncResult.AsyncState as PerRequest;
                try
                {
                    CompleteCheck(pereq, InternalError.InvalidEndWriteCompleted);
                    pereq.RequestCompletedSynchronously &= asyncResult.CompletedSynchronously; // BeginWrite

                    EqualRefCheck(this.request, pereq, InternalError.InvalidEndWrite);

                    PerRequest.ContentStream contentStream = pereq.RequestContentStream;
                    Util.NullCheck(contentStream, InternalError.InvalidEndWriteStream);
                    Util.NullCheck(contentStream.Stream, InternalError.InvalidEndWriteStream);
                    Stream stream = Util.NullCheck(pereq.RequestStream, InternalError.InvalidEndWriteStream);
                    Util.DebugInjectFault("SaveAsyncResult::AsyncEndWrite_BeforeEndWrite");
                    stream.EndWrite(asyncResult);

                    // If the write completed synchronously just return. The caller (AsyncRequestContentEndRead)
                    //   will loop and initiate the next read.
                    // If the write completed asynchronously we need to start the next read here. Note that we start the read
                    //   regardless if the stream has other data to offer or not. This is to avoid dealing with the end
                    //   of the read/write loop in several places. We simply issue a read which (if the stream is at the end)
                    //   will return 0 bytes and we will deal with that in the AsyncRequestContentEndRead method.
                    if (!asyncResult.CompletedSynchronously)
                    {
                        Util.DebugInjectFault("SaveAsyncResult::AsyncEndWrite_BeforeBeginRead");
                        asyncResult = BaseAsyncResult.InvokeAsync(contentStream.Stream.BeginRead, pereq.RequestContentBuffer, 0, pereq.RequestContentBuffer.Length, this.AsyncRequestContentEndRead, pereq);
                        pereq.RequestCompletedSynchronously &= asyncResult.CompletedSynchronously;
                    }
                }
                catch (Exception e)
                {
                    if (this.HandleFailure(pereq, e))
                    {
                        throw;
                    }
                }
                finally
                {
                    this.HandleCompleted(pereq);
                }
            }

            /// <summary>handle request.BeginGetResponse with request.EndGetResponse and then copy response stream</summary>
            /// <param name="asyncResult">async result</param>
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes", Justification = "required for this feature")]
            private void AsyncEndGetResponse(IAsyncResult asyncResult)
            {
                Debug.Assert(asyncResult != null && asyncResult.IsCompleted, "asyncResult.IsCompleted");
                PerRequest pereq = asyncResult == null ? null : asyncResult.AsyncState as PerRequest;
                try
                {
                    CompleteCheck(pereq, InternalError.InvalidEndGetResponseCompleted);
                    pereq.RequestCompletedSynchronously &= asyncResult.CompletedSynchronously; // BeginGetResponse

                    EqualRefCheck(this.request, pereq, InternalError.InvalidEndGetResponse);
                    HttpWebRequest httpWebRequest = Util.NullCheck(pereq.Request, InternalError.InvalidEndGetResponseRequest);

                    // the httpWebResponse is kept for batching, discarded by non-batch
                    HttpWebResponse response = null;
                    try
                    {
                        Util.DebugInjectFault("SaveAsyncResult::AsyncEndGetResponse::BeforeEndGetResponse");
                        response = (HttpWebResponse)httpWebRequest.EndGetResponse(asyncResult);
                    }
                    catch (WebException e)
                    {
                        response = (HttpWebResponse)e.Response;
                        if (null == response)
                        {
                            throw;
                        }
                    }

                    pereq.HttpWebResponse = Util.NullCheck(response, InternalError.InvalidEndGetResponseResponse);

                    if (!IsFlagSet(this.options, SaveChangesOptions.Batch))
                    {
                        this.HandleOperationResponse(response);
                    }

                    this.copiedContentLength = 0;
                    Util.DebugInjectFault("SaveAsyncResult::AsyncEndGetResponse_BeforeGetStream");
                    Stream stream = response.GetResponseStream();
                    pereq.ResponseStream = stream;
                    if ((null != stream) && stream.CanRead)
                    {
                        if (null != this.buildBatchWriter)
                        {
                            this.buildBatchWriter.Flush();
                        }

                        if (null == this.buildBatchBuffer)
                        {
                            this.buildBatchBuffer = new byte[8000];
                        }

                        do
                        {
                            Util.DebugInjectFault("SaveAsyncResult::AsyncEndGetResponse_BeforeBeginRead");
                            asyncResult = BaseAsyncResult.InvokeAsync(stream.BeginRead, this.buildBatchBuffer, 0, this.buildBatchBuffer.Length, this.AsyncEndRead, pereq);
                            pereq.RequestCompletedSynchronously &= asyncResult.CompletedSynchronously; // BeginRead
                        }
                        while (asyncResult.CompletedSynchronously && !pereq.RequestCompleted && !this.IsCompletedInternally && stream.CanRead);
                    }
                    else
                    {
                        pereq.SetComplete();

                        // BeginGetResponse could fail and callback still invoked
                        if (!this.IsCompletedInternally)
                        {
                            this.SaveNextChange(pereq);
                        }
                    }
                }
                catch (Exception e)
                {
                    if (this.HandleFailure(pereq, e))
                    {
                        throw;
                    }
                }
                finally
                {
                    this.HandleCompleted(pereq);
                }
            }

            /// <summary>handle responseStream.BeginRead with responseStream.EndRead</summary>
            /// <param name="asyncResult">async result</param>
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes", Justification = "required for this feature")]
            private void AsyncEndRead(IAsyncResult asyncResult)
            {
                Debug.Assert(asyncResult != null && asyncResult.IsCompleted, "asyncResult.IsCompleted");
                PerRequest pereq = asyncResult.AsyncState as PerRequest;
                int count = 0;
                try
                {
                    CompleteCheck(pereq, InternalError.InvalidEndReadCompleted);
                    pereq.RequestCompletedSynchronously &= asyncResult.CompletedSynchronously; // BeginRead

                    EqualRefCheck(this.request, pereq, InternalError.InvalidEndRead);
                    Stream stream = Util.NullCheck(pereq.ResponseStream, InternalError.InvalidEndReadStream);

                    Util.DebugInjectFault("SaveAsyncResult::AsyncEndRead_BeforeEndRead");
                    count = stream.EndRead(asyncResult);
                    if (0 < count)
                    {
                        Stream outputResponse = Util.NullCheck(this.httpWebResponseStream, InternalError.InvalidEndReadCopy);
                        outputResponse.Write(this.buildBatchBuffer, 0, count);
                        this.copiedContentLength += count;

                        if (!asyncResult.CompletedSynchronously && stream.CanRead)
                        {   
                            // if CompletedSynchronously then caller will call and we reduce risk of stack overflow
                            do
                            {
                                asyncResult = BaseAsyncResult.InvokeAsync(stream.BeginRead, this.buildBatchBuffer, 0, this.buildBatchBuffer.Length, this.AsyncEndRead, pereq);
                                pereq.RequestCompletedSynchronously &= asyncResult.CompletedSynchronously; // BeginRead
                            }
                            while (asyncResult.CompletedSynchronously && !pereq.RequestCompleted && !this.IsCompletedInternally && stream.CanRead);
                        }
                    }
                    else
                    {
                        pereq.SetComplete();

                        // BeginRead could fail and callback still invoked
                        if (!this.IsCompletedInternally)
                        {
                            this.SaveNextChange(pereq);
                        }
                    }
                }
                catch (Exception e)
                {
                    if (this.HandleFailure(pereq, e))
                    {
                        throw;
                    }
                }
                finally
                {
                    this.HandleCompleted(pereq);
                }
            }

            /// <summary>continue with the next change</summary>
            /// <param name="pereq">the completed per request object</param>
            private void SaveNextChange(PerRequest pereq)
            {
                Debug.Assert(this.executeAsync, "should be async");
                if (!pereq.RequestCompleted)
                {
                    Error.ThrowInternalError(InternalError.SaveNextChangeIncomplete);
                }

                EqualRefCheck(this.request, pereq, InternalError.InvalidSaveNextChange);

                if (IsFlagSet(this.options, SaveChangesOptions.Batch))
                {
                    this.httpWebResponseStream.Position = 0;
                    this.request = null;
                    this.SetCompleted();
                }
                else
                {
                    if (0 == this.copiedContentLength)
                    {
                        this.HandleOperationResponseNoData();
                    }

                    this.HandleOperationEnd();

                    if (!this.processingMediaLinkEntry)
                    {
                        this.changesCompleted++;
                    }

                    pereq.Dispose();
                    this.request = null;
                    if (!pereq.RequestCompletedSynchronously)
                    {   // you can't chain synchronously completed responses without risking StackOverflow, caller will loop to next
                        if (!this.IsCompletedInternally)
                        {
                            this.BeginNextChange(IsFlagSet(this.options, SaveChangesOptions.ReplaceOnUpdate));
                        }
                    }
                }
            }
            #endregion

            /// <summary>wrap the full request</summary>
            private sealed class PerRequest
            {
                /// <summary>
                /// did the sequence (BeginGetRequest, EndGetRequest, ... complete. 0 = In Progress, 1 = Completed, 2 = Aborted
                /// </summary>
                private int requestStatus;

                /// <summary>
                /// Buffer used when pumping data from the write stream to the request content stream
                /// </summary>
                private byte[] requestContentBuffer;

                /// <summary>ctor</summary>
                internal PerRequest()
                {
                    this.RequestCompletedSynchronously = true;
                }

                /// <summary>active web request</summary>
                internal HttpWebRequest Request
                {
                    get;
                    set;
                }

                /// <summary>active web request stream</summary>
                internal Stream RequestStream
                {
                    get;
                    set;
                }

                /// <summary>content to write to request stream</summary>
                internal ContentStream RequestContentStream
                {
                    get;
                    set;
                }

                /// <summary>web response</summary>
                internal HttpWebResponse HttpWebResponse
                {
                    get;
                    set;
                }

                /// <summary>async web response stream</summary>
                internal Stream ResponseStream
                {
                    get;
                    set;
                }

                /// <summary>did the request complete all of its steps synchronously?</summary>
                internal bool RequestCompletedSynchronously
                {
                    get;
                    set;
                }

                /// <summary>
                /// Short cut for testing if request has finished (either completed or aborted)
                /// </summary>
                internal bool RequestCompleted
                {
                    get { return this.requestStatus != 0; }
                }

                /// <summary>
                /// Short cut for testing request status is 2 (Aborted)
                /// </summary>
                internal bool RequestAborted
                {
                    get { return this.requestStatus == 2; }
                }

                /// <summary>
                /// Buffer used when pumping data from the write stream to the request content stream
                /// </summary>
                internal byte[] RequestContentBuffer
                {
                    get
                    {
                        if (this.requestContentBuffer == null)
                        {
                            this.requestContentBuffer = new byte[64 * 1024];
                        }

                        return this.requestContentBuffer;
                    }
                }

                /// <summary>
                /// The length of the valid content in the RequestContentBuffer
                /// Once the data is read from the request content stream into the RequestContent buffer
                /// this length is set to the amount of data read.
                /// When the data is written into the request stream it is set back to 0.
                /// </summary>
                internal int RequestContentBufferValidLength
                {
                    get;
                    set;
                }

                /// <summary>
                /// Change the request status to completed
                /// </summary>
                internal void SetComplete()
                {
                    System.Threading.Interlocked.CompareExchange(ref this.requestStatus, 1, 0);
                }

                /// <summary>
                /// Change the request status to aborted
                /// </summary>
                internal void SetAborted()
                {
                    System.Threading.Interlocked.Exchange(ref this.requestStatus, 2);
                }
                
                /// <summary>
                /// dispose of the request object
                /// </summary>
                internal void Dispose()
                {
                    Stream stream = null;

                    if (null != (stream = this.ResponseStream))
                    {
                        this.ResponseStream = null;
                        stream.Dispose();
                    }

                    if (null != this.RequestContentStream)
                    {
                        if (this.RequestContentStream.Stream != null && this.RequestContentStream.IsKnownMemoryStream)
                        {
                            this.RequestContentStream.Stream.Dispose();
                        }

                        // We must not dispose the stream which came from outside
                        //   the disposing/closing of that stream depends on parameter passed to us and is dealt with
                        //   at the end of SaveChanges process.
                        this.RequestContentStream = null;
                    }
                    
                    if (null != (stream = this.RequestStream))
                    {
                        this.RequestStream = null;
                        try
                        {
                            Util.DebugInjectFault("PerRequest::Dispose_BeforeRequestStreamDisposed");
                            stream.Dispose();
                        }
                        catch (WebException)
                        {
                            // if the request is aborted, then the connect stream 
                            // cannot be disposed - since not all bytes are written to it yet
                            // In this case, we eat the exception
                            // Otherwise, keep throwing
                            if (!this.RequestAborted)
                            {
                                throw;
                            }

                            // Call Injector to report the exception so test code can verify it is thrown
                            Util.DebugInjectFault("PerRequest::Dispose_WebExceptionThrown");
                        }
                    }

                    HttpWebResponse response = this.HttpWebResponse;
                    if (null != response)
                    {
                        response.Close();
                    }

                    this.Request = null;
                    this.SetComplete();
                }

                /// <summary>
                /// Helper class to wrap the stream with the content of the request.
                /// We need to remember if the stream came from us (IsKnownMemoryStream is true)
                /// or if it came from outside. For backward compatibility we set the Content-Length for our streams
                /// since they are always MemoryStream and thus know their length.
                /// For outside streams (content of the MR requests) we don't set Content-Length since the stream
                /// might not be able to answer to the Length call.
                /// </summary>
                internal class ContentStream
                {
                    /// <summary>
                    /// The stream with the content of the request
                    /// </summary>
                    private readonly Stream stream;

                    /// <summary>
                    /// Set to true if the stream is a MemoryStream and we produced it (so it does have the buffer accesible)
                    /// </summary>
                    private readonly bool isKnownMemoryStream;

                    /// <summary>
                    /// Constructor
                    /// </summary>
                    /// <param name="stream">The stream with the request content</param>
                    /// <param name="isKnownMemoryStream">The stream was create by us and it's a MemoryStream</param>
                    public ContentStream(Stream stream, bool isKnownMemoryStream)
                    {
                        this.stream = stream;
                        this.isKnownMemoryStream = isKnownMemoryStream;
                    }

                    /// <summary>
                    /// The stream with the content of the request
                    /// </summary>
                    public Stream Stream
                    {
                        get { return this.stream; }
                    }

                    /// <summary>
                    /// Set to true if the stream is a MemoryStream and we produced it (so it does have the buffer accesible)
                    /// </summary>
                    public bool IsKnownMemoryStream
                    {
                        get { return this.isKnownMemoryStream; }
                    }
                }
            }
        }
    }
}
