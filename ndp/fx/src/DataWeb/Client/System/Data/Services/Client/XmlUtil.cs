//---------------------------------------------------------------------
// <copyright file="XmlUtil.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// static xml utility functions
// </summary>
//---------------------------------------------------------------------

#if ASTORIA_CLIENT
namespace System.Data.Services.Client
#else
namespace System.Data.Services
#endif
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Xml;

    /// <summary>
    /// static uri utility functions
    /// </summary>
    internal static partial class UriUtil
    {
#if !ASTORIA_CLIENT
        /// <summary>Creates a URI suitable for host-agnostic comparison purposes.</summary>
        /// <param name="uri">URI to compare.</param>
        /// <returns>URI suitable for comparison.</returns>
        private static Uri CreateBaseComparableUri(Uri uri)
        {
            Debug.Assert(uri != null, "uri != null");

            uri = new Uri(CommonUtil.UriToString(uri).ToUpper(CultureInfo.InvariantCulture), UriKind.RelativeOrAbsolute);

            UriBuilder builder = new UriBuilder(uri);
            builder.Host = "h";
            builder.Port = 80;
            builder.Scheme = "http";
            return builder.Uri;
        }
#endif

        /// <summary>Parse the atom link relation attribute value</summary>
        /// <param name="value">atom link relation attribute value</param>
        /// <returns>the assocation name or null</returns>
        internal static string GetNameFromAtomLinkRelationAttribute(string value)
        {
            string name = null;
            if (!String.IsNullOrEmpty(value))
            {
                Uri uri = null;
                try
                {
                    uri = new Uri(value, UriKind.RelativeOrAbsolute);
                }
                catch (UriFormatException)
                {   // ignore the exception, obviously we don't have our expected relation value
                }

                if ((null != uri) && uri.IsAbsoluteUri)
                {
                    string unescaped = uri.GetComponents(UriComponents.AbsoluteUri, UriFormat.SafeUnescaped);
                    if (unescaped.StartsWith(XmlConstants.DataWebRelatedNamespace, StringComparison.Ordinal))
                    {
                        name = unescaped.Substring(XmlConstants.DataWebRelatedNamespace.Length);
                    }
                }
            }

            return name;
        }
#if !ASTORIA_CLIENT
        /// <summary>is the serviceRoot the base of the request uri</summary>
        /// <param name="baseUriWithSlash">baseUriWithSlash</param>
        /// <param name="requestUri">requestUri</param>
        /// <returns>true if the serviceRoot is the base of the request uri</returns>
        internal static bool IsBaseOf(Uri baseUriWithSlash, Uri requestUri)
        {
            return baseUriWithSlash.IsBaseOf(requestUri);
        }
        /// <summary>
        /// Determines whether the <paramref name="current"/> Uri instance is a 
        /// base of the specified Uri instance. 
        /// </summary>
        /// <param name="current">Candidate base URI.</param>
        /// <param name="uri">The specified Uri instance to test.</param>
        /// <returns>true if the current Uri instance is a base of uri; otherwise, false.</returns>
        internal static bool UriInvariantInsensitiveIsBaseOf(Uri current, Uri uri)
        {
            Debug.Assert(current != null, "current != null");
            Debug.Assert(uri != null, "uri != null");

            Uri upperCurrent = CreateBaseComparableUri(current);
            Uri upperUri = CreateBaseComparableUri(uri);

            return UriUtil.IsBaseOf(upperCurrent, upperUri);
        }
#endif
    }

    /// <summary>
    /// static xml utility function
    /// </summary>
    internal static partial class XmlUtil
    {
        /// <summary>
        /// An initial nametable so that string comparisons during
        /// deserialization become reference comparisions
        /// </summary>
        /// <returns>nametable with element names used in application/atom+xml payload</returns>
        private static NameTable CreateAtomNameTable()
        {
            NameTable table = new NameTable();
            table.Add(XmlConstants.AtomNamespace);
            table.Add(XmlConstants.DataWebNamespace);
            table.Add(XmlConstants.DataWebMetadataNamespace);

            table.Add(XmlConstants.AtomContentElementName);
#if ASTORIA_CLIENT
            table.Add(XmlConstants.AtomContentSrcAttributeName);
#endif
            table.Add(XmlConstants.AtomEntryElementName);
            table.Add(XmlConstants.AtomETagAttributeName);
            table.Add(XmlConstants.AtomFeedElementName);
#if ASTORIA_CLIENT
            table.Add(XmlConstants.AtomIdElementName);
#endif
            table.Add(XmlConstants.AtomInlineElementName);
#if ASTORIA_CLIENT
            table.Add(XmlConstants.AtomLinkElementName);
            table.Add(XmlConstants.AtomLinkRelationAttributeName);
#endif
            table.Add(XmlConstants.AtomNullAttributeName);
            table.Add(XmlConstants.AtomPropertiesElementName);
            table.Add(XmlConstants.AtomTitleElementName);
            table.Add(XmlConstants.AtomTypeAttributeName);

            table.Add(XmlConstants.XmlErrorCodeElementName);
            table.Add(XmlConstants.XmlErrorElementName);
            table.Add(XmlConstants.XmlErrorInnerElementName);
            table.Add(XmlConstants.XmlErrorMessageElementName);
            table.Add(XmlConstants.XmlErrorTypeElementName);
            return table;
        }

        /// <summary>
        /// Creates a new XmlReader instance using the specified stream reader
        /// </summary>
        /// <param name="stream">The stream reader from which you want to read</param>
        /// <param name="encoding">The encoding of the stream</param>
        /// <returns>XmlReader with the appropriate xml settings</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "Caller is responsible for disposing the XmlReader")]
        internal static XmlReader CreateXmlReader(Stream stream, Encoding encoding)
        {
            Debug.Assert(null != stream, "null stream");

            XmlReaderSettings settings = new XmlReaderSettings();
            settings.CheckCharacters = false;
#if ASTORIA_CLIENT
            // we should close the underlying stream only for the client.
            // In server, we should never close the underlying stream - neither after reading nor after writing,
            // since the underlying host owns the stream.
            settings.CloseInput = true;
#endif
            settings.IgnoreWhitespace = true;
            settings.NameTable = XmlUtil.CreateAtomNameTable();

            if (null == encoding)
            {   // auto-detect the encoding
                return XmlReader.Create(stream, settings);
            }

            return XmlReader.Create(new StreamReader(stream, encoding), settings);
        }

        /// <summary>
        /// Creates a new XmlWriterSettings instance using the encoding.
        /// </summary>
        /// <param name="encoding"> Encoding that you want to specify in the reader settings as well as the processing instruction </param>
        internal static XmlWriterSettings CreateXmlWriterSettings(Encoding encoding)
        {
            Debug.Assert(null != encoding, "null != encoding");

            // No need to close the underlying stream here for client,
            // since it always MemoryStream for writing i.e. it caches the response before processing.
            XmlWriterSettings settings = new XmlWriterSettings();
            settings.CheckCharacters = false;
            settings.ConformanceLevel = ConformanceLevel.Fragment;
            settings.Encoding = encoding;
            settings.Indent = true;
            settings.NewLineHandling = NewLineHandling.Entitize;

            Debug.Assert(!settings.CloseOutput, "!settings.CloseOutput -- otherwise default changed?");

            return settings;
        }

        /// <summary>
        /// Creates a new XmlWriter instance using the specified stream and writers the processing instruction
        /// with the given encoding value
        /// </summary>
        /// <param name="stream"> The stream to which you want to write</param>
        /// <param name="encoding"> Encoding that you want to specify in the reader settings as well as the processing instruction </param>
        /// <returns>XmlWriter with the appropriate xml settings and processing instruction</returns>
        internal static XmlWriter CreateXmlWriterAndWriteProcessingInstruction(Stream stream, Encoding encoding)
        {
            Debug.Assert(null != stream, "null != stream");
            Debug.Assert(null != encoding, "null != encoding");

            XmlWriterSettings settings = CreateXmlWriterSettings(encoding);
            XmlWriter writer = XmlWriter.Create(stream, settings);
            writer.WriteProcessingInstruction("xml", "version=\"1.0\" encoding=\"" + encoding.WebName + "\" standalone=\"yes\"");
            return writer;
        }

#if ASTORIA_CLIENT
        /// <summary>
        /// get attribute value from specified namespace or empty namespace
        /// </summary>
        /// <param name="reader">reader</param>
        /// <param name="attributeName">attributeName</param>
        /// <param name="namespaceUri">namespaceUri</param>
        /// <returns>attribute value</returns>
        internal static string GetAttributeEx(this XmlReader reader, string attributeName, string namespaceUri)
        {
            return reader.GetAttribute(attributeName, namespaceUri) ?? reader.GetAttribute(attributeName);
        }

        /// <summary>Recursively removes duplicate namespace attributes from the specified <paramref name="element"/>.</summary>
        /// <param name="element">Element to remove duplicate attributes from.</param>
        internal static void RemoveDuplicateNamespaceAttributes(System.Xml.Linq.XElement element)
        {
            Debug.Assert(element != null, "element != null");

            // Nesting XElements through our client wrappers may produce duplicate namespaces.
            // Might need an XML fix, but won't happen for 3.5 for back compat.
            HashSet<string> names = new HashSet<string>(EqualityComparer<string>.Default);
            foreach (System.Xml.Linq.XElement e in element.DescendantsAndSelf())
            {
                bool attributesFound = false;
                foreach (var attribute in e.Attributes())
                {
                    if (!attributesFound)
                    {
                        attributesFound = true;
                        names.Clear();
                    }

                    if (attribute.IsNamespaceDeclaration)
                    {
                        string localName = attribute.Name.LocalName;
                        bool alreadyPresent = names.Add(localName) == false;
                        if (alreadyPresent)
                        {
                            attribute.Remove();
                        }
                    }
                }
            }
        }
#endif
    }
}
