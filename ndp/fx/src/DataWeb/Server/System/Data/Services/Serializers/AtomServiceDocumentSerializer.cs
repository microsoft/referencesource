//---------------------------------------------------------------------
// <copyright file="AtomServiceDocumentSerializer.cs" company="Microsoft">
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

    /// <summary>
    /// Provides support for serializing service models as
    /// a Service Document.
    /// </summary>
    /// <remarks>
    /// For more information, see http://tools.ietf.org/html/rfc5023#section-8.
    /// </remarks>
    [DebuggerDisplay("AtomServiceDocumentSerializer={baseUri}")]
    internal sealed class AtomServiceDocumentSerializer : XmlDocumentSerializer
    {
        /// <summary>XML prefix for the Atom Publishing Protocol namespace.</summary>
        private const string AppNamespacePrefix = "app";

        /// <summary>XML prefix for the Atom namespace.</summary>
        private const string AtomNamespacePrefix = "atom";

        /// <summary>
        /// Initializes a new AtomServiceDocumentSerializer, ready to write
        /// out the Service Document for a data provider.
        /// </summary>
        /// <param name="output">Stream to which output should be sent.</param>
        /// <param name="baseUri">Base URI from which resources should be resolved.</param>
        /// <param name="provider">Data provider from which metadata should be gathered.</param>
        /// <param name="encoding">Text encoding for the response.</param>
        internal AtomServiceDocumentSerializer(
            Stream output, 
            Uri baseUri, 
            DataServiceProviderWrapper provider, 
            Encoding encoding)
            : base(output, baseUri, provider, encoding)
        {
        }

        /// <summary>Writes the Service Document to the output stream.</summary>
        /// <param name="service">Data service instance.</param>
        internal override void WriteRequest(IDataService service)
        {
            try
            {
                this.Writer.WriteStartElement(XmlConstants.AtomPublishingServiceElementName, XmlConstants.AppNamespace);
                this.IncludeCommonNamespaces();
                this.Writer.WriteStartElement("", XmlConstants.AtomPublishingWorkspaceElementName, XmlConstants.AppNamespace);

                this.Writer.WriteStartElement(XmlConstants.AtomTitleElementName, XmlConstants.AtomNamespace);
                this.Writer.WriteString(XmlConstants.AtomPublishingWorkspaceDefaultValue);
                this.Writer.WriteEndElement();

                foreach (ResourceSetWrapper container in this.Provider.ResourceSets)
                {
                    this.Writer.WriteStartElement("", XmlConstants.AtomPublishingCollectionElementName, XmlConstants.AppNamespace);
                    this.Writer.WriteAttributeString(XmlConstants.AtomHRefAttributeName, container.Name);

                    this.Writer.WriteStartElement(XmlConstants.AtomTitleElementName, XmlConstants.AtomNamespace);
                    this.Writer.WriteString(container.Name);
                    this.Writer.WriteEndElement();  // Close 'title' element.

                    this.Writer.WriteEndElement();  // Close 'collection' element.
                }

                this.Writer.WriteEndElement();  // Close 'workspace' element.
                this.Writer.WriteEndElement();  // Close 'service' element.
            }
            finally
            {
                this.Writer.Close();
            }
        }

        /// <summary>Ensures that common namespaces are included in the topmost tag.</summary>
        /// <remarks>
        /// This method should be called by any method that may write a 
        /// topmost element tag.
        /// </remarks>
        private void IncludeCommonNamespaces()
        {
            Debug.Assert(
                this.Writer.WriteState == WriteState.Element, 
                "this.writer.WriteState == WriteState.Element - otherwise, not called at beginning - " + this.Writer.WriteState);
            this.Writer.WriteAttributeString(XmlConstants.XmlNamespacePrefix, XmlConstants.XmlBaseAttributeName, null, this.BaseUri.AbsoluteUri);
            this.Writer.WriteAttributeString(XmlConstants.XmlnsNamespacePrefix, AtomNamespacePrefix, null, XmlConstants.AtomNamespace);
            this.Writer.WriteAttributeString(XmlConstants.XmlnsNamespacePrefix, AppNamespacePrefix, null, XmlConstants.AppNamespace);
        }
    }
}
