//---------------------------------------------------------------------
// <copyright file="PlainXmlDeserializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a deserializer for plain XML content.
// </summary>
//
// @owner Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    using System;
    using System.Data.Services.Common;
    using System.Data.Services.Parsing;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml;

    /// <summary>Provides a deserializer for plain XML content.</summary>
    internal class PlainXmlDeserializer : Deserializer
    {
        /// <summary>reader to read xml from the request stream</summary>
        private readonly XmlReader xmlReader;

        /// <summary>Whether <see cref="xmlReader"/> is owned and should be disposed of.</summary>
        private readonly bool xmlReaderOwned;

        /// <summary>Properties already applied based on content</summary>
        private readonly EpmContentDeSerializer.EpmAppliedPropertyInfo propertiesApplied;

        /// <summary>Prefix of the path to use in <see cref="propertiesApplied"/></summary>
        private String currentPathPrefix;

        /// <summary>Initializes a new <see cref="PlainXmlDeserializer"/> for the specified stream.</summary>
        /// <param name="stream">Input stream reader from which POX content must be read.</param>
        /// <param name="encoding">Encoding to use for the stream (null to auto-discover).</param>
        /// <param name="dataService">Data service for which the deserializer will act.</param>
        /// <param name="update">Indicates whether this is a PUT operation (rather than POST).</param>
        /// <param name="tracker">Tracker to use for modifications.</param>
        internal PlainXmlDeserializer(Stream stream, Encoding encoding, IDataService dataService, bool update, UpdateTracker tracker)
            : base(update, dataService, tracker)
        {
            Debug.Assert(stream != null, "stream != null");
            this.xmlReader = XmlUtil.CreateXmlReader(stream, encoding);
            this.xmlReaderOwned = true;
        }

        /// <summary>Initializes a new <see cref="PlainXmlDeserializer"/> based on the settings for another one.</summary>
        /// <param name="reader">Reader for content.</param>
        /// <param name="deserializer">Parent deserializer.</param>
        /// <param name="propertiesApplied">Properties already applied based on content</param>
        internal PlainXmlDeserializer(XmlReader reader, Deserializer deserializer, EpmContentDeSerializer.EpmAppliedPropertyInfo propertiesApplied)
            : base(deserializer)
        {
            Debug.Assert(reader != null, "reader != null");
            this.xmlReader = reader;
            Debug.Assert(propertiesApplied != null, "Requires valid collection for applied properties");
            this.propertiesApplied = propertiesApplied;
            this.currentPathPrefix = String.Empty;

            // this.xmlReaderOwned = false;
        }

        /// <summary>Returns the content format for the deserializer.</summary>
        protected override ContentFormat ContentFormat
        {
            get
            {
                return ContentFormat.PlainXml;
            }
        }

        /// <summary>Applies properties from the reader to the specified resource.</summary>
        /// <param name="deserializer">Deserializer which is driving the <paramref name="reader"/>.</param>
        /// <param name='reader'>XmlReader to read from.</param>
        /// <param name='resourceType'>Type of resource.</param>
        /// <param name='resource'>Resource to set value on.</param>
        /// <param name="propertiesApplied">Properties already applied based on content</param>
        /// <param name="currentObjectCount">current object count for this operation.</param>
        /// <remarks>
        /// This method will end as soon as it find something that is not an
        /// XML element to process.
        /// </remarks>
        internal static void ApplyContent(Deserializer deserializer, XmlReader reader, ResourceType resourceType, object resource, EpmContentDeSerializer.EpmAppliedPropertyInfo propertiesApplied, int currentObjectCount)
        {
            Debug.Assert(deserializer != null, "deserializer != null");
            Debug.Assert(reader != null, "reader != null");
            using (PlainXmlDeserializer xml = new PlainXmlDeserializer(reader, deserializer, propertiesApplied))
            {
                // Initialize the new deserializer instance with the current object count
                xml.UpdateObjectCount(currentObjectCount);

                // load all the properties
                xml.ApplyContent(xml.xmlReader, resourceType, resource);

                // After all the properties have been loaded, initialize the current deserializer value with the object count
                deserializer.UpdateObjectCount(xml.MaxObjectCount);
            }
        }

        /// <summary>
        /// Converts the given value to the expected type as per XML serializer rules.
        /// Make sure these rules are in sync with PlainXmlSerializer.
        /// </summary>
        /// <param name="value">value to the converted</param>
        /// <param name="propertyName">name of the property whose value is getting converted</param>
        /// <param name="typeToBeConverted">clr type to which the value needs to be converted to</param>
        /// <returns>object which is in sync with the properties type</returns>
        internal static object ConvertValuesForXml(object value, string propertyName, Type typeToBeConverted)
        {
            Debug.Assert(WebUtil.IsPrimitiveType(typeToBeConverted), "WebUtil.IsPrimitiveType(typeToBeConverted)"); 
            Debug.Assert(value == null || value is string, "This method should be used only for converting string to a primitve value.");

            string stringValue = value as string;
            if (stringValue != null)
            {
                try
                {
                    value = WebConvert.StringToPrimitive(stringValue, typeToBeConverted);
                }
                catch (FormatException e)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequest_ErrorInConvertingPropertyValue(propertyName, typeToBeConverted), e);
                }
            }

            return value;
        }

        /// <summary>
        /// Assumes the payload to represent a single object and processes accordingly
        /// </summary>
        /// <param name="segmentInfo">info about the object being created</param>
        /// <returns>the newly formed object that the payload represents</returns>
        protected override object CreateSingleObject(SegmentInfo segmentInfo)
        {
            Debug.Assert(
                segmentInfo.TargetKind == RequestTargetKind.OpenProperty ||
                segmentInfo.TargetKind == RequestTargetKind.ComplexObject ||
                segmentInfo.TargetKind == RequestTargetKind.Primitive,
                segmentInfo.TargetKind + " is one of open property; complex object; primitive -- otherwise the wrong serializer was chosen");

            if (!WebUtil.XmlReaderEnsureElement(this.xmlReader))
            {
                throw DataServiceException.CreateBadRequestError(Strings.PlainXml_PayloadLacksElement);
            }

            if (HasNullAttributeWithTrueValue(this.xmlReader))
            {
                return null;
            }

            string propertyName;
            if (segmentInfo.TargetKind == RequestTargetKind.OpenProperty)
            {
                propertyName = this.xmlReader.LocalName;
            }
            else
            {
                Debug.Assert(segmentInfo.ProjectedProperty != null, "segmentInfo.ProjectedProperty != null");
                Debug.Assert(
                    segmentInfo.ProjectedProperty.ResourceType.ResourceTypeKind != ResourceTypeKind.EntityType,
                    "entity types not expected in the PlainXmlDeserializer");

                propertyName = segmentInfo.ProjectedProperty.Name;
                if (propertyName != this.xmlReader.LocalName)
                {
                    throw DataServiceException.CreateBadRequestError(
                        Strings.PlainXml_IncorrectElementName(propertyName, this.xmlReader.LocalName));
                }
            }

            object result = this.ReadPropertyWithType(this.xmlReader, propertyName, segmentInfo.ProjectedProperty);
            return result;
        }

        /// <summary>Provides an opportunity to clean-up resources.</summary>
        /// <param name="disposing">
        /// Whether the call is being made from an explicit call to 
        /// IDisposable.Dispose() rather than through the finalizer.
        /// </param>
        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);
            if (disposing && this.xmlReaderOwned)
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
            if (!WebUtil.XmlReaderEnsureElement(this.xmlReader))
            {
                throw DataServiceException.CreateBadRequestError(Strings.PlainXml_PayloadLacksElement);
            }

            string uri = null;
            bool skipped;
            do
            {
                skipped = false;
                switch (this.xmlReader.NodeType)
                {
                    case XmlNodeType.Element:
                        string localName = this.xmlReader.LocalName;
                        string elementNamespace = this.xmlReader.NamespaceURI;
                        if ((elementNamespace != XmlConstants.DataWebMetadataNamespace && elementNamespace != XmlConstants.DataWebNamespace) ||
                            localName != XmlConstants.UriElementName)
                        {
                            this.xmlReader.Skip();
                            skipped = true;
                            continue;
                        }

                        if (uri != null)
                        {
                            throw DataServiceException.CreateBadRequestError(Strings.BadRequest_MoreThanOneUriElementSpecified);
                        }

                        uri = ReadElementString(this.xmlReader, localName);
                        if (String.IsNullOrEmpty(uri))
                        {
                            throw DataServiceException.CreateBadRequestError(Strings.BadRequest_MissingUriForLinkOperation);
                        }

                        break;
                    default:
                        break;
                }
            }
            while (skipped || this.xmlReader.Read());

            Debug.Assert(
                this.xmlReader.NodeType != XmlNodeType.Element,
                "reader.NodeType != XmlNodeType.Element -- otherwise we should have kept processing");

            if (String.IsNullOrEmpty(uri))
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_MissingUriForLinkOperation);
            }

            return uri;
        }

        /// <summary>
        /// returns true if the null attribute is specified and the value is true
        /// </summary>
        /// <param name="reader">xml reader from which attribute needs to be read</param>
        /// <returns>true if the null attribute is specified and the attribute value is true</returns>
        private static bool HasNullAttributeWithTrueValue(XmlReader reader)
        {
            Debug.Assert(reader != null, "reader != null");
            Debug.Assert(reader.NodeType == XmlNodeType.Element, "reader.NodeType == XmlNodeType.Element");

            string elementValue = reader.GetAttribute(XmlConstants.AtomNullAttributeName, XmlConstants.DataWebMetadataNamespace);

            // If the null attribute is specified and the value is true, then set the property value to null,
            // otherwise set the value to empty string
            if ((null != elementValue) && XmlConvert.ToBoolean(elementValue))
            {
                string elementName = reader.LocalName;
                if (!reader.IsEmptyElement)
                {
                    reader.Read();
                    if (reader.NodeType != XmlNodeType.EndElement)
                    {
                        throw DataServiceException.CreateBadRequestError(
                            Strings.BadRequest_CannotSpecifyValueOrChildElementsForNullElement(elementName));
                    }
                }

                return true;
            }

            return false;
        }

        /// <summary>
        /// Reads the value from the given element. This leaves the reader in the EndElement.
        /// </summary>
        /// <param name="reader">xml reader from which the value needs to be read</param>
        /// <param name="elementName">name of the element whose value is getting read</param>
        /// <returns>returns the xml string values as specified in the payload</returns>
        private static string ReadElementString(XmlReader reader, string elementName)
        {
            Debug.Assert(reader != null, "reader != null");
            Debug.Assert(XmlNodeType.Element == reader.NodeType, "not positioned on Element");
            string elementValue = null;

            if (HasNullAttributeWithTrueValue(reader))
            {
                return null;
            }
            else if (reader.IsEmptyElement)
            {
                return String.Empty;
            }

            StringBuilder builder = null;
            bool done = false;
            while (!done && reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.EndElement:
                        done = true;
                        break;
                    case XmlNodeType.CDATA:
                    case XmlNodeType.Text:
                    case XmlNodeType.SignificantWhitespace:
                        if (elementValue == null)
                        {
                            elementValue = reader.Value;
                        }
                        else if (builder == null)
                        {
                            string newValue = reader.Value;
                            builder = new StringBuilder(newValue.Length + elementValue.Length);
                            builder.Append(elementValue);
                            builder.Append(newValue);
                        }
                        else
                        {
                            builder.Append(reader.Value);
                        }

                        break;
                    case XmlNodeType.Comment:
                    case XmlNodeType.Whitespace:
                        break;

                    #region XmlNodeType error
                    case XmlNodeType.None:
                    case XmlNodeType.XmlDeclaration:
                    case XmlNodeType.Attribute:
                    case XmlNodeType.EndEntity:
                    case XmlNodeType.EntityReference:
                    case XmlNodeType.Entity:
                    case XmlNodeType.Document:
                    case XmlNodeType.DocumentType:
                    case XmlNodeType.DocumentFragment:
                    case XmlNodeType.Notation:
                    case XmlNodeType.ProcessingInstruction:
                    default:
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidValue(elementName));
                    #endregion
                }
            }

            if (builder != null)
            {
                elementValue = builder.ToString();
            }
            else if (elementValue == null)
            {
                elementValue = String.Empty;
            }

            return elementValue;
        }

        /// <summary>Applies properties from the reader to the specified resource.</summary>
        /// <param name='reader'>XmlReader to read from.</param>
        /// <param name='resourceType'>Type of resource.</param>
        /// <param name='resource'>Resource to set value on.</param>
        /// <remarks>
        /// This method will end as soon as it find something that is not an
        /// XML element to process.
        /// </remarks>
        private void ApplyContent(XmlReader reader, ResourceType resourceType, object resource)
        {
            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(resource != null, "resource != null");

            if (!WebUtil.XmlReaderEnsureElement(reader))
            {
                return;
            }

            this.RecurseEnter();
            bool skipped;
            do
            {
                skipped = false;
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        string localName = reader.LocalName;
                        string elementNamespace = reader.NamespaceURI;
                        if (elementNamespace != XmlConstants.DataWebNamespace)
                        {
                            reader.Skip();
                            skipped = true;
                            continue;
                        }

                        this.ApplyProperty(reader, localName, resourceType, resource);
                        break;
                    default:
                        break;
                }
            }
            while (skipped || reader.Read());

            Debug.Assert(
                reader.NodeType != XmlNodeType.Element,
                "reader.NodeType != XmlNodeType.Element -- otherwise we should have kept processing");
            this.RecurseLeave();
        }

        /// <summary>Applies a property from the reader to the specified resource.</summary>
        /// <param name='reader'>XmlReader to read from.</param>
        /// <param name='propertyName'>Name of property to set on the specified resource.</param>
        /// <param name='resourceType'>Type of resource.</param>
        /// <param name='resource'>Resource to set value on.</param>
        private void ApplyProperty(XmlReader reader, string propertyName, ResourceType resourceType, object resource)
        {
            Debug.Assert(reader != null, "reader != null");
            Debug.Assert(propertyName != null, "propertyName != null");
            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(resource != null, "resource != null");

            ResourceProperty property = resourceType.TryResolvePropertyName(propertyName);
            bool ignoreValue = false;
            if (property == null)
            {
                if (resourceType.IsOpenType == false)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidPropertyNameSpecified(propertyName, resourceType.FullName));
                }
            }
            else
            {
                if (this.Update && property.IsOfKind(ResourcePropertyKind.Key))
                {
                    ignoreValue = true;
                }
            }

            object propertyValue = this.ReadPropertyWithType(reader, propertyName, property);
            if (!ignoreValue)
            {
                if (property == null)
                {
                    Deserializer.SetOpenPropertyValue(resource, propertyName, propertyValue, this.Service);
                }
                else
                {
                    Deserializer.SetPropertyValue(property, resource, propertyValue, this.ContentFormat, this.Service);
                }
            }
        }

        /// <summary>Gets the type attribute and resolves the type.</summary>
        /// <param name="reader">reader from which type attribute needs to be read</param>
        /// <returns>resolved type</returns>
        private ResourceType ReadOpenPropertyTypeAttribute(XmlReader reader)
        {
            Debug.Assert(reader != null, "reader != null");
            string typeName = reader.GetAttribute(XmlConstants.AtomTypeAttributeName, XmlConstants.DataWebMetadataNamespace);
            ResourceType resourceType = null;

            // If the type is not specified in the payload, we assume the type to be the expected type
            if (String.IsNullOrEmpty(typeName))
            {
                resourceType = ResourceType.PrimitiveStringResourceType;
            }
            else
            {
                // try and resolve the name specified in the payload
                resourceType = WebUtil.TryResolveResourceType(this.Service.Provider, typeName);
                if (resourceType == null)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidTypeName(typeName));
                }

                if (resourceType.ResourceTypeKind == ResourceTypeKind.EntityType)
                {
                    throw DataServiceException.CreateBadRequestError(
                        Strings.PlainXml_EntityTypeNotSupported(resourceType.FullName));
                }
            }

            return resourceType;
        }

        /// <summary>Reads a typed property from the specified XmlReader.</summary>
        /// <param name='reader'>XmlReader to read from.</param>
        /// <param name='propertyName'>Name of property to read.</param>
        /// <param name='resourceProperty'>resourceProperty whose value is to be read.</param>
        /// <returns>The instance read, possibly null.</returns>
        private object ReadPropertyWithType(XmlReader reader, string propertyName, ResourceProperty resourceProperty)
        {
            Debug.Assert(reader != null, "reader != null");
            Debug.Assert(propertyName != null, "propertyName != null");
            ResourceType propertyType = null;

            // If this is a open property OR we do not have to do type conversion for primitive types,
            // read the type from the payload.
            bool readTypeFromWire = 
                resourceProperty == null ||
                (!this.Service.Configuration.EnableTypeConversion && resourceProperty.ResourceType.ResourceTypeKind == ResourceTypeKind.Primitive);

            if (readTypeFromWire)
            {
                propertyType = this.ReadOpenPropertyTypeAttribute(reader);
            }
            else
            {
                propertyType = resourceProperty.ResourceType;
            }
            
            if (this.propertiesApplied != null)
            {
                this.propertiesApplied.AddPropertyToTypeMapItem(
                                            this.BuildPropertyPath(propertyName), 
                                            propertyType.FullName);
            }

            object propertyValue;
            String appliedPropertyName = resourceProperty != null ? resourceProperty.Name : propertyName;

            switch (propertyType.ResourceTypeKind)
            {
                case ResourceTypeKind.ComplexType:
                    this.CheckAndIncrementObjectCount();
                    
                    bool isNull = HasNullAttributeWithTrueValue(reader);
                    if (this.propertiesApplied != null)
                    {
                        this.propertiesApplied.AddAppliedProperty(this.BuildPropertyPath(appliedPropertyName), isNull);
                    }

                    if (isNull)
                    {
                        propertyValue = null;
                    }
                    else
                    {
                        propertyValue = this.Updatable.CreateResource(null, propertyType.FullName);
                        if (!reader.IsEmptyElement)
                        {
                            // Step inside the complex type, apply its properties, and pop back out.
                            using (XmlReader propertyReader = reader.ReadSubtree())
                            {
                                if (!WebUtil.XmlReaderEnsureElement(propertyReader))
                                {
                                    throw DataServiceException.CreateBadRequestError(
                                        Strings.PlainXml_PropertyLacksElement(propertyName));
                                }

                                propertyReader.ReadStartElement();
                                String savedPath = this.currentPathPrefix;
                                this.currentPathPrefix = String.IsNullOrEmpty(this.currentPathPrefix) ? appliedPropertyName : this.currentPathPrefix + "/" + appliedPropertyName;
                                ApplyContent(propertyReader, propertyType, propertyValue);
                                this.currentPathPrefix = savedPath;
                            }

                            Debug.Assert(
                                reader.NodeType == XmlNodeType.EndElement,
                                "reader.NodeType == XmlNodeType.EndElement -- otherwise ReadSubtree left outer read in incorrect position.");
                        }
                    }

                    break;
                case ResourceTypeKind.EntityType:
                    throw DataServiceException.CreateBadRequestError(
                        Strings.PlainXml_NavigationPropertyNotSupported(propertyName));
                default:
                    Debug.Assert(
                        propertyType.ResourceTypeKind == ResourceTypeKind.Primitive,
                        "property.TypeKind == ResourceTypeKind.Primitive -- metadata shouldn't return " + propertyType.ResourceTypeKind);

                    propertyValue = ReadElementString(reader, propertyName);

                    // We need to convert the type to the wire type. Conversion to the property type happens in Deserializer.SetValue method.
                    if (readTypeFromWire)
                    {
                        propertyValue = PlainXmlDeserializer.ConvertValuesForXml(propertyValue, propertyName, propertyType.InstanceType);
                    }

                    if (this.propertiesApplied != null)
                    {
                        this.propertiesApplied.AddAppliedProperty(this.BuildPropertyPath(appliedPropertyName), false);
                    }

                    break;
            }

            return propertyValue;
        }
        
        /// <summary>Gives an absolute property path based on current prefix</summary>
        /// <param name="propertyName">Name of property</param>
        /// <returns>Absolute path to the property</returns>
        private String BuildPropertyPath(String propertyName)
        {
            return String.IsNullOrEmpty(this.currentPathPrefix) ? propertyName : this.currentPathPrefix + "/" + propertyName;
        }
    }
}
