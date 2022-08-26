//---------------------------------------------------------------------
// <copyright file="BatchServiceHost.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides an internal implementation for IDataServiceHost to keep track of states
//      for batch operations
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Diagnostics;
    using System.IO;
    using System.Web;
    using System.Net;

    #endregion Namespaces.

    /// <summary>
    /// Keeps track of the request and response headers for each
    /// operation in the batch
    /// </summary>
    internal class BatchServiceHost : IDataServiceHost2
    {
        #region Private fields.

        /// <summary>Response separator string.</summary>
        private readonly string boundary;

        /// <summary>Request Stream.</summary>
        private readonly Stream requestStream;

        /// <summary>Content Id for this operation.</summary>
        private readonly string contentId;

        /// <summary>Output writer.</summary>
        private readonly StreamWriter writer;

        /// <summary>Gets the absolute URI to the resource upon which to apply the request.</summary>
        private readonly Uri absoluteRequestUri;

        /// <summary>Gets the absolute URI to the service.</summary>
        private readonly Uri absoluteServiceUri;

        /// <summary>Request Http Method</summary>
        private readonly string requestHttpMethod;

        /// <summary>Collection of request headers for the current batch operation.</summary>
        private readonly WebHeaderCollection requestHeaders;

        /// <summary>Collection of response headers for the current batch operation.</summary>
        private readonly WebHeaderCollection responseHeaders;

        /// <summary>List of query parameters as specified in the request uri.</summary>
        private NameValueCollection queryParameters;

        /// <summary>Value of the response StatusCode header.</summary>
        private int responseStatusCode;

        #endregion Private fields.

        #region Constructors.

        /// <summary>
        /// Initializes a new dummy host for the batch request.
        /// This host represents a single operation in the batch.
        /// </summary>
        /// <param name="absoluteServiceUri">absolute Uri to the service</param>
        /// <param name="batchStream">batch stream which contains the header information.</param>
        /// <param name="contentId">content id for the given operation host.</param>
        /// <param name='boundary'>Response separator string.</param>
        /// <param name='writer'>Output writer.</param>
        internal BatchServiceHost(Uri absoluteServiceUri, BatchStream batchStream, string contentId, string boundary, StreamWriter writer)
            : this(boundary, writer)
        {
            Debug.Assert(absoluteServiceUri != null && absoluteServiceUri.IsAbsoluteUri, "absoluteServiceUri != null && absoluteServiceUri.IsAbsoluteUri");
            Debug.Assert(batchStream != null, "batchStream != null");

            this.absoluteServiceUri = absoluteServiceUri;
            this.absoluteRequestUri = RequestUriProcessor.GetAbsoluteUriFromReference(batchStream.ContentUri, absoluteServiceUri);
            this.requestHttpMethod = GetHttpMethodName(batchStream.State);
            this.requestStream = batchStream.GetContentStream();
            this.contentId = contentId;

            foreach (KeyValuePair<string, string> header in batchStream.ContentHeaders)
            {
                this.requestHeaders.Add(header.Key, header.Value);
            }
        }

        /// <summary>
        /// Initializes a host for error scenarios - something to which we can write the response header values
        /// and write them to the underlying stream.
        /// </summary>
        /// <param name='boundary'>Response separator string.</param>
        /// <param name='writer'>Output writer.</param>
        internal BatchServiceHost(string boundary, StreamWriter writer)
        {
            Debug.Assert(!string.IsNullOrEmpty(boundary), "!string.IsNullOrEmpty(boundary)");
            Debug.Assert(writer != null, "writer != null");

            this.boundary = boundary;
            this.writer = writer;
            this.requestHeaders = new WebHeaderCollection();
            this.responseHeaders = new WebHeaderCollection();
        }

        #endregion Constructors.

        #region Properties.

        /// <summary>Gets the absolute URI to the resource upon which to apply the request.</summary>
        Uri IDataServiceHost.AbsoluteRequestUri
        {
            [DebuggerStepThrough]
            get { return this.absoluteRequestUri; }
        }

        /// <summary>Gets the absolute URI to the service.</summary>
        Uri IDataServiceHost.AbsoluteServiceUri
        {
            [DebuggerStepThrough]
            get { return this.absoluteServiceUri; }
        }

        /// <summary>
        /// Gets the character set encoding that the client requested,
        /// possibly null.
        /// </summary>
        string IDataServiceHost.RequestAccept
        {
            get { return this.requestHeaders[HttpRequestHeader.Accept]; }
        }

        /// <summary>
        /// Gets the character set encoding that the client requested,
        /// possibly null.
        /// </summary>
        string IDataServiceHost.RequestAcceptCharSet
        {
            get { return this.requestHeaders[HttpRequestHeader.AcceptCharset]; }
        }

        /// <summary>Gets the HTTP MIME type of the input stream.</summary>
        string IDataServiceHost.RequestContentType
        {
            get { return this.requestHeaders[HttpRequestHeader.ContentType]; }
        }

        /// <summary>
        /// Gets the HTTP data transfer method (such as GET, POST, or HEAD) used by the client.
        /// </summary>
        string IDataServiceHost.RequestHttpMethod
        {
            [DebuggerStepThrough]
            get { return this.requestHttpMethod; }
        }

        /// <summary>Gets the value of the If-Match header from the request made</summary>
        string IDataServiceHost.RequestIfMatch
        {
            get { return this.requestHeaders[HttpRequestHeader.IfMatch]; }
        }

        /// <summary>Gets the value of the If-None-Match header from the request made</summary>
        string IDataServiceHost.RequestIfNoneMatch
        {
            get { return this.requestHeaders[HttpRequestHeader.IfNoneMatch]; }
        }

        /// <summary>Gets the value for the MaxDataServiceVersion request header.</summary>
        string IDataServiceHost.RequestMaxVersion
        {
            get { return this.requestHeaders[XmlConstants.HttpMaxDataServiceVersion]; }
        }

        /// <summary>Gets the value for the DataServiceVersion request header.</summary>
        string IDataServiceHost.RequestVersion
        {
            get { return this.requestHeaders[XmlConstants.HttpDataServiceVersion]; }
        }

        /// <summary>Gets or sets the Cache-Control header on the response.</summary>
        string IDataServiceHost.ResponseCacheControl
        {
            get { return this.responseHeaders[HttpResponseHeader.CacheControl]; }
            set { this.responseHeaders[HttpResponseHeader.CacheControl] = value; }
        }

        /// <summary>Gets or sets the HTTP MIME type of the output stream.</summary>
        string IDataServiceHost.ResponseContentType
        {
            get { return this.responseHeaders[HttpResponseHeader.ContentType]; }
            set { this.responseHeaders[HttpResponseHeader.ContentType] = value; }
        }

        /// <summary>Gets/Sets the value of the ETag header on the outgoing response</summary>
        string IDataServiceHost.ResponseETag
        {
            get { return this.responseHeaders[HttpResponseHeader.ETag]; }
            set { this.responseHeaders[HttpResponseHeader.ETag] = value; }
        }

        /// <summary>Gets or sets the Location header on the response.</summary>
        string IDataServiceHost.ResponseLocation
        {
            get { return this.responseHeaders[HttpResponseHeader.Location]; }
            set { this.responseHeaders[HttpResponseHeader.Location] = value; }
        }

        /// <summary>
        /// Gets/Sets the status code for the request made.
        /// </summary>
        int IDataServiceHost.ResponseStatusCode
        {
            get { return this.responseStatusCode; }
            set { this.responseStatusCode = value; }
        }

        /// <summary>
        /// Gets the <see cref="Stream"/> to be written to send a response
        /// to the client.
        /// </summary>
        Stream IDataServiceHost.ResponseStream
        {
            get
            {
                // There is a batch stream for writing requests for batch operations.
                // Hence this method should never be called.
                throw Error.NotSupported();
            }
        }

        /// <summary>Gets or sets the value for the DataServiceVersion response header.</summary>
        string IDataServiceHost.ResponseVersion
        {
            get { return this.responseHeaders[XmlConstants.HttpDataServiceVersion]; }
            set { this.responseHeaders[XmlConstants.HttpDataServiceVersion] = value; }
        }

        /// <summary>
        /// Gets the <see cref="Stream"/> from which the request data can be read from
        /// to the client.
        /// </summary>
        Stream IDataServiceHost.RequestStream
        {
            [DebuggerStepThrough]
            get { return this.requestStream; }
        }

        #region IDataServiceHost2 Properties

        /// <summary>Dictionary of all request headers.</summary>
        WebHeaderCollection IDataServiceHost2.RequestHeaders
        {
            get { return this.requestHeaders; }
        }

        /// <summary>Enumerates all response headers that has been set.</summary>
        WebHeaderCollection IDataServiceHost2.ResponseHeaders
        {
            get { return this.responseHeaders; }
        }

        #endregion IDataServiceHost2 Properties

        /// <summary>Response separator string.</summary>
        internal string BoundaryString
        {
            get { return this.boundary; }
        }

        /// <summary>
        /// Gets/Sets the content id as specified in the batch request.
        /// This same value is written out in the response headers also to allow mapping requests on the client.
        /// </summary>
        internal string ContentId
        {
            get { return this.contentId; }
        }

        /// <summary>Output writer.</summary>
        internal StreamWriter Writer
        {
            get { return this.writer; }
        }

        #endregion Properties.

        #region Methods.

        /// <summary>Gets the value for the specified item in the request query string.</summary>
        /// <param name="item">Item to return.</param>
        /// <returns>
        /// The value for the specified item in the request query string;
        /// null if <paramref name="item"/> is not found.
        /// </returns>
        string IDataServiceHost.GetQueryStringItem(string item)
        {
            this.GetUriAndQueryParameters();

            string[] result = this.queryParameters.GetValues(item);
            if (result == null || result.Length == 0)
            {
                return null;
            }
            else if (result.Length == 1)
            {
                return result[0];
            }
            else
            {
                throw DataServiceException.CreateBadRequestError(
                    Strings.DataServiceHost_MoreThanOneQueryParameterSpecifiedWithTheGivenName(item, this.absoluteRequestUri));
            }
        }

        /// <summary>Method to handle a data service exception during processing.</summary>
        /// <param name="args">Exception handling description.</param>
        void IDataServiceHost.ProcessException(HandleExceptionArgs args)
        {
            // This would typically set headers on the host.
            WebUtil.CheckArgumentNull(args, "args");
            Debug.Assert(WebUtil.IsCatchableExceptionType(args.Exception), "WebUtil.IsCatchableExceptionType(args.Exception)");
            this.responseStatusCode = args.ResponseStatusCode;
            this.responseHeaders[HttpResponseHeader.ContentType] = args.ResponseContentType;
            this.responseHeaders[HttpResponseHeader.Allow] = args.ResponseAllowHeader;

            // Only write the headers if the response is not written
            if (!args.ResponseWritten)
            {
                System.Data.Services.Serializers.BatchWriter.WriteBoundaryAndHeaders(this.writer, this, this.contentId, this.boundary);
            }
        }

        /// <summary>
        /// Returns the http method name given the batch stream state
        /// </summary>
        /// <param name="state">state of the batch stream.</param>
        /// <returns>returns the http method name</returns>
        private static string GetHttpMethodName(BatchStreamState state)
        {
            Debug.Assert(
                state == BatchStreamState.Get ||
                state == BatchStreamState.Post ||
                state == BatchStreamState.Put ||
                state == BatchStreamState.Delete ||
                state == BatchStreamState.Merge,
                "Expecting BatchStreamState (" + state + ") to be Delete, Get, Post or Put");

            switch (state)
            {
                case BatchStreamState.Delete:
                    return XmlConstants.HttpMethodDelete;
                case BatchStreamState.Get:
                    return XmlConstants.HttpMethodGet;
                case BatchStreamState.Post:
                    return XmlConstants.HttpMethodPost;
                case BatchStreamState.Merge:
                    return XmlConstants.HttpMethodMerge;
                default:
                    Debug.Assert(BatchStreamState.Put == state, "BatchStreamState.Put == state");
                    return XmlConstants.HttpMethodPut;
            }
        }

        /// <summary>
        /// Given the request uri, parse the uri and query parameters and cache them
        /// </summary>
        private void GetUriAndQueryParameters()
        {
            if (this.queryParameters == null)
            {
                this.queryParameters = HttpUtility.ParseQueryString(this.absoluteRequestUri.Query);
            }
        }

        #endregion Methods.
    }
}
