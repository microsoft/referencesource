//---------------------------------------------------------------------
// <copyright file="PlainXmlSerializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a serializer for non-reference entities in plain XML.
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
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml;

    #endregion Namespaces.

    /// <summary>This class serializes primitive and complex type as name/value pairs.</summary>
    internal sealed class PlainXmlSerializer : Serializer
    {
        /// <summary>Writer to which output is sent.</summary>
        private XmlWriter writer;

        /// <summary>Initializes a new SyndicationSerializer instance.</summary>
        /// <param name="requestDescription">Description of request.</param>
        /// <param name="absoluteServiceUri">Base URI from which resources should be resolved.</param>
        /// <param name="service">Service with configuration and provider from which metadata should be gathered.</param>
        /// <param name="output">Output stream.</param>
        /// <param name="encoding">Encoding for output.</param>
        internal PlainXmlSerializer(
            RequestDescription requestDescription,
            Uri absoluteServiceUri,
            IDataService service,
            Stream output, 
            Encoding encoding)
            : base(requestDescription, absoluteServiceUri, service, null)
        {
            Debug.Assert(output != null, "output != null");
            Debug.Assert(encoding != null, "encoding != null");

            this.writer = XmlUtil.CreateXmlWriterAndWriteProcessingInstruction(output, encoding);
        }

        /// <summary>Serializes exception information.</summary>
        /// <param name="args">Description of exception to serialize.</param>
        public override void WriteException(HandleExceptionArgs args)
        {
            ErrorHandler.SerializeXmlError(args, this.writer);
        }

        /// <summary>Converts the specified value to a serializable string.</summary>
        /// <param name="value">Non-null value to convert.</param>
        /// <returns>The specified value converted to a serializable string.</returns>
        internal static string PrimitiveToString(object value)
        {
            Debug.Assert(value != null, "value != null");
            string result;
            if (!System.Data.Services.Parsing.WebConvert.TryXmlPrimitiveToString(value, out result))
            {
                throw new InvalidOperationException(Strings.Serializer_CannotConvertValue(value));
            }

            return result;
        }

        /// <summary>Writes a property with a null value.</summary>
        /// <param name='writer'>XmlWriter to write to.</param>
        /// <param name='propertyName'>Property name.</param>
        /// <param name='expectedTypeName'>Type name for value.</param>
        internal static void WriteNullValue(XmlWriter writer, string propertyName, string expectedTypeName)
        {
            Debug.Assert(writer != null, "writer != null");
            Debug.Assert(propertyName != null, "propertyName != null");
            Debug.Assert(expectedTypeName != null, "expectedTypeName != null");
            WriteStartElementWithType(writer, propertyName, expectedTypeName);
            writer.WriteAttributeString(XmlConstants.AtomNullAttributeName, XmlConstants.DataWebMetadataNamespace, XmlConstants.XmlTrueLiteral);
            writer.WriteEndElement();
        }

        /// <summary>Writes the start tag for a property.</summary>
        /// <param name='writer'>XmlWriter to write to.</param>
        /// <param name='propertyName'>Property name.</param>
        /// <param name='propertyTypeName'>Type name for value.</param>
        internal static void WriteStartElementWithType(XmlWriter writer, string propertyName, string propertyTypeName)
        {
            Debug.Assert(writer != null, "writer != null");
            Debug.Assert(propertyName != null, "propertyName != null");
            Debug.Assert(propertyTypeName != null, "expectedType != null");
            Debug.Assert(
               object.ReferenceEquals(propertyTypeName, XmlConstants.EdmStringTypeName) == (propertyTypeName == XmlConstants.EdmStringTypeName),
               "object equality and string equality are the same for Edm.String");
            writer.WriteStartElement(propertyName, XmlConstants.DataWebNamespace);
            if (!object.ReferenceEquals(propertyTypeName, XmlConstants.EdmStringTypeName))
            {
                writer.WriteAttributeString(XmlConstants.AtomTypeAttributeName, XmlConstants.DataWebMetadataNamespace, propertyTypeName);
            }
        }

        /// <summary>Writes an already-formatted property.</summary>
        /// <param name='writer'>XmlWriter to write to.</param>
        /// <param name='propertyName'>Property name.</param>
        /// <param name='propertyTypeName'>Type name for value.</param>
        /// <param name='propertyText'>Property value in text form.</param>
        internal static void WriteTextValue(XmlWriter writer, string propertyName, string propertyTypeName, string propertyText)
        {
            Debug.Assert(writer != null, "writer != null");
            Debug.Assert(propertyName != null, "propertyName != null");
            WriteStartElementWithType(writer, propertyName, propertyTypeName);
            WebUtil.WriteSpacePreserveAttributeIfNecessary(writer, propertyText);
            writer.WriteString(propertyText);
            writer.WriteEndElement();
        }

        /// <summary>Flushes the writer to the underlying stream.</summary>
        protected override void Flush()
        {
            this.writer.Flush();
        }

        /// <summary>Writes a single top-level element.</summary>
        /// <param name="expandedResult">Expandd results on the specified <paramref name="element"/>.</param>
        /// <param name="element">Element to write, possibly null.</param>
        protected override void WriteTopLevelElement(IExpandedResult expandedResult, object element)
        {
            Debug.Assert(
                element != null || this.RequestDescription.TargetResourceType != null || this.RequestDescription.TargetKind == RequestTargetKind.OpenProperty,
                "element != null || this.RequestDescription.TargetResourceType != null || this.RequestDescription.TargetKind == RequestTargetKind.OpenProperty");
            Debug.Assert(
                this.RequestDescription.IsSingleResult, 
                "this.RequestDescription.IsSingleResult -- primitive collections not currently supported");
            string propertyName = this.RequestDescription.ContainerName;
            ResourceType resourceType;
            
            if (element == null)
            {
                if (this.RequestDescription.TargetKind == RequestTargetKind.OpenProperty)
                {
                    resourceType = ResourceType.PrimitiveStringResourceType;
                }
                else
                {
                    resourceType = this.RequestDescription.TargetResourceType;
                }
            }
            else
            {
                resourceType = WebUtil.GetResourceType(this.Provider, element);
            }

            if (resourceType == null)
            {
                throw new InvalidOperationException(Strings.Serializer_UnsupportedTopLevelType(element.GetType()));
            }

            this.WriteValueWithName(element, propertyName, resourceType);
        }

        /// <summary>Writes multiple top-level elements, possibly none.</summary>
        /// <param name="expanded">Expanded results for elements.</param>
        /// <param name="elements">Enumerator for elements to write.</param>
        /// <param name="hasMoved">Whether <paramref name="elements"/> was succesfully advanced to the first element.</param>
        protected override void WriteTopLevelElements(IExpandedResult expanded, IEnumerator elements, bool hasMoved)
        {
            Debug.Assert(
                !this.RequestDescription.IsSingleResult,
                "!this.RequestDescription.IsSingleResult -- otherwise WriteTopLevelElement should have been called");
            this.writer.WriteStartElement(this.RequestDescription.ContainerName, XmlConstants.DataWebNamespace);
            while (hasMoved)
            {
                object element = elements.Current;
                ResourceType resourceType = element == null ?
                    this.RequestDescription.TargetResourceType : WebUtil.GetResourceType(this.Provider, element);
                if (resourceType == null)
                {
                    throw new InvalidOperationException(Strings.Serializer_UnsupportedTopLevelType(element.GetType()));
                }

                this.WriteValueWithName(element, XmlConstants.XmlCollectionItemElementName, resourceType);
                hasMoved = elements.MoveNext();
            }

            this.writer.WriteEndElement();
        }

        /// <summary>
        /// Writes the row count.
        /// </summary>
        protected override void WriteRowCount()
        {
            this.writer.WriteStartElement(
                XmlConstants.DataWebMetadataNamespacePrefix,
                XmlConstants.RowCountElement,
                XmlConstants.DataWebMetadataNamespace);
            this.writer.WriteValue(this.RequestDescription.CountValue);
            this.writer.WriteEndElement();
        }

        /// <summary>
        /// Write out the uri for the given element
        /// </summary>
        /// <param name="element">element whose uri needs to be written out.</param>
        protected override void WriteLink(object element)
        {
            Debug.Assert(element != null, "element != null");
            this.IncrementSegmentResultCount();
            Uri uri = Serializer.GetUri(element, this.Provider, this.CurrentContainer, this.AbsoluteServiceUri);
            this.writer.WriteStartElement(XmlConstants.UriElementName, XmlConstants.DataWebNamespace);
            this.writer.WriteValue(uri.AbsoluteUri);
            this.writer.WriteEndElement();
        }

        /// <summary>
        /// Write out the uri for the given elements
        /// </summary>
        /// <param name="elements">elements whose uri need to be writtne out</param>
        /// <param name="hasMoved">the current state of the enumerator.</param>
        protected override void WriteLinkCollection(IEnumerator elements, bool hasMoved)
        {
            this.writer.WriteStartElement(XmlConstants.LinkCollectionElementName, XmlConstants.DataWebNamespace);
            
            // write count?
            if (this.RequestDescription.CountOption == RequestQueryCountOption.Inline)
            {
                this.WriteRowCount();
            }

            object lastObject = null;
            IExpandedResult lastExpandedSkipToken = null;
            while (hasMoved)
            {
                object element = elements.Current;
                IExpandedResult skipToken = null;
                if (element != null)
                {
                    IExpandedResult expanded = element as IExpandedResult;
                    if (expanded != null)
                    {
                        element = GetExpandedElement(expanded);
                        skipToken = this.GetSkipToken(expanded);
                    }
                }
                
                this.WriteLink(element);
                hasMoved = elements.MoveNext();
                lastObject = element;
                lastExpandedSkipToken = skipToken;
            }

            if (this.NeedNextPageLink(elements))
            {
                this.WriteNextPageLink(lastObject, lastExpandedSkipToken, this.RequestDescription.ResultUri);
            }

            this.writer.WriteEndElement();
        }

        /// <summary>
        /// Writes the next page link to the current xml writer corresponding to the feed
        /// </summary>
        /// <param name="lastElement">Object that will contain the keys for skip token</param>
        /// <param name="expandedResult">The <see cref="IExpandedResult"/> value of the $skiptoken property of the object being written</param>
        /// <param name="absoluteUri">Absolute URI for the result</param>
        private void WriteNextPageLink(object lastElement, IExpandedResult expandedResult, Uri absoluteUri)
        {
            this.writer.WriteStartElement(XmlConstants.NextElementName, XmlConstants.DataWebNamespace);
            this.writer.WriteValue(this.GetNextLinkUri(lastElement, expandedResult, absoluteUri));
            this.writer.WriteEndElement();
        }

        /// <summary>Writes all the properties of the specified resource or complex object.</summary>
        /// <param name="element">Resource or complex object with properties to write out.</param>
        /// <param name="resourceType">Resource type describing the element type.</param>
        private void WriteObjectProperties(object element, ResourceType resourceType)
        {
            Debug.Assert(element != null, "element != null");
            Debug.Assert(resourceType != null, "resourceType != null");

            foreach (ResourceProperty property in resourceType.Properties)
            {
                string propertyName = property.Name;
                object propertyValue = WebUtil.GetPropertyValue(this.Provider, element, resourceType, property, null);
                bool needPop = this.PushSegmentForProperty(property);

                if (property.TypeKind == ResourceTypeKind.ComplexType)
                {
                    this.WriteComplexObjectValue(propertyValue, propertyName, property.ResourceType);
                }
                else
                {
                    Debug.Assert(property.TypeKind == ResourceTypeKind.Primitive, "property.TypeKind == ResourceTypeKind.Primitive");
                    this.WritePrimitiveValue(propertyValue, property.Name, property.ResourceType);
                }

                this.PopSegmentName(needPop);
            }

            if (resourceType.IsOpenType)
            {
                IEnumerable<KeyValuePair<string, object>> properties = this.Provider.GetOpenPropertyValues(element);
                foreach (KeyValuePair<string, object> property in properties)
                {
                    string propertyName = property.Key;
                    object value = property.Value;
                    if (value == null || value == DBNull.Value)
                    {
                        continue;
                    }

                    ResourceType propertyResourceType = WebUtil.GetResourceType(this.Provider, value);
                    if (propertyResourceType == null)
                    {
                        continue;
                    }

                    if (propertyResourceType.ResourceTypeKind == ResourceTypeKind.Primitive)
                    {
                        this.WritePrimitiveValue(value, propertyName, propertyResourceType);
                    }
                    else
                    {
                        if (propertyResourceType.ResourceTypeKind == ResourceTypeKind.ComplexType)
                        {
                            Debug.Assert(propertyResourceType.InstanceType == value.GetType(), "propertyResourceType.Type == valueType");
                            this.WriteComplexObjectValue(value, propertyName, propertyResourceType);
                        }
                        else
                        {
                            Debug.Assert(propertyResourceType.ResourceTypeKind == ResourceTypeKind.EntityType, "EntityType expected.");

                            // Open navigation properties are not supported on OpenTypes
                            throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(propertyName));
                        }
                    }
                }
            }
        }

        /// <summary>Writes a property with a primitive value.</summary>
        /// <param name='element'>Value.</param>
        /// <param name='propertyName'>Property name.</param>
        /// <param name='type'>Type for element.</param>
        private void WritePrimitiveValue(object element, string propertyName, ResourceType type)
        {
            Debug.Assert(propertyName != null, "propertyName != null");
            Debug.Assert(type != null, "type != null");

            if (element == null)
            {
                WriteNullValue(this.writer, propertyName, type.FullName);
                return;
            }

            string propertyText = PrimitiveToString(element);
            WriteTextValue(this.writer, propertyName, type.FullName, propertyText);
        }

        /// <summary>Writes the value of a complex object.</summary>
        /// <param name="element">Element to write.</param>
        /// <param name="propertyName">Property name.</param>
        /// <param name="expectedType">Type for element.</param>
        private void WriteComplexObjectValue(object element, string propertyName, ResourceType expectedType)
        {
            Debug.Assert(!String.IsNullOrEmpty(propertyName), "!String.IsNullOrEmpty(propertyName)");
            Debug.Assert(expectedType != null, "expectedType != null");
            Debug.Assert(expectedType.ResourceTypeKind == ResourceTypeKind.ComplexType, "Must be complex type");

            if (element == null)
            {
                WriteNullValue(this.writer, propertyName, expectedType.FullName);
                return;
            }

            // Non-value complex types may form a cycle.
            // PERF: we can keep a single element around and save the HashSet initialization
            // until we find a second complex type - this saves the allocation on trees
            // with shallow (single-level) complex types.
            this.RecurseEnter();
            Debug.Assert(!expectedType.InstanceType.IsValueType, "!expectedType.Type.IsValueType -- checked in the resource type constructor.");
            if (this.AddToComplexTypeCollection(element))
            {
                ResourceType resourceType = WebUtil.GetNonPrimitiveResourceType(this.Provider, element);
                WriteStartElementWithType(this.writer, propertyName, resourceType.FullName);
                this.WriteObjectProperties(element, resourceType);
                this.writer.WriteEndElement();
                this.RemoveFromComplexTypeCollection(element);
            }
            else
            {
                throw new InvalidOperationException(Strings.Serializer_LoopsNotAllowedInComplexTypes(propertyName));
            }

            this.RecurseLeave();
        }

        /// <summary>Writes the <paramref name="element"/> value with the specified name.</summary>
        /// <param name="element">Element to write out.</param>
        /// <param name="propertyName">Property name for element.</param>
        /// <param name="resourceType">Type of resource to write.</param>
        private void WriteValueWithName(object element, string propertyName, ResourceType resourceType)
        {
            Debug.Assert(propertyName != null, "propertyName != null");
            Debug.Assert(resourceType != null, "resourceType != null");

            if (resourceType.ResourceTypeKind == ResourceTypeKind.Primitive)
            {
                this.WritePrimitiveValue(element, propertyName, resourceType);
            }
            else
            {
                Debug.Assert(
                    resourceType.ResourceTypeKind == ResourceTypeKind.ComplexType,
                    "resourceType.ResourceTypeKind == ResourceTypeKind.ComplexType -- POX doesn't support " + resourceType.ResourceTypeKind);
                this.WriteComplexObjectValue(element, propertyName, resourceType);
            }
        }
    }
}
