//---------------------------------------------------------------------
// <copyright file="Atom10FormatterFactory.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a formatter factory for ATOM 1.0.
// </summary>
//
// @owner Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    using System.Diagnostics;
    using System.IO;
    using System.ServiceModel.Syndication;
    using System.Text;
    using System.Xml;

    /// <summary>Provides support for serializing responses in ATOM 1.0 format.</summary>
    /// <remarks>
    /// For more information, see http://tools.ietf.org/html/rfc4287.
    /// </remarks>
    internal sealed class Atom10FormatterFactory : SyndicationFormatterFactory
    {
        /// <summary>Creates a new instance of the <see cref="Atom10FeedFormatter"/> class.</summary>
        /// <returns>A new instance of the <see cref="Atom10FeedFormatter"/> class.</returns>
        internal override SyndicationFeedFormatter CreateSyndicationFeedFormatter()
        {
            return new Atom10FeedFormatter();
        }

        /// <summary>
        /// Creates a new instance of the <see cref="Atom10FeedFormatter"/> class with the specified
        /// <see cref="SyndicationFeed"/> instance.
        /// </summary>
        /// <param name="feedToWrite">The <see cref="SyndicationFeed"/> to serialize.</param>
        /// <returns>
        /// A new instance of the <see cref="Atom10FeedFormatter"/> class with the specified
        /// <see cref="SyndicationFeed"/> instance.
        /// </returns>
        internal override SyndicationFeedFormatter CreateSyndicationFeedFormatter(SyndicationFeed feedToWrite)
        {
            Debug.Assert(feedToWrite != null, "feedToWrite != null");
            return new Atom10FeedFormatter(feedToWrite);
        }

        /// <summary>Creates a new instance of the <see cref="Atom10ItemFormatter"/> class.</summary>
        /// <returns>A new instance of the <see cref="Atom10ItemFormatter"/> class.</returns>
        internal override SyndicationItemFormatter CreateSyndicationItemFormatter()
        {
            return new Atom10ItemFormatter();
        }

        /// <summary>
        /// Creates a new instance of the <see cref="Atom10ItemFormatter"/> class with the specified
        /// <see cred="SyndicationItem" /> instance.
        /// </summary>
        /// <param name="itemToWrite">The <see cref="SyndicationItem"/> to serialize.</param>
        /// <returns>A new instance of the <see cref="Atom10ItemFormatter"/> class.</returns>
        internal override SyndicationItemFormatter CreateSyndicationItemFormatter(SyndicationItem itemToWrite)
        {
            Debug.Assert(itemToWrite != null, "itemToWrite != null");
            string value;
            if (itemToWrite.AttributeExtensions.TryGetValue(SyndicationSerializer.QualifiedNullAttribute, out value) &&
                value == XmlConstants.XmlTrueLiteral)
            {
                return null;
            }

            return new Atom10ItemFormatter(itemToWrite);
        }

        /// <summary>
        /// Creates an <see cref="XmlReader"/> over the specified <paramref name="stream"/> with the given
        /// <paramref name="encoding"/>, to be used with an appropriate formatter.
        /// </summary>
        /// <param name="stream">Stream over which to read (the reader should close it when it's done with it).</param>
        /// <param name="encoding">Encoding of the stream, possibly null.</param>
        /// <returns>A new <see cref="XmlReader"/> instance.</returns>
        internal override XmlReader CreateReader(Stream stream, Encoding encoding)
        {
            Debug.Assert(stream != null, "stream != null");
            return XmlUtil.CreateXmlReader(stream, encoding);
        }

        /// <summary>
        /// Creates an <see cref="XmlWriter"/> into the specified <paramref name="stream"/> with the given
        /// <paramref name="encoding"/>, to be used with an appropriate formatter.
        /// </summary>
        /// <param name="stream">Stream over which to write (the writer should close it when it's done with it).</param>
        /// <param name="encoding">Encoding of the stream.</param>
        /// <returns>A new <see cref="XmlWriter"/> instance.</returns>
        internal override XmlWriter CreateWriter(Stream stream, Encoding encoding)
        {
            Debug.Assert(stream != null, "stream != null");
            Debug.Assert(encoding != null, "encoding != null");
            return XmlUtil.CreateXmlWriterAndWriteProcessingInstruction(stream, encoding);
        }
    }
}
