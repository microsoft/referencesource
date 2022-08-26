//---------------------------------------------------------------------
// <copyright file="AtomParser.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Parses an XML stream into structured AtomEntry or related instances.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Linq;
    using System.Text;

    #endregion Namespaces.

    /// <summary>Parser for DataService payloads (mostly ATOM).</summary>
    /// <remarks>
    /// There are four types of documents parsed:
    /// 1. A single non-entity value.
    /// 2. A list of non-entity values.
    /// 3. An entity.
    /// 4. A feed.
    /// 
    /// In case of (1), the parser will go through these states:
    ///  None -> Custom -> Finished
    ///  
    /// In case of (2), the parser will go through these states:
    ///  None -> Custom -> Custom -> Finished
    /// 
    /// In case of (3), the parser will go through these states:
    ///  None -> Entry -> Finished
    /// 
    /// In case of (4), the parser will go through these states:
    ///  None -> (FeedData | Entry) -> (FeedData | Entry) -> Finished
    /// 
    /// Note that the parser will always stop on 'top-level' packets; all recursive data
    /// structures are handled internally.
    /// </remarks>
    [DebuggerDisplay("AtomParser {kind} {reader}")]
    internal class AtomParser
    {
        #region Private fields.

        /// <summary>Callback invoked each time an ATOM entry is found.</summary>
        /// <remarks>
        /// This callback takes the current XmlReader and returns a 
        /// subtree XmlReader and an object that is assigned to the 
        /// entry's Tag property.
        /// </remarks>
        private readonly Func<XmlReader, KeyValuePair<XmlReader, object>> entryCallback;

        /// <summary>Stack of available XmlReaders.</summary>
        private readonly Stack<XmlReader> readers;

        /// <summary>Scheme used to find type information on ATOM category elements.</summary>
        private readonly string typeScheme;

        /// <summary>ATOM entry being parsed.</summary>
        private AtomEntry entry;

        /// <summary>ATOM feed being parsed.</summary>
        private AtomFeed feed;

        /// <summary>Current data kind (nothing, entry, feed, custom-top-level-thingy, etc).</summary>
        private AtomDataKind kind;

        /// <summary>Current <see cref="XmlReader"/>.</summary>
        private XmlReader reader;

        /// <summary>The data namespace</summary>
        private string currentDataNamespace;

        #endregion Private fields.

        #region Constructors.

        /// <summary>Initializes a new <see cref="AtomParser"/> instance.</summary>
        /// <param name="reader"><see cref="XmlReader"/> to parse content from.</param>
        /// <param name="entryCallback">
        /// Callback invoked each time an ATOM entry is found; see the comments
        /// on the entryCallback field.
        /// </param>
        /// <param name="typeScheme">
        /// Scheme used to find type information on ATOM category elements.
        /// </param>
        /// <param name="currentDataNamespace">The xml document's DataWeb Namespace</param>
        internal AtomParser(XmlReader reader, Func<XmlReader, KeyValuePair<XmlReader, object>> entryCallback, string typeScheme, string currentDataNamespace)
        {
            Debug.Assert(reader != null, "reader != null");
            Debug.Assert(typeScheme != null, "typeScheme != null");
            Debug.Assert(entryCallback != null, "entryCallback != null");
            Debug.Assert(!String.IsNullOrEmpty(currentDataNamespace), "currentDataNamespace is empty or null");

            this.reader = reader;
            this.readers = new Stack<XmlReader>();
            this.entryCallback = entryCallback;
            this.typeScheme = typeScheme;
            this.currentDataNamespace = currentDataNamespace;

            Debug.Assert(this.kind == AtomDataKind.None, "this.kind == AtomDataKind.None -- otherwise not initialized correctly");
        }

        #endregion Constructors.

        #region Internal properties.

        /// <summary>Entry being materialized; possibly null.</summary>
        internal AtomEntry CurrentEntry
        {
            get
            {
                return this.entry;
            }
        }

        /// <summary>Feed being materialized; possibly null.</summary>
        internal AtomFeed CurrentFeed
        {
            get
            {
                return this.feed;
            }
        }

        /// <summary>Kind of ATOM data available on the parser.</summary>
        internal AtomDataKind DataKind
        {
            get
            {
                return this.kind;
            }
        }

        /// <summary>
        /// Returns true if the current element is in the data web namespace
        /// </summary>
        internal bool IsDataWebElement
        {
            get { return this.reader.NamespaceURI == this.currentDataNamespace; }
        }

        #endregion Internal properties.

        #region Internal methods.

        /// <summary>
        /// Creates an <see cref="XElement"/> instance for ATOM entries.
        /// </summary>
        /// <param name="reader">Reader being used.</param>
        /// <returns>
        /// A pair of an XmlReader instance and an object to be assigned 
        /// to the Tag on the entry (available for materialization callbacks
        /// later in the pipeline).
        /// </returns>
        /// <remarks>
        /// A no-op implementation would do this instead:
        /// 
        /// return new KeyValuePair&lt;XmlReader, object&gt;(reader.ReadSubtree(), null);
        /// </remarks>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "not required")]
        internal static KeyValuePair<XmlReader, object> XElementBuilderCallback(XmlReader reader)
        {
            Debug.Assert(reader != null, "reader != null");
            Debug.Assert(reader is Xml.XmlWrappingReader, "reader must be a instance of XmlWrappingReader");

            string readerBaseUri = reader.BaseURI;
            XElement element = XElement.Load(reader.ReadSubtree(), LoadOptions.None);
            return new KeyValuePair<XmlReader, object>(Xml.XmlWrappingReader.CreateReader(readerBaseUri, element.CreateReader()), element);
        }

        #endregion Internal methods.

        #region Internal methods.

        /// <summary>Consumes the next chunk of content from the underlying XML reader.</summary>
        /// <returns>
        /// true if another piece of content is available, identified by DataKind.
        /// false if there is no more content.
        /// </returns>
        internal bool Read()
        {
            // When an external caller 'insists', we'll come all the way down (which is the 'most local'
            // scope at which this is known), and unwind as a no-op.
            if (this.DataKind == AtomDataKind.Finished)
            {
                return false;
            }

            while (this.reader.Read())
            {
                if (ShouldIgnoreNode(this.reader))
                {
                    continue;
                }

                Debug.Assert(
                    this.reader.NodeType == XmlNodeType.Element || this.reader.NodeType == XmlNodeType.EndElement,
                    "this.reader.NodeType == XmlNodeType.Element || this.reader.NodeType == XmlNodeType.EndElement -- otherwise we should have ignored or thrown");

                AtomDataKind readerData = ParseStateForReader(this.reader);

                if (this.reader.NodeType == XmlNodeType.EndElement)
                {
                    // The only case in which we expect to see an end-element at the top level
                    // is for a feed. Custom elements and entries should be consumed by
                    // their own parsing methods. However we are tolerant of additional EndElements,
                    // which at this point mean we have nothing else to consume.
                    break;
                }

                switch (readerData)
                {
                    case AtomDataKind.Custom:
                        if (this.DataKind == AtomDataKind.None)
                        {
                            this.kind = AtomDataKind.Custom;
                            return true;
                        }
                        else
                        {
                            MaterializeAtom.SkipToEnd(this.reader);
                            continue;
                        }

                    case AtomDataKind.Entry:
                        this.kind = AtomDataKind.Entry;
                        this.ParseCurrentEntry(out this.entry);
                        return true;

                    case AtomDataKind.Feed:
                        if (this.DataKind == AtomDataKind.None)
                        {
                            this.feed = new AtomFeed();
                            this.kind = AtomDataKind.Feed;
                            return true;
                        }

                        throw new InvalidOperationException(Strings.AtomParser_FeedUnexpected);

                    case AtomDataKind.FeedCount:
                        this.ParseCurrentFeedCount();
                        break;

                    case AtomDataKind.PagingLinks:
                        if (this.feed == null)
                        {
                            // paging link outside of feed?
                            throw new InvalidOperationException(Strings.AtomParser_PagingLinkOutsideOfFeed);
                        }

                        this.kind = AtomDataKind.PagingLinks;
                        this.ParseCurrentFeedPagingLinks();
                        return true;

                    default:
                        Debug.Assert(false, "Atom Parser is in a wrong state...Did you add a new AtomDataKind?");
                        break;
                }
            }

            this.kind = AtomDataKind.Finished;
            this.entry = null;
            return false;
        }

        /// <summary>Reads the current property value from the reader.</summary>
        /// <returns>A structured property instance.</returns>
        /// <remarks>
        /// This method should only be called for top-level complex properties.
        /// 
        /// For top-level primitive values, <see cref="ReadCustomElementString"/>
        /// should be used to preserve V1 behavior in which mixed-content
        /// XML elements are allowed.
        /// </remarks>
        internal AtomContentProperty ReadCurrentPropertyValue()
        {
            Debug.Assert(
                this.kind == AtomDataKind.Custom,
                "this.kind == AtomDataKind.Custom -- otherwise caller shouldn't invoke ReadCurrentPropertyValue");
            return this.ReadPropertyValue();
        }

        /// <summary>Reads the current property value from the reader as a string.</summary>
        /// <returns>A structured property instance.</returns>
        /// <remarks>
        /// This method should only be called for top-level primitive types.
        /// 
        /// For top-level complex type values, <see cref="ReadCurrentPropertyValue"/>
        /// should be used to capture all information.
        /// </remarks>
        internal string ReadCustomElementString()
        {
            Debug.Assert(
                this.kind == AtomDataKind.Custom,
                "this.kind == AtomDataKind.Custom -- otherwise caller shouldn't invoke ReadCustomElementString");
            return MaterializeAtom.ReadElementString(this.reader, true);
        }

        /// <summary>Replaces the current reader with the specified reader.</summary>
        /// <param name="newReader">New reader to use.</param>
        /// <remarks>
        /// This is a very odd method, here only to allow inline count to
        /// read everything ahead and "reopen" where we are. No checks are
        /// done as to whether we are in a safe place to do so.
        /// </remarks>
        internal void ReplaceReader(XmlReader newReader)
        {
            Debug.Assert(newReader != null, "newReader != null");
            this.reader = newReader;
        }

        #endregion Internal methods.

        #region Private methods.

        /// <summary>
        /// Determines what the parse state should be for the specified 
        /// <paramref name="reader"/>.
        /// </summary>
        /// <param name="reader">Reader to check.</param>
        /// <returns>The data kind derived from the current element.</returns>
        /// <remarks>
        /// Note that no previous state is considered, so state transitions
        /// aren't handled by the method - instead, certain known elements
        /// are mapped to parser states.
        /// </remarks>
        private static AtomDataKind ParseStateForReader(XmlReader reader)
        {
            Debug.Assert(reader != null, "reader != null");
            Debug.Assert(
                reader.NodeType == XmlNodeType.Element || reader.NodeType == XmlNodeType.EndElement,
                "reader.NodeType == XmlNodeType.Element || EndElement -- otherwise can't determine");

            AtomDataKind result = AtomDataKind.Custom;
            string elementName = reader.LocalName;
            string namespaceURI = reader.NamespaceURI;
            if (Util.AreSame(XmlConstants.AtomNamespace, namespaceURI))
            {
                if (Util.AreSame(XmlConstants.AtomEntryElementName, elementName))
                {
                    result = AtomDataKind.Entry;
                }
                else if (Util.AreSame(XmlConstants.AtomFeedElementName, elementName))
                {
                    result = AtomDataKind.Feed;
                }
                else if (Util.AreSame(XmlConstants.AtomLinkElementName, elementName) &&
                    Util.AreSame(XmlConstants.AtomLinkNextAttributeString, reader.GetAttribute(XmlConstants.AtomLinkRelationAttributeName)))
                {
                    result = AtomDataKind.PagingLinks;
                }
            }
            else if (Util.AreSame(XmlConstants.DataWebMetadataNamespace, namespaceURI))
            {
                if (Util.AreSame(XmlConstants.RowCountElement, elementName))
                {
                    result = AtomDataKind.FeedCount;
                }
            }

            return result;
        }

        /// <summary>
        /// Reads from the specified <paramref name="reader"/> and moves to the 
        /// child element which should match the specified name.
        /// </summary>
        /// <param name="reader">Reader to consume.</param>
        /// <param name="localName">Expected local name of child element.</param>
        /// <param name="namespaceUri">Expected namespace of child element.</param>
        /// <returns>
        /// true if the <paramref name="reader"/> is left position on a child
        /// with the given name; false otherwise.
        /// </returns>
        private static bool ReadChildElement(XmlReader reader, string localName, string namespaceUri)
        {
            Debug.Assert(localName != null, "localName != null");
            Debug.Assert(namespaceUri != null, "namespaceUri != null");
            Debug.Assert(!reader.IsEmptyElement, "!reader.IsEmptyElement");
            Debug.Assert(reader.NodeType != XmlNodeType.EndElement, "reader.NodeType != XmlNodeType.EndElement");

            return reader.Read() && reader.IsStartElement(localName, namespaceUri);
        }

        /// <summary>
        /// Skips all content on the specified <paramref name="reader"/> until
        /// the specified <paramref name="depth"/> is reached; such that
        /// a call to .Read() will move to the sibling of the element at
        /// <paramref name="depth"/>.
        /// </summary>
        /// <param name="reader">Reader to advance.</param>
        /// <param name="depth">Desired depth.</param>
        /// <remarks>
        /// The reader may already be on an element at <paramref name="depth"/>;
        /// if it's an empty element (&lt;foo /&gt;) then the reader isn't
        /// moved.
        /// </remarks>
        private static void SkipToEndAtDepth(XmlReader reader, int depth)
        {
            Debug.Assert(reader != null, "reader != null");
            Debug.Assert(reader.Depth >= depth, "reader.Depth >= depth");

            while (!(reader.Depth == depth &&
                     (reader.NodeType == XmlNodeType.EndElement ||
                      (reader.NodeType == XmlNodeType.Element && reader.IsEmptyElement))))
            {
                reader.Read();
            }
        }

        /// <summary>
        /// Reads the text inside the element on the <paramref name="reader"/>.
        /// </summary>
        /// <param name="reader">Reader to get text from.</param>
        /// <returns>The text inside the specified <paramref name="reader"/>.</returns>
        /// <remarks>
        /// This method was designed to be compatible with the results
        /// of evaluating the text of an XElement.
        /// 
        /// In short, this means that nulls are never returned, and
        /// that all non-text nodes are ignored (but elements are
        /// recursed into).
        /// </remarks>
        private static string ReadElementStringForText(XmlReader reader)
        {
            Debug.Assert(reader != null, "reader != null");
            if (reader.IsEmptyElement)
            {
                return String.Empty;
            }

            StringBuilder result = new StringBuilder();
            int depth = reader.Depth;
            while (reader.Read())
            {
                if (reader.Depth == depth)
                {
                    Debug.Assert(
                        reader.NodeType == XmlNodeType.EndElement,
                        "reader.NodeType == XmlNodeType.EndElement -- otherwise XmlReader is acting odd");
                    break;
                }

                if (reader.NodeType == XmlNodeType.SignificantWhitespace ||
                    reader.NodeType == XmlNodeType.Text)
                {
                    result.Append(reader.Value);
                }
            }

            return result.ToString();
        }

        /// <summary>
        /// Checks whether the current node on the specified <paramref name="reader"/> 
        /// should be ignored.
        /// </summary>
        /// <param name="reader">Reader to check.</param>
        /// <returns>true if the node should be ignored; false if it should be processed.</returns>
        /// <remarks>
        /// This method will throw an exception on unexpected content (CDATA, entity references,
        /// text); therefore it should not be used if mixed content is allowed.
        /// </remarks>
        private static bool ShouldIgnoreNode(XmlReader reader)
        {
            Debug.Assert(reader != null, "reader != null");

            switch (reader.NodeType)
            {
                case XmlNodeType.CDATA:
                case XmlNodeType.EntityReference:
                case XmlNodeType.EndEntity:
                    Error.ThrowInternalError(InternalError.UnexpectedXmlNodeTypeWhenReading);
                    break;
                case XmlNodeType.Text:
                case XmlNodeType.SignificantWhitespace:
                    // throw Error.InvalidOperation(Strings.Deserialize_MixedContent(currentType.ElementTypeName));
                    Error.ThrowInternalError(InternalError.UnexpectedXmlNodeTypeWhenReading);
                    break;
                case XmlNodeType.Element:
                case XmlNodeType.EndElement:
                    return false;
                default:
                    break;
            }

            return true;
        }

        /// <summary>
        /// Checks if the given content type string matches with 'application/xml' or 
        /// 'application/atom+xml' case insensitively.
        /// </summary>
        /// <param name="contentType">Input content type.</param>
        /// <returns>true if match found, false otherwise.</returns>
        private static bool IsAllowedContentType(string contentType)
        {
            return (String.Equals(XmlConstants.MimeApplicationXml, contentType, StringComparison.OrdinalIgnoreCase) ||
                    String.Equals(XmlConstants.MimeApplicationAtom, contentType, StringComparison.OrdinalIgnoreCase));
        }

        /// <summary>
        /// Checks if the given link type matches 'application/atom+xml;type=feed' or
        /// 'application/atom+xml;type=entry' case insensitively.
        /// </summary>
        /// <param name="linkType">Input link type.</param>
        /// <param name="isFeed">Output parameter indicating whether we are reading a feed or an entry inline.</param>
        /// <returns>true if match found, false otherwise.</returns>
        private static bool IsAllowedLinkType(string linkType, out bool isFeed)
        {
            isFeed = String.Equals(XmlConstants.LinkMimeTypeFeed, linkType, StringComparison.OrdinalIgnoreCase);
            return isFeed ? true : String.Equals(XmlConstants.LinkMimeTypeEntry, linkType, StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>
        /// Parses the content on the reader into the specified <paramref name="targetEntry"/>.
        /// </summary>
        /// <param name="targetEntry">Target to read values into.</param>
        private void ParseCurrentContent(AtomEntry targetEntry)
        {
            Debug.Assert(targetEntry != null, "targetEntry != null");
            Debug.Assert(this.reader.NodeType == XmlNodeType.Element, "this.reader.NodeType == XmlNodeType.Element");

            string propertyValue = this.reader.GetAttributeEx(XmlConstants.AtomContentSrcAttributeName, XmlConstants.AtomNamespace);
            if (propertyValue != null)
            {
                // This is a media link entry
                // Note that in this case we don't actually use this URL (or the link/edit-media URL)
                // for editing. We rely on the Astoria URL convention (/propname/$value or just /$value)
                if (!this.reader.IsEmptyElement)
                {
                    throw Error.InvalidOperation(Strings.Deserialize_ExpectedEmptyMediaLinkEntryContent);
                }

                targetEntry.MediaLinkEntry = true;
                targetEntry.MediaContentUri = new Uri(propertyValue, UriKind.RelativeOrAbsolute);
            }
            else
            {
                // This is a regular (non-media link) entry
                if (targetEntry.MediaLinkEntry.HasValue && targetEntry.MediaLinkEntry.Value)
                {
                    // This means we saw a <m:Properties> element but now we have a Content element
                    // that's not just a media link entry pointer (src)
                    throw Error.InvalidOperation(Strings.Deserialize_ContentPlusPropertiesNotAllowed);
                }

                targetEntry.MediaLinkEntry = false;

                propertyValue = this.reader.GetAttributeEx(XmlConstants.AtomTypeAttributeName, XmlConstants.AtomNamespace);
                if (AtomParser.IsAllowedContentType(propertyValue))
                {
                    if (this.reader.IsEmptyElement)
                    {
                        return;
                    }

                    if (ReadChildElement(this.reader, XmlConstants.AtomPropertiesElementName, XmlConstants.DataWebMetadataNamespace))
                    {
                        this.ReadCurrentProperties(targetEntry.DataValues);
                    }
                    else if (this.reader.NodeType != XmlNodeType.EndElement)
                    {
                        throw Error.InvalidOperation(Strings.Deserialize_NotApplicationXml);
                    }
                }
            }
        }

        /// <summary>Parses a link for the specified <paramref name="targetEntry"/>.</summary>
        /// <param name="targetEntry">Entry to update with link information.</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "not required to dispose of the nested XmlReader")]
        private void ParseCurrentLink(AtomEntry targetEntry)
        {
            Debug.Assert(targetEntry != null, "targetEntry != null");
            Debug.Assert(
                this.reader.NodeType == XmlNodeType.Element, 
                "this.reader.NodeType == XmlNodeType.Element -- otherwise we shouldn't try to parse a link");
            Debug.Assert(
                this.reader.LocalName == "link",
                "this.reader.LocalName == 'link' -- otherwise we shouldn't try to parse a link");

            string relation = this.reader.GetAttribute(XmlConstants.AtomLinkRelationAttributeName);
            if (relation == null)
            {
                return;
            }

            if (relation == XmlConstants.AtomEditRelationAttributeValue && targetEntry.EditLink == null)
            {
                // Only process the first link that has @rel='edit'.
                string href = this.reader.GetAttribute(XmlConstants.AtomHRefAttributeName);
                if (String.IsNullOrEmpty(href))
                {
                    throw Error.InvalidOperation(Strings.Context_MissingEditLinkInResponseBody);
                }

                targetEntry.EditLink = this.ConvertHRefAttributeValueIntoURI(href);
            }
            else if (relation == XmlConstants.AtomSelfRelationAttributeValue && targetEntry.QueryLink == null)
            {
                // Only process the first link that has @rel='self'.
                string href = this.reader.GetAttribute(XmlConstants.AtomHRefAttributeName);
                if (String.IsNullOrEmpty(href))
                {
                    throw Error.InvalidOperation(Strings.Context_MissingSelfLinkInResponseBody);
                }

                targetEntry.QueryLink = this.ConvertHRefAttributeValueIntoURI(href);
            }
            else if (relation == XmlConstants.AtomEditMediaRelationAttributeValue && targetEntry.MediaEditUri == null)
            {
                string href = this.reader.GetAttribute(XmlConstants.AtomHRefAttributeName);
                if (String.IsNullOrEmpty(href))
                {
                    throw Error.InvalidOperation(Strings.Context_MissingEditMediaLinkInResponseBody);
                }

                targetEntry.MediaEditUri = this.ConvertHRefAttributeValueIntoURI(href);
                targetEntry.StreamETagText = this.reader.GetAttribute(XmlConstants.AtomETagAttributeName, XmlConstants.DataWebMetadataNamespace);
            }

            if (!this.reader.IsEmptyElement)
            {
                string propertyName = UriUtil.GetNameFromAtomLinkRelationAttribute(relation);
                if (propertyName == null)
                {
                    return;
                }

                string propertyValueText = this.reader.GetAttribute(XmlConstants.AtomTypeAttributeName);
                bool isFeed;

                if (!IsAllowedLinkType(propertyValueText, out isFeed))
                {
                    return;
                }

                if (!ReadChildElement(this.reader, XmlConstants.AtomInlineElementName, XmlConstants.DataWebMetadataNamespace))
                {
                    return;
                }

                bool emptyInlineCollection = this.reader.IsEmptyElement;
                object propertyValue = null;

                if (!emptyInlineCollection)
                {
                    AtomFeed nestedFeed = null;
                    AtomEntry nestedEntry = null;
                    List<AtomEntry> feedEntries = null;

                    Debug.Assert(this.reader is Xml.XmlWrappingReader, "reader must be a instance of XmlWrappingReader");
                    string readerBaseUri = this.reader.BaseURI;
                    XmlReader nestedReader = Xml.XmlWrappingReader.CreateReader(readerBaseUri, this.reader.ReadSubtree());
                    nestedReader.Read();
                    Debug.Assert(nestedReader.LocalName == "inline", "nestedReader.LocalName == 'inline'");

                    AtomParser nested = new AtomParser(nestedReader, this.entryCallback, this.typeScheme, this.currentDataNamespace);
                    while (nested.Read())
                    {
                        switch (nested.DataKind)
                        {
                            case AtomDataKind.Feed:
                                feedEntries = new List<AtomEntry>();
                                nestedFeed = nested.CurrentFeed;
                                propertyValue = nestedFeed;
                                break;
                            case AtomDataKind.Entry:
                                nestedEntry = nested.CurrentEntry;
                                if (feedEntries != null)
                                {
                                    feedEntries.Add(nestedEntry);
                                }
                                else
                                {
                                    propertyValue = nestedEntry;
                                }

                                break;
                            case AtomDataKind.PagingLinks:
                                // Here the inner feed parser found a paging link, and stored it on nestedFeed.NextPageLink
                                // we are going to add it into a link table and associate
                                // with the collection at AtomMaterializer::Materialize()
                                // Do nothing for now.
                                break;
                            default:
                                throw new InvalidOperationException(Strings.AtomParser_UnexpectedContentUnderExpandedLink);
                        }
                    }

                    if (nestedFeed != null)
                    {
                        Debug.Assert(
                            nestedFeed.Entries == null,
                            "nestedFeed.Entries == null -- otherwise someone initialized this for us");
                        nestedFeed.Entries = feedEntries;
                    }
                }

                AtomContentProperty property = new AtomContentProperty();
                property.Name = propertyName;

                if (emptyInlineCollection || propertyValue == null)
                {
                    property.IsNull = true;
                    if (isFeed)
                    {
                        property.Feed = new AtomFeed();
                        property.Feed.Entries = Enumerable.Empty<AtomEntry>();
                    }
                    else
                    {
                        property.Entry = new AtomEntry();
                        property.Entry.IsNull = true;
                    }
                }
                else
                {
                    property.Feed = propertyValue as AtomFeed;
                    property.Entry = propertyValue as AtomEntry;
                }

                targetEntry.DataValues.Add(property);
            }
        }

        /// <summary>
        /// Reads a property value and adds it as a text or a sub-property of 
        /// the specified <paramref name="property"/>.
        /// </summary>
        /// <param name="property">Property to read content into.</param>
        private void ReadPropertyValueIntoResult(AtomContentProperty property)
        {
            Debug.Assert(this.reader != null, "reader != null");
            Debug.Assert(property != null, "property != null");

            switch (this.reader.NodeType)
            {
                case XmlNodeType.CDATA:
                case XmlNodeType.SignificantWhitespace:
                case XmlNodeType.Text:
                    if (!String.IsNullOrEmpty(property.Text))
                    {
                        throw Error.InvalidOperation(Strings.Deserialize_MixedTextWithComment);
                    }

                    property.Text = this.reader.Value;
                    break;

                case XmlNodeType.Comment:
                case XmlNodeType.Whitespace:
                case XmlNodeType.ProcessingInstruction:
                case XmlNodeType.EndElement:
                    // Do nothing.
                    // ProcessingInstruction, Whitespace would have thrown before
                    break;

                case XmlNodeType.Element:
                    // We found an element while reading a property value. This should be
                    // a complex type.
                    if (!String.IsNullOrEmpty(property.Text))
                    {
                        throw Error.InvalidOperation(Strings.Deserialize_ExpectingSimpleValue);
                    }

                    property.EnsureProperties();
                    AtomContentProperty prop = this.ReadPropertyValue();

                    if (prop != null)
                    {
                        property.Properties.Add(prop);
                    }

                    break;

                default:
                    throw Error.InvalidOperation(Strings.Deserialize_ExpectingSimpleValue);
            }
        }

        /// <summary>This method will read a string or a complex type.</summary>
        /// <returns>The property value read.</returns>
        /// <remarks>Always checks for null attribute.</remarks>
        private AtomContentProperty ReadPropertyValue()
        {
            Debug.Assert(this.reader != null, "reader != null");
            Debug.Assert(
                this.reader.NodeType == XmlNodeType.Element,
                "reader.NodeType == XmlNodeType.Element -- otherwise caller is confused as to where the reader is");

            if (!this.IsDataWebElement)
            {
                // we expect <d:PropertyName>...</d:PropertyName> only
                SkipToEndAtDepth(this.reader, this.reader.Depth);
                return null;
            }

            AtomContentProperty result = new AtomContentProperty();
            result.Name = this.reader.LocalName;
            result.TypeName = this.reader.GetAttributeEx(XmlConstants.AtomTypeAttributeName, XmlConstants.DataWebMetadataNamespace);
            result.IsNull = Util.DoesNullAttributeSayTrue(this.reader);
            result.Text = result.IsNull ? null : String.Empty;

            if (!this.reader.IsEmptyElement)
            {
                int depth = this.reader.Depth;
                while (this.reader.Read())
                {
                    this.ReadPropertyValueIntoResult(result);
                    if (this.reader.Depth == depth)
                    {
                        break;
                    }
                }
            }

            return result;
        }

        /// <summary>
        /// Reads properties from the current reader into the 
        /// specified <paramref name="values"/> collection.
        /// </summary>
        /// <param name="values">Values to read into.</param>
        private void ReadCurrentProperties(List<AtomContentProperty> values)
        {
            Debug.Assert(values != null, "values != null");
            Debug.Assert(this.reader.NodeType == XmlNodeType.Element, "this.reader.NodeType == XmlNodeType.Element");

            while (this.reader.Read())
            {
                if (ShouldIgnoreNode(this.reader))
                {
                    continue;
                }

                if (this.reader.NodeType == XmlNodeType.EndElement)
                {
                    return;
                }

                if (this.reader.NodeType == XmlNodeType.Element)
                {
                    AtomContentProperty prop = this.ReadPropertyValue();

                    if (prop != null)
                    {
                        values.Add(prop);
                    }
                }
            }
        }

        /// <summary>
        /// Parses the current reader into a new <paramref name="targetEntry"/> 
        /// instance.
        /// </summary>
        /// <param name="targetEntry">
        /// After invocation, the target entry that was created as a result
        /// of parsing the current reader.
        /// </param>
        private void ParseCurrentEntry(out AtomEntry targetEntry)
        {
            Debug.Assert(this.reader.NodeType == XmlNodeType.Element, "this.reader.NodeType == XmlNodeType.Element");

            // Push reader.
            var callbackResult = this.entryCallback(this.reader);
            Debug.Assert(callbackResult.Key != null, "callbackResult.Key != null");
            this.readers.Push(this.reader);
            this.reader = callbackResult.Key;

            this.reader.Read();
            Debug.Assert(this.reader.LocalName == "entry", "this.reader.LocalName == 'entry' - otherwise we're not reading the subtree");

            bool hasContent = false;
            targetEntry = new AtomEntry();
            targetEntry.DataValues = new List<AtomContentProperty>();
            targetEntry.Tag = callbackResult.Value;
            targetEntry.ETagText = this.reader.GetAttribute(XmlConstants.AtomETagAttributeName, XmlConstants.DataWebMetadataNamespace);

            while (this.reader.Read())
            {
                if (ShouldIgnoreNode(this.reader))
                {
                    continue;
                }

                if (this.reader.NodeType == XmlNodeType.Element)
                {
                    int depth = this.reader.Depth;
                    string elementName = this.reader.LocalName;
                    string namespaceURI = this.reader.NamespaceURI;
                    if (namespaceURI == XmlConstants.AtomNamespace)
                    {
                        if (elementName == XmlConstants.AtomCategoryElementName && targetEntry.TypeName == null)
                        {
                            string text = this.reader.GetAttributeEx(XmlConstants.AtomCategorySchemeAttributeName, XmlConstants.AtomNamespace);
                            if (text == this.typeScheme)
                            {
                                targetEntry.TypeName = this.reader.GetAttributeEx(XmlConstants.AtomCategoryTermAttributeName, XmlConstants.AtomNamespace);
                            }
                        }
                        else if (elementName == XmlConstants.AtomContentElementName)
                        {
                            hasContent = true;
                            this.ParseCurrentContent(targetEntry);
                        }
                        else if (elementName == XmlConstants.AtomIdElementName && targetEntry.Identity == null)
                        {
                            // The .Identity == null check ensures that only the first id element is processed.
                            string idText = ReadElementStringForText(this.reader);
                            idText = Util.ReferenceIdentity(idText);

                            // here we could just assign idText to Identity
                            // however we used to check for AbsoluteUri, thus we need to 
                            // convert string to Uri and check for absoluteness
                            Uri idUri = Util.CreateUri(idText, UriKind.RelativeOrAbsolute);
                            if (!idUri.IsAbsoluteUri)
                            {
                                throw Error.InvalidOperation(Strings.Context_TrackingExpectsAbsoluteUri);
                            }

                            targetEntry.Identity = idText;
                        }
                        else if (elementName == XmlConstants.AtomLinkElementName)
                        {
                            this.ParseCurrentLink(targetEntry);
                        }
                    }
                    else if (namespaceURI == XmlConstants.DataWebMetadataNamespace)
                    {
                        if (elementName == XmlConstants.AtomPropertiesElementName)
                        {
                            if (targetEntry.MediaLinkEntry.HasValue && !targetEntry.MediaLinkEntry.Value)
                            {
                                // This means we saw a non-empty <atom:Content> element but now we have a Properties element
                                // that also carries properties
                                throw Error.InvalidOperation(Strings.Deserialize_ContentPlusPropertiesNotAllowed);
                            }

                            targetEntry.MediaLinkEntry = true;

                            if (!this.reader.IsEmptyElement)
                            {
                                this.ReadCurrentProperties(targetEntry.DataValues);
                            }
                        }
                    }

                    SkipToEndAtDepth(this.reader, depth);
                }
            }

            if (targetEntry.Identity == null)
            {
                throw Error.InvalidOperation(Strings.Deserialize_MissingIdElement);
            }

            if (!hasContent)
            {
                // Content is expected for the GetResponse operation
                throw Error.BatchStreamContentExpected(BatchStreamState.GetResponse);
            }

            this.reader = this.readers.Pop();
        }

        /// <summary>Parses the value for the current feed count.</summary>
        /// <remarks>This method will update the value on the current feed.</remarks>
        private void ParseCurrentFeedCount()
        {
            if (this.feed == null)
            {
                throw new InvalidOperationException(Strings.AtomParser_FeedCountNotUnderFeed);
            }

            if (this.feed.Count.HasValue)
            {
                throw new InvalidOperationException(Strings.AtomParser_ManyFeedCounts);
            }

            long countValue;
            if (!long.TryParse(MaterializeAtom.ReadElementString(this.reader, true), System.Globalization.NumberStyles.Integer, System.Globalization.CultureInfo.InvariantCulture, out countValue))
            {
                throw new FormatException(Strings.MaterializeFromAtom_CountFormatError);
            }

            if (countValue < 0)
            {
                throw new FormatException(Strings.MaterializeFromAtom_CountFormatError);
            }

            this.feed.Count = countValue;
        }

        /// <summary>
        /// Parsing paging links
        /// </summary>
        private void ParseCurrentFeedPagingLinks()
        {
            // feed should never be null here since there is an outer check
            // just need to assert
            Debug.Assert(this.feed != null, "Trying to parser paging links but feed is null.");

            if (this.feed.NextLink != null)
            {
                // we have set next link before, this is a duplicate
                // atom spec does not allow duplicated next links
                throw new InvalidOperationException(Strings.AtomMaterializer_DuplicatedNextLink);
            }

            string nextLink = this.reader.GetAttribute(XmlConstants.AtomHRefAttributeName);

            if (nextLink == null)
            {
                throw new InvalidOperationException(Strings.AtomMaterializer_LinksMissingHref);
            }
            else
            {
                this.feed.NextLink = this.ConvertHRefAttributeValueIntoURI(nextLink);
            }
        }

        /// <summary>
        /// creates a new uri instance which takes into account the base uri of the reader.
        /// </summary>
        /// <param name="href">href attribute value.</param>
        /// <returns>a new instance of uri as refered by the <paramref name="href"/></returns>
        private Uri ConvertHRefAttributeValueIntoURI(string href)
        {
            Uri uri = Util.CreateUri(href, UriKind.RelativeOrAbsolute);
            if (!uri.IsAbsoluteUri && !String.IsNullOrEmpty(this.reader.BaseURI))
            {
                Uri baseUri = Util.CreateUri(this.reader.BaseURI, UriKind.RelativeOrAbsolute);

                // The reason why we can't use Util.CreateUri function here, is that the util method
                // checks for trailing slashes in the baseuri and starting forward slashes in the request uri
                // and does some tricks which is not consistent with the uri class behaviour. Hence using the
                // uri class directly here.
                uri = new Uri(baseUri, uri);
            }

            return uri;
        }

        #endregion Private methods.
    }
}
