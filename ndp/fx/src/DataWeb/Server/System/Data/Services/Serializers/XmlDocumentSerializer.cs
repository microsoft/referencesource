//---------------------------------------------------------------------
// <copyright file="XmlDocumentSerializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a serializer for the Atom Service Document format.
// </summary>
//
// @owner Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    using System;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml;

    /// <summary>Provides support for serializing generic XML documents.</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1001:TypesThatOwnDisposableFieldsShouldBeDisposable", Justification = "Pending review.")]
    [DebuggerDisplay("XmlDocumentSerializer={baseUri}")]
    internal abstract class XmlDocumentSerializer : IExceptionWriter
    {
        /// <summary>Base URI from which resources should be resolved.</summary>
        private readonly Uri baseUri;

        /// <summary>Data provider from which metadata should be gathered.</summary>
        private readonly DataServiceProviderWrapper provider;

        /// <summary>Writer to which output is sent.</summary>
        private readonly XmlWriter writer;

        /// <summary>
        /// Initializes a new XmlDocumentSerializer, ready to write
        /// out an XML document
        /// </summary>
        /// <param name="output">Stream to which output should be sent.</param>
        /// <param name="baseUri">Base URI from which resources should be resolved.</param>
        /// <param name="provider">Data provider from which metadata should be gathered.</param>
        /// <param name="encoding">Text encoding for the response.</param>
        internal XmlDocumentSerializer(
            Stream output, 
            Uri baseUri, 
            DataServiceProviderWrapper provider, 
            Encoding encoding)
        {
            Debug.Assert(output != null, "output != null");
            Debug.Assert(provider != null, "provider != null");
            Debug.Assert(baseUri != null, "baseUri != null");
            Debug.Assert(encoding != null, "encoding != null");
            Debug.Assert(baseUri.IsAbsoluteUri, "baseUri.IsAbsoluteUri(" + baseUri + ")");
            Debug.Assert(baseUri.AbsoluteUri[baseUri.AbsoluteUri.Length - 1] == '/', "baseUri(" + baseUri.AbsoluteUri + ") ends with '/'");

            this.writer = XmlUtil.CreateXmlWriterAndWriteProcessingInstruction(output, encoding);
            this.provider = provider;
            this.baseUri = baseUri;
        }

        /// <summary>Base URI from which resources should be resolved.</summary>
        protected Uri BaseUri
        {
            get { return this.baseUri; }
        }

        /// <summary>Data provider from which metadata should be gathered.</summary>
        protected DataServiceProviderWrapper Provider
        {
            get { return this.provider; }
        }

        /// <summary>Writer to which output is sent.</summary>
        protected XmlWriter Writer
        {
            get { return this.writer; }
        }

        /// <summary>Serializes exception information.</summary>
        /// <param name="args">Description of exception to serialize.</param>
        public void WriteException(HandleExceptionArgs args)
        {
            ErrorHandler.SerializeXmlError(args, this.writer);
        }

        /// <summary>Writes the document for this request..</summary>
        /// <param name="service">Data service instance.</param>
        internal abstract void WriteRequest(IDataService service);
    }
}
