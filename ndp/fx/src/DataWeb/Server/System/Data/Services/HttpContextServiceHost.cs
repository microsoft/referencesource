//---------------------------------------------------------------------
// <copyright file="HttpContextServiceHost.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides an HttpContext-based implementation.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Net;
    using System.ServiceModel;
    using System.ServiceModel.Web;

    #endregion Namespaces.

    /// <summary>
    /// Provides access to the environment for a DataService, including information about the current request, based
    /// on the current WebOperationContext.
    /// </summary>
    internal class HttpContextServiceHost : IDataServiceHost2
    {
        #region Private fields.

        /// <summary>Message sent to server.</summary>
        private readonly Stream incomingMessageBody;

        /// <summary>The WCF-based operation context.</summary>
        private readonly WebOperationContext operationContext;

        /// <summary>Whether an error was found when processing this request.</summary>
        private bool errorFound;

        /// <summary>Gets the absolute URI to the resource upon which to apply the request.</summary>
        private Uri absoluteRequestUri;

        /// <summary>Gets the absolute URI to the service.</summary>
        private Uri absoluteServiceUri;

        #endregion Private fields.

        #region Constructors.

        /// <summary>
        /// Initializes a new System.Data.Services.HttpContextServiceHost instance.
        /// </summary>
        /// <param name='messageBody'>Incoming message body to process.</param>
        internal HttpContextServiceHost(Stream messageBody)
        {
            // We capture the current context at initialization time rather
            // than accessing it repeatedly.
            this.incomingMessageBody = messageBody;
            this.operationContext = WebOperationContext.Current;
            if (this.operationContext == null)
            {
                throw new InvalidOperationException(Strings.HttpContextServiceHost_WebOperationContextCurrentMissing);
            }
        }

        #endregion Constructors.

        #region Properties.

        /// <summary>
        /// Gets the character set encoding that the client requested,
        /// possibly null.
        /// </summary>
        string IDataServiceHost.RequestAcceptCharSet
        {
            get
            {
                // Returns a string that contains the comma-separated list of values 
                // associated with the specified key, if found; otherwise, null.
                return this.operationContext.IncomingRequest.Headers[HttpRequestHeader.AcceptCharset]; 
            }
        }

        /// <summary>Gets or sets the HTTP MIME type of the output stream.</summary>
        string IDataServiceHost.ResponseContentType
        {
            get
            {
                return this.operationContext.OutgoingResponse.ContentType;
            }
            
            set
            {
                this.operationContext.OutgoingResponse.ContentType = value;
            }
        }

        /// <summary>Gets the HTTP MIME type of the input stream.</summary>
        string IDataServiceHost.RequestContentType
        {
            get
            {
                return this.operationContext.IncomingRequest.ContentType;
            }
        }

        /// <summary>
        /// Gets a comma-separated list of client-supported MIME Accept types.
        /// </summary>
        string IDataServiceHost.RequestAccept
        {
            get
            {
                return this.operationContext.IncomingRequest.Accept;
            }
        }

        /// <summary>
        /// Gets the HTTP data transfer method (such as GET, POST, or HEAD) used by the client.
        /// </summary>
        string IDataServiceHost.RequestHttpMethod
        {
            get
            {
                string result;
                string[] methodValues = this.operationContext.IncomingRequest.Headers.GetValues(XmlConstants.HttpXMethod);
                if (methodValues == null || methodValues.Length == 0)
                {
                    result = this.operationContext.IncomingRequest.Method;
                }
                else if (methodValues.Length == 1)
                {
                    result = methodValues[0];
                    if (this.operationContext.IncomingRequest.Method != XmlConstants.HttpMethodPost)
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.HttpContextServiceHost_XMethodNotUsingPost);
                    }
                    
                    if (result != XmlConstants.HttpMethodDelete && result != XmlConstants.HttpMethodPut && result != XmlConstants.HttpMethodMerge)
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.HttpContextServiceHost_XMethodIncorrectValue(result));
                    }
                }
                else
                {
                    throw DataServiceException.CreateBadRequestError(Strings.HttpContextServiceHost_XMethodIncorrectCount(methodValues.Length));
                }

                return result;
            }
        }

        /// <summary>Gets the value of the If-Match header from the request made</summary>
        string IDataServiceHost.RequestIfMatch
        {
            get
            {
                return this.operationContext.IncomingRequest.Headers[HttpRequestHeader.IfMatch];
            }
        }

        /// <summary>Gets the value of the If-None-Match header from the request made</summary>
        string IDataServiceHost.RequestIfNoneMatch
        {
            get
            {
                return this.operationContext.IncomingRequest.Headers[HttpRequestHeader.IfNoneMatch];
            }
        }

        /// <summary>Gets the value for the MaxDataServiceVersion request header.</summary>
        string IDataServiceHost.RequestMaxVersion
        {
            get
            {
                return this.operationContext.IncomingRequest.Headers[XmlConstants.HttpMaxDataServiceVersion];
            }
        }

        /// <summary>Gets the value for the DataServiceVersion request header.</summary>
        string IDataServiceHost.RequestVersion
        {
            get
            {
                return this.operationContext.IncomingRequest.Headers[XmlConstants.HttpDataServiceVersion];
            }
        }

        /// <summary>Gets the absolute URI to the resource upon which to apply the request.</summary>
        Uri IDataServiceHost.AbsoluteRequestUri
        {
            get
            {
                if (this.absoluteRequestUri == null)
                {
                    object property;
                    if (OperationContext.Current.IncomingMessageProperties.TryGetValue(XmlConstants.MicrosoftDataServicesRequestUri, out property))
                    {
                        this.absoluteRequestUri = property as Uri;
                        if (this.absoluteRequestUri == null)
                        {
                            throw new InvalidOperationException(Strings.HttpContextServiceHost_IncomingMessagePropertyMustBeValidUriInstance(XmlConstants.MicrosoftDataServicesRequestUri));
                        }
                    }

                    if (this.absoluteRequestUri == null)
                    {
                        UriTemplateMatch match = this.operationContext.IncomingRequest.UriTemplateMatch;
                        this.absoluteRequestUri = WebUtil.ApplyHostHeader(match.RequestUri, this.HostHeader);
                    }
                }

                return this.absoluteRequestUri;
            }
        }

        /// <summary>Gets or sets the Cache-Control header on the response.</summary>
        string IDataServiceHost.ResponseCacheControl
        {
            get { return this.operationContext.OutgoingResponse.Headers[HttpResponseHeader.CacheControl]; }
            set { this.operationContext.OutgoingResponse.Headers[HttpResponseHeader.CacheControl] = value; }
        }

        /// <summary>Gets/Sets the value of the ETag header on the outgoing response</summary>
        string IDataServiceHost.ResponseETag
        {
            get
            {
                return this.operationContext.OutgoingResponse.ETag;
            }

            set
            {
                this.operationContext.OutgoingResponse.ETag = value;
            }
        }

        /// <summary>Gets or sets the Location header on the response.</summary>
        string IDataServiceHost.ResponseLocation
        {
            get { return this.operationContext.OutgoingResponse.Headers[HttpResponseHeader.Location]; }
            set { this.operationContext.OutgoingResponse.Headers[HttpResponseHeader.Location] = value; }
        }

        /// <summary>
        /// Gets/Sets the status code for the request made.
        /// </summary>
        int IDataServiceHost.ResponseStatusCode
        {
            get
            {
                return (int) this.operationContext.OutgoingResponse.StatusCode;
            }

            set
            {
                HttpStatusCode statusCode = (HttpStatusCode)value;
                this.operationContext.OutgoingResponse.StatusCode = statusCode;

                // Some status codes such as NoContent or NotModified MUST NOT include a message-body in the response.
                // We need to set SupressEntityBody to true so that in the case of chuncked encoding WCF won't write
                // a '0' in the message body to indicate there is no more content.
                // Note that not setting this will result in a HTTP Protocol Violation exception when chuncked encoding is on.
                this.operationContext.OutgoingResponse.SuppressEntityBody = MustNotReturnMessageBody(statusCode);
            }
        }

        /// <summary>
        /// Gets the <see cref="Stream"/> to be written to send a response
        /// to the client.
        /// </summary>
        Stream IDataServiceHost.ResponseStream
        {
            get
            {
                // The ResponseStream is not directly accessible - this would
                // prevent WCF from streaming results back. For the WCF host,
                // a Message subclass should write directly in the OnBodyWrite
                // method.
                throw Error.NotSupported();
            }
        }

        /// <summary>Gets or sets the value for the DataServiceVersion response header.</summary>
        string IDataServiceHost.ResponseVersion
        {
            get { return this.operationContext.OutgoingResponse.Headers[XmlConstants.HttpDataServiceVersion]; }
            set { this.operationContext.OutgoingResponse.Headers[XmlConstants.HttpDataServiceVersion] = value; }
        }

        /// <summary>Gets the absolute URI to the service.</summary>
        Uri IDataServiceHost.AbsoluteServiceUri
        {
            get
            {
                if (this.absoluteServiceUri == null)
                {
                    object property;
                    if (OperationContext.Current.IncomingMessageProperties.TryGetValue(XmlConstants.MicrosoftDataServicesRootUri, out property))
                    {
                        this.absoluteServiceUri = property as Uri;
                        if (this.absoluteServiceUri == null)
                        {
                            throw new InvalidOperationException(Strings.HttpContextServiceHost_IncomingMessagePropertyMustBeValidUriInstance(XmlConstants.MicrosoftDataServicesRootUri));
                        }
                    }

                    if (this.absoluteServiceUri == null)
                    {
                        UriTemplateMatch match = this.operationContext.IncomingRequest.UriTemplateMatch;

                        // We never want to consider the last segment of the base URI a 'document' type
                        // of segment to be replaced, ie, http://foo/svc.svc should never remove svc.svc
                        // from the path.
                        this.absoluteServiceUri = WebUtil.ApplyHostHeader(match.BaseUri, this.HostHeader);
                    }

                    if (!String.IsNullOrEmpty(this.absoluteServiceUri.Fragment))
                    {
                        throw new InvalidOperationException(Strings.HttpContextServiceHost_IncomingTemplateMatchFragment(this.absoluteServiceUri));
                    }

                    if (!String.IsNullOrEmpty(this.absoluteServiceUri.Query))
                    {
                        throw new InvalidOperationException(Strings.HttpContextServiceHost_IncomingTemplateMatchQuery(this.absoluteServiceUri));
                    }

                    this.absoluteServiceUri = WebUtil.EnsureLastSegmentEmpty(this.absoluteServiceUri);
                }

                return this.absoluteServiceUri;
            }
        }

        /// <summary>
        /// Gets the <see cref="Stream"/> from which the request data can be read from
        /// to the client.
        /// </summary>
        Stream IDataServiceHost.RequestStream
        {
            [DebuggerStepThrough]
            get { return this.incomingMessageBody; }
        }

        #region IDataServiceHost2 Properties

        /// <summary>Dictionary of all request headers from the host.</summary>
        WebHeaderCollection IDataServiceHost2.RequestHeaders
        {
            [DebuggerStepThrough]
            get { return this.operationContext.IncomingRequest.Headers; }
        }

        /// <summary>Enumerates all response headers that has been set.</summary>
        WebHeaderCollection IDataServiceHost2.ResponseHeaders
        {
            [DebuggerStepThrough]
            get { return this.operationContext.OutgoingResponse.Headers; }
        }

        #endregion IDataServiceHost2 Properties

        /// <summary>Whether an error was found when processing this request.</summary>
        internal bool ErrorFound
        {
            get { return this.errorFound; }
        }

        /// <summary>The value for the Host header in the request, possibly null.</summary>
        private string HostHeader
        {
            get { return this.operationContext.IncomingRequest.Headers[HttpRequestHeader.Host]; }
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
            Debug.Assert(item != null, "item != null");
            Debug.Assert(item.Trim() == item, "item.Trim() == item - otherwise, there are leading/trailing spaces in the name");

            System.Collections.Specialized.NameValueCollection collection = this.operationContext.IncomingRequest.UriTemplateMatch.QueryParameters;
            string[] values = collection.GetValues(item);
            if (values == null || values.Length == 0)
            {
                // Do a scan of arguments ignoring whitespace (SQLBUDT #555944).
                string keyFound = null;
                foreach (string key in collection.Keys)
                {
                    if (key != null && StringComparer.OrdinalIgnoreCase.Equals(key.Trim(), item))
                    {
                        if (keyFound != null)
                        {
                            throw DataServiceException.CreateBadRequestError(Strings.HttpContextServiceHost_AmbiguousItemName(item, keyFound, key));
                        }

                        keyFound = key;
                        values = collection.GetValues(key);
                    }
                }

                if (values == null || values.Length == 0)
                {
                    return null;
                }
            }

            Debug.Assert(values != null && values.Length > 0, "values != null && values.Length > 0 - otherwise we should have returned already");
            if (values.Length == 1)
            {
                return values[0];
            }
            else
            {
                throw DataServiceException.CreateSyntaxError();
            }
        }

        /// <summary>
        /// Method to handle a data service exception during processing.
        /// </summary>
        /// <param name="args">Exception handling description.</param>
        void IDataServiceHost.ProcessException(HandleExceptionArgs args)
        {
            Debug.Assert(this.operationContext != null, "this.operationContext != null");
            this.errorFound = true;
            if (!args.ResponseWritten)
            {
                ((IDataServiceHost)this).ResponseStatusCode = args.ResponseStatusCode;
                ((IDataServiceHost)this).ResponseContentType = args.ResponseContentType;
                if (args.ResponseAllowHeader != null)
                {
                    this.operationContext.OutgoingResponse.Headers[HttpResponseHeader.Allow] = args.ResponseAllowHeader;
                }
            }
        }

        /// <summary>Verifies that query parameters are valid.</summary>
        internal void VerifyQueryParameters()
        {
            HashSet<string> namesFound = new HashSet<string>(StringComparer.Ordinal);
            System.Collections.Specialized.NameValueCollection collection = this.operationContext.IncomingRequest.UriTemplateMatch.QueryParameters;
            for (int i = 0; i < collection.Count; i++)
            {
                string name = collection.GetKey(i);
                if (name == null)
                {
                    // These are values of the form a&b&c, without '='. We just make sure they aren't system
                    // values at all.
                    string[] values = collection.GetValues(i);
                    if (values != null)
                    {
                        for (int j = 0; j < values.Length; j++)
                        {
                            string value = values[j].Trim();
                            if (value.Length > 0 && value[0] == '$')
                            {
                                throw DataServiceException.CreateBadRequestError(
                                    Strings.HttpContextServiceHost_QueryParameterMustBeSpecifiedOnce(value));
                            }
                        }
                    }

                    continue;
                }

                name = name.Trim();
                if (!namesFound.Add(name))
                {
                    throw DataServiceException.CreateBadRequestError(
                        Strings.HttpContextServiceHost_QueryParameterMustBeSpecifiedOnce(name));
                }

                if (name.Length > 0 && name[0] == '$')
                {
                    if (name != XmlConstants.HttpQueryStringExpand &&
                        name != XmlConstants.HttpQueryStringFilter &&
                        name != XmlConstants.HttpQueryStringOrderBy &&
                        name != XmlConstants.HttpQueryStringSkip &&
                        name != XmlConstants.HttpQueryStringSkipToken &&
                        name != XmlConstants.HttpQueryStringInlineCount &&
                        name != XmlConstants.HttpQueryStringTop &&
                        name != XmlConstants.HttpQueryStringSelect)
                    {
                        throw DataServiceException.CreateBadRequestError(
                            Strings.HttpContextServiceHost_UnknownQueryParameter(name));
                    }

                    string[] values = collection.GetValues(i);
                    if (values == null || values.Length != 1)
                    {
                        throw DataServiceException.CreateBadRequestError(
                            Strings.HttpContextServiceHost_QueryParameterMustBeSpecifiedOnce(name));
                    }
                }
            }
        }

        /// <summary>
        /// Check to see if the given status code expects an empty message-body.
        /// </summary>
        /// <param name="statusCode">Http status code</param>
        /// <returns>True if the message-body must be empty for the given status code, false otherwise.</returns>
        private static bool MustNotReturnMessageBody(HttpStatusCode statusCode)
        {
            // Both 204 and 304 must not include a message-body in the response.
            switch (statusCode)
            {
                case HttpStatusCode.NoContent:    // 204
                case HttpStatusCode.ResetContent: // 205
                case HttpStatusCode.NotModified:  // 304
                    return true;

                default:
                    return false;
            }
        }

        #endregion Methods.
    }
}
