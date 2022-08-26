//---------------------------------------------------------------------
// <copyright file="IDataServiceHost.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides the interface definition for DataService hosts.
// </summary>
//
// @owner  mruiz
//---------------------------------------------------------------------

namespace System.Data.Services
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Net;

    #endregion Namespaces.

    /// <summary>
    /// Provides access to the environment for a DataService,
    /// including information about the current request.
    /// </summary>
    public interface IDataServiceHost
    {
        #region Properties.

        /// <summary>Gets the absolute resource upon which to apply the request.</summary>
        Uri AbsoluteRequestUri
        {
            get;
        }

        /// <summary>Gets the absolute URI to the service.</summary>
        Uri AbsoluteServiceUri
        {
            get;
        }

        /// <summary>
        /// Gets a comma-separated list of client-supported MIME Accept types.
        /// </summary>
        string RequestAccept
        {
            get;
        }

        /// <summary>
        /// Gets the string with the specification for the character set 
        /// encoding that the client requested, possibly null.
        /// </summary>
        string RequestAcceptCharSet
        {
            get;
        }

        /// <summary>Gets the HTTP MIME type of the request stream.</summary>
        string RequestContentType
        {
            get;
        }

        /// <summary>
        /// Gets the HTTP data transfer method (such as GET, POST, or HEAD) used by the client.
        /// </summary>
        string RequestHttpMethod
        {
            get;
        }

        /// <summary>Gets the value of the If-Match header from the request made</summary>
        string RequestIfMatch
        {
            get;
        }

        /// <summary>Gets the value of the If-None-Match header from the request made</summary>
        string RequestIfNoneMatch
        {
            get;
        }

        /// <summary>Gets the value for the MaxDataServiceVersion request header.</summary>
        string RequestMaxVersion
        {
            get;
        }

        /// <summary>
        /// Gets the <see cref="Stream"/> from which the input must be read
        /// to the client.
        /// </summary>
        Stream RequestStream
        {
            get;
        }

        /// <summary>Gets the value for the DataServiceVersion request header.</summary>
        string RequestVersion
        {
            get;
        }

        /// <summary>Gets or sets the Cache-Control header on the response.</summary>
        string ResponseCacheControl
        {
            get;
            set;
        }

        /// <summary>Gets or sets the HTTP MIME type of the output stream.</summary>
        string ResponseContentType
        {
            get;
            set;
        }

        /// <summary>Gets/Sets the value of the ETag header on the response</summary>
        string ResponseETag
        {
            get;
            set;
        }

        /// <summary>Gets or sets the Location header on the response.</summary>
        string ResponseLocation
        {
            get;
            set;
        }

        /// <summary>
        /// Returns the status code for the request made
        /// </summary>
        int ResponseStatusCode
        {
            get;
            set;
        }

        /// <summary>
        /// Gets the <see cref="Stream"/> to be written to send a response
        /// to the client.
        /// </summary>
        Stream ResponseStream
        {
            get;
        }

        /// <summary>Gets or sets the value for the DataServiceVersion response header.</summary>
        string ResponseVersion
        {
            get;
            set;
        }

        #endregion Properties.

        #region Methods.

        /// <summary>Gets the value for the specified item in the request query string.</summary>
        /// <param name="item">Item to return.</param>
        /// <returns>
        /// The value for the specified item in the request query string;
        /// null if <paramref name="item"/> is not found.
        /// </returns>
        string GetQueryStringItem(string item);

        /// <summary>Method to handle a data service exception during processing.</summary>
        /// <param name="args">Exception handling description.</param>
        void ProcessException(HandleExceptionArgs args);

        #endregion Methods.
    }

    /// <summary>
    /// Extends IDataServiceHost to include extra request and response headers.
    /// </summary>
    public interface IDataServiceHost2 : IDataServiceHost
    {
        /// <summary>Request headers</summary>
        WebHeaderCollection RequestHeaders
        {
            get;
        }

        /// <summary>Response headers</summary>
        WebHeaderCollection ResponseHeaders
        {
            get;
        }
    }
}
