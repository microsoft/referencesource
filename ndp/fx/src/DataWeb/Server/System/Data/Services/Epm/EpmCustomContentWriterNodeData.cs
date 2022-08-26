//---------------------------------------------------------------------
// <copyright file="EpmCustomContentWriterNodeData.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Data held by each node in the EpmTargetTree containing information used 
// by the EpmCustomContentWriter visitor
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
    using System;
    using System.IO;
    using System.Xml;

#if !ASTORIA_CLIENT
    using System.ServiceModel.Syndication;
    using System.Data.Services.Serializers;
    using System.Data.Services.Providers;
#else
    using System.Data.Services.Client;
#endif

    /// <summary>
    /// Data held by each node in the EpmTargetTree containing information used by the
    /// EpmCustomContentWriter visitor
    /// </summary>
    internal sealed class EpmCustomContentWriterNodeData : IDisposable
    {
        /// <summary>
        /// IDisposable helper state
        /// </summary>
        private bool disposed;

#if ASTORIA_CLIENT
        /// <summary>Initializes the per node data for custom serializer</summary>
        /// <param name="segment">Segment in target tree corresponding to this node</param>
        /// <param name="element">Object from which to read properties</param>
        internal EpmCustomContentWriterNodeData(EpmTargetPathSegment segment, object element)
#else
        /// <summary>Initializes the per node data for custom serializer</summary>
        /// <param name="segment">Segment in target tree corresponding to this node</param>
        /// <param name="element">Object from which to read properties</param>
        /// <param name="nullValuedProperties">Null valued properties found during serialization</param>
        /// <param name="provider">Data Service provider used for rights verification.</param>
        internal EpmCustomContentWriterNodeData(EpmTargetPathSegment segment, object element, EpmContentSerializer.EpmNullValuedPropertyTree nullValuedProperties, DataServiceProviderWrapper provider)
#endif
        {
            this.XmlContentStream = new MemoryStream();
            XmlWriterSettings customContentWriterSettings = new XmlWriterSettings();
            customContentWriterSettings.OmitXmlDeclaration = true;
            customContentWriterSettings.ConformanceLevel = ConformanceLevel.Fragment;
            this.XmlContentWriter = XmlWriter.Create(this.XmlContentStream, customContentWriterSettings);
#if ASTORIA_CLIENT
            this.PopulateData(segment, element);
#else
            this.PopulateData(segment, element, nullValuedProperties, provider);
#endif
        }

#if ASTORIA_CLIENT
        /// <summary>Initializes the per node data for custom serializer</summary>
        /// <param name="parentData">Parent node whose xml writer we are going to reuse</param>
        /// <param name="segment">Segment in target tree corresponding to this node</param>
        /// <param name="element">Object from which to read properties</param>
        internal EpmCustomContentWriterNodeData(EpmCustomContentWriterNodeData parentData, EpmTargetPathSegment segment, object element)
#else
        /// <summary>Initializes the per node data for custom serializer</summary>
        /// <param name="parentData">Parent node whose xml writer we are going to reuse</param>
        /// <param name="segment">Segment in target tree corresponding to this node</param>
        /// <param name="element">Object from which to read properties</param>
        /// <param name="nullValuedProperties">Null valued properties found during serialization</param>
        /// <param name="provider">Data Service provider used for rights verification.</param>
        internal EpmCustomContentWriterNodeData(EpmCustomContentWriterNodeData parentData, EpmTargetPathSegment segment, object element, EpmContentSerializer.EpmNullValuedPropertyTree nullValuedProperties, DataServiceProviderWrapper provider)
#endif
        {
            this.XmlContentStream = parentData.XmlContentStream;
            this.XmlContentWriter = parentData.XmlContentWriter;
#if ASTORIA_CLIENT
            this.PopulateData(segment, element);
#else
            this.PopulateData(segment, element, nullValuedProperties, provider);
#endif
        }

        /// <summary>
        /// Memory stream on top of which XmlWriter works
        /// </summary>
        internal MemoryStream XmlContentStream
        {
            get;
            private set;
        }

        /// <summary>
        /// Xml writer used for holding custom content fragment
        /// </summary>
        internal XmlWriter XmlContentWriter
        {
            get;
            private set;
        }

        /// <summary>Data for current node</summary>
        internal String Data
        {
            get;
            private set;
        }

        /// <summary>
        /// Closes XmlWriter and disposes the MemoryStream
        /// </summary>
        public void Dispose()
        {
            if (!this.disposed)
            {
                if (this.XmlContentWriter != null)
                {
                    this.XmlContentWriter.Close();
                    this.XmlContentWriter = null;
                }

                if (this.XmlContentStream != null)
                {
                    this.XmlContentStream.Dispose();
                    this.XmlContentStream = null;
                }

                this.disposed = true;
            }
        }

        /// <summary>
        /// Adds the content generated through custom serialization to the SyndicationItem or XmlWriter
        /// </summary>
        /// <param name="target">SyndicationItem or XmlWriter being serialized</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "XmlReader on MemoryStream does not require disposal")]
#if ASTORIA_CLIENT
        internal void AddContentToTarget(XmlWriter target)
#else
        internal void AddContentToTarget(SyndicationItem target)
#endif
        {
#if ASTORIA_CLIENT
            Util.CheckArgumentNull(target, "target");
#else
            WebUtil.CheckArgumentNull(target, "target");
#endif
            this.XmlContentWriter.Close();
            this.XmlContentWriter = null;
            this.XmlContentStream.Seek(0, SeekOrigin.Begin);
            XmlReaderSettings customContentReaderSettings = new XmlReaderSettings();
            customContentReaderSettings.ConformanceLevel = ConformanceLevel.Fragment;
            XmlReader reader = XmlReader.Create(this.XmlContentStream, customContentReaderSettings);
            this.XmlContentStream = null;
#if ASTORIA_CLIENT
            target.WriteNode(reader, false);
#else
            target.ElementExtensions.Add(reader);
#endif
        }

#if ASTORIA_CLIENT
        /// <summary>
        /// Populates the data value corresponding to this node, also updates the list of null attributes
        /// in the parent null attribute list if current node is attribute with null value
        /// </summary>
        /// <param name="segment">Segment being populated</param>
        /// <param name="element">Object whose property will be read</param>
        private void PopulateData(EpmTargetPathSegment segment, object element)
#else
        /// <summary>
        /// Populates the data value corresponding to this node, also updates the list of null attributes
        /// in the parent null attribute list if current node is attribute with null value
        /// </summary>
        /// <param name="segment">Segment being populated</param>
        /// <param name="element">Object whose property will be read</param>
        /// <param name="nullValuedProperties">Null valued properties found during serialization</param>
        /// <param name="provider">Data Service provider used for rights verification.</param>
        private void PopulateData(EpmTargetPathSegment segment, object element, EpmContentSerializer.EpmNullValuedPropertyTree nullValuedProperties, DataServiceProviderWrapper provider)
#endif
        {
            if (segment.EpmInfo != null)
            {
                Object propertyValue;

                try
                {
#if ASTORIA_CLIENT
                    propertyValue = segment.EpmInfo.ReadPropertyValue(element);
#else
                    propertyValue = segment.EpmInfo.ReadPropertyValue(element, provider);
#endif
                }
                catch 
#if ASTORIA_CLIENT
                (System.Reflection.TargetInvocationException)
#else
                (System.Reflection.TargetInvocationException e)
#endif
                {
#if !ASTORIA_CLIENT
                    ErrorHandler.HandleTargetInvocationException(e);
#endif
                    throw;
                }

#if ASTORIA_CLIENT
                this.Data = propertyValue == null ? String.Empty : ClientConvert.ToString(propertyValue, false /* atomDateConstruct */);
#else
                if (propertyValue == null || propertyValue == DBNull.Value)
                {
                    this.Data = String.Empty;
                    nullValuedProperties.Add(segment.EpmInfo);
                }
                else
                {
                    this.Data = PlainXmlSerializer.PrimitiveToString(propertyValue);
                }
#endif
            }
        }
    }
}
