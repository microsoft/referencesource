//---------------------------------------------------------------------
// <copyright file="EpmCustomContentDeSerializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Deserializer for the EPM content on the server
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
#region Namespaces
    using System.Collections.Generic;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.Linq;
    using System.ServiceModel.Syndication;
    using System.Xml;
    using System.Text;

#endregion

    /// <summary>Custom content reader for EPM content</summary>
    internal sealed class EpmCustomContentDeSerializer : EpmContentDeSerializerBase
    {
        /// <summary>Constructor</summary>
        /// <param name="item"><see cref="SyndicationItem"/> to read content from</param>
        /// <param name="state">State of the deserializer</param>
        internal EpmCustomContentDeSerializer(SyndicationItem item, EpmContentDeSerializer.EpmContentDeserializerState state)
        : base(item, state)
        {
        }

        /// <summary>Publicly accessible deserialization entry point</summary>
        /// <param name="resourceType">Type of resource to deserialize</param>
        /// <param name="element">Token corresponding to object of <paramref name="resourceType"/></param>
        internal void DeSerialize(ResourceType resourceType, object element)
        {
            foreach (SyndicationElementExtension extension in this.Item.ElementExtensions)
            {
                using (XmlReader extensionReader = extension.GetReader())
                {
                    this.DeSerialize(extensionReader, resourceType.EpmTargetTree.NonSyndicationRoot, resourceType, element);
                }
            }
        }

        /// <summary>Called internally to deserialize each <see cref="SyndicationElementExtension"/></summary>
        /// <param name="reader">XmlReader for current extension</param>
        /// <param name="currentRoot">Node in the target path being processed</param>
        /// <param name="resourceType">ResourceType</param>
        /// <param name="element">object being deserialized</param>
        private void DeSerialize(XmlReader reader, EpmTargetPathSegment currentRoot, ResourceType resourceType, object element)
        {
            EpmValueBuilder currentValue = new EpmValueBuilder();

            do
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        if (currentRoot.HasContent)
                        {
                            // Throw an exception that we hit mixed-content.
                            // <contentElement>value<someElement /></contentElement>
                            // <contentElement><someElement />value</contentElement>
                            throw DataServiceException.CreateBadRequestError(Strings.EpmDeserialize_MixedContent(resourceType.FullName));
                        }

                        String elementName = reader.LocalName;
                        String namespaceUri = reader.NamespaceURI;
                        EpmTargetPathSegment newRoot = currentRoot.SubSegments
                                                                  .SingleOrDefault(s => s.SegmentNamespaceUri == namespaceUri && s.SegmentName == elementName);
                        if (newRoot == null)
                        {
                            WebUtil.SkipToEnd(reader, elementName, namespaceUri);
                            continue;
                        }

                        currentRoot = newRoot;
                        
                        this.DeserializeAttributes(reader, currentRoot, element, resourceType);
                        
                        if (currentRoot.HasContent)
                        {
                            if (reader.IsEmptyElement)
                            {
                                if (!EpmContentDeSerializerBase.Match(currentRoot, this.PropertiesApplied))
                                {
                                    resourceType.SetEpmValue(currentRoot, element, String.Empty, this);
                                }

                                currentRoot = currentRoot.ParentSegment;
                            }
                        }
                        
                        break;

                    case XmlNodeType.CDATA:
                    case XmlNodeType.Text:
                    case XmlNodeType.SignificantWhitespace:
                        if (!currentRoot.HasContent)
                        {
                            // Throw an exception that we hit mixed-content.
                            // <noContentElement>value<contentElement>value</contentElement></noContentElement>
                            // <noContentElement><contentElement>value</contentElement>value</noContentElement>
                            throw DataServiceException.CreateBadRequestError(Strings.EpmDeserialize_MixedContent(resourceType.FullName));
                        }

                        currentValue.Append(reader.Value);
                        break;
                        
                    case XmlNodeType.EndElement:
                        if (currentRoot.HasContent)
                        {
                            if (!EpmContentDeSerializerBase.Match(currentRoot, this.PropertiesApplied))
                            {
                                resourceType.SetEpmValue(currentRoot, element, currentValue.Value, this);
                            }
                        }

                        currentRoot = currentRoot.ParentSegment;
                        currentValue.Reset();
                        break;

                    case XmlNodeType.Comment:
                    case XmlNodeType.Whitespace:
                        break;

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
                        throw DataServiceException.CreateBadRequestError(Strings.EpmDeserialize_InvalidXmlEntity);
                }
            }
            while (currentRoot.ParentSegment != null && reader.Read());
        }

        /// <summary>
        /// Deserializes the attributes from the <paramref name="reader"/> and sets values on <paramref name="element"/>
        /// </summary>
        /// <param name="reader">Current content reader.</param>
        /// <param name="currentRoot">Segment which has child attribute segments.</param>
        /// <param name="element">Current object.</param>
        /// <param name="resourceType">Resource type of <paramref name="element"/></param>
        private void DeserializeAttributes(XmlReader reader, EpmTargetPathSegment currentRoot, object element, ResourceType resourceType)
        {
            foreach (var attributeSegment in currentRoot.SubSegments.Where(s => s.IsAttribute))
            {
                String attribValue = WebUtil.GetAttributeEx(reader, attributeSegment.SegmentName.Substring(1), attributeSegment.SegmentNamespaceUri);
                if (attribValue != null)
                {
                    if (!EpmContentDeSerializerBase.Match(attributeSegment, this.PropertiesApplied))
                    {
                        resourceType.SetEpmValue(attributeSegment, element, attribValue, this);
                    }
                }
            }        
        }

        /// <summary>Collects current XmlReader values into a single string.</summary>
        private class EpmValueBuilder
        {
            /// <summary>Current value when single text content value is seen.</summary>
            private string elementValue;
            
            /// <summary>Current value if multiple text content values are seen together.</summary>
            private StringBuilder builder;
            
            /// <summary>Final value which is concatenation of all text content.</summary>
            internal string Value
            {
                get
                {
                    if (this.builder != null)
                    {
                        return this.builder.ToString();
                    }

                    return this.elementValue ?? String.Empty;
                }
            }

            /// <summary>Appends the current text content value to already held values.</summary>
            /// <param name="value">Current text content value.</param>
            internal void Append(string value)
            {
                if (this.elementValue == null)
                {
                    this.elementValue = value;
                }
                else
                if (this.builder == null)
                {
                    string newValue = value;
                    this.builder = new StringBuilder(elementValue.Length + newValue.Length)
                        .Append(elementValue)
                        .Append(newValue);
                }
                else
                {
                    this.builder.Append(value);
                }
            }

            /// <summary>Once value is read, resets the current content to null.</summary>
            internal void Reset()
            {
                this.elementValue = null;
                this.builder = null;
            }
        }        
    }
}
