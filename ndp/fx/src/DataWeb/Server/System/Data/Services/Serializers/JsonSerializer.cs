//---------------------------------------------------------------------
// <copyright file="JsonSerializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a serializer for the Json format.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Text;

    /// <summary>
    /// Provides support for serializing responses in JSON format.
    /// </summary>
    /// <remarks>For more information, see http://www.json.org/.</remarks>
    [DebuggerDisplay("Json={absoluteServiceUri}")]
    internal sealed class JsonSerializer : Serializer
    {
        /// <summary>
        /// The response version threshold to turn on response format v2
        /// </summary>
        private static readonly Version JsonResponseVersion1 = new Version(1, 0, 0);

        /// <summary>JsonWriter to write out strings in Json format.</summary>
        private readonly JsonWriter writer;

        /// <summary>
        /// Initializes a new <see cref="JsonSerializer"/>, ready to write out a description.
        /// </summary>
        /// <param name="requestDescription">Description for the requested results.</param>
        /// <param name="output">Stream to which output should be sent.</param>
        /// <param name="absoluteServiceUri">Absolute URI to the service entry point.</param>
        /// <param name="service">Service with configuration and provider from which metadata should be gathered.</param>
        /// <param name="encoding">Text encoding for the response.</param>
        /// <param name="httpETagHeaderValue">HTTP ETag header value.</param>
        internal JsonSerializer(
            RequestDescription requestDescription, 
            Stream output, 
            Uri absoluteServiceUri,
            IDataService service,
            Encoding encoding,
            string httpETagHeaderValue)
            : base(requestDescription, absoluteServiceUri, service, httpETagHeaderValue)
        {
            Debug.Assert(output != null, "output != null");
            Debug.Assert(encoding != null, "encoding != null");

            this.writer = new JsonWriter(new StreamWriter(output, encoding));
        }

        /// <summary>
        /// The Json Format version to use for this response
        /// </summary>
        private int JsonFormatVersion
        {
            get
            {
                return this.RequestDescription.ResponseVersion > JsonResponseVersion1 ? 2 : 1;
            }
        }

        /// <summary>Serializes exception information.</summary>
        /// <param name="args">Description of exception to serialize.</param>
        public override void WriteException(HandleExceptionArgs args)
        {
            ErrorHandler.SerializeJsonError(args, this.writer);
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

            bool needPop = this.PushSegmentForRoot();
            this.writer.StartObjectScope(); // {
            this.writer.WriteDataWrapper(); // "d" :
            this.WriteElementWithName(expanded, element, this.RequestDescription.ContainerName, this.RequestDescription.ResultUri, true /*topLevel*/);
            this.writer.EndScope();         // }
            this.PopSegmentName(needPop);
        }

        /// <summary>Writes multiple top-level elements, possibly none.</summary>
        /// <param name="expanded">Expanded properties for the result.</param>
        /// <param name="elements">Enumerator for elements to write.</param>
        /// <param name="hasMoved">Whether <paramref name="elements"/> was succesfully advanced to the first element.</param>
        protected override void WriteTopLevelElements(IExpandedResult expanded, IEnumerator elements, bool hasMoved)
        {
            Debug.Assert(elements != null, "elements != null");
            Debug.Assert(!this.RequestDescription.IsSingleResult, "!this.RequestDescription.SingleResult");

            bool needPop = this.PushSegmentForRoot();
            Uri parentUri = this.RequestDescription.ResultUri;
            this.writer.StartObjectScope();
            this.writer.WriteDataWrapper();

            if (this.JsonFormatVersion >= 2)
            {
                // Json Format V2:
                // { "d" :
                //      {
                //          "__results": [],
                //          "__count": 0,
                //          "__next" : uri
                //      }
                // }
                this.writer.StartObjectScope();
                this.writer.WriteDataArrayName();
            }

            // Json Format V1:
            // { "d" : [] }
            this.writer.StartArrayScope();

            object lastObject = null;
            IExpandedResult lastExpandedSkipToken = null;
            while (hasMoved)
            {
                object o = elements.Current;
                IExpandedResult skipToken = this.GetSkipToken(expanded);
                if (o != null)
                {
                    IExpandedResult expandedObject = o as IExpandedResult;
                    if (expandedObject != null)
                    {
                        expanded = expandedObject;
                        o = GetExpandedElement(expanded);
                        skipToken = this.GetSkipToken(expanded);
                    }
                
                    this.WriteElementWithName(expanded, o, null, parentUri, false /*topLevel*/);
                }

                hasMoved = elements.MoveNext();
                lastObject = o;
                lastExpandedSkipToken = skipToken;
            }
            
            this.writer.EndScope();     // end data array

            if (this.JsonFormatVersion >= 2)
            {
                // $count=inline support
                if (this.RequestDescription.CountOption == RequestQueryCountOption.Inline)
                {
                    this.WriteRowCount();
                }

                // After looping through the objects in the sequence, decide if we need to write the next
                // page link and if yes, write it by invoking the delegate
                if (this.NeedNextPageLink(elements))
                {
                    this.WriteNextPageLink(lastObject, lastExpandedSkipToken, parentUri);
                }

                this.writer.EndScope();     // end object scope
            }

            this.writer.EndScope();     // end "d" wrapper

            this.PopSegmentName(needPop);
        }

        /// <summary>Writes the Row count</summary>
        protected override void WriteRowCount()
        {
            this.writer.WriteName(XmlConstants.JsonRowCountString);
            this.writer.WriteValue(RequestDescription.CountValue);
        }

        /// <summary>Writes out the uri for the given element.</summary>
        /// <param name="element">element whose uri needs to be written out.</param>
        /// <remarks>This method accounts for a written entity on the current segment.</remarks>
        protected override void WriteLink(object element)
        {
            Debug.Assert(element != null, "element != null");
            Uri uri = Serializer.GetUri(element, this.Provider, this.CurrentContainer, this.AbsoluteServiceUri);
            this.writer.StartObjectScope();     // {
            this.writer.WriteDataWrapper();     // "d" :
            this.WriteLinkObject(uri);
            this.writer.EndScope();             // }
        }

        /// <summary>
        /// Write out the uri for the given elements.
        /// </summary>
        /// <param name="elements">elements whose uri need to be writtne out</param>
        /// <param name="hasMoved">the current state of the enumerator.</param>
        /// <remarks>This method accounts for each link as a written entity on the current segment.</remarks>
        protected override void WriteLinkCollection(IEnumerator elements, bool hasMoved)
        {
            this.writer.StartObjectScope();
            this.writer.WriteDataWrapper();

            if (this.JsonFormatVersion >= 2)
            {
                // see comments in WriteTopLevelElements
                this.writer.StartObjectScope();
                this.writer.WriteDataArrayName();
            }

            this.writer.StartArrayScope();

            object lastObject = null;
            IExpandedResult lastExpandedSkipToken = null;
            while (hasMoved)
            {
                object o = elements.Current;
                IExpandedResult skipToken = null;
                if (o != null)
                {
                    IExpandedResult expanded = o as IExpandedResult;
                    if (expanded != null)
                    {
                        o = GetExpandedElement(expanded);
                        skipToken = this.GetSkipToken(expanded);
                    }

                    Uri uri = Serializer.GetUri(o, this.Provider, this.CurrentContainer, this.AbsoluteServiceUri);
                    this.WriteLinkObject(uri);
                }
                
                hasMoved = elements.MoveNext();
                lastObject = o;
                lastExpandedSkipToken = skipToken;
            }

            this.writer.EndScope();

            if (this.JsonFormatVersion >= 2)
            {
                // $count=inline support
                if (this.RequestDescription.CountOption == RequestQueryCountOption.Inline)
                {
                    this.WriteRowCount();
                }

                if (this.NeedNextPageLink(elements))
                {
                    this.WriteNextPageLink(lastObject, lastExpandedSkipToken, this.RequestDescription.ResultUri);
                }

                this.writer.EndScope();
            }

            this.writer.EndScope();
        }

        /// <summary>Gets the tag name to be used when writing the specified type.</summary>
        /// <param name='resourceType'>Type of resource to be written.</param>
        /// <returns>The tag name to be used when writing he specified type.</returns>
        private static string GetTagNameForType(ResourceType resourceType)
        {
            Debug.Assert(resourceType != null, "resourceType != null");

            ResourceType elementType = WebUtil.GetRootType(resourceType);
            string tagName = elementType.Name;
            if (!System.Xml.XmlReader.IsName(tagName))
            {
                tagName = System.Xml.XmlConvert.EncodeName(elementType.Name);
            }

            return tagName;
        }

        /// <summary>
        /// Writes the next page link to the current xml writer corresponding to the feed
        /// </summary>
        /// <param name="lastElement">Object that will contain the keys for skip token</param>
        /// <param name="skipTokenExpandedResult">The <see cref="IExpandedResult"/> of the $skiptoken property of the object being written</param>
        /// <param name="absoluteUri">Absolute URI for the result</param>
        private void WriteNextPageLink(object lastElement, IExpandedResult skipTokenExpandedResult, Uri absoluteUri)
        {
            this.writer.WriteName(XmlConstants.JsonNextString);
            this.writer.WriteValue(this.GetNextLinkUri(lastElement, skipTokenExpandedResult, absoluteUri));
        }

        /// <summary>Write the link uri in the payload.</summary>
        /// <param name="uri">uri which needs to be written.</param>
        /// <remarks>This method accounts for a written entity on the current segment.</remarks>
        private void WriteLinkObject(Uri uri)
        {
            this.IncrementSegmentResultCount();
            this.writer.StartObjectScope();
            this.writer.WriteName(XmlConstants.UriElementName);
            this.writer.WriteValue(uri.AbsoluteUri);
            this.writer.EndScope();
        }

        /// <summary>
        /// Attempts to convert the specified primitive value to a serializable string.
        /// </summary>
        /// <param name="value">Non-null value to convert.</param>
        private void WritePrimitiveValue(object value)
        {
            Debug.Assert(value != null, "value != null");

            Type valueType = value.GetType();
            if (typeof(String) == valueType)
            {
                this.writer.WriteValue((string)value);
            }
            else if (typeof(System.Xml.Linq.XElement) == valueType)
            {
                this.writer.WriteValue(((System.Xml.Linq.XElement)value).ToString(System.Xml.Linq.SaveOptions.None));
            }
            else if (typeof(SByte) == valueType)
            {
                this.writer.WriteValue((SByte)value);
            }
            else if (typeof(Boolean) == value.GetType())
            {
                this.writer.WriteValue((bool)value);
            }
            else if (typeof(Byte) == value.GetType())
            {
                this.writer.WriteValue((byte)value);
            }
            else if (typeof(DateTime) == value.GetType())
            {
                this.writer.WriteValue((DateTime)value);
            }
            else if (typeof(Decimal) == value.GetType())
            {
                this.writer.WriteValue((Decimal)value);
            }
            else if (typeof(Double) == value.GetType())
            {
                this.writer.WriteValue((Double)value);
            }
            else if (typeof(Guid) == value.GetType())
            {
                this.writer.WriteValue((Guid)value);
            }
            else if (typeof(Int16) == value.GetType())
            {
                this.writer.WriteValue((Int16)value);
            }
            else if (typeof(Int32) == value.GetType())
            {
                this.writer.WriteValue((Int32)value);
            }
            else if (typeof(Int64) == value.GetType())
            {
                this.writer.WriteValue((Int64)value);
            }
            else if (typeof(Single) == value.GetType())
            {
                this.writer.WriteValue((Single)value);
            }
            else if (typeof(byte[]) == value.GetType())
            {
                byte[] byteArray = (byte[])value;
                string result = Convert.ToBase64String(byteArray, Base64FormattingOptions.None);
                this.writer.WriteValue(result);
            }
            else
            {
                Debug.Assert(typeof(System.Data.Linq.Binary) == value.GetType(), "typeof(Binary) == value.GetType() (" + value.GetType() + ")");
                this.WritePrimitiveValue(((System.Data.Linq.Binary)value).ToArray());
            }
        }

        /// <summary>Writes the ID and uri path information for the specified resource.</summary>
        /// <param name='resource'>Resource for which URI information should be written.</param>
        /// <param name="resourceType">type of the resource</param>
        /// <param name="uriPath">uri of the resource for which the metadata is getting written</param>
        /// <returns>The tag name for the resource that was written.</returns>
        private string WriteMetadataObject(object resource, ResourceType resourceType, Uri uriPath)
        {
            Debug.Assert(resource != null, "resource != null");
            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(uriPath != null || resourceType.ResourceTypeKind == ResourceTypeKind.ComplexType, "uri can be null for complex types");

            this.writer.WriteName(XmlConstants.JsonMetadataString);
            this.writer.StartObjectScope();

            // Write uri value only for entity types
            if (resourceType.ResourceTypeKind == ResourceTypeKind.EntityType)
            {
                this.writer.WriteName(XmlConstants.JsonUriString);
                this.writer.WriteValue(uriPath.AbsoluteUri);

                // Write the etag property, if the type has etag properties
                string etag = this.GetETagValue(resource);
                if (etag != null)
                {
                    this.writer.WriteName(XmlConstants.JsonETagString);
                    this.writer.WriteValue(etag);
                }
            }

            this.writer.WriteName(XmlConstants.JsonTypeString);

            // For generic types, we need to remove the assembly qualified names for the argument types
            this.writer.WriteValue(resourceType.FullName);

            if (resourceType.IsMediaLinkEntry)
            {
                string editMediaUri = DataServiceStreamProviderWrapper.GetStreamEditMediaUri(uriPath.AbsoluteUri);

                string mediaETag;
                Uri readStreamUri;
                string mediaContentType;
                this.Service.StreamProvider.GetStreamDescription(resource, this.Service.OperationContext, uriPath.AbsoluteUri, out mediaETag, out readStreamUri, out mediaContentType);

                this.writer.WriteName(XmlConstants.JsonEditMediaString);
                this.writer.WriteValue(editMediaUri);
                
                this.writer.WriteName(XmlConstants.JsonMediaSrcString);
                this.writer.WriteValue(readStreamUri.OriginalString);
                
                this.writer.WriteName(XmlConstants.JsonContentTypeString);
                this.writer.WriteValue(mediaContentType);

                if (!string.IsNullOrEmpty(mediaETag))
                {
                    this.writer.WriteName(XmlConstants.JsonMediaETagString);
                    this.writer.WriteValue(mediaETag);
                }
            }

            this.writer.EndScope();

            return GetTagNameForType(resourceType);
        }

        /// <summary>Writes an element with an optional specified name.</summary>
        /// <param name="expanded">Expanded properties for the result.</param>
        /// <param name='element'>Element to write, possibly null.</param>
        /// <param name='elementName'>Name of element to write, possibly null.</param>
        /// <param name='elementUri'>URI of element to write.</param>
        /// <param name="topLevel">whether the element is a top level element or not.</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801", MessageId = "elementName", Justification = "Pending review")]
        private void WriteElementWithName(IExpandedResult expanded, object element, string elementName, Uri elementUri, bool topLevel)
        {
            Debug.Assert(elementName == null || elementName.Length > 0, "elementName == null || elementName.Length > 0");
            Debug.Assert(elementUri != null, "elementUri != null");

            this.RecurseEnter();
            try
            {
                if (element == null)
                {
                    if (topLevel)
                    {
                        this.writer.StartObjectScope();
                        this.writer.WriteName(elementName);
                        this.WriteNullValue();
                        this.writer.EndScope();
                    }
                    else
                    {
                        this.WriteNullValue();
                    }
                }
                else
                {
                    IEnumerable enumerableElement;
                    if (WebUtil.IsElementIEnumerable(element, out enumerableElement))
                    {
                        if (this.JsonFormatVersion >= 2)
                        {
                            // JSON V2 Nested Collections:
                            // {
                            //      "results": []
                            //      "__next" : uri
                            // }
                            this.writer.StartObjectScope();
                            this.writer.WriteDataArrayName();
                        }

                        this.writer.StartArrayScope();
                        IEnumerator elements = enumerableElement.GetEnumerator();

                        try
                        {
                            IExpandedResult expandedEnumerator = elements as IExpandedResult;
                            object lastObject = null;
                            IExpandedResult lastExpandedSkipToken = null;

                            while (elements.MoveNext())
                            {
                                object elementInCollection = elements.Current;
                                IExpandedResult skipToken = this.GetSkipToken(expandedEnumerator);
                                if (elementInCollection != null)
                                {
                                    IExpandedResult expandedElementInCollection = elementInCollection as IExpandedResult;
                                    if (expandedElementInCollection != null)
                                    {
                                        expandedEnumerator = expandedElementInCollection;
                                        elementInCollection = GetExpandedElement(expandedEnumerator);
                                        skipToken = this.GetSkipToken(expandedEnumerator);
                                    }

                                    this.WriteElementWithName(expandedEnumerator, elementInCollection, null, elementUri, false /*topLevel*/);
                                }

                                lastObject = elementInCollection;
                                lastExpandedSkipToken = skipToken;
                            }

                            this.writer.EndScope(); // array scope

                            if (this.JsonFormatVersion >= 2)
                            {
                                // After looping through the objects in the sequence, decide if we need to write the next
                                // page link and if yes, write it by invoking the delegate
                                if (this.NeedNextPageLink(elements))
                                {
                                    this.WriteNextPageLink(lastObject, lastExpandedSkipToken, elementUri);
                                }

                                this.writer.EndScope(); // expanded object scope
                            }
                        }
                        finally
                        {
                            WebUtil.Dispose(elements);
                        }

                        return;
                    }

                    ResourceType resourceType = WebUtil.GetResourceType(this.Provider, element);
                    if (resourceType == null)
                    {
                        // Skip this element.
                        return;
                    }

                    switch (resourceType.ResourceTypeKind)
                    {
                        case ResourceTypeKind.ComplexType:
                            if (topLevel)
                            {
                                this.writer.StartObjectScope();
                                this.writer.WriteName(elementName);
                            }

                            // Non-value complex types may form a cycle.
                            // PERF: we can keep a single element around and save the HashSet initialization
                            // until we find a second complex type - this saves the allocation on trees
                            // with shallow (single-level) complex types.
                            if (this.AddToComplexTypeCollection(element))
                            {
                                this.WriteComplexTypeProperties(element, resourceType, elementUri);
                                this.RemoveFromComplexTypeCollection(element);
                            }
                            else
                            {
                                throw new InvalidOperationException(Strings.Serializer_LoopsNotAllowedInComplexTypes(elementName));
                            }

                            if (topLevel)
                            {
                                this.writer.EndScope();
                            }

                            break;
                        case ResourceTypeKind.EntityType:
                            this.IncrementSegmentResultCount();
                            this.writer.StartObjectScope();
                            Uri entityUri = Serializer.GetUri(element, this.Provider, this.CurrentContainer, this.AbsoluteServiceUri);
                            this.WriteMetadataObject(element, resourceType, entityUri);
                            this.WriteResourceProperties(expanded, element, resourceType, entityUri);
                            this.writer.EndScope();
                            break;
                        default:
                            Debug.Assert(resourceType.ResourceTypeKind == ResourceTypeKind.Primitive, "resourceType.ResourceTypeKind == ResourceTypeKind.Primitive");
                            if (topLevel)
                            {
                                this.writer.StartObjectScope();
                                this.writer.WriteName(elementName);
                                this.WritePrimitiveValue(element);
                                this.writer.EndScope();
                            }
                            else
                            {
                                this.WritePrimitiveValue(element);
                            }

                            break;
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

        /// <summary>Writes all the properties of the specified resource.</summary>
        /// <param name="expanded">Expanded properties for the result.</param>
        /// <param name="resource">Resource with properties to write out.</param>
        /// <param name="resourceType">Type for the specified resource (saves the lookup in this method).</param>
        /// <param name="uri">uri of the resource whose properties are getting written</param>
        private void WriteResourceProperties(IExpandedResult expanded, object resource, ResourceType resourceType, Uri uri)
        {
            Debug.Assert(resource != null, "resource != null");
            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(resourceType.ResourceTypeKind == ResourceTypeKind.EntityType, "resource must be entity type");
            Debug.Assert(uri != null, "uri != null");

            this.WriteObjectProperties(expanded, resource, resourceType, uri, true);
        }

        /// <summary>Writes all the properties of the specified complex type.</summary>
        /// <param name="complexObject">Object of a complex type with properties to be written out.</param>
        /// <param name="resourceType">resource type representing the current object</param>
        /// <param name="parentUri">uri of the complex type whose properties needs to be written</param>
        private void WriteComplexTypeProperties(object complexObject, ResourceType resourceType, Uri parentUri)
        {
            Debug.Assert(complexObject != null, "complexObject != null");
            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(resourceType.ResourceTypeKind == ResourceTypeKind.ComplexType, "resource must be complex type");

            this.writer.StartObjectScope();
            this.WriteMetadataObject(complexObject, resourceType, null);
            this.WriteObjectProperties(null, complexObject, resourceType, parentUri, false);
            this.writer.EndScope();
        }

        /// <summary>Writes a JSON _deferred element.</summary>
        /// <param name="uri">uri of the element which is getting deferred</param>
        private void WriteDeferredContentElement(Uri uri)
        {
            Debug.Assert(uri != null, "uri != null");

            this.writer.StartObjectScope();
            this.writer.WriteName(XmlConstants.JsonDeferredString);
            this.writer.StartObjectScope();
            this.writer.WriteName(XmlConstants.JsonUriString);
            this.writer.WriteValue(uri.AbsoluteUri);
            this.writer.EndScope();
            this.writer.EndScope();
        }

        /// <summary>Writes an attribute to indicate that there is no value.</summary>
        private void WriteNullValue()
        {
            this.writer.WriteValue((string)null);
        }

        /// <summary>Writes a single declared property of the specified resource or complex object</summary>
        /// <param name="expanded">Expanded properties for the result/</param>
        /// <param name="customObject">Resource or complex object with property to write out.</param>
        /// <param name="property">The resource property to write out.</param>
        /// <param name="parentUri">Uri of the object whose properties are getting written.</param>
        private void WriteObjectDeclaredProperty(IExpandedResult expanded, object customObject, ResourceProperty property, Uri parentUri)
        {
            string propertyName = property.Name;
            this.writer.WriteName(propertyName);

            // For any navigation property, we just stick the deferred element with the uri
            // This uri is different from the canonical uri: we just append the property name
            // to the parent uri. We don't want to analyze the nav property value in either case
            bool mayDefer = property.TypeKind == ResourceTypeKind.EntityType;
            if (mayDefer && !this.ShouldExpandSegment(propertyName))
            {
                this.WriteDeferredContentElement(Serializer.AppendEntryToUri(parentUri, propertyName));
            }
            else
            {
                object propertyValue;
                IExpandedResult expandedValue = null;
                if (mayDefer)
                {
                    propertyValue = GetExpandedProperty(this.Provider, expanded, customObject, property);
                    expandedValue = propertyValue as IExpandedResult;
                    if (expandedValue != null)
                    {
                        propertyValue = GetExpandedElement(expandedValue);
                    }
                }
                else
                {
                    propertyValue = GetExpandedProperty(this.Provider, null, customObject, property);
                }

                if (propertyValue == null || propertyValue == DBNull.Value)
                {
                    propertyValue = null;
                }

                bool needPop = this.PushSegmentForProperty(property);
                this.WriteElementWithName(expandedValue, propertyValue, propertyName, Serializer.AppendEntryToUri(parentUri, propertyName), false /*topLevel*/);
                this.PopSegmentName(needPop);
            }
        }

        /// <summary>Writes a single open property of a resource or complex object.</summary>
        /// <param name="propertyName">The name of the property to write.</param>
        /// <param name="propertyValue">The value of the property to write.</param>
        /// <param name="parentUri">Uri of the object whose properties are getting written.</param>
        private void WriteObjectOpenProperty(string propertyName, object propertyValue, Uri parentUri)
        {
            IExpandedResult expandedValue = propertyValue as IExpandedResult;
            if (expandedValue != null)
            {
                propertyValue = GetExpandedElement(expandedValue);
            }

            ResourceType propertyResourceType;

            if (propertyValue == null || propertyValue == DBNull.Value)
            {
                propertyValue = null;
                propertyResourceType = ResourceType.PrimitiveStringResourceType;
            }
            else
            {
                // We need to ensure that strings and other primitive types which implement IEnumerable
                // are not considered as containing elements.
                propertyResourceType = WebUtil.GetResourceType(this.Provider, propertyValue);
                if (propertyResourceType == null)
                {
                    // Black-listed types are not supported.
                    return;
                }
            }

            this.writer.WriteName(propertyName);

            bool needPop = this.PushSegmentForOpenProperty(propertyName, propertyResourceType);
            this.WriteElementWithName(expandedValue, propertyValue, propertyName, Serializer.AppendEntryToUri(parentUri, propertyName), false /*topLevel*/);
            this.PopSegmentName(needPop);
        }

        /// <summary>Writes all the properties of the specified resource or complex object.</summary>
        /// <param name="expanded">Expanded properties for the result.</param>
        /// <param name="customObject">Resource or complex object with properties to write out.</param>
        /// <param name="resourceType">Resource Type representing the given object instance.</param>
        /// <param name="parentUri">uri of the object whose properties are getting written</param>
        /// <param name="objectIsResource">true if the specified object is a resource; false otherwise.</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801", MessageId = "objectIsResource", Justification = "Pending review")]
        private void WriteObjectProperties(IExpandedResult expanded, object customObject, ResourceType resourceType, Uri parentUri, bool objectIsResource)
        {
            Debug.Assert(customObject != null, "customObject != null");
            Debug.Assert(resourceType != null, "customObjectType != null");
            Debug.Assert(
                ((customObject is IProjectedResult) && (resourceType.FullName == ((IProjectedResult)customObject).ResourceTypeName)) ||
                    (resourceType.InstanceType.IsAssignableFrom(customObject.GetType())),
                "The type of the object doesn't match the resource type specified.");
            Debug.Assert(parentUri != null, "parentUri != null");
            Debug.Assert(resourceType.ResourceTypeKind != ResourceTypeKind.Primitive, "resourceType.ResourceTypeKind == ResourceTypeKind.Primitive");

            // We should throw while if there are navigation properties in the derived entity type
            if (this.CurrentContainer != null && this.Provider.IsEntityTypeDisallowedForSet(this.CurrentContainer, resourceType))
            {
                throw new InvalidOperationException(Strings.BaseServiceProvider_NavigationPropertiesOnDerivedEntityTypesNotSupported(resourceType.FullName, this.CurrentContainer.Name));
            }

            IEnumerable<ProjectionNode> projectionNodes = null;
            if (resourceType.ResourceTypeKind == ResourceTypeKind.EntityType)
            {
                projectionNodes = this.GetProjections();
            }

            if (projectionNodes == null)
            {
                foreach (ResourceProperty property in this.Provider.GetResourceProperties(this.CurrentContainer, resourceType))
                {
                    Debug.Assert(
                        objectIsResource || !property.IsOfKind(ResourcePropertyKind.Key),
                        "objectIsResource || property.Kind != ResourcePropertyKind.KeyPrimitive - complex types shouldn't have key properties");

                    this.WriteObjectDeclaredProperty(expanded, customObject, property, parentUri);
                }

                if (resourceType.IsOpenType)
                {
                    foreach (var pair in this.Provider.GetOpenPropertyValues(customObject))
                    {
                        this.WriteObjectOpenProperty(pair.Key, pair.Value, parentUri);
                    }
                }
            }
            else
            {
                foreach (ProjectionNode projectionNode in projectionNodes)
                {
                    string propertyName = projectionNode.PropertyName;
                    ResourceProperty property = resourceType.TryResolvePropertyName(propertyName);

                    if (property != null)
                    {
                        if (property.TypeKind != ResourceTypeKind.EntityType ||
                            this.Provider.GetResourceProperties(this.CurrentContainer, resourceType).Contains(property))
                        {
                            this.WriteObjectDeclaredProperty(expanded, customObject, property, parentUri);
                        }
                    }
                    else
                    {
                        object propertyValue = WebUtil.GetPropertyValue(this.Provider, customObject, resourceType, null, propertyName);
                        this.WriteObjectOpenProperty(propertyName, propertyValue, parentUri);
                    }
                }
            }
        }
    }
}
