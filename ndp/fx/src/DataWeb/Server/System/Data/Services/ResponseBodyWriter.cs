//---------------------------------------------------------------------
// <copyright file="ResponseBodyWriter.cs" company="Microsoft">
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
    using System;
    using System.Collections;
    using System.Data.Services.Providers;
    using System.Data.Services.Serializers;
    using System.Diagnostics;
    using System.IO;
    using System.Text;

    /// <summary>
    /// Use this class to encapsulate writing the body of the outgoing response 
    /// for a data request.
    /// </summary>
    internal class ResponseBodyWriter
    {
        /// <summary>Encoding, if available.</summary>
        private readonly Encoding encoding;

        /// <summary>Whether <see cref="queryResults"/> has already moved.</summary>
        private readonly bool hasMoved;

        /// <summary>Host for the request being processed.</summary>
        private readonly IDataService service;

        /// <summary>Enumerator for results.</summary>
        private readonly IEnumerator queryResults;

        /// <summary>Description of request made to the system.</summary>
        private readonly RequestDescription requestDescription;

        /// <summary>Content format for response.</summary>
        private readonly ContentFormat responseFormat;

        /// <summary>If the target is a Media Resource, this holds the read stream for the Media Resource.</summary>
        private Stream mediaResourceStream;

        /// <summary>Initializes a new <see cref="ResponseBodyWriter"/> that can write the body of a response.</summary>
        /// <param name="encoding">Encoding, if available.</param>
        /// <param name="hasMoved">Whether <paramref name="queryResults"/> has already moved.</param>
        /// <param name="service">Service for the request being processed.</param>
        /// <param name="queryResults">Enumerator for results.</param>
        /// <param name="requestDescription">Description of request made to the system.</param>
        /// <param name="responseFormat">Content format for response.</param>
        internal ResponseBodyWriter(
            Encoding encoding,
            bool hasMoved,
            IDataService service,
            IEnumerator queryResults,
            RequestDescription requestDescription,
            ContentFormat responseFormat)
        {
            Debug.Assert(responseFormat != ContentFormat.Unknown, "responseFormat != ContentFormat.Unknown");
            this.encoding = encoding;
            this.hasMoved = hasMoved;
            this.service = service;
            this.queryResults = queryResults;
            this.requestDescription = requestDescription;
            this.responseFormat = responseFormat;

            if (this.requestDescription.TargetKind == RequestTargetKind.MediaResource)
            {
                // Note that GetReadStream will set the ResponseETag before it returns
                this.mediaResourceStream = service.StreamProvider.GetReadStream(this.queryResults.Current, this.service.OperationContext);
            }
        }

        /// <summary>Gets the absolute URI to the service.</summary>
        internal Uri AbsoluteServiceUri
        {
            get { return this.service.OperationContext.AbsoluteServiceUri; }
        }

        /// <summary>Gets the <see cref="IDataServiceHost"/> for this response.</summary>
        internal DataServiceHostWrapper Host
        {
            get { return this.service.OperationContext.Host; }
        }

        /// <summary>Gets the <see cref="DataServiceProviderWrapper"/> for this response.</summary>
        internal DataServiceProviderWrapper Provider
        {
            get { return this.service.Provider; }
        }

        /// <summary>Writes the request body to the specified <see cref="Stream"/>.</summary>
        /// <param name="stream">Stream to write to.</param>
        internal void Write(Stream stream)
        {
            Debug.Assert(stream != null, "stream != null");
            IExceptionWriter exceptionWriter = null;

            try
            {
                switch (this.responseFormat)
                {
                    case ContentFormat.Binary:
                        Debug.Assert(
                            this.requestDescription.TargetKind == RequestTargetKind.OpenPropertyValue ||
                            this.requestDescription.TargetKind == RequestTargetKind.PrimitiveValue ||
                            this.requestDescription.TargetKind == RequestTargetKind.MediaResource,
                            this.requestDescription.TargetKind + " is PrimitiveValue or OpenPropertyValue or StreamPropertyValue");

                        BinarySerializer binarySerializer = new BinarySerializer(stream);
                        exceptionWriter = binarySerializer;
                        if (this.requestDescription.TargetKind == RequestTargetKind.MediaResource)
                        {
                            Debug.Assert(this.mediaResourceStream != null, "this.mediaResourceStream != null");

                            binarySerializer.WriteRequest(this.mediaResourceStream, this.service.StreamProvider.StreamBufferSize);
                        }
                        else
                        {
                            binarySerializer.WriteRequest(this.queryResults.Current);
                        }

                        break;

                    case ContentFormat.Text:
                        Debug.Assert(
                            this.requestDescription.TargetKind == RequestTargetKind.OpenPropertyValue ||
                            this.requestDescription.TargetKind == RequestTargetKind.PrimitiveValue,
                            this.requestDescription.TargetKind + " is PrimitiveValue or OpenPropertyValue");

                        TextSerializer textSerializer = new TextSerializer(stream, this.encoding);
                        exceptionWriter = textSerializer;
                        textSerializer.WriteRequest(this.queryResults.Current);
                        break;

                    case ContentFormat.Atom:
                    case ContentFormat.Json:
                    case ContentFormat.PlainXml:
                        Debug.Assert(this.requestDescription.TargetKind != RequestTargetKind.PrimitiveValue, "this.requestDescription.TargetKind != RequestTargetKind.PrimitiveValue");
                        Debug.Assert(this.requestDescription.TargetKind != RequestTargetKind.OpenPropertyValue, "this.requestDescription.TargetKind != RequestTargetKind.OpenPropertyValue");
                        Debug.Assert(this.requestDescription.TargetKind != RequestTargetKind.Metadata, "this.requestDescription.TargetKind != RequestTargetKind.Metadata");

                        if (this.requestDescription.TargetKind == RequestTargetKind.ServiceDirectory)
                        {
                            if (this.responseFormat == ContentFormat.Json)
                            {
                                JsonServiceDocumentSerializer serviceSerializer = new JsonServiceDocumentSerializer(stream, this.Provider, this.encoding);
                                exceptionWriter = serviceSerializer;
                                serviceSerializer.WriteRequest();
                                break;
                            }
                            else
                            {
                                AtomServiceDocumentSerializer serviceSerializer = new AtomServiceDocumentSerializer(stream, this.AbsoluteServiceUri, this.Provider, this.encoding);
                                exceptionWriter = serviceSerializer;
                                serviceSerializer.WriteRequest(this.service);
                            }
                        }
                        else
                        {
                            Serializer serializer;
                            if (ContentFormat.Json == this.responseFormat)
                            {
                                serializer = new JsonSerializer(this.requestDescription, stream, this.AbsoluteServiceUri, this.service, this.encoding, this.Host.ResponseETag);
                            }
                            else if (ContentFormat.PlainXml == this.responseFormat)
                            {
                                serializer = new PlainXmlSerializer(this.requestDescription, this.AbsoluteServiceUri, this.service, stream, this.encoding);
                            }
                            else
                            {
                                Debug.Assert(
                                    this.requestDescription.TargetKind == RequestTargetKind.OpenProperty ||
                                    this.requestDescription.TargetKind == RequestTargetKind.Resource,
                                    "TargetKind " + this.requestDescription.TargetKind + " == Resource || OpenProperty -- POX should have handled it otherwise.");
                                serializer = new SyndicationSerializer(
                                    this.requestDescription,
                                    this.AbsoluteServiceUri,
                                    this.service,
                                    stream,
                                    this.encoding,
                                    this.Host.ResponseETag,
                                    new Atom10FormatterFactory());
                            }

                            exceptionWriter = serializer;
                            Debug.Assert(exceptionWriter != null, "this.exceptionWriter != null");
                            serializer.WriteRequest(this.queryResults, this.hasMoved);
                        }

                        break;

                    default:
                        Debug.Assert(
                            this.responseFormat == ContentFormat.MetadataDocument,
                            "responseFormat(" + this.responseFormat + ") == ContentFormat.MetadataDocument -- otherwise exception should have been thrown before");
                        Debug.Assert(this.requestDescription.TargetKind == RequestTargetKind.Metadata, "this.requestDescription.TargetKind == RequestTargetKind.Metadata");
                        MetadataSerializer metadataSerializer = new MetadataSerializer(stream, this.AbsoluteServiceUri, this.Provider, this.encoding);
                        exceptionWriter = metadataSerializer;
                        metadataSerializer.WriteRequest(this.service);
                        break;
                }
            }
            catch (Exception exception)
            {
                if (!WebUtil.IsCatchableExceptionType(exception))
                {
                    throw;
                }

                // Only JSON and XML are supported.
                string contentType = (this.responseFormat == ContentFormat.Json) ? XmlConstants.MimeApplicationJson : XmlConstants.MimeApplicationXml;
                ErrorHandler.HandleDuringWritingException(exception, this.service, contentType, exceptionWriter);
            }
            finally
            {
                WebUtil.Dispose(this.queryResults);
                WebUtil.Dispose(this.mediaResourceStream);
            }
        }
    }
}
