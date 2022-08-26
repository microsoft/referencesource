//---------------------------------------------------------------------
// <copyright file="IDataServiceStreamProvider.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      The IDataServiceStreamProvider interface defines the contract
//      between the data services framework server component and a data
//      source's stream implementation (ie. a stream provider).
// </summary>
//
// @owner  jli
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    using System;
    using System.IO;

    /// <summary>
    /// The IDataServiceStreamProvider interface defines the contract between the data services framework server component
    /// and a data source's stream implementation (ie. a stream provider).
    /// </summary>
    public interface IDataServiceStreamProvider
    {
        /// <summary>
        /// Specifies the buffer size the data service will use when reading from read stream or writing to the write stream.
        /// If the size is 0, the default of 64k will be used.
        /// </summary>
        int StreamBufferSize
        {
            get;
        }

        /// <summary>
        /// This method is invoked by the data services framework to retrieve the default stream associated
        /// with the Entity Type specified by the <paramref name="entity"/> parameter.
        /// 
        /// Notes to Interface Implementers
        /// The host is passed as an argument as it is likely that an implementer of this interface method
        /// will need information from the HTTP request headers in order to construct a stream.  Likely header
        /// values required are: 
        ///   'Accept'
        ///   'Accept-Charset'
        ///   'Accept-Encoding'
        ///   
        /// An implementer of this method MUST perform concurrency checks as needed in their implementation of
        /// this method.  If an If-Match or If-None-Match request header was included in the request, then the
        /// etag parameter will be non null, which indicates this method MUST perform the appropriate concurrency
        /// check.  If the concurrency check passes, this method should return the requested stream.  If the
        /// concurrency checks fails, the method should throw  a DataServiceException with the appropriate HTTP
        /// response code as defined in HTTP RFC 2616 section 14.24 and section 14.26.
        ///   If the etag was sent as the value of an If-Match request header, the value of the ‘checkETagForEquality’
        ///   header will be set to true
        ///   If the etag was sent as the value of an If-None-Match request header, the value of the
        ///   ‘checkETagForEquality’ header will be set to false
        ///
        /// It is the implementer of this methods responsibility to honor the values of the appropriate request
        /// headers when generating the returned response stream.
        /// 
        /// An implementer of this method MUST NOT set the following HTTP response headers (on the host instance
        /// passed as an argument) as they are set by the data service runtime:
        ///   Content-Type
        ///   ETag   
        /// An implementer of this method may set HTTP response headers (other than those forbidden above) on the
        /// host instance passed as an argument.
        ///
        /// NOTE: an implementer of this method should only set the properties on the host instance which it
        /// requires to be set for a successful response.  Altering other properties on the host instance may
        /// corrupt the response from the data service.
        /// 
        /// Stream Ownership
        /// The data service framework will close the stream once all byte have been successfully read.
        /// 
        /// If an error occurs while reading the stream, then the data services framework will generate an
        /// in-stream error which is sent back to the client.  See the error contract specification for a 
        /// description of the format of in-stream errors
        /// </summary>
        /// <param name="entity">The stream returned should be the default stream associated with this entity.</param>
        /// <param name="etag">
        /// The etag value sent by the client (as the value of an If[-None-]Match header) as part of the HTTP request
        /// sent to the data service
        /// This parameter will be null if no If[-None-]Match header was present
        /// </param>
        /// <param name="checkETagForEquality">
        /// True if an value of the etag parameter was sent to the server as the value of an If-Match HTTP request header
        /// False if an value of the etag parameter was sent to the server as the value of an If-None-Match HTTP request header
        /// null if the HTTP request for the stream was not a conditional request 
        /// </param>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        /// <returns>A valid stream the data service use to query / read a streamed BLOB which is associated with the <paramref name="entity"/>.</returns>
        /// <remarks>An implementer of this method of this method should thrown the following exceptions under the specified conditions:</remarks>
        /// <exception cref="ArgumentNullException">entity or host parameters are null.</exception>
        /// <exception cref="ArgumentException">entity does not represent an Entity Type which has a default stream attached to it.</exception>
        /// <exception cref="DataServiceException">if a valid stream cannot be returned.  Null should never be returned from this method.</exception>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704", Justification = "etag is not Hungarian notation")]
        Stream GetReadStream(object entity, string etag, bool? checkETagForEquality, DataServiceOperationContext operationContext);

        /// <summary>
        /// This method is invoked by the data services framework whenever an insert or update operation is 
        /// being processed for the stream associated with the Entity Type specified via the entity parameter.
        /// 
        /// Notes to Interface Implementers
        /// The host is passed as an argument as it is likely that an implementer of this interface method will 
        /// need information from the HTTP request headers in order to construct a write stream.  Likely header 
        /// values required are: 
        ///   'Content-Type'
        ///   'Content-Disposition'
        ///   'Slug' (as specified in the AtomPub RFC 5023)
        ///   
        /// An implementer of this method MUST perform concurrency checks as needed in their implementation of this method.
        /// If an If-Match or If-None-Match request header was included in the request, then the etag parameter will be non null,
        /// which indicates this method MUST perform the appropriate concurrency check.  If the concurrency check passes, this
        /// method should return the requested stream.  If the concurrency checks fails, the method should throw  a DataServiceException
        /// with the appropriate HTTP response code as defined in HTTP RFC 2616 section 14.24 and section 14.26. 
        ///   If the etag was sent as the value of an If-Match request header, the value of the ‘checkETagForEquality’ header will be set to true
        ///   If the etag was sent as the value of an If-None-Match request header, the value of the ‘checkETagForEquality’ header will be set to false
        ///   
        /// An implementer of this method MUST NOT set the following HTTP response headers (on the host instance passed as an argument)
        /// as they are set by the data service runtime:
        ///   Content-Type
        ///   ETag
        ///   
        /// An implementer of this method may set HTTP response headers (other than those forbidden above) on the host instance passed as an argument.
        /// 
        /// NOTE: an implementer of this method should only set the properties on the host instance which it requires to be set for a successful
        /// response.  Altering other properties on the host instance may corrupt the response from the data service.
        /// 
        /// Stream Ownership
        /// The data service framework will close the stream once all bytes have been successfully written to
        /// the stream.
        /// 
        /// If an error occurs while writing to the stream, then the data services framework will generate an 
        /// error response to the client as per the "error contract" semantics followed by V1 data services
        /// </summary>
        /// <param name="entity">The stream returned should be the default stream associated with this entity.</param>
        /// <param name="etag">
        /// The etag value sent by the client (as the value of an If[-None-]Match header) as part of the HTTP request sent to the data service
        /// This parameter will be null if no If[-None-]Match header was present
        /// </param>
        /// <param name="checkETagForEquality">
        /// True if an value of the etag parameter was sent to the server as the value of an If-Match HTTP request header
        /// False if an value of the etag parameter was sent to the server as the value of an If-None-Match HTTP request header
        /// null if the HTTP request for the stream was not a conditional request
        /// </param>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        /// <returns>A valid stream the data service use to write the contents of a BLOB which is associated with <paramref name="entity"/>.</returns>
        /// <remarks>An implementer of this method of this method should thrown the following exceptions under the specified conditions:</remarks>
        /// <exception cref="ArgumentNullException">entity or host parameters are null.</exception>
        /// <exception cref="ArgumentException">entity does not represent an Entity Type which has a default stream attached to it.</exception>
        /// <exception cref="DataServiceException">if a valid stream cannot be returned.  Null should never be returned from this method.</exception>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704", Justification = "etag is not Hungarian notation")]
        Stream GetWriteStream(object entity, string etag, bool? checkETagForEquality, DataServiceOperationContext operationContext);

        /// <summary>
        /// This method is invoked by the data services framework whenever an delete operation is being processed for the stream associated with
        /// the Entity Type specified via the entity parameter.
        /// 
        /// Notes to Interface Implementers
        /// If this method is being invoked as part of a request to delete the MLE and its associated stream (ie. MR):
        ///   This method will be invoked AFTER IUpdatable.DeleteResource(entity) is called.  An implementer of this method must be able to
        ///   delete a stream even if the associated entity (passed as a parameter to this method) has already been removed from the
        ///   underlying data source.
        /// 
        /// The host is passed as an argument as a means for this method to read the HTTP request headers provided with the delete request.
        /// 
        /// An implementer of this method MUST NOT set the following HTTP response headers (on the host instance passed as an argument) as
        /// they are set by the data service runtime:
        ///   Content-Type
        ///   ETag
        ///   
        /// An implementer of this method may set HTTP response headers (other than those forbidden above) on the host instance passed as an argument.
        /// 
        /// An implementer of this method should only set the properties on the host instance which it requires to be set for a successful response.
        /// Altering other properties on the host instance may corrupt the response from the data service.
        /// </summary>
        /// <param name="entity">The stream deleted should be the default stream associated with this entity.</param>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        /// <remarks>An implementer of this method of this method should thrown the following exceptions under the specified conditions:</remarks>
        /// <exception cref="ArgumentNullException">entity or host parameters are null.</exception>
        /// <exception cref="ArgumentException">entity does not represent an Entity Type which has a default stream attached to it.</exception>
        /// <exception cref="DataServiceException">if the stream associated with the entity specified could not be deleted.</exception>
        void DeleteStream(object entity, DataServiceOperationContext operationContext);

        /// <summary>
        /// This method is invoked by the data services framework to obtain the IANA content type (aka media type) of the stream associated
        /// with the specified entity.  This metadata is needed when constructing the payload for the Media Link Entry associated with the
        /// stream (aka Media Resource) or setting the Content-Type HTTP response header.
        /// 
        /// The string should be returned in a format which is directly usable as the value of an HTTP Content-Type response header.
        /// For example, if the stream represented a PNG image the return value would be "image/png"
        /// 
        /// This method MUST always return a valid content type string.  If null or string.empty is returned the data service framework will
        /// consider that an error case and return a 500 (Internal Server Error) to the client.
        /// </summary>
        /// <param name="entity">The entity associated with the stream for which the content type is to be obtained</param>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        /// <returns>Valid Content-Type string for the stream associated with the entity</returns>
        /// <remarks>An implementer of this method of this method should thrown the following exceptions under the specified conditions:</remarks>
        /// <exception cref="ArgumentNullException">Host parameter is null.</exception>
        string GetStreamContentType(object entity, DataServiceOperationContext operationContext);

        /// <summary>
        /// This method is invoked by the data services framework to obtain the URI clients should use when making retrieve (ie. GET)
        /// requests to the stream(ie. Media Resource).   This metadata is needed when constructing the payload for the Media Link Entry
        /// associated with the stream (aka Media Resource).
        /// 
        /// NOTE: This method was added such that a Media Link Entry’s representation could state that a stream (Media Resource) is to
        /// be edited using one URI and read using another.   This is supported such that a data service could leverage a Content
        /// Distribute Network is required for its stream content.
        /// 
        /// The URI returned maps to the value of the src attribute on the atom:content element of a payload representing the Media
        /// Link Entry associated with the stream described by this DataServiceStreamDescriptor instance.  If the JSON format is
        /// used (as noted in section 3.2.3) this URI represents the value of the src_media name/value pair.
        /// 
        /// The returned URI MUST be an absolute URI and represents the location where a consumer (reader) of the stream should send
        /// requests to in order to obtain the contents of the stream.  
        /// 
        /// If URI returned is null or empty, then the data service runtime will automatically generate the URI representing the location
        /// where the stream can be read from.  The URI generated by the runtime will equal the canonical URI for the associated Media Link
        /// Entry followed by a “/$value” path segment. 
        /// </summary>
        /// <param name="entity">The entity associated with the stream for which a “read stream” is to be obtained</param>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        /// <returns>The URI clients should use when making retrieve (ie. GET) requests to the stream(ie. Media Resource).</returns>
        /// <remarks>An implementer of this method of this method should thrown the following exceptions under the specified conditions:</remarks>
        /// <exception cref="ArgumentNullException">Host parameter is null.</exception>
        Uri GetReadStreamUri(object entity, DataServiceOperationContext operationContext);

        /// <summary>
        /// This method is invoked by the data services framework to obtain the ETag of the stream associated with the entity specified.
        /// This metadata is needed when constructing the payload for the Media Link Entry associated with the stream (aka Media Resource)
        /// as well as to be used as the value of the ETag HTTP response header.
        /// 
        /// This method enables a stream (Media Resource) to have an ETag which is different from that of its associated Media Link Entry.
        /// The returned string MUST be formatted such that it is directly usable as the value of an HTTP ETag response header.
        /// If null is returned the data service framework will assume that no ETag is associated with the stream
        /// </summary>
        /// <param name="entity">The entity associated with the stream for which an etag is to be obtained</param>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        /// <returns>ETag of the stream associated with the entity specified</returns>
        /// <remarks>An implementer of this method of this method should thrown the following exceptions under the specified conditions:</remarks>
        /// <exception cref="ArgumentNullException">Host parameter is null.</exception>
        string GetStreamETag(object entity, DataServiceOperationContext operationContext);

        /// <summary>
        /// This method is invoked by the data services framework when a request is received to insert into an Entity Set with an associated
        /// Entity Type hierarchy that has > 1 Entity Type and >= 1 Entity Type which is tagged as an MLE (ie. includes a stream).
        /// 
        /// An implementer of this method should inspect the request headers provided by the ‘host’ argument and return the namespace
        /// qualified type name which represents the type the Astoria framework should instantiate to create the MLE associated with the
        /// BLOB/MR being inserted.  The string representing the MLE type name returned from this method will subsequently be passed to
        /// IUpdatable.CreateResource to create the MLE (of the specified type).
        /// </summary>
        /// <param name="entitySetName">Fully qualified name entity set name.</param>
        /// <param name="operationContext">A reference to the context for the current operation.</param>
        /// <returns>
        /// Namespace qualified type name which represents the type the Astoria framework should instantiate to create the MLE associated
        /// with the BLOB/MR being inserted.
        /// </returns>
        /// <remarks>An implementer of this method of this method should thrown the following exceptions under the specified conditions:</remarks>
        /// <exception cref="ArgumentNullException">Host parameter is null.</exception>
        /// <exception cref="DataServiceException">An entity type name cannot be resolved given the host parameter provided.</exception>
        string ResolveType(string entitySetName, DataServiceOperationContext operationContext);
    }
}
