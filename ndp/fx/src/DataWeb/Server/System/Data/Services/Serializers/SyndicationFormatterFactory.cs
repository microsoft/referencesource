//---------------------------------------------------------------------
// <copyright file="SyndicationFormatterFactory.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a class that creates formatters and XML readers and
//      writers for a given content type.
// </summary>
//
// @owner mruiz
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    using System.IO;
    using System.ServiceModel.Syndication;
    using System.Text;
    using System.Xml;

    /// <summary>
    /// An abstract class used as a base class for other classes that can create formatters, readers and writers
    /// for different content types (for example <see cref="Atom10FormatterFactory"/>).
    /// </summary>
    internal abstract class SyndicationFormatterFactory
    {
        /// <summary>Creates a new instance of an <see cref="SyndicationFeedFormatter"/> class.</summary>
        /// <returns>A new instance of an <see cref="SyndicationFeedFormatter"/> class.</returns>
        internal abstract SyndicationFeedFormatter CreateSyndicationFeedFormatter();

        /// <summary>
        /// Creates a new instance of an <see cref="SyndicationFeedFormatter"/> class with the specified
        /// <see cref="SyndicationFeed"/> instance.
        /// </summary>
        /// <param name="feedToWrite">The <see cref="SyndicationFeed"/> to serialize.</param>
        /// <returns>
        /// A new instance of the <see cref="SyndicationFeedFormatter"/> class with the specified
        /// <see cref="SyndicationFeed"/> instance.
        /// </returns>
        internal abstract SyndicationFeedFormatter CreateSyndicationFeedFormatter(SyndicationFeed feedToWrite);

        /// <summary>Creates a new instance of an <see cref="SyndicationItemFormatter"/> class.</summary>
        /// <returns>A new instance of an <see cref="SyndicationItemFormatter"/> class.</returns>
        internal abstract SyndicationItemFormatter CreateSyndicationItemFormatter();

        /// <summary>
        /// Creates a new instance of an <see cref="SyndicationItemFormatter"/> class with the specified
        /// <see cref="SyndicationItem"/> instance.
        /// </summary>
        /// <param name="itemToWrite">The <see cref="SyndicationItem"/> to serialize.</param>
        /// <returns>
        /// A new instance of the <see cref="SyndicationItemFormatter"/> class with the specified
        /// <see cref="SyndicationItem"/> instance.
        /// </returns>
        internal abstract SyndicationItemFormatter CreateSyndicationItemFormatter(SyndicationItem itemToWrite);

        /// <summary>
        /// Creates an <see cref="XmlReader"/> over the specified <paramref name="stream"/> with the given
        /// <paramref name="encoding"/>, to be used with an appropriate formatter.
        /// </summary>
        /// <param name="stream">Stream over which to read (the reader should close it when it's done with it).</param>
        /// <param name="encoding">Encoding of the stream.</param>
        /// <returns>A new <see cref="XmlReader"/> instance.</returns>
        internal abstract XmlReader CreateReader(Stream stream, Encoding encoding);

        /// <summary>
        /// Creates an <see cref="XmlWriter"/> into the specified <paramref name="stream"/> with the given
        /// <paramref name="encoding"/>, to be used with an appropriate formatter.
        /// </summary>
        /// <param name="stream">Stream over which to write (the writer should close it when it's done with it).</param>
        /// <param name="encoding">Encoding of the stream.</param>
        /// <returns>A new <see cref="XmlWriter"/> instance.</returns>
        internal abstract XmlWriter CreateWriter(Stream stream, Encoding encoding);
    }
}
