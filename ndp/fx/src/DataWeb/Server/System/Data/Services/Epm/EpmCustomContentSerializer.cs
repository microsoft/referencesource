//---------------------------------------------------------------------
// <copyright file="EpmCustomContentSerializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Class used for serializing custom EntityPropertyMappingAttribute content
// for non-syndication mappings
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
    using System.Collections.Generic;
    using System.Diagnostics;

#if !ASTORIA_CLIENT
    using System.ServiceModel.Syndication;
    using System.Data.Services.Providers;
#else
    using System.Data.Services.Client;
    using System.Xml;
#endif

    /// <summary>
    /// Base visitor class for performing serialization of custom content in the feed entry whose mapping
    /// is provided through EntityPropertyMappingAttributes
    /// </summary>
    internal sealed class EpmCustomContentSerializer : EpmContentSerializerBase, IDisposable
    {
        /// <summary>IDisposable helper state</summary>
        private bool disposed;

        /// <summary>
        /// Dictionary mapping visitor content and with target paths
        /// </summary>
        private Dictionary<EpmTargetPathSegment, EpmCustomContentWriterNodeData> visitorContent;

#if ASTORIA_CLIENT
        /// <summary>
        /// Constructor initializes the base class be identifying itself as a custom content serializer
        /// </summary>
        /// <param name="targetTree">Target tree containing mapping information</param>
        /// <param name="element">Object to be serialized</param>
        /// <param name="target">SyndicationItem to which content will be added</param>
        internal EpmCustomContentSerializer(EpmTargetTree targetTree, object element, XmlWriter target)
            : base(targetTree, false, element, target)
        {
            this.InitializeVisitorContent();
        }
#else
        /// <summary>
        /// Constructor initializes the base class be identifying itself as a custom content serializer
        /// </summary>
        /// <param name="targetTree">Target tree containing mapping information</param>
        /// <param name="element">Object to be serialized</param>
        /// <param name="target">SyndicationItem to which content will be added</param>
        /// <param name="nullValuedProperties">Null valued properties found during serialization</param>
        /// <param name="provider">Data Service provider used for rights verification.</param>
        internal EpmCustomContentSerializer(EpmTargetTree targetTree, object element, SyndicationItem target, EpmContentSerializer.EpmNullValuedPropertyTree nullValuedProperties, DataServiceProviderWrapper provider)
            : base(targetTree, false, element, target)
        {
            this.InitializeVisitorContent(nullValuedProperties, provider);
        }
#endif

        /// <summary>
        /// Closes all the XmlWriter and MemoryStream objects in the tree and adds them to the SyndicationItem 
        /// as ElementExtensions. Invokes the NodeDataCleaner to dispose off any existing memory stream and 
        /// XmlWriter objects
        /// </summary>
        public void Dispose()
        {
            if (!this.disposed)
            {
                foreach (EpmTargetPathSegment subSegmentOfRoot in this.Root.SubSegments)
                {
                    EpmCustomContentWriterNodeData c = this.visitorContent[subSegmentOfRoot];
                    Debug.Assert(c != null, "Must have custom data for all the children of root");
                    if (this.Success)
                    {
                        c.AddContentToTarget(this.Target);
                    }

                    c.Dispose();
                }
                
                this.disposed = true;
            }
        }

#if ASTORIA_CLIENT
        /// <summary>
        /// Override of the base Visitor method, which actually performs mapping search and serialization
        /// </summary>
        /// <param name="targetSegment">Current segment being checked for mapping</param>
        /// <param name="kind">Which sub segments to serialize</param>
        protected override void Serialize(EpmTargetPathSegment targetSegment, EpmSerializationKind kind)
#else
        /// <summary>
        /// Override of the base Visitor method, which actually performs mapping search and serialization
        /// </summary>
        /// <param name="targetSegment">Current segment being checked for mapping</param>
        /// <param name="kind">Which sub segments to serialize</param>
        /// <param name="provider">Data Service provider used for rights verification.</param>
        protected override void Serialize(EpmTargetPathSegment targetSegment, EpmSerializationKind kind, DataServiceProviderWrapper provider)
#endif
        {
            if (targetSegment.IsAttribute)
            {
                this.WriteAttribute(targetSegment);
            }
            else
            {
#if ASTORIA_CLIENT
                this.WriteElement(targetSegment);
#else
                this.WriteElement(targetSegment, provider);
#endif
            }
        }

        /// <summary>
        /// Given a segment, writes the attribute to xml writer corresponding to it
        /// </summary>
        /// <param name="targetSegment">Segment being written</param>
        private void WriteAttribute(EpmTargetPathSegment targetSegment)
        {
            // Content to be written in an attribute
            Debug.Assert(targetSegment.HasContent, "Must have content for attributes");

            EpmCustomContentWriterNodeData currentContent = this.visitorContent[targetSegment];
            currentContent.XmlContentWriter.WriteAttributeString(
                                    targetSegment.SegmentNamespacePrefix,
                                    targetSegment.SegmentName.Substring(1),
                                    targetSegment.SegmentNamespaceUri,
                                    currentContent.Data);
        }

#if ASTORIA_CLIENT
        /// <summary>
        /// Given a segment, writes the element to xml writer corresponding to it, works recursively to write child elements/attributes
        /// </summary>
        /// <param name="targetSegment">Segment being written</param>
        private void WriteElement(EpmTargetPathSegment targetSegment)
#else
        /// <summary>
        /// Given a segment, writes the element to xml writer corresponding to it, works recursively to write child elements/attributes
        /// </summary>
        /// <param name="targetSegment">Segment being written</param>
        /// <param name="provider">Data Service provider used for rights verification.</param>
        private void WriteElement(EpmTargetPathSegment targetSegment, DataServiceProviderWrapper provider)
#endif
        {
            // Content to be written in an element 
            EpmCustomContentWriterNodeData currentContent = this.visitorContent[targetSegment];

            currentContent.XmlContentWriter.WriteStartElement(
                targetSegment.SegmentNamespacePrefix,
                targetSegment.SegmentName,
                targetSegment.SegmentNamespaceUri);

#if ASTORIA_CLIENT
            // Serialize the attributes and children before serializing the data itself
            base.Serialize(targetSegment, EpmSerializationKind.Attributes);
#else
            // Serialize the attributes and children before serializing the data itself
            base.Serialize(targetSegment, EpmSerializationKind.Attributes, provider);
#endif

            if (targetSegment.HasContent)
            {
                Debug.Assert(currentContent.Data != null, "Must always have non-null data content value");
                currentContent.XmlContentWriter.WriteString(currentContent.Data);
            }

#if ASTORIA_CLIENT
            // Serialize the attributes and children before serializing the data itself
            base.Serialize(targetSegment, EpmSerializationKind.Elements);
#else
            // Serialize the attributes and children before serializing the data itself
            base.Serialize(targetSegment, EpmSerializationKind.Elements, provider);
#endif

            currentContent.XmlContentWriter.WriteEndElement();
        }

#if ASTORIA_CLIENT
        /// <summary>Initializes content for the serializer visitor</summary>
        private void InitializeVisitorContent()
        {
            this.visitorContent = new Dictionary<EpmTargetPathSegment, EpmCustomContentWriterNodeData>(ReferenceEqualityComparer<EpmTargetPathSegment>.Instance);

            // Initialize all the root's children's xml writers
            foreach (EpmTargetPathSegment subSegmentOfRoot in this.Root.SubSegments)
            {
                this.visitorContent.Add(subSegmentOfRoot, new EpmCustomContentWriterNodeData(subSegmentOfRoot, this.Element));
                this.InitializeSubSegmentVisitorContent(subSegmentOfRoot);
            }
        }

        /// <summary>Initialize the visitor content for all of root's grandchildren and beyond</summary>
        /// <param name="subSegment">One of root's children</param>
        private void InitializeSubSegmentVisitorContent(EpmTargetPathSegment subSegment)
        {
            foreach (EpmTargetPathSegment segment in subSegment.SubSegments)
            {
                this.visitorContent.Add(segment, new EpmCustomContentWriterNodeData(this.visitorContent[subSegment], segment, this.Element));
                this.InitializeSubSegmentVisitorContent(segment);
            }
        }
#else
        /// <summary>Initializes content for the serializer visitor</summary>
        /// <param name="nullValuedProperties">Null valued properties found during serialization</param>
        /// <param name="provider">Data Service provider used for rights verification.</param>
        private void InitializeVisitorContent(EpmContentSerializer.EpmNullValuedPropertyTree nullValuedProperties, DataServiceProviderWrapper provider)
        {
            this.visitorContent = new Dictionary<EpmTargetPathSegment, EpmCustomContentWriterNodeData>(ReferenceEqualityComparer<EpmTargetPathSegment>.Instance);

            // Initialize all the root's children's xml writers
            foreach (EpmTargetPathSegment subSegmentOfRoot in this.Root.SubSegments)
            {
                this.visitorContent.Add(subSegmentOfRoot, new EpmCustomContentWriterNodeData(subSegmentOfRoot, this.Element, nullValuedProperties, provider));
                this.InitializeSubSegmentVisitorContent(subSegmentOfRoot, nullValuedProperties, provider);
            }
        }

        /// <summary>Initialize the visitor content for all of root's grandchildren and beyond</summary>
        /// <param name="subSegment">One of root's children</param>
        /// <param name="nullValuedProperties">Null valued properties found during serialization</param>
        /// <param name="provider">Data Service provider used for rights verification.</param>
        private void InitializeSubSegmentVisitorContent(EpmTargetPathSegment subSegment, EpmContentSerializer.EpmNullValuedPropertyTree nullValuedProperties, DataServiceProviderWrapper provider)
        {
            foreach (EpmTargetPathSegment segment in subSegment.SubSegments)
            {
                this.visitorContent.Add(segment, new EpmCustomContentWriterNodeData(this.visitorContent[subSegment], segment, this.Element, nullValuedProperties, provider));
                this.InitializeSubSegmentVisitorContent(segment, nullValuedProperties, provider);
            }
        }
#endif
    }
}
