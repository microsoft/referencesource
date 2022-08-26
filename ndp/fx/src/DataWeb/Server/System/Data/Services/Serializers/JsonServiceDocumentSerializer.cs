//---------------------------------------------------------------------
// <copyright file="JsonServiceDocumentSerializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a serializer for the Json Service Document format.
// </summary>
//
// @owner Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.IO;
    using System.Text;

    /// <summary>
    /// Provides support for serializing service models as
    /// a Service Document.
    /// </summary>
    [DebuggerDisplay("JsonServiceDocumentSerializer={baseUri}")]
    internal sealed class JsonServiceDocumentSerializer : IExceptionWriter
    {
        /// <summary>JsonWriter to write out strings in Json format.</summary>
        private readonly JsonWriter writer;

        /// <summary>Data provider from which metadata should be gathered.</summary>
        private readonly DataServiceProviderWrapper provider;

        /// <summary>Element name for the json service document.</summary>
        private const string JsonEntitySetsElementName = "EntitySets";

        /// <summary>
        /// Initializes a new JsonServiceDocumentSerializer, ready to write
        /// out the Service Document for a data provider.
        /// </summary>
        /// <param name="output">Stream to which output should be sent.</param>
        /// <param name="provider">Data provider from which metadata should be gathered.</param>
        /// <param name="encoding">Text encoding for the response.</param>
        internal JsonServiceDocumentSerializer(
            Stream output,
            DataServiceProviderWrapper provider,
            Encoding encoding)
        {
            Debug.Assert(output != null, "output != null");
            Debug.Assert(provider != null, "provider != null");

            this.writer = new JsonWriter(new StreamWriter(output, encoding));
            this.provider = provider;
        }

        /// <summary>Serializes exception information.</summary>
        /// <param name="args">Description of exception to serialize.</param>
        public void WriteException(HandleExceptionArgs args)
        {
            ErrorHandler.SerializeJsonError(args, this.writer);
        }

        /// <summary>Writes the Service Document to the output stream.</summary>
        internal void WriteRequest()
        {
            try
            {
                this.writer.StartObjectScope(); // {
                this.writer.WriteDataWrapper(); // "d" :

                this.writer.StartObjectScope();
                this.writer.WriteName(JsonEntitySetsElementName);
                this.writer.StartArrayScope();
                foreach (ResourceSetWrapper container in this.provider.ResourceSets)
                {
                    this.writer.WriteValue(container.Name);
                }

                this.writer.EndScope(); // end the array scope
                this.writer.EndScope(); // end the object scope
                this.writer.EndScope(); // end "d" scope
            }
            finally
            {
                this.writer.Flush();
            }
        }
    }
}
