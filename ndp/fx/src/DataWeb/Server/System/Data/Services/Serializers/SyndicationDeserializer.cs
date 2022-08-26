//---------------------------------------------------------------------
// <copyright file="SyndicationDeserializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a deserializer for atom content structured as 
//      syndication items or feeds.
// </summary>
//
// @owner Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Data.Services.Common;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.ServiceModel.Syndication;
    using System.Text;
    using System.Xml;

    #endregion Namespaces.

    /// <summary>Provides a deserializer for structured content.</summary>
    internal class SyndicationDeserializer : Deserializer
    {
        /// <summary>Factory for syndication formatting objects.</summary>
        private readonly SyndicationFormatterFactory factory;

        /// <summary>reader to read xml from the request stream</summary>
        private readonly XmlReader xmlReader;

        /// <summary>Initializes a new <see cref="SyndicationDeserializer"/> for the specified stream.</summary>
        /// <param name="stream">Input stream reader from which ATOM content must be read.</param>
        /// <param name="encoding">Encoding to use for the stream (null to auto-discover).</param>
        /// <param name="dataService">Data service for which the deserializer will act.</param>
        /// <param name="update">indicates whether this is a update operation or not</param>
        /// <param name="factory">Factory for formatter objects.</param>
        /// <param name="tracker">Tracker to use for modifications.</param>
        internal SyndicationDeserializer(Stream stream, Encoding encoding, IDataService dataService, bool update, SyndicationFormatterFactory factory, UpdateTracker tracker)
            : base(update, dataService, tracker)
        {
            Debug.Assert(stream != null, "stream != null");
            Debug.Assert(factory != null, "factory != null");

            this.factory = factory;
            this.xmlReader = factory.CreateReader(stream, encoding);
        }

        /// <summary>
        /// Indicates the various form of data in the inline xml element
        /// </summary>
        private enum LinkContent
        {
            /// <summary>If the link element didn't not contain an inline element at all.</summary>
            NoInlineElementSpecified,

            /// <summary>If the link element contained an empty inline element.</summary>
            EmptyInlineElementSpecified,

            /// <summary>If the inline element under the link element contained some data.</summary>
            InlineElementContainsData
        }

        /// <summary>returns the content format for the deserializer</summary>
        protected override ContentFormat ContentFormat
        {
            get
            {
                return ContentFormat.Atom;
            }
        }

        /// <summary>
        /// Assumes the payload to represent a single object and processes accordingly
        /// </summary>
        /// <param name="segmentInfo">info about the object being created</param>
        /// <returns>the newly formed object that the payload represents</returns>
        protected override object CreateSingleObject(SegmentInfo segmentInfo)
        {
            SyndicationItem item = ReadSyndicationItem(this.factory.CreateSyndicationItemFormatter(), this.xmlReader);
            return this.CreateObject(segmentInfo, true /*topLevel*/, item);
        }

        /// <summary>Provides an opportunity to clean-up resources.</summary>
        /// <param name="disposing">
        /// Whether the call is being made from an explicit call to 
        /// IDisposable.Dispose() rather than through the finalizer.
        /// </param>
        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);
            if (disposing)
            {
                this.xmlReader.Close();
            }
        }

        /// <summary>
        /// Get the resource referred by the uri in the payload
        /// </summary>
        /// <returns>resource referred by the uri in the payload.</returns>
        protected override string GetLinkUriFromPayload()
        {
            throw Error.NotImplemented();
        }

        /// <summary>Checks whether the specified item has a payload.</summary>
        /// <param name='item'>Item to check.</param>
        /// <returns>true if the item has content or links specified; false otherwise.</returns>
        private static bool HasContent(SyndicationItem item)
        {
            return item.Content != null || item.Links.Count > 0;
        }

        /// <summary>Gets the text for the type annotated on the specified <paramref name='item' />.</summary>
        /// <param name='item'>Item to read type from.</param>
        /// <returns>The text for the type annotated on the specified item; null if none is set.</returns>
        private static string SyndicationItemGetType(SyndicationItem item)
        {
            Debug.Assert(item != null, "item != null");
            SyndicationCategory category = item.Categories.Where(c => c.Scheme == XmlConstants.DataWebSchemeNamespace).FirstOrDefault();
            return (category == null) ? null : category.Name;
        }

        /// <summary>Reads a SyndicationFeed object from the specified XmlReader.</summary>
        /// <param name='formatter'>Formatter to use when reading content.</param>
        /// <param name='reader'>Read to read feed from.</param>
        /// <returns>A new SyndicationFeed instance.</returns>
        private static SyndicationFeed ReadSyndicationFeed(SyndicationFeedFormatter formatter, XmlReader reader)
        {
            Debug.Assert(formatter != null, "formatter != null");
            Debug.Assert(reader != null, "reader != null");

            try
            {
                formatter.ReadFrom(reader);
            }
            catch (XmlException exception)
            {
                throw DataServiceException.CreateBadRequestError(Strings.Syndication_ErrorReadingFeed(exception.Message), exception);
            }

            Debug.Assert(formatter.Feed != null, "formatter.Feed != null");
            return formatter.Feed;
        }

        /// <summary>Reads a SyndicationItem object from the specified XmlReader.</summary>
        /// <param name='formatter'>Formatter to use when reading content.</param>
        /// <param name='reader'>Read to read feed from.</param>
        /// <returns>A new SyndicationItem instance.</returns>
        private static SyndicationItem ReadSyndicationItem(SyndicationItemFormatter formatter, XmlReader reader)
        {
            Debug.Assert(formatter != null, "formatter != null");
            Debug.Assert(reader != null, "reader != null");

            try
            {
                formatter.ReadFrom(reader);
            }
            catch (XmlException exception)
            {
                throw DataServiceException.CreateBadRequestError(Strings.Syndication_ErrorReadingEntry(exception.Message), exception);
            }

            Debug.Assert(formatter.Item != null, "formatter.Item != null");
            return formatter.Item;
        }

        /// <summary>
        /// Read the link media type and validate for non open property types
        /// </summary>
        /// <param name="mediaType">media type as specified on the link element.</param>
        /// <param name="property">property which the link represents.</param>
        /// <returns>returns the type parameters specified in the media link.</returns>
        private static string ValidateTypeParameterForNonOpenTypeProperties(string mediaType, ResourceProperty property)
        {
            string typeParameterValue = null;
            if (!String.IsNullOrEmpty(mediaType))
            {
                string mime;
                Encoding encoding;
                KeyValuePair<string, string>[] contentTypeParameters = HttpProcessUtility.ReadContentType(mediaType, out mime, out encoding);
                if (mime != XmlConstants.MimeApplicationAtom)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequest_MimeTypeMustBeApplicationAtom(mime, XmlConstants.MimeApplicationAtom));
                }

                // If the type parameter is specified, make sure its correct. We do the validation for known properties here
                // and for open-properties, the validation is done if the link is expanded. Otherwise, there is no good way of
                // doing the validation.
                typeParameterValue = HttpProcessUtility.GetParameterValue(contentTypeParameters, XmlConstants.AtomTypeAttributeName);
                if (!String.IsNullOrEmpty(typeParameterValue) && property != null)
                {
                    if (property.Kind == ResourcePropertyKind.ResourceReference)
                    {
                        if (typeParameterValue != XmlConstants.AtomEntryElementName)
                        {
                            throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidTypeParameterSpecifiedInMimeType(typeParameterValue, XmlConstants.AtomEntryElementName));
                        }
                    }
                    else if (typeParameterValue != XmlConstants.AtomFeedElementName)
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidTypeParameterSpecifiedInMimeType(typeParameterValue, XmlConstants.AtomFeedElementName));
                    }
                }
            }

            return typeParameterValue;
        }

        /// <summary>
        /// Gets the XmlReader for the m:properties element under the atom:entry element
        /// </summary>
        /// <param name="item">item to read from</param>
        /// <returns>XmlReader for the m:properties element if found, null otherwise.</returns>
        private static XmlReader GetPropertiesReaderFromEntry(SyndicationItem item)
        {
            SyndicationElementExtension properties = item.ElementExtensions.Where(p => p.OuterName == XmlConstants.AtomPropertiesElementName && p.OuterNamespace == XmlConstants.DataWebMetadataNamespace).FirstOrDefault();
            if (properties != null)
            {
                return properties.GetReader();
            }

            return null;
        }

        /// <summary>
        /// Gets the XmlReader for the m:properties element under the atom:content element
        /// </summary>
        /// <param name="item">item to read from</param>
        /// <returns>XmlReader for the m:properties element if found, null otherwise.</returns>
        private static XmlReader GetPropertiesReaderFromContent(SyndicationItem item)
        {
            XmlSyndicationContent itemContent = item.Content as XmlSyndicationContent;
            XmlReader reader = null;
            if (itemContent != null)
            {
                string contentType = itemContent.Type;
                if (!WebUtil.CompareMimeType(contentType, XmlConstants.MimeApplicationXml))
                {
                    throw DataServiceException.CreateBadRequestError(
                        Strings.Syndication_EntryContentTypeUnsupported(contentType));
                }

                bool shouldDispose = false;
                try
                {
                    reader = itemContent.GetReaderAtContent();
                    WebUtil.XmlReaderEnsureElement(reader);
                    Debug.Assert(
                        reader.NodeType == XmlNodeType.Element,
                        reader.NodeType.ToString() + " == XmlNodeType.Element -- otherwise XmlSyndicationContent didn't see a 'content' tag");

                    reader.ReadStartElement(XmlConstants.AtomContentElementName, XmlConstants.AtomNamespace);
                    if (!reader.IsStartElement(XmlConstants.AtomPropertiesElementName, XmlConstants.DataWebMetadataNamespace))
                    {
                        shouldDispose = true;
                    }
                }
                catch
                {
                    shouldDispose = true;
                    throw;
                }
                finally
                {
                    if (shouldDispose)
                    {
                        WebUtil.Dispose(reader);
                        reader = null;
                    }
                }
            }

            return reader;
        }

        /// <summary>Applies the properties in the plain XML content to the specified resource.</summary>
        /// <param name="item">item to read from.</param>
        /// <param name='resourceType'>Type of resource whose values are being set.</param>
        /// <param name="propertiesApplied">Properties that have been applied to the <paramref name="resource"/></param>
        /// <param name='resource'>Target resource.</param>
        private void ApplyProperties(SyndicationItem item, ResourceType resourceType, EpmContentDeSerializer.EpmAppliedPropertyInfo propertiesApplied, object resource)
        {
            Debug.Assert(item != null, "item != null");
            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(propertiesApplied != null, "propertiesApplied != null");
            Debug.Assert(resource != null, "resource != null");

            using (XmlReader propertiesFromEntry = GetPropertiesReaderFromEntry(item))
            using (XmlReader propertiesFromContent = GetPropertiesReaderFromContent(item))
            {
                XmlReader reader = null;

                if (resourceType.IsMediaLinkEntry)
                {
                    if (propertiesFromContent != null)
                    {
                        // The server is expecting a MLE payload, however we found a <m:properties> element under the <atom:content> element.
                        // The client must assumed the resource type is a non-MLE type and serialized as such.
                        //
                        // We must throw here otherwise the client would falsely assume all the properties are correctly deserialized on
                        // the server where as in fact the properties element is under the wrong node.
                        throw DataServiceException.CreateBadRequestError(Strings.DataServiceException_GeneralError);
                    }

                    reader = propertiesFromEntry;
                }
                else
                {
                    if (propertiesFromEntry != null)
                    {
                        // The server is expecting a non-MLE payload, however we found a <m:properties> element under the <entry> element.
                        // The client must assumed the resource type is a MLE type and serialized as such.
                        //
                        // We must throw here otherwise the client would falsely assume all the properties are correctly deserialized on
                        // the server where as in fact the properties element is under the wrong node.
                        throw DataServiceException.CreateBadRequestError(Strings.DataServiceException_GeneralError);
                    }

                    reader = propertiesFromContent;
                }

                if (reader != null)
                {
                    reader.ReadStartElement(XmlConstants.AtomPropertiesElementName, XmlConstants.DataWebMetadataNamespace);
                    PlainXmlDeserializer.ApplyContent(this, reader, resourceType, resource, propertiesApplied, this.MaxObjectCount);
                }
            }
        }

        /// <summary>Reads the current object from the <paramref name="item"/>.</summary>
        /// <param name="segmentInfo">segmentinfo containing information about the current element that is getting processes</param>
        /// <param name="topLevel">true if the element currently pointed by the xml reader refers to a top level element</param>
        /// <param name="item">Item to read from.</param>
        /// <returns>returns the clr object with the data populated</returns>
        private object CreateObject(SegmentInfo segmentInfo, bool topLevel, SyndicationItem item)
        {
            Debug.Assert(item != null, "item != null");
            Debug.Assert(topLevel || !this.Update, "deep updates not supported");

            this.RecurseEnter();
            object result;

            // update the object count everytime you encounter a new resource
            this.CheckAndIncrementObjectCount();

            // Process the type annotation.
            ResourceType currentResourceType = this.GetResourceType(item, segmentInfo.TargetResourceType);
            if (currentResourceType.ResourceTypeKind != ResourceTypeKind.EntityType)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_OnlyEntityTypesMustBeSpecifiedInEntryElement(currentResourceType.FullName));
            }

            // We have the actual type info from the payload. Update the request/response DSV if any property is FF mapped with KeepInContent=false.
            this.UpdateAndCheckEpmRequestResponseDSV(currentResourceType, topLevel);

            // Get a resource cookie from the provider.
            ResourceSetWrapper container;
            if (segmentInfo.TargetKind == RequestTargetKind.OpenProperty)
            {
                // Open navigation properties are not supported on OpenTypes.
                throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(segmentInfo.Identifier));
            }
            else
            {
                Debug.Assert(segmentInfo.TargetKind == RequestTargetKind.Resource, "segmentInfo.TargetKind == RequestTargetKind.Resource");
                container = segmentInfo.TargetContainer;
            }

            DataServiceHostWrapper host = this.Service.OperationContext.Host;
            if (this.Update)
            {
                Debug.Assert(currentResourceType.ResourceTypeKind == ResourceTypeKind.EntityType, "only expecting entity types");

                // Only verify ETag if there is going to be some update applied (that's the idea)
                // In reality:
                //   - for normal entities (V1 compatible) - don't check ETags if there is no content element. (Same as in V1)
                //   - for V2 stuff - check ETags always as we can't tell if there's going to be something modified or not
                //       with EPM properties can be anywhere in the payload and thus even without content there still can be updates
                //       with MLE the properties are not in the content element but in their own element
                // It's hard to recognize if there's going to be update up front and so this below is an approximation
                //   which seems to be good enough. Note that if we add new ways of handling properties in the content
                //   the condition below might need to change.
                bool verifyETag = 
                    topLevel && 
                    (HasContent(item) || currentResourceType.HasEntityPropertyMappings || currentResourceType.IsMediaLinkEntry);
                bool replaceResource = topLevel && host.AstoriaHttpVerb == AstoriaVerbs.PUT;

                // if its a top level resource, then it cannot be null
                result = this.GetObjectFromSegmentInfo(currentResourceType, segmentInfo, verifyETag, topLevel /*checkForNull*/, replaceResource);
                if (this.Tracker != null)
                {
                    this.Tracker.TrackAction(result, container, UpdateOperations.Change);
                }
            }
            else
            {
                if (segmentInfo.TargetKind == RequestTargetKind.Resource)
                {
                    DataServiceConfiguration.CheckResourceRights(segmentInfo.TargetContainer, EntitySetRights.WriteAppend);
                }

                result = this.Updatable.CreateResource(container.Name, currentResourceType.FullName);
                if (this.Tracker != null)
                {
                    this.Tracker.TrackAction(result, container, UpdateOperations.Add);
                }
            }

            // Process the content in the entry.
            EpmContentDeSerializer.EpmAppliedPropertyInfo propertiesApplied = new EpmContentDeSerializer.EpmAppliedPropertyInfo();
            this.ApplyProperties(item, currentResourceType, propertiesApplied, result);

            // Perform application of epm properties here
            if (currentResourceType.HasEntityPropertyMappings)
            {
                new EpmContentDeSerializer(currentResourceType, result).DeSerialize(
                        item, 
                        new EpmContentDeSerializer.EpmContentDeserializerState { IsUpdateOperation = this.Update, Updatable = this.Updatable, Service = this.Service, PropertiesApplied = propertiesApplied });
            }

            // Process the links in the entry.
            foreach (SyndicationLink link in item.Links)
            {
                string navigationPropertyName = UriUtil.GetNameFromAtomLinkRelationAttribute(link.RelationshipType);

                if (null == navigationPropertyName)
                {
                    continue;
                }

                Deserializer.CheckForBindingInPutOperations(host.AstoriaHttpVerb);
                Debug.Assert(segmentInfo.TargetContainer != null, "segmentInfo.TargetContainer != null");
                this.ApplyLink(link, segmentInfo.TargetContainer, currentResourceType, result, navigationPropertyName);
            }

            this.RecurseLeave();
            return result;
        }

        /// <summary>Applies the information from a link to the specified resource.</summary>
        /// <param name='link'>LinkDescriptor with information to apply.</param>
        /// <param name="resourceSet">Set for the target resource.</param>
        /// <param name='resourceType'>Type for the target resource.</param>
        /// <param name='resource'>Target resource to which information will be applied.</param>
        /// <param name="propertyName">Name of the property that this link represents.</param>
        private void ApplyLink(SyndicationLink link, ResourceSetWrapper resourceSet, ResourceType resourceType, object resource, string propertyName)
        {
            Debug.Assert(link != null, "link != null");
            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(resource != null, "resource != null");

            ResourceProperty property = resourceType.TryResolvePropertyName(propertyName);
            if (property == null)
            {
                // Open navigation properties are not supported on OpenTypes
                throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(propertyName));
            }

            if (property.TypeKind != ResourceTypeKind.EntityType)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidNavigationPropertyName(propertyName, resourceType.FullName));
            }

            string typeParameterValue = ValidateTypeParameterForNonOpenTypeProperties(link.MediaType, property);
            LinkContent linkContent = this.HandleLinkContent(link, resource, resourceSet, resourceType, property, typeParameterValue, propertyName);

            #region Handle bind/unbind operation
            // If the href was specified empty or an empty inline element was specified, then we will set the 
            // reference to null - this helps in overrriding if there was a default non-value for this property
            // else if only link element was specified, and then href points to a single result, then we will
            // perform a bind operation
            if ((linkContent == LinkContent.NoInlineElementSpecified && link.Uri != null && String.IsNullOrEmpty(link.Uri.OriginalString)) ||
                linkContent == LinkContent.EmptyInlineElementSpecified)
            {
                // update the object count when you are performing a bind operation
                this.CheckAndIncrementObjectCount();
                if (property != null && property.Kind == ResourcePropertyKind.ResourceSetReference)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequest_CannotSetCollectionsToNull(propertyName));
                }

                // For open properties, we will assume that this is a reference property and set it to null
                this.Updatable.SetReference(resource, propertyName, null);
            }
            else if (linkContent == LinkContent.NoInlineElementSpecified && link.Uri != null && !String.IsNullOrEmpty(link.Uri.OriginalString))
            {
                // update the object count when you are performing a bind operation
                this.CheckAndIncrementObjectCount();

                // If the link points to a reference navigation property, then update the link
                Uri referencedUri = RequestUriProcessor.GetAbsoluteUriFromReference(link.Uri.OriginalString, this.Service.OperationContext);
                RequestDescription description = RequestUriProcessor.ProcessRequestUri(referencedUri, this.Service);
                if (!description.IsSingleResult)
                {
                    if (property != null && property.Kind == ResourcePropertyKind.ResourceReference)
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequest_LinkHrefMustReferToSingleResource(propertyName));
                    }

                    return;
                }

                // no need to check for null. For collection properties, they can never be null and that
                // check has been added below. For reference properties, if they are null, it means unbind
                // and hence no need to check for null.
                // Get the resource
                object targetResource = this.Service.GetResource(description, description.SegmentInfos.Length - 1, null);
                if (property.Kind == ResourcePropertyKind.ResourceReference)
                {
                    this.Updatable.SetReference(resource, propertyName, targetResource);
                }
                else
                {
                    WebUtil.CheckResourceExists(targetResource != null, description.LastSegmentInfo.Identifier);
                    this.Updatable.AddReferenceToCollection(resource, propertyName, targetResource);
                }
            }
            #endregion Handle bind/unbind operation
        }

        /// <summary>Gets the type attribute and resolves the type.</summary>
        /// <param name="item">Item from which type attribute needs to be read</param>
        /// <param name="expectedType">Expected base type for the item.</param>
        /// <returns>Resolved type.</returns>
        private ResourceType GetResourceType(SyndicationItem item, ResourceType expectedType)
        {
            Debug.Assert(item != null, "item != null");
            string typeName = SyndicationItemGetType(item);
            ResourceType resourceType;

            // If the type is not specified in the payload, we assume the type to be the expected type
            if (String.IsNullOrEmpty(typeName))
            {
                // check if the expected type takes part in inheritance
                resourceType = expectedType;
                if (this.Service.Provider.HasDerivedTypes(resourceType))
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequest_TypeInformationMustBeSpecifiedForInhertiance);
                }
            }
            else
            {
                // Otherwise, try and resolve the name specified in the payload
                resourceType = this.Service.Provider.TryResolveResourceType(typeName);
                if (resourceType == null)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidTypeName(typeName));
                }
            }

            return resourceType;
        }

        /// <summary>
        /// Handle the contents under the link element
        /// </summary>
        /// <param name="link">syndication link element</param>
        /// <param name="parentResource">parent resource which contains the link.</param>
        /// <param name="parentResourceSet">resource set of the parent resource</param>
        /// <param name="parentResourceType">resource type of the parent resource</param>
        /// <param name="property">property representing the link.</param>
        /// <param name="typeParameterValue">type parameter value as specified in the type attribute.</param>
        /// <param name="propertyName">name of the property that this link represents.</param>
        /// <returns>returns whether there are child elements under link element.</returns>
        private LinkContent HandleLinkContent(
            SyndicationLink link,
            object parentResource,
            ResourceSetWrapper parentResourceSet,
            ResourceType parentResourceType,
            ResourceProperty property,
            string typeParameterValue,
            string propertyName)
        {
            Debug.Assert(parentResource != null, "parent resource cannot be null");
            Debug.Assert(property != null, "property != null");
            Debug.Assert(link != null, "link != null");

            LinkContent linkContent = LinkContent.NoInlineElementSpecified;
            foreach (var e in link.ElementExtensions)
            {
                // link can contain other elements apart from the inline elements.
                if (e.OuterNamespace != XmlConstants.DataWebMetadataNamespace ||
                    e.OuterName != XmlConstants.AtomInlineElementName)
                {
                    continue;
                }

                // Deep payload cannot be specified for update
                if (this.Update)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequest_DeepUpdateNotSupported);
                }

                linkContent = LinkContent.EmptyInlineElementSpecified;
                using (XmlReader linkReader = e.GetReader())
                {
                    while (linkReader.Read())
                    {
                        if (linkReader.NodeType == XmlNodeType.Element)
                        {
                            string elementName = linkReader.LocalName;
                            string namespaceUri = linkReader.NamespaceURI;
                            if (namespaceUri != XmlConstants.AtomNamespace)
                            {
                                throw DataServiceException.CreateBadRequestError(
                                    Strings.BadRequest_InlineElementMustContainValidElement(
                                        elementName,
                                        XmlConstants.AtomInlineElementName,
                                        XmlConstants.AtomFeedElementName,
                                        XmlConstants.AtomEntryElementName));
                            }

                            ResourceSetWrapper targetSet = this.Service.Provider.GetContainer(parentResourceSet, parentResourceType, property);
                            if (targetSet == null)
                            {
                                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidPropertyNameSpecified(propertyName, parentResourceType.FullName));
                            }

                            // FeatureVersion needs to be 2.0 if any of the property in the types contained in the resource set has KeepInContent false
                            this.RequestDescription.UpdateAndCheckEpmFeatureVersion(targetSet, this.Service);

                            linkContent = LinkContent.InlineElementContainsData;
                            if (elementName == XmlConstants.AtomEntryElementName)
                            {
                                if (property.Kind != ResourcePropertyKind.ResourceReference)
                                {
                                    throw DataServiceException.CreateBadRequestError(Strings.Syndication_EntryElementForReferenceProperties(e.OuterName, propertyName));
                                }

                                // Make sure if the media type is specified. If its specified, it should better be link
                                SyndicationItem propertyItem;
                                propertyItem = ReadSyndicationItem(this.factory.CreateSyndicationItemFormatter(), linkReader);

                                SegmentInfo propertySegment = CreateSegment(property, propertyName, targetSet, true /* singleResult */);
                                Debug.Assert(propertySegment.TargetKind != RequestTargetKind.OpenProperty, "Open navigation properties are not supported on OpenTypes.");

                                object propertyValue = this.CreateObject(propertySegment, false /* topLevel */, propertyItem);
                                this.Updatable.SetReference(parentResource, propertyName, propertyValue);
                            }
                            else if (elementName == XmlConstants.AtomFeedElementName)
                            {
                                    if (property.Kind != ResourcePropertyKind.ResourceSetReference)
                                    {
                                        throw DataServiceException.CreateBadRequestError(Strings.Syndication_FeedElementForCollections(e.OuterName, propertyName));
                                    }

                                SyndicationFeed propertyFeed;
                                propertyFeed = ReadSyndicationFeed(this.factory.CreateSyndicationFeedFormatter(), linkReader);

                                SegmentInfo propertySegment = CreateSegment(property, propertyName, targetSet, false /* singleResult */);
                                Debug.Assert(propertySegment.TargetKind != RequestTargetKind.OpenProperty, "Open navigation properties are not supported on OpenTypes.");

                                foreach (SyndicationItem item in propertyFeed.Items)
                                {
                                    object propertyValue = this.CreateObject(propertySegment, false /* topLevel */, item);
                                    if (propertyValue == null)
                                    {
                                        if (propertySegment.ProjectedProperty != null &&
                                            propertySegment.ProjectedProperty.Kind == ResourcePropertyKind.ResourceSetReference)
                                        {
                                            throw DataServiceException.CreateBadRequestError(
                                                Strings.BadRequest_CannotSetCollectionsToNull(propertyName));
                                        }
                                    }

                                    Debug.Assert(
                                            propertySegment.TargetSource == RequestTargetSource.Property &&
                                            propertySegment.TargetKind == RequestTargetKind.Resource &&
                                            propertySegment.SingleResult == false,
                                            "Must be navigation set property.");

                                    this.Updatable.AddReferenceToCollection(parentResource, propertyName, propertyValue);
                                }
                            }
                            else
                            {
                                throw DataServiceException.CreateBadRequestError(
                                    Strings.BadRequest_InlineElementMustContainValidElement(
                                        elementName,
                                        XmlConstants.AtomInlineElementName,
                                        XmlConstants.AtomFeedElementName,
                                        XmlConstants.AtomEntryElementName));
                            }
                        }
                    }
                }
            }

            return linkContent;
        }
    }
}
