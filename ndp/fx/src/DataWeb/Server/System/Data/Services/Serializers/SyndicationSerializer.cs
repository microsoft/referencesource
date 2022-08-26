//---------------------------------------------------------------------
// <copyright file="SyndicationSerializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a serializer that creates syndication objects and
//      then formatts them.
// </summary>
//
// @owner Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Data.Services.Common;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.ServiceModel.Syndication;
    using System.Text;
    using System.Xml;
    using System.Xml.Linq;
    using System.Xml.Serialization;

    #endregion Namespaces.

    /// <summary>Serializes results into System.ServiceModel.Syndication objects, which can then be formatted.</summary>
    internal sealed class SyndicationSerializer : Serializer
    {
        #region Fields.

        /// <summary>Namespace-qualified attribute for null value annotations.</summary>
        internal static readonly XmlQualifiedName QualifiedNullAttribute = new XmlQualifiedName(XmlConstants.AtomNullAttributeName, XmlConstants.DataWebMetadataNamespace);

        /// <summary>Empty person singleton.</summary>
        private static readonly SyndicationPerson EmptyPerson = new SyndicationPerson(null, String.Empty, null);

        /// <summary>Namespace-qualified namespace prefix for the DataWeb namespace.</summary>
        private static readonly XmlQualifiedName QualifiedDataWebPrefix = new XmlQualifiedName(XmlConstants.DataWebNamespacePrefix, XmlConstants.XmlNamespacesNamespace);

        /// <summary>Namespace-qualified namespace prefix for the DataWebMetadata namespace.</summary>
        private static readonly XmlQualifiedName QualifiedDataWebMetadataPrefix = new XmlQualifiedName(XmlConstants.DataWebMetadataNamespacePrefix, XmlConstants.XmlNamespacesNamespace);

        /// <summary>Factory for syndication formatter implementation.</summary>
        private readonly SyndicationFormatterFactory factory;

        /// <summary>Last updated time for <see cref="SyndicationItem"/> elements.</summary>
        /// <remarks>
        /// While this is currently an arbitrary decision, it at least saves us from re-querying the system time
        /// every time an item is generated.
        /// </remarks>
        private readonly DateTimeOffset lastUpdatedTime = DateTimeOffset.UtcNow;

        /// <summary>Writer to which output is sent.</summary>
        private readonly XmlWriter writer;

        /// <summary>Top-level feed being built.</summary>
        private SyndicationFeed resultFeed;

        /// <summary>Top-level item being built.</summary>
        private SyndicationItem resultItem;

        #endregion Fields.

        /// <summary>Initializes a new SyndicationSerializer instance.</summary>
        /// <param name="requestDescription">Request description.</param>
        /// <param name="absoluteServiceUri">Absolute URI to the service entry point.</param>
        /// <param name="service">Service with configuration and provider from which metadata should be gathered.</param>
        /// <param name="output">Stream to write to.</param>
        /// <param name="encoding">Encoding for text in output stream.</param>
        /// <param name="etag">HTTP ETag header value.</param>
        /// <param name="factory">Factory for formatter objects.</param>
        internal SyndicationSerializer(
            RequestDescription requestDescription,
            Uri absoluteServiceUri,
            IDataService service,
            Stream output,
            Encoding encoding,
            string etag,
            SyndicationFormatterFactory factory)
            : base(requestDescription, absoluteServiceUri, service, etag)
        {
            Debug.Assert(service != null, "service != null");
            Debug.Assert(output != null, "output != null");
            Debug.Assert(encoding != null, "encoding != null");
            Debug.Assert(factory != null, "factory != null");

            this.factory = factory;
            this.writer = factory.CreateWriter(output, encoding);
        }

        /// <summary>Serializes exception information.</summary>
        /// <param name="args">Description of exception to serialize.</param>
        public override void WriteException(HandleExceptionArgs args)
        {
            ErrorHandler.SerializeXmlError(args, this.writer);
        }

        /// <summary>Writes a primitive value to the specified output.</summary>
        /// <param name="primitive">Primitive value to write.</param>
        /// <param name="propertyName">name of the property whose value needs to be written</param>
        /// <param name="expectedTypeName">Type name of the property</param>
        /// <param name="content">Content dictionary to which the value should be written.</param>
        internal static void WritePrimitiveValue(object primitive, string propertyName, string expectedTypeName, DictionaryContent content)
        {
            Debug.Assert(!String.IsNullOrEmpty(propertyName), "!String.IsNullOrEmpty(propertyName)");
            Debug.Assert(expectedTypeName != null, "expectedTypeName != null");
            if (primitive == null)
            {
                content.AddNull(expectedTypeName, propertyName);
            }
            else
            {
                string primitiveString = PlainXmlSerializer.PrimitiveToString(primitive);
                Debug.Assert(primitiveString != null, "primitiveString != null");
                content.Add(propertyName, expectedTypeName, primitiveString);
            }
        }

        /// <summary>Writes an Atom link element.</summary>
        /// <param name="linkRelation">relation of the link element with the parent element</param>
        /// <param name="title">title of the deferred element</param>
        /// <param name="href">uri for the deferred element</param>
        /// <param name="linkType">link type for the deferred element</param>
        /// <param name="item">Item to write link in.</param>
        internal static void WriteDeferredContentElement(string linkRelation, string title, string href, string linkType, SyndicationItem item)
        {
            Debug.Assert(linkRelation != null, "linkRelation != null");
            Debug.Assert(item != null, "item != null");
            Debug.Assert(linkType != null, "linkType != null");

            SyndicationLink link = new SyndicationLink();
            link.RelationshipType = linkRelation;
            link.Title = title;
            link.Uri = new Uri(href, UriKind.RelativeOrAbsolute);
            link.MediaType = linkType;
            item.Links.Add(link);
        }

        /// <summary>Flushes the writer to the underlying stream.</summary>
        protected override void Flush()
        {
            this.writer.Flush();
        }

        /// <summary>Writes a single top-level element.</summary>
        /// <param name="expanded">Expanded properties for the result.</param>
        /// <param name="element">Element to write, possibly null.</param>
        protected override void WriteTopLevelElement(IExpandedResult expanded, object element)
        {
            Debug.Assert(this.RequestDescription.IsSingleResult, "this.RequestDescription.SingleResult");
            Debug.Assert(element != null, "element != null");

            this.resultItem = new SyndicationItem();
            this.resultItem.BaseUri = this.AbsoluteServiceUri;
            IncludeCommonNamespaces(this.resultItem.AttributeExtensions);

            if (this.RequestDescription.TargetSource == RequestTargetSource.EntitySet ||
                this.RequestDescription.TargetSource == RequestTargetSource.ServiceOperation)
            {
                bool needPop = this.PushSegmentForRoot();
                this.WriteEntryElement(
                    expanded,
                    element,
                    this.RequestDescription.TargetResourceType,
                    this.RequestDescription.ResultUri,
                    this.RequestDescription.ContainerName,
                    this.resultItem);
                this.PopSegmentName(needPop);
            }
            else
            {
                Debug.Assert(
                    this.RequestDescription.TargetSource == RequestTargetSource.Property,
                    "TargetSource(" + this.RequestDescription.TargetSource + ") == Property -- otherwise control shouldn't be here.");
                ResourceType resourcePropertyType;
                if (this.RequestDescription.TargetKind == RequestTargetKind.OpenProperty)
                {
                    resourcePropertyType = (element == null) ? ResourceType.PrimitiveStringResourceType : WebUtil.GetResourceType(this.Provider, element);
                    if (resourcePropertyType == null)
                    {
                        Type propertyType = element == null ? typeof(string) : element.GetType();
                        throw new InvalidOperationException(Strings.Serializer_UnsupportedTopLevelType(propertyType));
                    }
                }
                else
                {
                    Debug.Assert(this.RequestDescription.Property != null, "this.RequestDescription.Property - otherwise Property source set with no Property specified.");
                    ResourceProperty property = this.RequestDescription.Property;
                    resourcePropertyType = property.ResourceType;
                }

                Debug.Assert(
                    resourcePropertyType.ResourceTypeKind == ResourceTypeKind.EntityType,
                    "Open ResourceTypeKind == EnityType -- temporarily, because ATOM is the only implemented syndication serializer and doesn't support it.");

                bool needPop = this.PushSegmentForRoot();
                this.WriteEntryElement(
                    expanded,                               // expanded
                    element,                                // element
                    resourcePropertyType,              // expectedType
                    this.RequestDescription.ResultUri,      // absoluteUri
                    this.RequestDescription.ContainerName,  // relativeUri
                    this.resultItem);                       // target
                this.PopSegmentName(needPop);
            }

            // Since the element is not equal to null, the factory should never return null
            SyndicationItemFormatter formatter = this.factory.CreateSyndicationItemFormatter(this.resultItem);
            formatter.WriteTo(this.writer);
        }

        /// <summary>Writes multiple top-level elements, possibly none.</summary>
        /// <param name="expanded">Expanded properties for the result.</param>
        /// <param name="elements">Enumerator for elements to write.</param>
        /// <param name="hasMoved">Whether <paramref name="elements"/> was succesfully advanced to the first element.</param>
        protected override void WriteTopLevelElements(IExpandedResult expanded, IEnumerator elements, bool hasMoved)
        {
            Debug.Assert(elements != null, "elements != null");
            Debug.Assert(!this.RequestDescription.IsSingleResult, "!this.RequestDescription.SingleResult");

            string title;
            if (this.RequestDescription.TargetKind != RequestTargetKind.OpenProperty &&
                this.RequestDescription.TargetSource == RequestTargetSource.Property)
            {
                title = this.RequestDescription.Property.Name;
            }
            else
            {
                title = this.RequestDescription.ContainerName;
            }

            this.resultFeed = new SyndicationFeed();
            IncludeCommonNamespaces(this.resultFeed.AttributeExtensions);
            this.resultFeed.BaseUri = RequestUriProcessor.AppendEscapedSegment(this.AbsoluteServiceUri, "");
            string relativeUri = this.RequestDescription.LastSegmentInfo.Identifier;

            // support for $count
            if (this.RequestDescription.CountOption == RequestQueryCountOption.Inline)
            {
                this.WriteRowCount();
            }
            
            bool needPop = this.PushSegmentForRoot();

            this.WriteFeedElements(
                expanded,
                elements,
                this.RequestDescription.TargetResourceType,
                title,                                      // title
                this.RequestDescription.ResultUri,          // absoluteUri
                relativeUri,                                // relativeUri 
                hasMoved,                                   // hasMoved
                this.resultFeed,                            // feed
                false);

            this.PopSegmentName(needPop);

            SyndicationFeedFormatter formatter = this.factory.CreateSyndicationFeedFormatter(this.resultFeed);
            formatter.WriteTo(this.writer);
        }

        /// <summary>
        /// Write out the entry count
        /// </summary>
        protected override void WriteRowCount()
        {
            XElement rowCountElement = new XElement(
                XName.Get(XmlConstants.RowCountElement, XmlConstants.DataWebMetadataNamespace),
                RequestDescription.CountValue);

            this.resultFeed.ElementExtensions.Add(rowCountElement);
        }

        /// <summary>
        /// Write out the uri for the given element
        /// </summary>
        /// <param name="element">element whose uri needs to be written out.</param>
        protected override void WriteLink(object element)
        {
            throw Error.NotImplemented();
        }

        /// <summary>
        /// Write out the uri for the given elements
        /// </summary>
        /// <param name="elements">elements whose uri need to be writtne out</param>
        /// <param name="hasMoved">the current state of the enumerator.</param>
        protected override void WriteLinkCollection(IEnumerator elements, bool hasMoved)
        {
            throw Error.NotImplemented();
        }

        /// <summary>Ensures that common namespaces are included in the topmost tag.</summary>
        /// <param name='attributeExtensions'>Attribute extensions to write namespaces to.</param>
        /// <remarks>
        /// This method should be called by any method that may write a 
        /// topmost element tag.
        /// </remarks>
        private static void IncludeCommonNamespaces(Dictionary<XmlQualifiedName, string> attributeExtensions)
        {
            attributeExtensions.Add(QualifiedDataWebPrefix, XmlConstants.DataWebNamespace);
            attributeExtensions.Add(QualifiedDataWebMetadataPrefix, XmlConstants.DataWebMetadataNamespace);
        }

        /// <summary>Sets the type name for the specified syndication entry.</summary>
        /// <param name="item">Item on which to set the type name.</param>
        /// <param name="fullName">Full type name for the entry.</param>
        private static void SetEntryTypeName(SyndicationItem item, string fullName)
        {
            Debug.Assert(item != null, "item != null");
            item.Categories.Add(new SyndicationCategory(fullName, XmlConstants.DataWebSchemeNamespace, null));
        }

        /// <summary>
        /// Write the link relation element
        /// </summary>
        /// <param name="title">title for the current element</param>
        /// <param name="linkRelation">link relation for the self uri</param>
        /// <param name="relativeUri">relative uri for the current element</param>
        /// <param name="item">Item to write to.</param>
        /// <param name="attributeExtensions">List of custom attributes to add to the link element</param>
        private static void WriteLinkRelations(string title, string linkRelation, string relativeUri, SyndicationItem item, params KeyValuePair<XmlQualifiedName, string>[] attributeExtensions)
        {
            Debug.Assert(item != null, "item != null");
            Debug.Assert(relativeUri != null, "relativeUri != null");

            // Write the link relation element
            var link = new SyndicationLink();
            link.RelationshipType = linkRelation;
            link.Title = title;
            link.Uri = new Uri(relativeUri, UriKind.Relative);
            foreach (KeyValuePair<XmlQualifiedName, string> attributeExtension in attributeExtensions)
            {
                link.AttributeExtensions.Add(attributeExtension.Key, attributeExtension.Value);
            }

            item.Links.Add(link);
        }

        /// <summary>
        /// Checks if a particular property value should be skipped from the content section due to 
        /// EntityProperty mappings for friendly feeds
        /// </summary>
        /// <param name="currentSourceRoot">Current root segment in the source tree for a resource type</param>
        /// <param name="propertyName">Name of the property being checked for</param>
        /// <returns>true if skipping of property value is needed, false otherwise</returns>
        private static bool EpmNeedToSkip(EpmSourcePathSegment currentSourceRoot, String propertyName)
        {
            if (currentSourceRoot != null)
            {
                EpmSourcePathSegment epmProperty = currentSourceRoot.SubProperties.Find(subProp => subProp.PropertyName == propertyName);
                if (epmProperty != null)
                {
                    Debug.Assert(epmProperty.SubProperties.Count == 0, "Complex type added as leaf node in EPM tree.");
                    Debug.Assert(epmProperty.EpmInfo != null, "Found a non-leaf property for which EpmInfo is not set.");
                    Debug.Assert(epmProperty.EpmInfo.Attribute != null, "Attribute should always be initialized for EpmInfo.");
                    if (epmProperty.EpmInfo.Attribute.KeepInContent == false)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Obtains the child EPM segment corresponding to the given <paramref name="propertyName"/>
        /// </summary>
        /// <param name="currentSourceRoot">Current root segment</param>
        /// <param name="propertyName">Name of property</param>
        /// <returns>Child segment or null if there is not segment corresponding to the given <paramref name="propertyName"/></returns>
        private static EpmSourcePathSegment EpmGetComplexPropertySegment(EpmSourcePathSegment currentSourceRoot, String propertyName)
        {
            if (currentSourceRoot != null)
            {
                return currentSourceRoot.SubProperties.Find(subProp => subProp.PropertyName == propertyName);
            }
            else
            {
                return null;
            }
        }

        /// <summary>Writes the value of a complex object.</summary>
        /// <param name="element">Element to write.</param>
        /// <param name="propertyName">name of the property whose value needs to be written</param>
        /// <param name="expectedType">expected type of the property</param>
        /// <param name="relativeUri">relative uri for the complex type element</param>
        /// <param name="content">Content to write to.</param>
        /// <param name="currentSourceRoot">Epm source sub-tree corresponding to <paramref name="element"/></param>
        private void WriteComplexObjectValue(object element, string propertyName, ResourceType expectedType, string relativeUri, DictionaryContent content, EpmSourcePathSegment currentSourceRoot)
        {
            Debug.Assert(!String.IsNullOrEmpty(propertyName), "!String.IsNullOrEmpty(propertyName)");
            Debug.Assert(expectedType != null, "expectedType != null");
            Debug.Assert(!String.IsNullOrEmpty(relativeUri), "!String.IsNullOrEmpty(relativeUri)");
            Debug.Assert(expectedType.ResourceTypeKind == ResourceTypeKind.ComplexType, "Must be complex type");
            Debug.Assert(content != null, "content != null");

            // Non-value complex types may form a cycle.
            // PERF: we can keep a single element around and save the HashSet initialization
            // until we find a second complex type - this saves the allocation on trees
            // with shallow (single-level) complex types.
            Debug.Assert(!expectedType.IsMediaLinkEntry, "!expectedType.IsMediaLinkEntry");
            DictionaryContent valueProperties = new DictionaryContent(expectedType.Properties.Count);
            Debug.Assert(!expectedType.InstanceType.IsValueType, "!expectedType.Type.IsValueType -- checked in the resource type constructor.");

            if (element == null)
            {
                content.AddNull(expectedType.FullName, propertyName);
            }
            else
            {
                if (this.AddToComplexTypeCollection(element))
                {
                    ResourceType resourceType = WebUtil.GetNonPrimitiveResourceType(this.Provider, element);
                    this.WriteObjectProperties(null, element, resourceType, null, relativeUri, null, valueProperties, currentSourceRoot);
                    if (!valueProperties.IsEmpty)
                    {
                        content.Add(propertyName, resourceType.FullName, valueProperties);
                    }

                    this.RemoveFromComplexTypeCollection(element);
                }
                else
                {
                    throw new InvalidOperationException(Strings.Serializer_LoopsNotAllowedInComplexTypes(propertyName));
                }
            }
        }

        /// <summary>Write the entry element.</summary>
        /// <param name="expanded">Expanded result provider for the specified <paramref name="element"/>.</param>
        /// <param name="element">element representing the entry element</param>
        /// <param name="expectedType">expected type of the entry element</param>
        /// <param name="absoluteUri">absolute uri for the entry element</param>
        /// <param name="relativeUri">relative uri for the entry element</param>
        /// <param name="target">Target to write to.</param>
        private void WriteEntryElement(IExpandedResult expanded, object element, ResourceType expectedType, Uri absoluteUri, string relativeUri, SyndicationItem target)
        {
            Debug.Assert(element != null || (absoluteUri != null && !String.IsNullOrEmpty(relativeUri)), "Uri's must be specified for null values");
            Debug.Assert(target != null, "target != null");

            this.IncrementSegmentResultCount();

            string title, fullName;
            if (expectedType == null)
            {
                // If the request uri is targetting some open type properties, then we don't know the type of the resource
                // Hence we assume it to be of object type. The reason we do this is that if the value is null, there is
                // no way to know what the type of the property would be, and then we write it out as object. If the value
                // is not null, then we do get the resource type from the instance and write out the actual resource type.
                title = typeof(object).Name;
                fullName = typeof(object).FullName;
            }
            else
            {
                title = expectedType.Name;
                fullName = expectedType.FullName;
            }

            target.Title = new TextSyndicationContent(String.Empty);
            if (element == null)
            {
                SetEntryTypeName(target, fullName);
                target.AttributeExtensions[QualifiedNullAttribute] = XmlConstants.XmlTrueLiteral;
                this.WriteOtherElements(
                    element,
                    expectedType,
                    title,
                    absoluteUri,
                    relativeUri,
                    null,
                    target);

                // Don't know when we hit this code path, keeping existing behaviour in this case
                target.Authors.Add(EmptyPerson);
            }
            else
            {
                absoluteUri = Serializer.GetUri(element, this.Provider, this.CurrentContainer, this.AbsoluteServiceUri);
                Debug.Assert(absoluteUri.AbsoluteUri.StartsWith(this.AbsoluteServiceUri.AbsoluteUri, StringComparison.Ordinal), "absoluteUri.AbsoluteUri.StartsWith(this.AbsoluteServiceUri.AbsoluteUri, StringComparison.Ordinal))");
                relativeUri = absoluteUri.AbsoluteUri.Substring(this.AbsoluteServiceUri.AbsoluteUri.Length);
                ResourceType actualResourceType = WebUtil.GetNonPrimitiveResourceType(this.Provider, element);

                string mediaETag = null;
                Uri readStreamUri = null;
                string mediaContentType = null;
                if (actualResourceType.IsMediaLinkEntry)
                {
                    this.Service.StreamProvider.GetStreamDescription(element, this.Service.OperationContext, relativeUri, out mediaETag, out readStreamUri, out mediaContentType);
                }

                SetEntryTypeName(target, actualResourceType.FullName);
                this.WriteOtherElements(
                    element,
                    actualResourceType,
                    title,
                    absoluteUri,
                    relativeUri,
                    mediaETag,
                    target);

                // Write the etag property, if the type has etag properties
                string etag = this.GetETagValue(element);
                if (etag != null)
                {
                    target.AttributeExtensions[new XmlQualifiedName(XmlConstants.AtomETagAttributeName, XmlConstants.DataWebMetadataNamespace)]
                        = etag;
                }

                DictionaryContent content = new DictionaryContent(actualResourceType.Properties.Count);

                using (EpmContentSerializer epmSerializer = new EpmContentSerializer(actualResourceType, element, target, this.Provider))
                {
                    this.WriteObjectProperties(expanded, element, actualResourceType, absoluteUri, relativeUri, target, content, actualResourceType.HasEntityPropertyMappings ? actualResourceType.EpmSourceTree.Root : null);
                    epmSerializer.Serialize(content, this.Provider);
                }

                if (actualResourceType.IsMediaLinkEntry)
                {
                    // Write <content type="..." src="..." />
                    Debug.Assert(readStreamUri != null, "readStreamUri != null");
                    Debug.Assert(!string.IsNullOrEmpty(mediaContentType), "!string.IsNullOrEmpty(mediaContentType)");
                    target.Content = new UrlSyndicationContent(readStreamUri, mediaContentType);
                    if (!content.IsEmpty)
                    {
                        // Since UrlSyndicationContent must have empty content, we write the <m:property /> node as SyndicationElementExtension.
                        target.ElementExtensions.Add(content.GetPropertyContentsReader());
                    }
                }
                else
                {
                    target.Content = content;
                }
            }

#if ASTORIA_FF_CALLBACKS
            this.Service.InternalOnWriteItem(target, element);
#endif                
        }

        /// <summary>
        /// Writes the feed element for the atom payload
        /// </summary>
        /// <param name="expanded">Expanded properties for the result.</param>
        /// <param name="elements">collection of entries in the feed element</param>
        /// <param name="expectedType">expectedType of the elements in the collection</param>
        /// <param name="title">title of the feed element</param>
        /// <param name="absoluteUri">absolute uri representing the feed element</param>
        /// <param name="relativeUri">relative uri representing the feed element</param>
        /// <param name="hasMoved">whether the enumerator has successfully moved to the first element</param>
        /// <param name='feed'>Feed to write to.</param>
        /// <param name="disposeElementsOnSuccess">If set to true the function should dispose the elements enumerator when it's done
        /// with it. Not in the case this method fails though.</param>
        private void WriteFeedElements(
            IExpandedResult expanded, 
            IEnumerator elements, 
            ResourceType expectedType, 
            string title, 
            Uri absoluteUri, 
            string relativeUri, 
            bool hasMoved, 
            SyndicationFeed feed, 
            bool disposeElementsOnSuccess)
        {
            Debug.Assert(feed != null, "feed != null");

            // Write the other elements for the feed
            feed.Id = absoluteUri.AbsoluteUri;
            feed.Title = new TextSyndicationContent(title);
            var uri = new Uri(relativeUri, UriKind.Relative);
            var link = new SyndicationLink(uri, XmlConstants.AtomSelfRelationAttributeValue, title, null, 0L);
            feed.Links.Add(link);

            if (!hasMoved)
            {
                // ATOM specification: if a feed contains no entries, then the feed should have at least one Author tag
                feed.Authors.Add(EmptyPerson);
            }
            
            // Instead of looping, create an item that will defer the production of SyndicationItem instances.
            // PERF: consider optimizing out empty collections when hasMoved is false.
            feed.Items = this.DeferredFeedItems(
                expanded, 
                elements, 
                expectedType, 
                hasMoved, 
                this.SaveSegmentNames(), 
                (o, e) => this.WriteNextPageLink(o, e, absoluteUri),
                disposeElementsOnSuccess);
#if ASTORIA_FF_CALLBACKS
            this.Service.InternalOnWriteFeed(feed);
#endif            
        }

        /// <summary>
        /// Writes the next page link to the current xml writer corresponding to the feed
        /// </summary>
        /// <param name="lastElement">Object that will contain the keys for skip token</param>
        /// <param name="expandedResult">The <see cref="IExpandedResult"/> of the $skiptoken property of the object being written</param>
        /// <param name="absoluteUri">Absolute URI for the result</param>
        private void WriteNextPageLink(object lastElement, IExpandedResult expandedResult, Uri absoluteUri)
        {
            this.writer.WriteStartElement("link", XmlConstants.AtomNamespace);
            this.writer.WriteAttributeString("rel", "next");
            this.writer.WriteAttributeString("href", this.GetNextLinkUri(lastElement, expandedResult, absoluteUri));
            this.writer.WriteEndElement();
        }

        /// <summary>Provides an enumeration of deferred feed items.</summary>
        /// <param name="expanded">Expanded properties for the result.</param>
        /// <param name="elements">Elements to enumerate.</param>
        /// <param name="expectedType">Expected type of elements.</param>
        /// <param name="hasMoved">Whether the enumerator moved to the first element.</param>
        /// <param name="activeSegmentNames">The segment names active at this point in serialization.</param>
        /// <param name="nextPageLinkWriter">Delegate that writes the next page link if necessity arises</param>
        /// <param name="disposeElements">If set to true the function should dispose the elements enumerator (always).</param>
        /// <returns>An object that can enumerate syndication items.</returns>
        private IEnumerable<SyndicationItem> DeferredFeedItems(
            IExpandedResult expanded, 
            IEnumerator elements,
            ResourceType expectedType, 
            bool hasMoved, 
            object activeSegmentNames,
            Action<object, IExpandedResult> nextPageLinkWriter,
            bool disposeElements)
        {
            try
            {
                object savedSegmentNames = this.SaveSegmentNames();
                this.RestoreSegmentNames(activeSegmentNames);
                object lastObject = null;
                IExpandedResult lastExpandedSkipToken = null;
                while (hasMoved)
                {
                    object o = elements.Current;
                    IExpandedResult skipToken = this.GetSkipToken(expanded);
                    
                    if (o != null)
                    {
                        SyndicationItem target = new SyndicationItem();
                        IExpandedResult expandedO = o as IExpandedResult;
                        if (expandedO != null)
                        {
                            expanded = expandedO;
                            o = GetExpandedElement(expanded);
                            skipToken = this.GetSkipToken(expanded);
                        }

                        this.WriteEntryElement(expanded, o, expectedType, null, null, target);
                        yield return target;
                    }

                    hasMoved = elements.MoveNext();
                    lastObject = o;
                    lastExpandedSkipToken = skipToken;
                }

                // After looping through the objects in the sequence, decide if we need to write the next
                // page link and if yes, write it by invoking the delegate
                if (this.NeedNextPageLink(elements))
                {
                    nextPageLinkWriter(lastObject, lastExpandedSkipToken);
                }

                this.RestoreSegmentNames(savedSegmentNames);
            }
            finally
            {
                if (disposeElements)
                {
                    WebUtil.Dispose(elements);
                }
            }
        }

        /// <summary>
        /// Write entry/feed elements, except the content element and related links
        /// </summary>
        /// <param name="element">entity instance being serialized</param>
        /// <param name="type">resource type of the entry element</param>
        /// <param name="title">title for the current element</param>
        /// <param name="absoluteUri">absolute uri for the current element</param>
        /// <param name="relativeUri">relative uri for the current element</param>
        /// <param name="mediaETag">entity tag for the Media Resource</param>
        /// <param name="item">Item to write to.</param>
        private void WriteOtherElements(object element, ResourceType type, string title, Uri absoluteUri, string relativeUri, string mediaETag, SyndicationItem item)
        {
            Debug.Assert(item != null, "item != null");
            Debug.Assert(absoluteUri != null, "absoluteUri != null");
            Debug.Assert(relativeUri != null, "relativeUri != null");

            // Write Id element
            item.Id = absoluteUri.AbsoluteUri;

            // Write Updated element
            item.LastUpdatedTime = this.lastUpdatedTime;

            // Write "edit-media" link
            if (type != null && type.IsMediaLinkEntry)
            {
                KeyValuePair<XmlQualifiedName, string>[] attributeExtensions = new KeyValuePair<XmlQualifiedName, string>[0];
                if (element != null && !string.IsNullOrEmpty(mediaETag))
                {
                    XmlQualifiedName mediaResourceETagKey = new XmlQualifiedName(XmlConstants.AtomETagAttributeName, XmlConstants.DataWebMetadataNamespace);
                    attributeExtensions = new KeyValuePair<XmlQualifiedName, string>[] { new KeyValuePair<XmlQualifiedName, string>(mediaResourceETagKey, mediaETag) };
                }

                WriteLinkRelations(
                    title,
                    XmlConstants.AtomEditMediaRelationAttributeValue,
                    DataServiceStreamProviderWrapper.GetStreamEditMediaUri(relativeUri),
                    item,
                    attributeExtensions);
            }

            // Write "edit" link
            WriteLinkRelations(
                title,
                XmlConstants.AtomEditRelationAttributeValue,
                relativeUri,
                item);
        }

        /// <summary>Writes all the properties of the specified resource or complex object.</summary>
        /// <param name="expanded">Expanded properties for the result.</param>
        /// <param name="customObject">Resource or complex object with properties to write out.</param>
        /// <param name="resourceType">resourceType containing metadata about the current custom object</param>
        /// <param name="absoluteUri">absolute uri for the given resource</param>
        /// <param name="relativeUri">relative uri for the given resource</param>
        /// <param name="item">Item in which to place links / expansions.</param>
        /// <param name="content">Content in which to place values.</param>
        /// <param name="currentSourceRoot">Epm source sub-tree corresponding to <paramref name="customObject"/></param>
        private void WriteObjectProperties(IExpandedResult expanded, object customObject, ResourceType resourceType, Uri absoluteUri, string relativeUri, SyndicationItem item, DictionaryContent content, EpmSourcePathSegment currentSourceRoot)
        {
            Debug.Assert(customObject != null, "customObject != null");
            Debug.Assert(resourceType != null, "resourceType != null");

            Debug.Assert(!String.IsNullOrEmpty(relativeUri), "!String.IsNullOrEmpty(relativeUri)");
            
            if (absoluteUri == null && resourceType.ResourceTypeKind == ResourceTypeKind.EntityType)
            {
                // entity type should have an URI, complex type should not have an URI
                // If the static type of the object is "Object", we will mistreat an entity type as complex type and hit this situation
                throw new DataServiceException(500, Strings.BadProvider_InconsistentEntityOrComplexTypeUsage(resourceType.Name));
            }

            this.RecurseEnter();
            try
            {
                List<ResourcePropertyInfo> navProperties = null;
                IEnumerable<ProjectionNode> projectionNodes = null;
                if (resourceType.ResourceTypeKind == ResourceTypeKind.EntityType)
                {
                    Debug.Assert(this.CurrentContainer != null, "this.CurrentContainer != null");
                    if (this.Provider.IsEntityTypeDisallowedForSet(this.CurrentContainer, resourceType))
                    {
                        throw new InvalidOperationException(Strings.BaseServiceProvider_NavigationPropertiesOnDerivedEntityTypesNotSupported(resourceType.FullName, this.CurrentContainer.Name));
                    }

                    navProperties = new List<ResourcePropertyInfo>(resourceType.Properties.Count);

                    projectionNodes = this.GetProjections();
                }

                if (projectionNodes == null)
                {
                    var action = resourceType.DictionarySerializerDelegate;
                    if (action == null && this.Provider.IsV1Provider)
                    {
                        Module module = typeof(SyndicationSerializer).Module;
                        Type customObjectType = customObject.GetType();
                        Type[] parameterTypes = new Type[] { typeof(object), typeof(DictionaryContent) };
                        DynamicMethod method = new DynamicMethod("content_populator", typeof(void), parameterTypes, module, false /* skipVisibility */);
                        ILGenerator generator = method.GetILGenerator();
                        MethodInfo methodWritePrimitiveValue = typeof(SyndicationSerializer).GetMethod("WritePrimitiveValue", BindingFlags.Static | BindingFlags.NonPublic);

                        // Downcast the argument.
                        generator.Emit(OpCodes.Ldarg_0);
                        generator.Emit(OpCodes.Castclass, customObjectType);

                        foreach (ResourceProperty property in resourceType.Properties.Where(p => p.TypeKind == ResourceTypeKind.Primitive))
                        {
                            if (SyndicationSerializer.EpmNeedToSkip(currentSourceRoot, property.Name))
                            {
                                continue;
                            }

                            // WritePrimitiveValue(propertyValue, property.Name, property.ResourceType, content);
                            generator.Emit(OpCodes.Dup);
                            generator.Emit(OpCodes.Call, resourceType.GetPropertyInfo(property).GetGetMethod());
                            if (property.Type.IsValueType)
                            {
                                generator.Emit(OpCodes.Box, property.Type);
                            }

                            generator.Emit(OpCodes.Ldstr, property.Name);
                            generator.Emit(OpCodes.Ldstr, property.ResourceType.FullName);
                            generator.Emit(OpCodes.Ldarg_1);
                            generator.Emit(OpCodes.Call, methodWritePrimitiveValue);
                        }

                        generator.Emit(OpCodes.Pop);
                        generator.Emit(OpCodes.Ret);
                        action = (Action<object, DictionaryContent>)method.CreateDelegate(typeof(Action<object, DictionaryContent>), null);
                        resourceType.DictionarySerializerDelegate = action;
                    }

                    if (action != null)
                    {
                        action(customObject, content);
                    }
                    else
                    {
                        foreach (ResourceProperty property in resourceType.Properties.Where(p => p.TypeKind == ResourceTypeKind.Primitive))
                        {
                            object propertyValue = WebUtil.GetPropertyValue(this.Provider, customObject, resourceType, property, null);
                            if (SyndicationSerializer.EpmNeedToSkip(currentSourceRoot, property.Name))
                            {
                                continue;
                            }

                            WritePrimitiveValue(propertyValue, property.Name, property.ResourceType.FullName, content);
                        }
                    }

                    foreach (ResourceProperty property in this.Provider.GetResourceProperties(this.CurrentContainer, resourceType))
                    {
                        string propertyName = property.Name;
                        if (property.TypeKind == ResourceTypeKind.EntityType)
                        {
                            Debug.Assert(navProperties != null, "navProperties list must be assigned for entity types");

                            object propertyValue =
                                (this.ShouldExpandSegment(property.Name)) ? GetExpandedProperty(this.Provider, expanded, customObject, property) : null;
                            navProperties.Add(new ResourcePropertyInfo(property, propertyValue));
                        }
                        else
                        {
                            if (property.TypeKind == ResourceTypeKind.ComplexType)
                            {
                                object propertyValue = WebUtil.GetPropertyValue(this.Provider, customObject, resourceType, property, null);
                                bool needPop = this.PushSegmentForProperty(property);
                                this.WriteComplexObjectValue(
                                        propertyValue,
                                        propertyName,
                                        property.ResourceType,
                                        relativeUri + "/" + property.Name,
                                        content,
                                        SyndicationSerializer.EpmGetComplexPropertySegment(currentSourceRoot, property.Name));
                                this.PopSegmentName(needPop);
                            }
                        }
                    }

                    if (resourceType.IsOpenType)
                    {
                        IEnumerable<KeyValuePair<string, object>> properties = this.Provider.GetOpenPropertyValues(customObject);
                        foreach (KeyValuePair<string, object> property in properties)
                        {
                            string propertyName = property.Key;

                            if (String.IsNullOrEmpty(propertyName))
                            {
                                throw new DataServiceException(500, Strings.Syndication_InvalidOpenPropertyName(resourceType.FullName));
                            }

                            Type valueType;
                            ResourceType propertyResourceType;

                            object value = property.Value;

                            if (value == null || value == DBNull.Value)
                            {
                                valueType = typeof(string);
                                propertyResourceType = ResourceType.PrimitiveStringResourceType;
                            }
                            else
                            {
                                valueType = value.GetType();
                                propertyResourceType = WebUtil.GetResourceType(this.Provider, value);
                            }

                            // A null ResourceType indicates a ----ed type (eg, IntPtr or DateTimeOffset). So ignore it.
                            if (propertyResourceType == null)
                            {
                                throw new DataServiceException(500, Strings.Syndication_InvalidOpenPropertyType(propertyName));
                            }

                            if (propertyResourceType.ResourceTypeKind == ResourceTypeKind.Primitive)
                            {
                                if (value != null && SyndicationSerializer.EpmNeedToSkip(currentSourceRoot, propertyName))
                                {
                                    continue;
                                }

                                WritePrimitiveValue(value, propertyName, propertyResourceType.FullName, content);
                            }
                            else
                            {
                                if (propertyResourceType.ResourceTypeKind == ResourceTypeKind.ComplexType)
                                {
                                    Debug.Assert(propertyResourceType.InstanceType == valueType, "propertyResourceType.Type == valueType");
                                    this.WriteComplexObjectValue(
                                            value,
                                            propertyName,
                                            propertyResourceType,
                                            relativeUri + "/" + propertyName,
                                            content,
                                            SyndicationSerializer.EpmGetComplexPropertySegment(currentSourceRoot, propertyName));
                                }
                                else
                                {
                                    Debug.Assert(
                                        propertyResourceType.ResourceTypeKind == ResourceTypeKind.EntityType,
                                        "propertyResourceType.ResourceTypeKind == ResourceTypeKind.EntityType -- otherwise should have been processed as primitve or complex type.");

                                    // Open navigation properties are not supported on OpenTypes
                                    throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(propertyName));
                                }
                            }
                        }
                    }
                }
                else
                {
                    foreach (ProjectionNode projectionNode in projectionNodes)
                    {
                        string propertyName = projectionNode.PropertyName;
                        ResourceProperty property = resourceType.TryResolvePropertyName(propertyName);

                        // First solve the normal entity type property - turn it into a nav. property record
                        if (property != null && property.TypeKind == ResourceTypeKind.EntityType)
                        {
                            Debug.Assert(navProperties != null, "navProperties list must be assigned for entity types");

                            // By calling the GetResourceProperties we will use the cached list of properties
                            //   for the given type and set. But we have to search through it.
                            // We could use the GetContainer (since that's what the GetResourceProperties does) and check
                            //   if it returns null, but result of that is only partially cached so it might be expensive
                            //   to evaluate for each item in the feed.
                            if (this.Provider.GetResourceProperties(this.CurrentContainer, resourceType).Contains(property))
                            {
                                object expandedPropertyValue =
                                    (this.ShouldExpandSegment(propertyName)) ? GetExpandedProperty(this.Provider, expanded, customObject, property) : null;
                                navProperties.Add(new ResourcePropertyInfo(property, expandedPropertyValue));
                            }

                            continue;
                        }

                        // Now get the property value
                        object propertyValue = WebUtil.GetPropertyValue(this.Provider, customObject, resourceType, property, property == null ? propertyName : null);

                        // Determine the type of the property
                        ResourceType propertyResourceType;
                        if (property != null)
                        {
                            propertyResourceType = property.ResourceType;
                        }
                        else
                        {
                            if (propertyValue == null || propertyValue == DBNull.Value)
                            {
                                propertyResourceType = ResourceType.PrimitiveStringResourceType;
                            }
                            else
                            {
                                propertyResourceType = WebUtil.GetResourceType(this.Provider, propertyValue);

                                // A null ResourceType indicates a ----ed type (eg, IntPtr or DateTimeOffset). So ignore it.
                                if (propertyResourceType == null)
                                {
                                    throw new DataServiceException(500, Strings.Syndication_InvalidOpenPropertyType(propertyName));
                                }
                            }
                        }

                        // And write out the value (depending on the type of the property)
                        if (propertyResourceType.ResourceTypeKind == ResourceTypeKind.Primitive)
                        {
                            if (propertyValue == DBNull.Value)
                            {
                                propertyValue = null;
                            }

                            if (propertyValue != null && SyndicationSerializer.EpmNeedToSkip(currentSourceRoot, propertyName))
                            {
                                continue;
                            }

                            WritePrimitiveValue(propertyValue, propertyName, propertyResourceType.FullName, content);
                        }
                        else if (propertyResourceType.ResourceTypeKind == ResourceTypeKind.ComplexType)
                        {
                            bool needPop = false;
                            if (property != null)
                            {
                                needPop = this.PushSegmentForProperty(property);
                            }

                            this.WriteComplexObjectValue(
                                    propertyValue,
                                    propertyName,
                                    propertyResourceType,
                                    relativeUri + "/" + propertyName,
                                    content,
                                    SyndicationSerializer.EpmGetComplexPropertySegment(currentSourceRoot, propertyName));
                            if (property != null)
                            {
                                this.PopSegmentName(needPop);
                            }
                        }
                        else
                        {
                            Debug.Assert(
                                propertyResourceType.ResourceTypeKind == ResourceTypeKind.EntityType,
                                "propertyResourceType.ResourceTypeKind == ResourceTypeKind.EntityType -- otherwise should have been processed as primitve or complex type.");

                            // Open navigation properties are not supported on OpenTypes
                            throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(propertyName));
                        }
                    }
                }

                if (resourceType.ResourceTypeKind == ResourceTypeKind.EntityType)
                {
                    for (int i = 0; i < navProperties.Count; i++)
                    {
                        ResourcePropertyInfo propertyInfo = navProperties[i];
                        ResourceProperty navProperty = propertyInfo.Property;

                        Debug.Assert(
                            navProperty.IsOfKind(ResourcePropertyKind.ResourceReference) ||
                            navProperty.IsOfKind(ResourcePropertyKind.ResourceSetReference),
                            "this must be nav property");

                        // Generate a link - see http://tools.ietf.org/html/rfc4287#section-4.2.7
                        string linkType = navProperty.IsOfKind(ResourcePropertyKind.ResourceReference) ? XmlConstants.AtomEntryElementName : XmlConstants.AtomFeedElementName;
                        linkType = String.Format(CultureInfo.InvariantCulture, "{0};{1}={2}", XmlConstants.MimeApplicationAtom, XmlConstants.AtomTypeAttributeName, linkType);
                        string segmentIdentifier = navProperty.Name;

                        if (!this.ShouldExpandSegment(navProperty.Name))
                        {
                            WriteDeferredContentElement(
                                XmlConstants.DataWebRelatedNamespace + navProperty.Name,
                                navProperty.Name,
                                relativeUri + "/" + segmentIdentifier,
                                linkType,
                                item);
                        }
                        else
                        {
                            object propertyValue = propertyInfo.Value;
                            IExpandedResult expandedResultPropertyValue = propertyValue as IExpandedResult;
                            object expandedPropertyValue =
                                expandedResultPropertyValue != null ?
                                GetExpandedElement(expandedResultPropertyValue) :
                                propertyValue;
                            string propertyRelativeUri = relativeUri + "/" + segmentIdentifier;
                            Uri propertyAbsoluteUri = RequestUriProcessor.AppendUnescapedSegment(absoluteUri, segmentIdentifier);

                            SyndicationLink link = new SyndicationLink();
                            link.RelationshipType = XmlConstants.DataWebRelatedNamespace + navProperty.Name;
                            link.Title = navProperty.Name;
                            link.Uri = new Uri(propertyRelativeUri, UriKind.RelativeOrAbsolute);
                            link.MediaType = linkType;
                            item.Links.Add(link);

                            bool needPop = this.PushSegmentForProperty(navProperty);

                            // if this.CurrentContainer is null, the target set of the navigation property is hidden.
                            if (this.CurrentContainer != null)
                            {
                                if (navProperty.IsOfKind(ResourcePropertyKind.ResourceSetReference))
                                {
                                    IEnumerable enumerable;
                                    bool collection = WebUtil.IsElementIEnumerable(expandedPropertyValue, out enumerable);
                                    Debug.Assert(collection, "metadata loading must have ensured that navigation set properties must implement IEnumerable");

                                    SyndicationFeed feed = new SyndicationFeed();
                                    InlineAtomFeed inlineFeedExtension = new InlineAtomFeed(feed, this.factory);
                                    link.ElementExtensions.Add(inlineFeedExtension);
                                    IEnumerator enumerator = enumerable.GetEnumerator();
                                    try
                                    {
                                        bool hasMoved = enumerator.MoveNext();
                                        this.WriteFeedElements(
                                            propertyValue as IExpandedResult, 
                                            enumerator, 
                                            navProperty.ResourceType, 
                                            navProperty.Name, 
                                            propertyAbsoluteUri, 
                                            propertyRelativeUri, 
                                            hasMoved, 
                                            feed,
                                            true);
                                    }
                                    catch
                                    {
                                        WebUtil.Dispose(enumerator);
                                        throw;
                                    }
                                }
                                else
                                {
                                    SyndicationItem inlineItem = new SyndicationItem();
                                    this.WriteEntryElement(propertyValue as IExpandedResult, expandedPropertyValue, navProperty.ResourceType, propertyAbsoluteUri, propertyRelativeUri, inlineItem);
                                    InlineAtomItem inlineItemExtension = new InlineAtomItem(inlineItem, this.factory);
                                    link.ElementExtensions.Add(inlineItemExtension);
                                }
                            }

                            this.PopSegmentName(needPop);
                        }
                    }
                }
            }
            finally
            {
                // The matching call to RecurseLeave is in a try/finally block not because it's necessary in the 
                // presence of an exception (progress will halt anyway), but because it's easier to maintain in the 
                // code in the presence of multiple exit points (returns).
                this.RecurseLeave();
            }
        }

        #region Inner types.

        /// <summary>Stores the resource property, its value and a flag which indicates whether this is a open property or not.</summary>
        private struct ResourcePropertyInfo
        {
            /// <summary>refers to the property that this instance represents.</summary>
            private ResourceProperty resourceProperty;

            /// <summary>Value of the property.</summary>
            private object value;

            /// <summary>
            /// Creates a new instance of ResourcePropertyInfo.
            /// </summary>
            /// <param name="resourceProperty">resource property instance.</param>
            /// <param name="value">value for the resource property.</param>
            public ResourcePropertyInfo(ResourceProperty resourceProperty, object value)
            {
                Debug.Assert(resourceProperty != null, "resourceProperty != null");
                this.resourceProperty = resourceProperty;
                this.value = value;
            }

            /// <summary>Returns the resource property.</summary>
            internal ResourceProperty Property
            {
                get { return this.resourceProperty; }
            }

            /// <summary>Returns the value of the resource property.</summary>
            internal object Value
            {
                get { return this.value; }
            }
        }

        /// <summary>Wrapper for an inline item.</summary>
        [XmlRoot(ElementName = XmlConstants.AtomInlineElementName, Namespace = XmlConstants.DataWebMetadataNamespace)]
        internal class InlineAtomItem : IXmlSerializable
        {
            /// <summary>Factory for item formatter.</summary>
            private readonly SyndicationFormatterFactory factory;

            /// <summary>Item being serialized.</summary>
            private SyndicationItem item;

            /// <summary>Empty constructor.</summary>
            internal InlineAtomItem()
            {
            }

            /// <summary>Initializing constructor.</summary>
            /// <param name="item">Item being serialized.</param>
            /// <param name="factory">Factory for item formatter.</param>
            internal InlineAtomItem(SyndicationItem item, SyndicationFormatterFactory factory)
            {
                this.item = item;
                this.factory = factory;
            }

            #region IXmlSerializable Members

            /// <summary>Reserved method.</summary>
            /// <returns>null</returns>
            public System.Xml.Schema.XmlSchema GetSchema()
            {
                return null;
            }

            /// <summary>Generates an object from its XML representation.</summary>
            /// <param name='reader'>XmlReader with representation.</param>
            public void ReadXml(XmlReader reader)
            {
                throw Error.NotImplemented();
            }

            /// <summary>Converts an object into its XML representation.</summary>
            /// <param name='writer'>Writer to write representation into.</param>
            public void WriteXml(XmlWriter writer)
            {
                SyndicationItemFormatter formatter = this.factory.CreateSyndicationItemFormatter(this.item);
                if (formatter != null)
                {
                    formatter.WriteTo(writer);
                }
            }

            #endregion
        }

        /// <summary>Wrapper for an inline feed.</summary>
        [XmlRoot(ElementName = XmlConstants.AtomInlineElementName, Namespace = XmlConstants.DataWebMetadataNamespace)]
        internal class InlineAtomFeed : IXmlSerializable
        {
            /// <summary>Factory for item formatter.</summary>
            private readonly SyndicationFormatterFactory factory;

            /// <summary>Feed being serialized.</summary>
            private SyndicationFeed feed;

            /// <summary>Empty constructor.</summary>
            internal InlineAtomFeed()
            {
            }

            /// <summary>Initializing constructor.</summary>
            /// <param name="feed">Feed being serialized.</param>
            /// <param name="factory">Factory for item formatter.</param>
            internal InlineAtomFeed(SyndicationFeed feed, SyndicationFormatterFactory factory)
            {
                this.feed = feed;
                this.factory = factory;
            }

            #region IXmlSerializable Members

            /// <summary>Reserved method.</summary>
            /// <returns>null</returns>
            public System.Xml.Schema.XmlSchema GetSchema()
            {
                return null;
            }

            /// <summary>Generates an object from its XML representation.</summary>
            /// <param name='reader'>XmlReader with representation.</param>
            public void ReadXml(XmlReader reader)
            {
                throw Error.NotImplemented();
            }

            /// <summary>Converts an object into its XML representation.</summary>
            /// <param name='writer'>Writer to write representation into.</param>
            public void WriteXml(XmlWriter writer)
            {
                this.factory.CreateSyndicationFeedFormatter(this.feed).WriteTo(writer);
            }

            #endregion
        }

        #endregion Inner types.
    }
}
