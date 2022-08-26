//---------------------------------------------------------------------
// <copyright file="DataServiceStreamProviderWrapper.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      This wrapper class forwards calls to the underlying IDataServiceStreamProvider
//      instance and validates responses from it.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    using System;
    using System.IO;
    using System.Diagnostics;
    using System.Collections;
    using System.Collections.Generic;
    using System.Linq;
    using System.Data.Services;

    /// <summary>
    /// Wrapper class to forward calls to the underlying IDataServiceStreamProvider instance and validates responses from it.
    /// </summary>
    internal class DataServiceStreamProviderWrapper
    {
        #region Private Fields

        /// <summary>
        /// Default buffer size used for stream copy.
        /// </summary>
        private const int DefaultBufferSize = 64 * 1024;

        /// <summary>
        /// Stream provider instance
        /// </summary>
        private IDataServiceStreamProvider streamProvider;

        /// <summary>
        /// Data service instance
        /// </summary>
        private IDataService dataService;

        #endregion Private Fields

        #region Constructor

        /// <summary>
        /// Constructs the wrapper class for IDataServiceStreamProvider
        /// </summary>
        /// <param name="dataService">Data service instance</param>
        public DataServiceStreamProviderWrapper(IDataService dataService)
        {
            Debug.Assert(dataService != null, "dataService != null");
            this.dataService = dataService;
        }

        #endregion Constructor

        #region Public Properties

        /// <summary>
        /// Gets buffer size the data service will use when reading from read stream or writing to the write stream.
        /// If the size is less than or equals to 0, the default of 64k will be used.
        /// </summary>
        public int StreamBufferSize
        {
            get
            {
                int size = this.StreamProvider.StreamBufferSize;
                return size > 0 ? size : DataServiceStreamProviderWrapper.DefaultBufferSize;
            }
        }

        #endregion Public Properties

        #region Private Properties

        /// <summary>
        /// Asks the service for an IDataServiceStreamProvider implementation
        /// </summary>
        private IDataServiceStreamProvider StreamProvider
        {
            get
            {
                if (this.streamProvider == null)
                {
                    this.streamProvider = LoadStreamProvider(this.dataService);
                    Debug.Assert(this.streamProvider != null, "this.streamProvider != null");
                }

                return this.streamProvider;
            }
        }

        #endregion Private Properties

        #region Internal Methods

        /// <summary>
        /// Take the given Media Link Entry uri, and construct the default Edit Media Uri.
        /// </summary>
        /// <param name="mediaLinkEntryUri">Uri to the Media Link Entry.</param>
        /// <returns>Uri to the Media Resource.</returns>
        internal static string GetStreamEditMediaUri(string mediaLinkEntryUri)
        {
            Debug.Assert(!string.IsNullOrEmpty(mediaLinkEntryUri), "!string.IsNullOrEmpty(mediaLinkEntryUri)");
            string result = mediaLinkEntryUri;

            if (!result.EndsWith(XmlConstants.UriValueSegment, StringComparison.Ordinal))
            {
                if (!result.EndsWith("/", StringComparison.Ordinal))
                {
                    result += "/";
                }

                result += XmlConstants.UriValueSegment;
            }

            return result;
        }

        /// <summary>
        /// Asks the data service for a stream provider instance.  Throw if none is implemented.
        /// </summary>
        /// <param name="dataService">data service instance</param>
        /// <returns>stream provider instance</returns>
        internal static IDataServiceStreamProvider LoadStreamProvider(IDataService dataService)
        {
            IDataServiceStreamProvider streamProvider = dataService.Provider.GetService<IDataServiceStreamProvider>(dataService);
            if (streamProvider == null)
            {
                throw new DataServiceException(500, Strings.DataServiceStreamProviderWrapper_MustImplementIDataServiceStreamProviderToSupportStreaming);
            }

            return streamProvider;
        }

        /// <summary>
        /// This method is invoked by the data services framework to retrieve the default stream associated
        /// with the Entity Type specified by the <paramref name="entity"/> parameter.
        /// Note that we set the response ETag in the host object before we return.
        /// </summary>
        /// <param name="entity">The stream returned should be the default stream associated with this entity.</param>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        /// <returns>A valid stream the data service use to query / read a streamed BLOB which is associated with the <paramref name="entity"/>.</returns>
        internal Stream GetReadStream(object entity, DataServiceOperationContext operationContext)
        {
            Debug.Assert(entity != null, "entity != null");
            Debug.Assert(operationContext != null, "operationContext != null");

            string etagFromHeader;
            bool? checkETagForEquality;
            DataServiceStreamProviderWrapper.GetETagFromHeaders(operationContext, out etagFromHeader, out checkETagForEquality);
            Debug.Assert(
                string.IsNullOrEmpty(etagFromHeader) && !checkETagForEquality.HasValue || !string.IsNullOrEmpty(etagFromHeader) && checkETagForEquality.HasValue,
                "etag and checkETagForEquality parameters must both be set or not set at the same time.");

            Stream readStream = null;
            try
            {
                readStream = InvokeApiCallAndValidateHeaders("IDataServiceStreamProvider.GetReadStream", () => this.StreamProvider.GetReadStream(entity, etagFromHeader, checkETagForEquality, operationContext), operationContext);
            }
            catch (DataServiceException e)
            {
                if (e.StatusCode == (int)System.Net.HttpStatusCode.NotModified)
                {
                    // For status code 304, we MUST set the etag value.  Our Error handler will translate
                    // DataServiceException(304) to a normal response with status code 304 and an empty message-body.
#if DEBUG
                    WebUtil.WriteETagValueInResponseHeader(null, this.GetStreamETag(entity, operationContext), operationContext.Host);
#else
                    WebUtil.WriteETagValueInResponseHeader(this.GetStreamETag(entity, operationContext), operationContext.Host);
#endif
                }

                throw;
            }

            try
            {
                if (readStream == null || !readStream.CanRead)
                {
                    throw new InvalidOperationException(Strings.DataService_InvalidStreamFromGetReadStream);
                }

                // GetStreamETag can throw and we need to catch and dispose the stream.
#if DEBUG
                WebUtil.WriteETagValueInResponseHeader(null, this.GetStreamETag(entity, operationContext), operationContext.Host);
#else
                WebUtil.WriteETagValueInResponseHeader(this.GetStreamETag(entity, operationContext), operationContext.Host);
#endif
            }
            catch
            {
                WebUtil.Dispose(readStream);
                throw;
            }

            return readStream;
        }

        /// <summary>
        /// This method is invoked by the data services framework whenever an insert or update operation is 
        /// being processed for the stream associated with the Entity Type specified via the entity parameter.
        /// </summary>
        /// <param name="entity">The stream returned should be the default stream associated with this entity.</param>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        /// <returns>A valid stream the data service use to write the contents of a BLOB which is associated with <paramref name="entity"/>.</returns>
        internal Stream GetWriteStream(object entity, DataServiceOperationContext operationContext)
        {
            Debug.Assert(entity != null, "entity != null");
            Debug.Assert(operationContext != null, "operationContext != null");

            string etag;
            bool? checkETagForEquality;
            DataServiceStreamProviderWrapper.GetETagFromHeaders(operationContext, out etag, out checkETagForEquality);
            Debug.Assert(
                string.IsNullOrEmpty(etag) && !checkETagForEquality.HasValue || !string.IsNullOrEmpty(etag) && checkETagForEquality.HasValue,
                "etag and checkETagForEquality parameters must both be set or not set at the same time.");

            Stream writeStream = InvokeApiCallAndValidateHeaders("IDataServiceStreamProvider.GetWriteStream", () => this.StreamProvider.GetWriteStream(entity, etag, checkETagForEquality, operationContext), operationContext);
            if (writeStream == null || !writeStream.CanWrite)
            {
                WebUtil.Dispose(writeStream);
                throw new InvalidOperationException(Strings.DataService_InvalidStreamFromGetWriteStream);
            }

            return writeStream;
        }

        /// <summary>
        /// This method is invoked by the data services framework whenever an delete operation is being processed for the stream associated with
        /// the Entity Type specified via the entity parameter.
        /// </summary>
        /// <param name="entity">The stream deleted should be the default stream associated with this entity.</param>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        internal void DeleteStream(object entity, DataServiceOperationContext operationContext)
        {
            Debug.Assert(entity != null, "entity != null");
            Debug.Assert(operationContext != null, "operationContext != null");
            InvokeApiCallAndValidateHeaders("IDataServiceStreamProvider.DeleteStream", () => { this.StreamProvider.DeleteStream(entity, operationContext); return true; }, operationContext);
        }

        /// <summary>
        /// This method is invoked by the data services framework to obtain the IANA content type (aka media type) of the stream associated
        /// with the specified entity.  This metadata is needed when constructing the payload for the Media Link Entry associated with the
        /// stream (aka Media Resource) or setting the Content-Type HTTP response header.
        /// </summary>
        /// <param name="entity">The entity associated with the stream for which the content type is to be obtained</param>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        /// <returns>Valid Content-Type string for the stream associated with the entity</returns>
        internal string GetStreamContentType(object entity, DataServiceOperationContext operationContext)
        {
            Debug.Assert(entity != null, "entity != null");
            Debug.Assert(operationContext != null, "operationContext != null");

            string contentType = InvokeApiCallAndValidateHeaders("IDataServiceStreamProvider.GetStreamContentType", () => this.StreamProvider.GetStreamContentType(entity, operationContext), operationContext);
            if (string.IsNullOrEmpty(contentType))
            {
                throw new InvalidOperationException(Strings.DataServiceStreamProviderWrapper_GetStreamContentTypeReturnsEmptyOrNull);
            }

            return contentType;
        }

        /// <summary>
        /// This method is invoked by the data services framework to obtain the URI clients should use when making retrieve (ie. GET)
        /// requests to the stream(ie. Media Resource).   This metadata is needed when constructing the payload for the Media Link Entry
        /// associated with the stream (aka Media Resource).
        /// 
        /// If IDataServiceStreamProvider.GetReadStreamUri returns a valid Uri, we return that as the Uri to the Media Resource.
        /// Otherwise we take the given Media Link Entry uri, and construct the default Media Resource Uri.
        /// </summary>
        /// <param name="entity">The entity associated with the stream for which a “read stream” is to be obtained</param>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        /// <param name="mediaLinkEntryUri">Uri to the Media Link Entry.</param>
        /// <returns>The URI clients should use when making retrieve (ie. GET) requests to the stream(ie. Media Resource).</returns>
        internal Uri GetReadStreamUri(object entity, DataServiceOperationContext operationContext, string mediaLinkEntryUri)
        {
            Debug.Assert(entity != null, "entity != null");
            Debug.Assert(operationContext != null, "operationContext != null");

            Uri readStreamUri = InvokeApiCallAndValidateHeaders("IDataServiceStreamProvider.GetReadStreamUri", () => this.StreamProvider.GetReadStreamUri(entity, operationContext), operationContext);
            if (readStreamUri != null)
            {
                if (!readStreamUri.IsAbsoluteUri)
                {
                    throw new InvalidOperationException(Strings.DataServiceStreamProviderWrapper_GetReadStreamUriMustReturnAbsoluteUriOrNull);
                }
                else
                {
                    return readStreamUri;
                }
            }
            else
            {
                return new Uri(DataServiceStreamProviderWrapper.GetStreamEditMediaUri(mediaLinkEntryUri), UriKind.RelativeOrAbsolute);
            }
        }

        /// <summary>
        /// This method is invoked by the data services framework to obtain the ETag of the stream associated with the entity specified.
        /// This metadata is needed when constructing the payload for the Media Link Entry associated with the stream (aka Media Resource)
        /// as well as to be used as the value of the ETag HTTP response header.
        /// </summary>
        /// <param name="entity">The entity associated with the stream for which an etag is to be obtained</param>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        /// <returns>ETag of the stream associated with the entity specified</returns>
        internal string GetStreamETag(object entity, DataServiceOperationContext operationContext)
        {
            Debug.Assert(entity != null, "entity != null");
            Debug.Assert(operationContext != null, "operationContext != null");

            string etag = InvokeApiCallAndValidateHeaders("IDataServiceStreamProvider.GetStreamETag", () => this.StreamProvider.GetStreamETag(entity, operationContext), operationContext);
            if (!WebUtil.IsETagValueValid(etag, true))
            {
                throw new InvalidOperationException(Strings.DataServiceStreamProviderWrapper_GetStreamETagReturnedInvalidETagFormat);
            }

            return etag;
        }

        /// <summary>
        /// This method is invoked by the data services framework when a request is received to insert into an Entity Set with an associated
        /// Entity Type hierarchy that has > 1 Entity Type and >= 1 Entity Type which is tagged as an MLE (ie. includes a stream).
        /// </summary>
        /// <param name="entitySetName">Fully qualified name entity set name.</param>
        /// <param name="service">Data service instance.</param>
        /// <returns>
        /// Namespace qualified type name which represents the type the Astoria framework should instantiate to create the MLE associated
        /// with the BLOB/MR being inserted.
        /// </returns>
        internal ResourceType ResolveType(string entitySetName, IDataService service)
        {
            DataServiceOperationContext operationContext = service.OperationContext;
            Debug.Assert(operationContext != null, "operationContext != null");
            string resourceTypeName = InvokeApiCallAndValidateHeaders("IDataServiceStreamProvider.ResolveType", () => this.StreamProvider.ResolveType(entitySetName, operationContext), operationContext);
            if (string.IsNullOrEmpty(resourceTypeName))
            {
                throw new InvalidOperationException(Strings.DataServiceStreamProviderWrapper_ResolveTypeMustReturnValidResourceTypeName);
            }

            ResourceType resourceType = service.Provider.TryResolveResourceType(resourceTypeName);
            if (resourceType == null)
            {
                throw new InvalidOperationException(Strings.DataServiceStreamProviderWrapper_ResolveTypeMustReturnValidResourceTypeName);
            }

            return resourceType;
        }

        /// <summary>
        /// Gets the ETag, ReadStreamUri and ContentType of the stream
        /// </summary>
        /// <param name="entity">MLE instance</param>
        /// <param name="operationContext">context of the current operation</param>
        /// <param name="mediaLinkEntryUri">Uri to the MLE</param>
        /// <param name="etag">returns the etag for the stream</param>
        /// <param name="readStreamUri">returns the read stream uri</param>
        /// <param name="contentType">returns the content type of the stream</param>
        internal void GetStreamDescription(object entity, DataServiceOperationContext operationContext, string mediaLinkEntryUri, out string etag, out Uri readStreamUri, out string contentType)
        {
            Debug.Assert(entity != null, "entity != null");
            Debug.Assert(operationContext != null, "operationContext != null");

            // Call order is part of our contract, do not change it.
            etag = this.GetStreamETag(entity, operationContext);
            readStreamUri = this.GetReadStreamUri(entity, operationContext, mediaLinkEntryUri);
            contentType = this.GetStreamContentType(entity, operationContext);
        }

        /// <summary>
        /// Dispose the stream provider instance
        /// </summary>
        internal void DisposeProvider()
        {
            if (this.streamProvider != null)
            {
                WebUtil.Dispose(this.streamProvider);
                this.streamProvider = null;
            }
        }

        #endregion Public Methods

        #region Private Methods

        /// <summary>
        /// Get the ETag header value from the request headers.
        /// </summary>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        /// <param name="etag">
        /// The etag value sent by the client (as the value of an If[-None-]Match header) as part of the HTTP request sent to the data service
        /// This parameter will be null if no If[-None-]Match header was present
        /// </param>
        /// <param name="checkETagForEquality">
        /// True if an value of the etag parameter was sent to the server as the value of an If-Match HTTP request header
        /// False if an value of the etag parameter was sent to the server as the value of an If-None-Match HTTP request header
        /// null if the HTTP request for the stream was not a conditional request
        /// </param>
        private static void GetETagFromHeaders(DataServiceOperationContext operationContext, out string etag, out bool? checkETagForEquality)
        {
            Debug.Assert(operationContext != null, "operationContext != null");
            Debug.Assert(operationContext.Host != null, "operationContext.Host != null");
            DataServiceHostWrapper host = operationContext.Host;
            Debug.Assert(string.IsNullOrEmpty(host.RequestIfMatch) || string.IsNullOrEmpty(host.RequestIfNoneMatch), "IfMatch and IfNoneMatch should not be both set.");

            if (string.IsNullOrEmpty(host.RequestIfMatch) && string.IsNullOrEmpty(host.RequestIfNoneMatch))
            {
                etag = null;
                checkETagForEquality = null;
            }
            else if (!string.IsNullOrEmpty(host.RequestIfMatch))
            {
                etag = host.RequestIfMatch;
                checkETagForEquality = true;
            }
            else
            {
                etag = host.RequestIfNoneMatch;
                checkETagForEquality = false;
            }
        }

        /// <summary>
        /// Invokes an API call and verifies the response Content-Type and ETag headers are not being modified by the API call.
        /// </summary>
        /// <typeparam name="T">Return type from the API call</typeparam>
        /// <param name="methodName">API name</param>
        /// <param name="apiCall">Delegate to be called</param>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        /// <returns>Returns the result from the api call</returns>
        private static T InvokeApiCallAndValidateHeaders<T>(string methodName, Func<T> apiCall, DataServiceOperationContext operationContext)
        {
            Debug.Assert(!string.IsNullOrEmpty(methodName), "!string.IsNullOrEmpty(methodName)");
            Debug.Assert(operationContext != null, "operationContext != null");
            Debug.Assert(apiCall != null, "apiCall != null");

            string responseContentType = operationContext.Host.ResponseContentType;
            string responseETag = operationContext.Host.ResponseETag;
            T result = apiCall();
            if (operationContext.Host.ResponseContentType != responseContentType || operationContext.Host.ResponseETag != responseETag)
            {
                throw new InvalidOperationException(Strings.DataServiceStreamProviderWrapper_MustNotSetContentTypeAndEtag(methodName));
            }

            return result;
        }
        
        #endregion Private Methods
    }
}
