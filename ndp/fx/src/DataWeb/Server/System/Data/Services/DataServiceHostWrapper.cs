//---------------------------------------------------------------------
// <copyright file="DataServiceHostWrapper.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides the wrapper class for DataService hosts.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Net;
    using System.Diagnostics;

    #endregion Namespaces.

    /// <summary>
    /// Enum to represent various http methods
    /// </summary>
    internal enum AstoriaVerbs
    {
        #region Values.

        /// <summary>Not Initialized.</summary>
        None,

        /// <summary>Represents the GET http method.</summary>
        GET,

        /// <summary>Represents the PUT http method.</summary>
        PUT,

        /// <summary>Represents the POST http method.</summary>
        POST,

        /// <summary>Represents the DELETE http method.</summary>
        DELETE,

        /// <summary>Represents the MERGE http method.</summary>
        MERGE

        #endregion Values.
    }

    /// <summary>
    /// Provides access to the environment for a DataService,
    /// including information about the current request.
    /// </summary>
    internal class DataServiceHostWrapper
    {
        #region Private Fields

        /// <summary>Reference to the IDataServiceHost object we are wrapping</summary>
        private readonly IDataServiceHost host;

        /// <summary>Gets a comma-separated list of client-supported MIME Accept types.</summary>
        private readonly string requestAccept;

        /// <summary>Gets the string with the specification for the character set encoding that the client requested, possibly null.</summary>
        private readonly string requestAcceptCharSet;

        /// <summary>Gets the HTTP MIME type of the request stream.</summary>
        private readonly string requestContentType;

        /// <summary>Gets the value of the If-Match header from the request made</summary>
        private readonly string requestIfMatch;

        /// <summary>Gets the value of the If-None-Match header from the request made</summary>
        private readonly string requestIfNoneMatch;
        
        /// <summary>Gets the value for the MaxDataServiceVersion request header.</summary>
        private readonly string requestMaxVersion;

        /// <summary>Gets the value for the DataServiceVersion request header.</summary>
        private readonly string requestVersion;

        /// <summary>
        /// Get the enum representing the http method name.
        /// We have this for perf reason since enum comparison is faster than string comparison.
        /// </summary>
        private AstoriaVerbs astoriaHttpVerb;

        /// <summary>Gets the HTTP data transfer method (such as GET, POST, or HEAD) used by the client.</summary>
        private string requestHttpMethod;

        /// <summary>Gets the absolute URI to the resource upon which to apply the request.</summary>
        private Uri absoluteRequestUri;

        /// <summary>Gets the absolute URI to the service.</summary>
        private Uri absoluteServiceUri;

        /// <summary>Gets the <see cref="Stream"/> from which the input must be read to the client.</summary>
        private Stream requestStream;

        /// <summary>Gets the <see cref="Stream"/> to be written to send a response to the client.</summary>
        private Stream responseStream;

        /// <summary>Request headers</summary>
        private WebHeaderCollection requestHeaders;

        /// <summary>Response headers</summary>
        private WebHeaderCollection responseHeaders;

        #endregion Private Fields

        #region Constructor

        /// <summary>
        /// Constructs an instance of the DataServiceHostWrapper object.
        /// </summary>
        /// <param name="host">IDataServiceHost instance to wrap.</param>
        internal DataServiceHostWrapper(IDataServiceHost host)
        {
            Debug.Assert(host != null, "host != null");
            this.host = host;
            this.astoriaHttpVerb = AstoriaVerbs.None;
            this.requestAccept = host.RequestAccept;
            this.requestAcceptCharSet = host.RequestAcceptCharSet;
            this.requestContentType = host.RequestContentType;
            this.requestIfMatch = host.RequestIfMatch;
            this.requestIfNoneMatch = host.RequestIfNoneMatch;
            this.requestMaxVersion = host.RequestMaxVersion;
            this.requestVersion = host.RequestVersion;
        }

        #endregion Constructor

        #region Properties.

        /// <summary>
        /// Get the enum representing the http method name.
        /// </summary>
        /// <param name="httpMethodName">http method used for the request.</param>
        /// <returns>enum representing the http method name.</returns>
        internal AstoriaVerbs AstoriaHttpVerb
        {
            get
            {
                if (this.astoriaHttpVerb == AstoriaVerbs.None)
                {
                    string requestMethod = this.RequestHttpMethod;
                    Debug.Assert(!string.IsNullOrEmpty(requestMethod), "!string.IsNullOrEmpty(requestMethod)");
                    switch (requestMethod)
                    {
                        case XmlConstants.HttpMethodGet:
                            this.astoriaHttpVerb = AstoriaVerbs.GET;
                            break;

                        case XmlConstants.HttpMethodPost:
                            this.astoriaHttpVerb = AstoriaVerbs.POST;
                            break;

                        case XmlConstants.HttpMethodPut:
                            this.astoriaHttpVerb = AstoriaVerbs.PUT;
                            break;

                        case XmlConstants.HttpMethodMerge:
                            this.astoriaHttpVerb = AstoriaVerbs.MERGE;
                            break;

                        case XmlConstants.HttpMethodDelete:
                            this.astoriaHttpVerb = AstoriaVerbs.DELETE;
                            break;

                        default:
                            // 501: Not Implemented (rather than 405 - Method Not Allowed, 
                            // which implies it was understood and rejected).
                            throw DataServiceException.CreateMethodNotImplemented(Strings.DataService_NotImplementedException);
                    }
                }

                return this.astoriaHttpVerb;
            }
        }

        /// <summary>Gets the absolute resource upon which to apply the request.</summary>
        internal Uri AbsoluteRequestUri
        {
            get
            {
                if (this.absoluteRequestUri == null)
                {
                    this.absoluteRequestUri = this.host.AbsoluteRequestUri;
                    if (this.absoluteRequestUri == null)
                    {
                        throw new InvalidOperationException(Strings.RequestUriProcessor_AbsoluteRequestUriCannotBeNull);
                    }
                    else if (!this.absoluteRequestUri.IsAbsoluteUri)
                    {
                        throw new InvalidOperationException(Strings.RequestUriProcessor_AbsoluteRequestUriMustBeAbsolute);
                    }
                }

                return this.absoluteRequestUri;
            }
        }

        /// <summary>Gets the absolute URI to the service.</summary>
        internal Uri AbsoluteServiceUri
        {
            get
            {
                if (this.absoluteServiceUri == null)
                {
                    this.absoluteServiceUri = this.host.AbsoluteServiceUri;
                    if (this.absoluteServiceUri == null)
                    {
                        throw new InvalidOperationException(Strings.RequestUriProcessor_AbsoluteServiceUriCannotBeNull);
                    }
                    else if (!this.absoluteServiceUri.IsAbsoluteUri)
                    {
                        throw new InvalidOperationException(Strings.RequestUriProcessor_AbsoluteServiceUriMustBeAbsolute);
                    }
                }

                return this.absoluteServiceUri;
            }
        }

        /// <summary>Gets a comma-separated list of client-supported MIME Accept types.</summary>
        internal string RequestAccept
        {
            get { return this.requestAccept; }
        }

        /// <summary>Gets the string with the specification for the character set encoding that the client requested, possibly null.</summary>
        internal string RequestAcceptCharSet
        {
            get { return this.requestAcceptCharSet; }
        }

        /// <summary>Gets the HTTP MIME type of the request stream.</summary>
        internal string RequestContentType
        {
            get { return this.requestContentType; }
        }

        /// <summary>Gets the HTTP data transfer method (such as GET, POST, or HEAD) used by the client.</summary>
        internal string RequestHttpMethod
        {
            get
            {
                if (string.IsNullOrEmpty(this.requestHttpMethod))
                {
                    this.requestHttpMethod = this.host.RequestHttpMethod;
                    if (String.IsNullOrEmpty(this.requestHttpMethod))
                    {
                        throw new InvalidOperationException(Strings.DataServiceHost_EmptyHttpMethod);
                    }
                }

                return this.requestHttpMethod;
            }
        }

        /// <summary>Gets the value of the If-Match header from the request made</summary>
        internal string RequestIfMatch
        {
            get { return this.requestIfMatch; }
        }

        /// <summary>Gets the value of the If-None-Match header from the request made</summary>
        internal string RequestIfNoneMatch
        {
            get { return this.requestIfNoneMatch; }
        }

        /// <summary>Gets the value for the MaxDataServiceVersion request header.</summary>
        internal string RequestMaxVersion
        {
            get { return this.requestMaxVersion; }
        }

        /// <summary>Gets the <see cref="Stream"/> from which the input must be read to the client.</summary>
        internal Stream RequestStream
        {
            get
            {
                if (this.requestStream == null)
                {
                    this.requestStream = this.host.RequestStream;
                    if (this.requestStream == null)
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequest_NullRequestStream);
                    }
                }

                return this.requestStream;
            }
        }

        /// <summary>Gets the value for the DataServiceVersion request header.</summary>
        internal string RequestVersion
        {
            get { return this.requestVersion; }
        }

        /// <summary>Request headers</summary>
        internal WebHeaderCollection RequestHeaders
        {
            get
            {
                if (this.requestHeaders == null)
                {
                    IDataServiceHost2 host2 = DataServiceHostWrapper.ValidateAndCast<IDataServiceHost2>(this.host);
                    this.requestHeaders = host2.RequestHeaders;
                    if (this.requestHeaders == null)
                    {
                        throw new InvalidOperationException(Strings.DataServiceHost_RequestHeadersCannotBeNull);
                    }
                }

                return this.requestHeaders;
            }
        }

        /// <summary>Response headers</summary>
        internal WebHeaderCollection ResponseHeaders
        {
            get
            {
                if (this.responseHeaders == null)
                {
                    IDataServiceHost2 host2 = DataServiceHostWrapper.ValidateAndCast<IDataServiceHost2>(this.host);
                    this.responseHeaders = host2.ResponseHeaders;
                    if (this.responseHeaders == null)
                    {
                        throw new InvalidOperationException(Strings.DataServiceHost_ResponseHeadersCannotBeNull);
                    }
                }

                return this.responseHeaders;
            }
        }

        /// <summary>Gets or sets the Cache-Control header on the response.</summary>
        internal string ResponseCacheControl
        {
            // No up stream caller
            // get { return this.host.ResponseCacheControl; }
            set { this.host.ResponseCacheControl = value; }
        }

        /// <summary>Gets or sets the HTTP MIME type of the output stream.</summary>
        internal string ResponseContentType
        {
            get { return this.host.ResponseContentType; }
            set { this.host.ResponseContentType = value; }
        }

        /// <summary>Gets/Sets the value of the ETag header on the response</summary>
        internal string ResponseETag
        {
            get { return this.host.ResponseETag; }
            set { this.host.ResponseETag = value; }
        }

        /// <summary>Gets or sets the Location header on the response.</summary>
        internal string ResponseLocation
        {
            // No up stream caller
            // get { return this.host.ResponseLocation; }
            set { this.host.ResponseLocation = value; }
        }

        /// <summary>
        /// Returns the status code for the request made
        /// </summary>
        internal int ResponseStatusCode
        {
            get { return this.host.ResponseStatusCode; }
            set { this.host.ResponseStatusCode = value; }
        }

        /// <summary>Gets the <see cref="Stream"/> to be written to send a response to the client.</summary>
        internal Stream ResponseStream
        {
            get
            {
                if (this.responseStream == null)
                {
                    this.responseStream = this.host.ResponseStream;
                    if (this.responseStream == null)
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequest_NullResponseStream);
                    }
                }

                return this.responseStream;
            }
        }

        /// <summary>Gets or sets the value for the DataServiceVersion response header.</summary>
        internal string ResponseVersion
        {
            set { this.host.ResponseVersion = value; }
        }

        /// <summary>If the wrapped host is a HttpContextServiceHost, returns the host</summary>
        internal HttpContextServiceHost HttpContextServiceHost
        {
            get { return this.host as HttpContextServiceHost; }
        }

        /// <summary>If the wrapped host is a BatchServiceHost, returns the batch host</summary>
        internal BatchServiceHost BatchServiceHost
        {
            get
            {
                Debug.Assert(this.host is BatchServiceHost, "We shouldn't be calling this outside of the batching code path.");
                return this.host as BatchServiceHost;
            }
        }

        #endregion Properties.

        #region Methods.

        /// <summary>Gets the value for the specified item in the request query string.</summary>
        /// <param name="item">Item to return.</param>
        /// <returns>
        /// The value for the specified item in the request query string;
        /// null if <paramref name="item"/> is not found.
        /// </returns>
        internal string GetQueryStringItem(string item)
        {
            return this.host.GetQueryStringItem(item);
        }

        /// <summary>Method to handle a data service exception during processing.</summary>
        /// <param name="args">Exception handling description.</param>
        internal void ProcessException(HandleExceptionArgs args)
        {
            this.host.ProcessException(args);
        }

        /// <summary>Verifies that query parameters are valid.</summary>
        internal void VerifyQueryParameters()
        {
            if (this.host is HttpContextServiceHost)
            {
                ((HttpContextServiceHost)this.host).VerifyQueryParameters();
            }
        }

        /// <summary>Checks whether the given object instance is of type T.</summary>
        /// <param name="instance">object instance.</param>
        /// <typeparam name="T">type which we need to cast the given instance to.</typeparam>
        /// <returns>Returns strongly typed instance of T, if the given object is an instance of type T.</returns>
        /// <exception cref="InvalidOperationException">If the given object is not of Type T.</exception>
        private static T ValidateAndCast<T>(object instance) where T : class
        {
            T instanceAsT = instance as T;
            if (instanceAsT == null)
            {
                throw new InvalidOperationException(Strings.DataServiceException_GeneralError);
            }

            return instanceAsT;
        }

        #endregion Methods.
    }
}
