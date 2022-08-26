//---------------------------------------------------------------------
// <copyright file="MetadataSerializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a serializer for CSDL documents.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    #region Namespaces.

    using System.Collections.Generic;
    using System.Data.Services.Common;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Xml;

    #endregion Namespaces.

    /// <summary>
    /// Provides support for serializing responses in CSDL format.
    /// </summary>
    internal sealed class MetadataSerializer : XmlDocumentSerializer
    {
        /// <summary>
        /// Initializes a new XmlDocumentSerializer, ready to write
        /// out an XML document
        /// </summary>
        /// <param name="output">Stream to which output should be sent.</param>
        /// <param name="baseUri">Base URI from which resources should be resolved.</param>
        /// <param name="provider">Data provider from which metadata should be gathered.</param>
        /// <param name="encoding">Text encoding for the response.</param>
        internal MetadataSerializer(
            Stream output,
            Uri baseUri,
            DataServiceProviderWrapper provider,
            Encoding encoding)
            : base(output, baseUri, provider, encoding)
        {
        }

        /// <summary>Gets the appropriate DataServiceVersion string for the metadata in the specified <paramref name="types"/>.</summary>
        /// <param name="types">Types to get version information for.</param>
        /// <param name="metadataEdmSchemaVersion">EDM schema version for metadata document.</param>
        /// <returns>The DataServiceVersion string for the metadata, possibly null.</returns>
        /// <remarks>
        /// This method should really hinge on the IDataServiceMetadataProvider, but this provides a convenient
        /// way of ensuring way enumerate types a single time, by allowing callers to cache
        /// the Types enumeration request.
        /// </remarks>
        internal static string GetVersionsForMetadata(IEnumerable<ResourceType> types, ref MetadataEdmSchemaVersion metadataEdmSchemaVersion)
        {
            Debug.Assert(types != null, "types != null");
            String dataServiceVersion = XmlConstants.DataServiceVersion1Dot0;

            foreach (ResourceType type in types)
            {
                Debug.Assert(type.EpmInfoInitialized, "type.EpmInfoInitialized -- otherwise the call is too soon to determine version.");
                if (!type.EpmIsV1Compatible)
                {
                    dataServiceVersion = XmlConstants.DataServiceVersion2Dot0;
                }

                // Open types force schema version to be 1.2.
                if (type.IsOpenType)
                {
                    if (metadataEdmSchemaVersion < MetadataEdmSchemaVersion.Version1Dot2)
                    {
                        metadataEdmSchemaVersion = MetadataEdmSchemaVersion.Version1Dot2;
                    }

                    // Since we have set both versions to their max values, we can leave the function immediately.
                    if (dataServiceVersion == XmlConstants.DataServiceVersion2Dot0)
                    {
                        break;
                    }

                    Debug.Assert(dataServiceVersion == XmlConstants.DataServiceVersion1Dot0, "Version is either 1.0 or 2.0 for DataService");
                }
            }

            return dataServiceVersion;
        }

        /// <summary>
        /// Writes the edmx elements for the metadata document.
        /// </summary>
        /// <param name="writer">XmlWriter into which the metadata document needs to be written.</param>
        /// <param name="dataServiceVersion">Data service version to include with top-level information.</param>
        internal static void WriteTopLevelSchemaElements(XmlWriter writer, string dataServiceVersion)
        {
            Debug.Assert(writer != null, "writer != null");
            Debug.Assert(dataServiceVersion != null, "dataServiceVersion != null");

            // <edmx:Edmx xmlns:edmx='http://schemas.microsoft.com/ado/2007/06/edmx'>
            writer.WriteStartElement(XmlConstants.EdmxNamespacePrefix, XmlConstants.EdmxElement, XmlConstants.EdmxNamespace);
            writer.WriteAttributeString(XmlConstants.EdmxVersion, XmlConstants.EdmxVersionValue);

            // <edmx:DataServices m:DataServiceVersion='?' xmlns:m='http://schemas.microsoft.com/ado/2007/08/dataservices/metadata'>
            writer.WriteStartElement(XmlConstants.EdmxNamespacePrefix, XmlConstants.EdmxDataServicesElement, XmlConstants.EdmxNamespace);
            writer.WriteAttributeString(XmlConstants.XmlnsNamespacePrefix, XmlConstants.DataWebMetadataNamespacePrefix, null, XmlConstants.DataWebMetadataNamespace);
            writer.WriteAttributeString(XmlConstants.DataWebMetadataNamespacePrefix, XmlConstants.HttpDataServiceVersion, XmlConstants.DataWebMetadataNamespace, dataServiceVersion);
        }

        /// <summary>
        /// Writes the lop level schema element
        /// </summary>
        /// <param name="writer">xml writer into which the schema node definition is written to</param>
        /// <param name="schemaNamespace">namespace of the schema</param>
        /// <param name="metadataEdmSchemaVersion">Version of metadata schema.</param>
        internal static void WriteSchemaElement(XmlWriter writer, string schemaNamespace, MetadataEdmSchemaVersion metadataEdmSchemaVersion)
        {
            writer.WriteStartElement(XmlConstants.Schema, GetSchemaNamespace(metadataEdmSchemaVersion));
            writer.WriteAttributeString(XmlConstants.Namespace, schemaNamespace);
            writer.WriteAttributeString(XmlConstants.XmlnsNamespacePrefix, XmlConstants.DataWebNamespacePrefix, null, XmlConstants.DataWebNamespace);
            writer.WriteAttributeString(XmlConstants.XmlnsNamespacePrefix, XmlConstants.DataWebMetadataNamespacePrefix, null, XmlConstants.DataWebMetadataNamespace);
        }

        /// <summary>Writes an attribute in the dataweb metadata namespace.</summary>
        /// <param name="writer">XmlWriter in which the attribute needs to be written</param>
        /// <param name="name">Attribute name.</param>
        /// <param name="value">Attribute value.</param>
        internal static void WriteDataWebMetadata(XmlWriter writer, string name, string value)
        {
            Debug.Assert(writer != null, "writer != null");
            Debug.Assert(name != null, "name != null");
            Debug.Assert(value != null, "value != null");
            writer.WriteAttributeString(XmlConstants.DataWebMetadataNamespacePrefix, name, XmlConstants.DataWebMetadataNamespace, value);
        }

        /// <summary>
        /// Writes the service operations as FunctionImports in the specified <paramref name="writer"/>.
        /// </summary>
        /// <param name="writer">Writer to which CSDL is being written to.</param>
        /// <param name="provider">Underlying data provider from which we need to get the list of service operations that we need to write.</param>
        internal static void WriteServiceOperations(
            XmlWriter writer,
            DataServiceProviderWrapper provider)
        {
            Debug.Assert(writer != null, "writer != null");

            foreach (ServiceOperationWrapper operation in provider.ServiceOperations)
            {
                string returnTypeString;
                string entitySetName = null;

                if (operation.ResultKind == ServiceOperationResultKind.Void)
                {
                    returnTypeString = null;
                }
                else
                {
                    ResourceType resourceType = operation.ResultType;
                    returnTypeString = resourceType.FullName;
                    if (resourceType.ResourceTypeKind == ResourceTypeKind.EntityType)
                    {
                        ResourceSetWrapper container = operation.ResourceSet;
                        Debug.Assert(
                            container != null,
                            "this.TryFindAnyContainerForType(operation.ResultType, out container) -- otherwise, we didn't trim operation '" + operation.Name + "' based on its type having no container.");
                        entitySetName = container.Name;
                    }

                    if (operation.ResultKind != ServiceOperationResultKind.QueryWithSingleResult &&
                        operation.ResultKind != ServiceOperationResultKind.DirectValue)
                    {
                        Debug.Assert(
                            operation.ResultKind == ServiceOperationResultKind.Enumeration ||
                            operation.ResultKind == ServiceOperationResultKind.QueryWithMultipleResults,
                            operation.ResultKind + " == Enumeration or QueryWithMultipleResults");
                        returnTypeString = String.Format(CultureInfo.InvariantCulture, XmlConstants.EdmCollectionTypeFormat, returnTypeString);
                    }
                }

                writer.WriteStartElement(XmlConstants.EdmFunctionImportElementName);
                writer.WriteAttributeString(XmlConstants.Name, operation.Name);

                if (entitySetName != null)
                {
                    writer.WriteAttributeString(XmlConstants.EdmEntitySetAttributeName, entitySetName);
                }

                if (returnTypeString != null)
                {
                    writer.WriteAttributeString(XmlConstants.EdmReturnTypeAttributeName, returnTypeString);
                }

                MetadataSerializer.WriteDataWebMetadata(writer, XmlConstants.ServiceOperationHttpMethodName, operation.Method);

                if (!String.IsNullOrEmpty(operation.MimeType))
                {
                    MetadataSerializer.WriteDataWebMetadata(writer, XmlConstants.DataWebMimeTypeAttributeName, operation.MimeType);
                }

                foreach (ServiceOperationParameter parameter in operation.Parameters)
                {
                    writer.WriteStartElement(XmlConstants.EdmParameterElementName);
                    writer.WriteAttributeString(XmlConstants.Name, parameter.Name);
                    writer.WriteAttributeString(XmlConstants.Type, parameter.ParameterType.FullName);
                    writer.WriteAttributeString(XmlConstants.EdmModeAttributeName, XmlConstants.EdmModeInValue);
                    writer.WriteEndElement();
                }

                writer.WriteEndElement();
            }
        }

        /// <summary>Handles the complete serialization for the specified content.</summary>
        /// <param name="service">Data service instance.</param>
        internal override void WriteRequest(IDataService service)
        {
            this.Provider.WriteMetadataDocument(this, this.Writer, service);
        }

        /// <summary>Gets the metadata document for this provider.</summary>
        /// <param name="metadataEdmSchemaVersion">EDM schema version.</param>
        /// <param name="service">Data service instance.</param>
        internal void GenerateMetadata(MetadataEdmSchemaVersion metadataEdmSchemaVersion, IDataService service)
        {
            Debug.Assert(this.Writer != null, "this.Writer != null");
            Debug.Assert(this.Provider != null, "this.Provider != null");

            MetadataManager metadataManager = new MetadataManager(this.Provider, service);
            string dataServiceVersion = MetadataSerializer.GetVersionsForMetadata(metadataManager.ResourceTypes, ref metadataEdmSchemaVersion);
            MetadataSerializer.WriteTopLevelSchemaElements(this.Writer, dataServiceVersion);
            HashSet<ResourceType> typesInEntityContainerNamespace = null;

            // Write the schema Element for every namespace
            foreach (KeyValuePair<string, HashSet<ResourceType>> namespaceAlongWithTypes in metadataManager.NamespaceAlongWithTypes)
            {
                Debug.Assert(!string.IsNullOrEmpty(namespaceAlongWithTypes.Key), "!string.IsNullOrEmpty(namespaceAlongWithTypes.Key)");

                // If the types live in the same namespace as that of entity container and types that don't live in any namespace,
                // should be written out in the service namespace. If the service type also doesn't have a namespace, we will use
                // the service name as the namespace. See code below for that.
                if (namespaceAlongWithTypes.Key == metadataManager.GetContainerNamespace())
                {
                    typesInEntityContainerNamespace = namespaceAlongWithTypes.Value;
                }
                else
                {
                    Dictionary<string, ResourceAssociationType> associationsInThisNamespace = metadataManager.GetAssociationTypesForNamespace(namespaceAlongWithTypes.Key);
                    MetadataSerializer.WriteSchemaElement(this.Writer, namespaceAlongWithTypes.Key, metadataEdmSchemaVersion);
                    WriteTypes(this.Writer, namespaceAlongWithTypes.Value, associationsInThisNamespace, metadataManager);
                    WriteAssociationTypes(this.Writer, new HashSet<ResourceAssociationType>(associationsInThisNamespace.Values, EqualityComparer<ResourceAssociationType>.Default));
                    this.Writer.WriteEndElement();
                }
            }

            // Write the entity container definition. Also if there are types in the same namespace,
            // we need to write them out too.
            string typeNamespace = metadataManager.GetContainerNamespace();
            MetadataSerializer.WriteSchemaElement(this.Writer, typeNamespace, metadataEdmSchemaVersion);
            if (typesInEntityContainerNamespace != null)
            {
                Dictionary<string, ResourceAssociationType> associationsInThisNamespace = metadataManager.GetAssociationTypesForNamespace(typeNamespace);
                WriteTypes(this.Writer, typesInEntityContainerNamespace, associationsInThisNamespace, metadataManager);
                WriteAssociationTypes(this.Writer, new HashSet<ResourceAssociationType>(associationsInThisNamespace.Values, EqualityComparer<ResourceAssociationType>.Default));
            }

            this.WriteEntityContainer(this.Writer, XmlConvert.EncodeName(this.Provider.ContainerName), metadataManager.ResourceSets, metadataManager.ResourceAssociationSets);
            this.Writer.WriteEndElement();

            // These end elements balance the elements written out in WriteTopLevelSchemaElements
            this.Writer.WriteEndElement();
            this.Writer.WriteEndElement();
            this.Writer.Flush();
        }

        /// <summary>
        /// Writes the definition of types in the XmlWriter
        /// </summary>
        /// <param name="xmlWriter">xmlWriter in which metadata needs to be written</param>
        /// <param name="types">resourceTypes whose metadata needs to be written</param>
        /// <param name="associationsInThisNamespace">list of associations in the current namespace.</param>
        /// <param name="metadataManager">metadata manager instance</param>
        private static void WriteTypes(
            XmlWriter xmlWriter,
            IEnumerable<ResourceType> types,
            Dictionary<string, ResourceAssociationType> associationsInThisNamespace,
            MetadataManager metadataManager)
        {
            foreach (ResourceType type in types)
            {
                if (ResourceTypeKind.EntityType == type.ResourceTypeKind)
                {
                    WriteEntityType(xmlWriter, type, associationsInThisNamespace, metadataManager);
                }
                else
                {
                    Debug.Assert(ResourceTypeKind.ComplexType == type.ResourceTypeKind, "this must be a complex type");
                    WriteComplexType(xmlWriter, type, metadataManager);
                }
            }
        }

        /// <summary>
        /// Write the metadata for the entityType in the xmlWriter
        /// </summary>
        /// <param name="xmlWriter">xmlWriter in which metadata needs to be written</param>
        /// <param name="entityType">entityType whose metadata needs to be written</param>
        /// <param name="associationsInThisNamespace">list of associations in the current namespace.</param>
        /// <param name="metadataManager">metadata manager instance</param>
        private static void WriteEntityType(
            XmlWriter xmlWriter,
            ResourceType entityType,
            Dictionary<string, ResourceAssociationType> associationsInThisNamespace,
            MetadataManager metadataManager)
        {
            Debug.Assert(xmlWriter != null, "XmlWriter cannot be null");
            Debug.Assert(entityType.ResourceTypeKind == ResourceTypeKind.EntityType, "Type must be entityType");

            xmlWriter.WriteStartElement(XmlConstants.EntityType);
            xmlWriter.WriteAttributeString(XmlConstants.Name, XmlConvert.EncodeName(entityType.Name));

            if (entityType.IsAbstract)
            {
                xmlWriter.WriteAttributeString(XmlConstants.Abstract, "true");
            }

            if (entityType.IsOpenType)
            {
                if (entityType.BaseType == null || entityType.BaseType.IsOpenType == false)
                {
                    WriteOpenTypeAttribute(xmlWriter);
                }
            }

            // Write the HasStream attribute only if this is an MLE and the base type isn't also an MLE (if it is,
            // it's sufficient that HasStream is written on the base type). 
            if (entityType.IsMediaLinkEntry && (entityType.BaseType == null || entityType.BaseType.IsMediaLinkEntry == false))
            {
                xmlWriter.WriteAttributeString(
                    XmlConstants.DataWebAccessHasStreamAttribute,
                    XmlConstants.DataWebMetadataNamespace,
                    XmlConstants.DataWebAccessDefaultStreamPropertyValue);
            }

            if (entityType.HasEntityPropertyMappings)
            {
                WriteEpmProperties(xmlWriter, entityType.InheritedEpmInfo, false, false);
            }

            if (entityType.BaseType != null)
            {
                xmlWriter.WriteAttributeString(XmlConstants.BaseType, XmlConvert.EncodeName(entityType.BaseType.FullName));
            }
            else
            {
                xmlWriter.WriteStartElement(XmlConstants.Key);
                foreach (ResourceProperty property in entityType.KeyProperties)
                {
                    xmlWriter.WriteStartElement(XmlConstants.PropertyRef);
                    xmlWriter.WriteAttributeString(XmlConstants.Name, property.Name);
                    xmlWriter.WriteEndElement();
                }

                xmlWriter.WriteEndElement();
            }

            WriteProperties(xmlWriter, entityType, associationsInThisNamespace, metadataManager);
            xmlWriter.WriteEndElement();
        }

        /// <summary>
        /// Writes the content for csdl corresponding to EpmProperties given in the <paramref name="epmInfos"/>
        /// </summary>
        /// <param name="xmlWriter">XmlWriter to write to</param>
        /// <param name="epmInfos">List of properties to write to csdl</param>
        /// <param name="skipSourcePath">Should source path be given</param>
        /// <param name="removePrefix">Remove prefix from source path for complex types</param>
        private static void WriteEpmProperties(XmlWriter xmlWriter, IEnumerable<EntityPropertyMappingAttribute> epmInfos, bool skipSourcePath, bool removePrefix)
        {
            EpmAttributeNameBuilder epmAttributeNameBuilder = new EpmAttributeNameBuilder();

            foreach (EntityPropertyMappingAttribute attr in epmInfos)
            {
                // Decide b/w atom & non-atom
                if (attr.TargetSyndicationItem == SyndicationItemProperty.CustomProperty)
                {               
                    xmlWriter.WriteAttributeString(
                                epmAttributeNameBuilder.EpmTargetPath,
                                XmlConstants.DataWebMetadataNamespace, 
                                attr.TargetPath);
                                
                    xmlWriter.WriteAttributeString(
                                epmAttributeNameBuilder.EpmNsUri,
                                XmlConstants.DataWebMetadataNamespace, 
                                attr.TargetNamespaceUri);
                                
                    if (!String.IsNullOrEmpty(attr.TargetNamespacePrefix))
                    {
                        xmlWriter.WriteAttributeString(
                                epmAttributeNameBuilder.EpmNsPrefix,
                                XmlConstants.DataWebMetadataNamespace, 
                                attr.TargetNamespacePrefix);
                    }
                }
                else
                {                   
                    xmlWriter.WriteAttributeString(
                                epmAttributeNameBuilder.EpmTargetPath,
                                XmlConstants.DataWebMetadataNamespace, 
                                ObjectContextServiceProvider.MapSyndicationPropertyToEpmTargetPath(attr.TargetSyndicationItem));
                    
                    xmlWriter.WriteAttributeString(
                                epmAttributeNameBuilder.EpmContentKind,
                                XmlConstants.DataWebMetadataNamespace, 
                                ObjectContextServiceProvider.MapSyndicationTextContentKindToEpmContentKind(attr.TargetTextContentKind));
                }

                if (!skipSourcePath)
                {
                    xmlWriter.WriteAttributeString(
                                epmAttributeNameBuilder.EpmSourcePath,
                                XmlConstants.DataWebMetadataNamespace,
                                removePrefix ? attr.SourcePath.Substring(attr.SourcePath.IndexOf('/') + 1) : attr.SourcePath);
                }

                xmlWriter.WriteAttributeString(
                            epmAttributeNameBuilder.EpmKeepInContent,
                            XmlConstants.DataWebMetadataNamespace, 
                            attr.KeepInContent ? XmlConstants.XmlTrueLiteral : XmlConstants.XmlFalseLiteral);
                            
                epmAttributeNameBuilder.MoveNext();
            }
        }

        /// <summary>
        /// Write the metadata for the complexType in the xmlWriter
        /// </summary>
        /// <param name="xmlWriter">xmlWriter in which metadata needs to be written</param>
        /// <param name="complexType">complexType whose metadata needs to be written</param>
        /// <param name="metadataManager">metadata manager instance</param>
        private static void WriteComplexType(XmlWriter xmlWriter, ResourceType complexType, MetadataManager metadataManager)
        {
            Debug.Assert(xmlWriter != null, "XmlWriter cannot be null");
            Debug.Assert(complexType.ResourceTypeKind == ResourceTypeKind.ComplexType, "Type must be complexType");
            Debug.Assert(complexType.IsOpenType == false, "Complex types cannot be open.");

            xmlWriter.WriteStartElement(XmlConstants.ComplexType);
            xmlWriter.WriteAttributeString(XmlConstants.Name, XmlConvert.EncodeName(complexType.Name));
            WriteProperties(xmlWriter, complexType, null, metadataManager);
            xmlWriter.WriteEndElement();
        }

        /// <summary>
        /// Adds the OpenType attribute to the Complex/Entity element.
        /// </summary>
        /// <param name="xmlWriter">Writer to which metadata is being written.</param>
        private static void WriteOpenTypeAttribute(XmlWriter xmlWriter)
        {
            xmlWriter.WriteAttributeString(XmlConstants.DataWebOpenTypeAttributeName, XmlConstants.XmlTrueLiteral);
        }

        /// <summary>
        /// Write the metadata of all the properties for the given typein the xmlWriter
        /// </summary>
        /// <param name="xmlWriter">xmlWriter in which metadata needs to be written</param>
        /// <param name="type">resource type whose property metadata needs to be written</param>
        /// <param name="associationsInThisNamespace">list of associations in the current namespace.</param>
        /// <param name="metadataManager">metadata manager instance</param>
        private static void WriteProperties(
            XmlWriter xmlWriter,
            ResourceType type,
            Dictionary<string, ResourceAssociationType> associationsInThisNamespace,
            MetadataManager metadataManager)
        {
            Debug.Assert(xmlWriter != null, "xmlWriter != null");
            Debug.Assert(type != null, "type != null");
            
            foreach (ResourceProperty resourceProperty in metadataManager.GetPropertiesDeclaredInThisType(type))
            {
                string elementName;

                // For primitive types, get the corresponding edm typename
                if (resourceProperty.TypeKind == ResourceTypeKind.Primitive)
                {
                    xmlWriter.WriteStartElement(XmlConstants.Property);
                    xmlWriter.WriteAttributeString(XmlConstants.Name, resourceProperty.Name);
                    xmlWriter.WriteAttributeString(XmlConstants.Type, resourceProperty.ResourceType.FullName);
                    WritePrimitivePropertyFacets(xmlWriter, resourceProperty);
                    if (!String.IsNullOrEmpty(resourceProperty.MimeType))
                    {
                        MetadataSerializer.WriteDataWebMetadata(xmlWriter, XmlConstants.DataWebMimeTypeAttributeName, resourceProperty.MimeType);
                    }

                    if (type.ResourceTypeKind == ResourceTypeKind.EntityType &&
                        type.ETagProperties.Contains(resourceProperty))
                    {
                        xmlWriter.WriteAttributeString(XmlConstants.ConcurrencyAttribute, XmlConstants.ConcurrencyFixedValue);
                    }
                    
                    if (type.HasEntityPropertyMappings)
                    {
                        var currentEpmInfos = type.OwnEpmInfo.Where(e => e.SourcePath.Split('/').First() == resourceProperty.Name);
                        WriteEpmProperties(xmlWriter, currentEpmInfos, true, false);
                    }
                }
                else if (resourceProperty.Kind == ResourcePropertyKind.ComplexType)
                {
                    xmlWriter.WriteStartElement(XmlConstants.Property);
                    xmlWriter.WriteAttributeString(XmlConstants.Name, resourceProperty.Name);
                    elementName = resourceProperty.ResourceType.FullName;
                    xmlWriter.WriteAttributeString(XmlConstants.Type, elementName);

                    // Edm doesn't support nullable complex type properties
                    xmlWriter.WriteAttributeString(XmlConstants.Nullable, XmlConstants.XmlFalseLiteral);
                    
                    if (type.HasEntityPropertyMappings)
                    {
                        var currentEpmInfos = type.OwnEpmInfo.Where(e => e.SourcePath.Split('/').First() == resourceProperty.Name);
                        WriteEpmProperties(xmlWriter, currentEpmInfos, currentEpmInfos.Any(ei => ei.SourcePath == resourceProperty.Name), true);
                    }
                }
                else
                {
                    Debug.Assert(
                        ResourceTypeKind.EntityType == resourceProperty.TypeKind,
                        "Unexpected property type kind");

                    xmlWriter.WriteStartElement(XmlConstants.NavigationProperty);
                    xmlWriter.WriteAttributeString(XmlConstants.Name, resourceProperty.Name);
                    elementName = resourceProperty.ResourceType.FullName;

                    string associationTypeLookupName = MetadataManager.GetAssociationTypeLookupName(type, resourceProperty);
                    ResourceAssociationType associationType;
                    if (!associationsInThisNamespace.TryGetValue(associationTypeLookupName, out associationType))
                    {
                        throw new InvalidOperationException(Strings.MetadataSerializer_NoResourceAssociationSetForNavigationProperty(resourceProperty.Name, type.FullName));
                    }

                    ResourceAssociationTypeEnd thisEnd = associationType.GetResourceAssociationTypeEnd(type, resourceProperty);
                    ResourceAssociationTypeEnd relatedEnd = associationType.GetRelatedResourceAssociationSetEnd(type, resourceProperty);
                    xmlWriter.WriteAttributeString(XmlConstants.Relationship, associationType.FullName);
                    xmlWriter.WriteAttributeString(XmlConstants.FromRole, thisEnd.Name);
                    xmlWriter.WriteAttributeString(XmlConstants.ToRole, relatedEnd.Name);
               }

                xmlWriter.WriteEndElement();
            }
        }

        /// <summary>
        /// Write the facets for clr types
        /// </summary>
        /// <param name="xmlWriter">XmlWriter in which facets needs to be written</param>
        /// <param name="resourceProperty">property which contains the primitive type for which facets needs to be written</param>
        private static void WritePrimitivePropertyFacets(XmlWriter xmlWriter, ResourceProperty resourceProperty)
        {
            Debug.Assert(resourceProperty.IsOfKind(ResourcePropertyKind.Primitive), "property must be of primitive type");

            bool nullable = true;
            if (resourceProperty.IsOfKind(ResourcePropertyKind.Key) || (resourceProperty.Type.IsValueType && Nullable.GetUnderlyingType(resourceProperty.Type) == null))
            {
                nullable = false;
            }

            xmlWriter.WriteAttributeString(XmlConstants.Nullable, nullable ? XmlConstants.XmlTrueLiteral : XmlConstants.XmlFalseLiteral);
        }

        /// <summary>
        /// Writes the metadata for the given associations.
        /// </summary>
        /// <param name="xmlWriter">xmlWriter in which metadata needs to be written.</param>
        /// <param name="associations">associations whose metadata need to be written.</param>
        private static void WriteAssociationTypes(XmlWriter xmlWriter, IEnumerable<ResourceAssociationType> associations)
        {
            foreach (ResourceAssociationType association in associations)
            {
                xmlWriter.WriteStartElement(XmlConstants.Association);
                xmlWriter.WriteAttributeString(XmlConstants.Name, association.Name);

                foreach (ResourceAssociationTypeEnd endInfo in new[] { association.End1, association.End2 })
                {
                    xmlWriter.WriteStartElement(XmlConstants.End);
                    xmlWriter.WriteAttributeString(XmlConstants.Role, endInfo.Name);

                    // The ends are of reference type
                    xmlWriter.WriteAttributeString(XmlConstants.Type, endInfo.ResourceType.FullName);

                    // Write the multiplicity value
                    xmlWriter.WriteAttributeString(XmlConstants.Multiplicity, endInfo.Multiplicity);
                    xmlWriter.WriteEndElement();
                }

                xmlWriter.WriteEndElement();
            }
        }

        /// <summary>
        /// Writes the metadata for association sets for the given associations.
        /// </summary>
        /// <param name="xmlWriter">xmlWriter in which metadata needs to be written</param>
        /// <param name="associationSets">associations for which association sets needs to be written.</param>
        private static void WriteAssociationSets(XmlWriter xmlWriter, IEnumerable<ResourceAssociationSet> associationSets)
        {
            Debug.Assert(xmlWriter != null, "xmlWriter != null");
            Debug.Assert(associationSets != null, "associationSets != null");

            foreach (ResourceAssociationSet associationSet in associationSets)
            {
                xmlWriter.WriteStartElement(XmlConstants.AssociationSet);
                xmlWriter.WriteAttributeString(XmlConstants.Name, associationSet.Name);
                Debug.Assert(associationSet.ResourceAssociationType != null, "associationSet.ResourceAssociationType != null");
                xmlWriter.WriteAttributeString(XmlConstants.Association, associationSet.ResourceAssociationType.FullName);
                
                // The Role names on the association set must match that of the corresponding association type
                // we'll just use the Role names from the AssociationType. we ignore the names from the association set ends.
                ResourceAssociationTypeEnd associationTypeEnd1 = associationSet.ResourceAssociationType.GetResourceAssociationTypeEnd(associationSet.End1.ResourceType, associationSet.End1.ResourceProperty);
                ResourceAssociationTypeEnd associationTypeEnd2 = associationSet.ResourceAssociationType.GetResourceAssociationTypeEnd(associationSet.End2.ResourceType, associationSet.End2.ResourceProperty);
                WriteAssociationSetEnd(xmlWriter, associationSet.End1, associationTypeEnd1);
                WriteAssociationSetEnd(xmlWriter, associationSet.End2, associationTypeEnd2);
                xmlWriter.WriteEndElement();
            }
        }

        /// <summary>
        /// Writes the metadata for association set end.
        /// </summary>
        /// <param name="xmlWriter">xmlWriter in which metadata needs to be written</param>
        /// <param name="associationSetEnd">association set end to be written.</param>
        /// <param name="associationTypeEnd">corresponding association type end</param>
        private static void WriteAssociationSetEnd(XmlWriter xmlWriter, ResourceAssociationSetEnd associationSetEnd, ResourceAssociationTypeEnd associationTypeEnd)
        {
            xmlWriter.WriteStartElement(XmlConstants.End);
            xmlWriter.WriteAttributeString(XmlConstants.Role, associationTypeEnd.Name);
            xmlWriter.WriteAttributeString(XmlConstants.EntitySet, associationSetEnd.ResourceSet.Name);
            xmlWriter.WriteEndElement();
        }

        /// <summary>Returns the schema namespace given the schema version.</summary>
        /// <param name="schemaVersion">EDM schema version.</param>
        /// <returns>Namespace corresponding to the schema version.</returns>
        private static string GetSchemaNamespace(MetadataEdmSchemaVersion schemaVersion)
        {
            switch (schemaVersion)
            {
                case MetadataEdmSchemaVersion.Version1Dot0:
                    return XmlConstants.EdmV1Namespace;

                case MetadataEdmSchemaVersion.Version1Dot1:
                    return XmlConstants.EdmV1dot1Namespace;

                case MetadataEdmSchemaVersion.Version1Dot2:
                    return XmlConstants.EdmV1dot2Namespace;

                default:
                    Debug.Assert(schemaVersion == MetadataEdmSchemaVersion.Version2Dot0, "Schema version must be 2.0.");
                    return XmlConstants.EdmV2Namespace;
            }
        }

        /// <summary>
        /// Writes the entity container definition
        /// </summary>
        /// <param name="xmlWriter">xmlWriter into which metadata is written</param>
        /// <param name="entityContainerName">name of the entity container</param>
        /// <param name="entitySets">list of entity set name and containing element type name</param>
        /// <param name="associationSets">associations for which association sets metadata needs to be written.</param>
        private void WriteEntityContainer(
            XmlWriter xmlWriter,
            string entityContainerName,
            IEnumerable<ResourceSetWrapper> entitySets,
            IEnumerable<ResourceAssociationSet> associationSets)
        {
            xmlWriter.WriteStartElement(XmlConstants.EntityContainer);
            xmlWriter.WriteAttributeString(XmlConstants.Name, entityContainerName);

            // Since reflection based provider supports only one entity container, we should write the 
            // default entity container attribute for the only entity container
            MetadataSerializer.WriteDataWebMetadata(xmlWriter, XmlConstants.IsDefaultEntityContainerAttribute, XmlConstants.XmlTrueLiteral);

            foreach (ResourceSetWrapper container in entitySets)
            {
                xmlWriter.WriteStartElement(XmlConstants.EntitySet);
                xmlWriter.WriteAttributeString(XmlConstants.Name, container.Name);
                xmlWriter.WriteAttributeString(XmlConstants.EntityType, container.ResourceType.FullName);
                xmlWriter.WriteEndElement();
            }

            WriteAssociationSets(xmlWriter, associationSets);

            // Write metadata for service operations
            MetadataSerializer.WriteServiceOperations(xmlWriter, this.Provider);

            xmlWriter.WriteEndElement();
        }

        /// <summary>
        /// Finds all visible resource sets, types, service ops, association sets, etc. from the given provider
        /// </summary>
        private sealed class MetadataManager
        {
            #region Private Fields

            /// <summary>data service provider instance</summary>
            private DataServiceProviderWrapper provider;

            /// <summary>List of namespace along with the types in that namespace</summary>
            private Dictionary<string, HashSet<ResourceType>> resourceTypes = new Dictionary<string, HashSet<ResourceType>>(StringComparer.Ordinal);

            /// <summary>
            /// List of namespace along with association types in that namespace
            /// The key of the dictionary is ResourceType_ResourceProperty, used for looking up the association type when
            /// given the navigation property.  This means the dictionary will contain duplicate association types when
            /// there are bi-directional associations.
            /// </summary>
            private Dictionary<string, Dictionary<string, ResourceAssociationType>> associationTypes = new Dictionary<string, Dictionary<string, ResourceAssociationType>>(EqualityComparer<string>.Default);

            /// <summary>Dictionary of visible association sets</summary>
            private Dictionary<string, ResourceAssociationSet> associationSets = new Dictionary<string, ResourceAssociationSet>(EqualityComparer<string>.Default);

            /// <summary>Set to true if we see any visible MLE.</summary>
            private bool hasVisibleMediaLinkEntry;

            #endregion Private Fields

            #region Constructor

            /// <summary>Constructs a metadata manager instance</summary>
            /// <param name="provider">Data service provider instance.</param>
            /// <param name="service">Data service instance.</param>
            internal MetadataManager(DataServiceProviderWrapper provider, IDataService service)
            {
                Debug.Assert(provider != null, "provider != null");
                this.provider = provider;

                if (this.provider.Configuration.AccessEnabledForAllResourceTypes)
                {
                    // If all types is marked visible, we can simply add them
                    foreach (ResourceType resourceType in this.provider.Types)
                    {
                        this.AddVisibleResourceType(resourceType);
                    }
                }
                else
                {
                    // Add entity types reachable from visible sets
                    foreach (ResourceSetWrapper resourceSet in this.provider.ResourceSets)
                    {
                        this.PopulateTypesForSet(resourceSet);
                    }

                    // Add resource types reachable from service operations
                    foreach (ServiceOperationWrapper serviceOperation in this.provider.ServiceOperations)
                    {
                        this.PopulateTypeForServiceOperation(serviceOperation);
                    }

                    // Add resource types marked visible by DataServiceConfiguration.EnableAccess().
                    foreach (string resourceTypeName in this.provider.Configuration.GetAccessEnabledResourceTypes())
                    {
                        ResourceType resourceType = this.provider.TryResolveResourceType(resourceTypeName);
                        if (resourceType == null)
                        {
                            throw new InvalidOperationException(Strings.MetadataSerializer_AccessEnabledTypeNoLongerExists(resourceTypeName));
                        }

                        this.AddVisibleResourceType(resourceType);
                    }
                }

                // Add association set and association types for visible sets
                // NOTE that this CANNOT be in the same loop as above. Reachable entity
                // types need to be populated before we can populate the associations.
                foreach (ResourceSetWrapper resourceSet in this.provider.ResourceSets)
                {
                    this.PopulateAssociationsForSet(resourceSet);
                }

                // If we have encountered a visible MLE type, we need to make sure there is a stream provider implementation
                // for this service or else we can end up with in-stream errors during runtime.
                if (this.hasVisibleMediaLinkEntry)
                {
                    // LoadStreamProvider will throw if we can't locate a stream provider implementation.
                    DataServiceStreamProviderWrapper.LoadStreamProvider(service);
                }
            }

            #endregion Constructor

            #region Properties

            /// <summary>List of visible resource sets</summary>
            internal IEnumerable<ResourceSetWrapper> ResourceSets
            {
                get { return this.provider.ResourceSets; }
            }
            
            /// <summary>Returns the list of namespace and the types in those namespaces.</summary>
            internal IEnumerable<KeyValuePair<string, HashSet<ResourceType>>> NamespaceAlongWithTypes
            {
                get { return this.resourceTypes; }
            }

            /// <summary>List of visible resource types</summary>
            internal IEnumerable<ResourceType> ResourceTypes
            {
                get
                {
                    foreach (var namespaceTypePair in this.resourceTypes)
                    {
                        foreach (ResourceType resourceType in namespaceTypePair.Value)
                        {
                            yield return resourceType;
                        }
                    }
                }
            }

            /// <summary>List of visible association sets</summary>
            internal IEnumerable<ResourceAssociationSet> ResourceAssociationSets
            {
                // The associationSets dictionary contains one entry per direction, we need to construct a hashset to return the unique values
                get { return new HashSet<ResourceAssociationSet>(this.associationSets.Values, EqualityComparer<ResourceAssociationSet>.Default); }
            }

            #endregion Properties

            #region Methods

            /// <summary>
            /// Given the association set, generate the association type name.
            /// </summary>
            /// <param name="associationSet">association set</param>
            /// <returns>association type name.</returns>
            internal static string GetAssociationTypeName(ResourceAssociationSet associationSet)
            {
                Debug.Assert(associationSet != null, "associationSet != null");

                // end1 is always the end with the navigation property
                ResourceAssociationSetEnd end1 = associationSet.End1.ResourceProperty != null ? associationSet.End1 : associationSet.End2;

                // end2 points to the other end and is null if the other end has null navigation property
                ResourceAssociationSetEnd end2 = (end1 == associationSet.End1 ? (associationSet.End2.ResourceProperty != null ? associationSet.End2 : null) : null);

                string associationTypeName = end1.ResourceType.Name + '_' + end1.ResourceProperty.Name;
                if (end2 != null)
                {
                    associationTypeName = associationTypeName + '_' + end2.ResourceType.Name + '_' + end2.ResourceProperty.Name;
                }

                return associationTypeName;
            }

            /// <summary>
            /// get the string key to look up an association type from the namespace.
            /// </summary>
            /// <param name="resourceType">resource type</param>
            /// <param name="resourceProperty">resource property</param>
            /// <returns>lookup key</returns>
            internal static string GetAssociationTypeLookupName(ResourceType resourceType, ResourceProperty resourceProperty)
            {
                Debug.Assert(resourceType != null, "resourceType != null");
                string lookupName = resourceType.Name;
                if (resourceProperty != null)
                {
                    lookupName = lookupName + '_' + resourceProperty.Name;
                }

                return lookupName;
            }

            /// <summary>
            /// Get the dictionary of ----ication types for the given namespace
            /// </summary>
            /// <param name="typeNamespace">namespace</param>
            /// <returns>association types</returns>
            internal Dictionary<string, ResourceAssociationType> GetAssociationTypesForNamespace(string typeNamespace)
            {
                Dictionary<string, ResourceAssociationType> associationsInThisNamespace;
                if (!this.associationTypes.TryGetValue(typeNamespace, out associationsInThisNamespace))
                {
                    associationsInThisNamespace = new Dictionary<string, ResourceAssociationType>(StringComparer.Ordinal);
                    this.associationTypes.Add(typeNamespace, associationsInThisNamespace);
                }

                return associationsInThisNamespace;
            }

            /// <summary>Gets the namespace of the container. if it's null, default to the container name</summary>
            /// <returns>namespace of the default container.</returns>
            internal string GetContainerNamespace()
            {
                string typeNamespace = this.provider.ContainerNamespace;
                if (String.IsNullOrEmpty(typeNamespace))
                {
                    typeNamespace = XmlConvert.EncodeName(this.provider.ContainerName);
                }

                return typeNamespace;
            }

            /// <summary>Gets the namespace of a resource type. if it's null, default to the container namespace</summary>
            /// <param name="resourceType">type in question</param>
            /// <returns>namespace of the type</returns>
            internal string GetTypeNamepace(ResourceType resourceType)
            {
                string typeNamespace = resourceType.Namespace;
                if (string.IsNullOrEmpty(typeNamespace))
                {
                    typeNamespace = this.GetContainerNamespace();
                }

                return typeNamespace;
            }

            /// <summary>Gets the list of visible resource properties from a type.</summary>
            /// <param name="resourceType">Type in question.</param>
            /// <returns>List of visible resource properties.</returns>
            internal IEnumerable<ResourceProperty> GetPropertiesDeclaredInThisType(ResourceType resourceType)
            {
                foreach (ResourceProperty property in resourceType.PropertiesDeclaredOnThisType)
                {
                    if (property.TypeKind == ResourceTypeKind.EntityType)
                    {
                        ResourceType propertyType = property.ResourceType;
                        string typeNamespace = this.GetTypeNamepace(resourceType);
                        HashSet<ResourceType> typesInNamespace = this.GetResourceTypesForNamespace(typeNamespace);
                        if (!typesInNamespace.Contains(propertyType))
                        {
                            continue;
                        }
                    }

                    yield return property;
                }
            }

            /// <summary>
            /// Get the resource type hash for the given namespace
            /// </summary>
            /// <param name="typeNamespace">namespace</param>
            /// <returns>resource type hash</returns>
            private HashSet<ResourceType> GetResourceTypesForNamespace(string typeNamespace)
            {
                HashSet<ResourceType> typesInSameNamespace;
                if (!this.resourceTypes.TryGetValue(typeNamespace, out typesInSameNamespace))
                {
                    typesInSameNamespace = new HashSet<ResourceType>(EqualityComparer<ResourceType>.Default);
                    this.resourceTypes.Add(typeNamespace, typesInSameNamespace);
                }

                return typesInSameNamespace;
            }

            /// <summary>Add a resource type to the list of visible types</summary>
            /// <param name="resourceType">resource type to add</param>
            /// <returns>True if we successfully added the type, false if the type is already in the hashset.</returns>
            private bool AddVisibleResourceType(ResourceType resourceType)
            {
                string typeNamespace = this.GetTypeNamepace(resourceType);
                HashSet<ResourceType> typesInSameNamespace = this.GetResourceTypesForNamespace(typeNamespace);
                if (resourceType.IsMediaLinkEntry)
                {
                    this.hasVisibleMediaLinkEntry = true;
                }

                return typesInSameNamespace.Add(resourceType);
            }

            /// <summary>Recursively add complex types reachable through resource properties of the given type</summary>
            /// <param name="resourceType">resource type to inspect</param>
            private void AddComplexPropertTypes(ResourceType resourceType)
            {
                Debug.Assert(resourceType != null, "resourceType != null");

                foreach (ResourceProperty property in resourceType.PropertiesDeclaredOnThisType)
                {
                    if (property.ResourceType.ResourceTypeKind == ResourceTypeKind.ComplexType)
                    {
                        if (this.AddVisibleResourceType(property.ResourceType))
                        {
                            this.AddComplexPropertTypes(property.ResourceType);
                        }
                    }
                }
            }
            
            /// <summary>Populate all reachable types from a resource set</summary>
            /// <param name="resourceSet">resource set to inspect</param>
            private void PopulateTypesForSet(ResourceSetWrapper resourceSet)
            {
                // Derived types of a visible type are visible
                foreach (ResourceType derivedType in this.provider.GetDerivedTypes(resourceSet.ResourceType))
                {
                    this.AddVisibleResourceType(derivedType);
                    this.AddComplexPropertTypes(derivedType);
                }

                // Base types of a visible type are visible
                ResourceType resourceType = resourceSet.ResourceType;
                while (resourceType != null)
                {
                    this.AddVisibleResourceType(resourceType);
                    this.AddComplexPropertTypes(resourceType);
                    resourceType = resourceType.BaseType;
                }
            }

            /// <summary>
            /// Get the resource association type from the given resource association set and one of its ends
            /// </summary>
            /// <param name="associationSet">Association set to get the association type</param>
            /// <param name="resourceSet">Resource set for one of the ends</param>
            /// <param name="resourceType">Resource type for one of the ends</param>
            /// <param name="navigationProperty">Resource property for one of the ends</param>
            /// <returns>Resource association type instance</returns>
            private ResourceAssociationType GetResourceAssociationType(ResourceAssociationSet associationSet, ResourceSetWrapper resourceSet, ResourceType resourceType, ResourceProperty navigationProperty)
            {
                string typeNamespace = this.GetTypeNamepace(resourceType);
                Dictionary<string, ResourceAssociationType> associationTypesInNamespece = this.GetAssociationTypesForNamespace(typeNamespace);

                ResourceAssociationType associationType;
                string associationTypeName = GetAssociationTypeName(associationSet);
                string associationTypeLookupKey = GetAssociationTypeLookupName(resourceType, navigationProperty);
                if (!associationTypesInNamespece.TryGetValue(associationTypeLookupKey, out associationType))
                {
                    string end1Name;
                    string end2Name;

                    bool isBiDirectional = associationSet.End1.ResourceProperty != null && associationSet.End2.ResourceProperty != null;
                    if (!isBiDirectional)
                    {
                        // If this association is not bi-directional, we use the type name as the from role name and the property name as the to role name
                        // This is the behavior for V1.
                        if (associationSet.End1.ResourceProperty != null)
                        {
                            end1Name = resourceType.Name;
                            end2Name = navigationProperty.Name;
                        }
                        else
                        {
                            end1Name = navigationProperty.Name;
                            end2Name = resourceType.Name;
                        }
                    }
                    else
                    {
                        // If the association is bi-directional, we use typeName_propertyName from each end as the name for that role
                        end1Name = GetAssociationTypeLookupName(associationSet.End1.ResourceType, associationSet.End1.ResourceProperty);
                        end2Name = GetAssociationTypeLookupName(associationSet.End2.ResourceType, associationSet.End2.ResourceProperty);
                        Debug.Assert(end1Name != end2Name, "end1Name != end2Name");
                    }

                    associationType = new ResourceAssociationType(
                        associationTypeName,
                        resourceType.Namespace,
                        new ResourceAssociationTypeEnd(end1Name, associationSet.End1.ResourceType, associationSet.End1.ResourceProperty, associationSet.End2.ResourceProperty),
                        new ResourceAssociationTypeEnd(end2Name, associationSet.End2.ResourceType, associationSet.End2.ResourceProperty, associationSet.End1.ResourceProperty));

                    associationTypesInNamespece.Add(associationTypeLookupKey, associationType);
                    if (isBiDirectional)
                    {
                        ResourceAssociationSetEnd relatedEnd = associationSet.GetRelatedResourceAssociationSetEnd(resourceSet, resourceType, navigationProperty);
                        string relatedEndLookupKey = GetAssociationTypeLookupName(relatedEnd.ResourceType, relatedEnd.ResourceProperty);
                        associationTypesInNamespece.Add(relatedEndLookupKey, associationType);
                    }
                }

                return associationType;
            }

            /// <summary>
            /// Gets and validates the ResourceAssociationSet instance when given the source association end.
            /// </summary>
            /// <param name="resourceSet">Resource set of the source association end.</param>
            /// <param name="resourceType">Resource type of the source association end.</param>
            /// <param name="navigationProperty">Resource property of the source association end.</param>
            /// <returns>ResourceAssociationSet instance.</returns>
            private ResourceAssociationSet GetAndValidateResourceAssociationSet(ResourceSetWrapper resourceSet, ResourceType resourceType, ResourceProperty navigationProperty)
            {
                Debug.Assert(resourceSet != null, "resourceSet != null");
                Debug.Assert(resourceType != null, "resourceType != null");
                Debug.Assert(navigationProperty != null, "navigationProperty != null");
                Debug.Assert(resourceType.TryResolvePropertiesDeclaredOnThisTypeByName(navigationProperty.Name) != null, "navigationProperty must be declared on resourceType.");

                string associationSetKey = resourceSet.Name + '_' + resourceType.FullName + '_' + navigationProperty.Name;
                ResourceAssociationSet associationSet;
                if (this.associationSets.TryGetValue(associationSetKey, out associationSet))
                {
                    return associationSet;
                }

                associationSet = this.provider.GetResourceAssociationSet(resourceSet, resourceType, navigationProperty);
                if (associationSet != null)
                {
                    ResourceAssociationSetEnd relatedEnd = associationSet.GetRelatedResourceAssociationSetEnd(resourceSet, resourceType, navigationProperty);

                    // If this is a two way relationship, GetResourceAssociationSet should return the same association set when called from either end.
                    // For example, it's not valid to have the association sets {GoodCustomerSet} <-> {OrdersSet} and {BadCustomerSet} <-> {OrdersSet}
                    // because starting from {OrderSet} we won't know which customers set to resolve to.
                    if (relatedEnd.ResourceProperty != null)
                    {
                        ResourceAssociationSet reverseAssociationSet = this.provider.GetResourceAssociationSet(this.provider.ValidateResourceSet(relatedEnd.ResourceSet), relatedEnd.ResourceType, relatedEnd.ResourceProperty);
                        if (reverseAssociationSet == null || associationSet.Name != reverseAssociationSet.Name)
                        {
                            throw new InvalidOperationException(Strings.ResourceAssociationSet_BidirectionalAssociationMustReturnSameResourceAssociationSetFromBothEnd);
                        }
                    }

                    // Cache the association set for the reverse direction.
                    string reverseAssociationSetKey;
                    if (relatedEnd.ResourceProperty != null)
                    {
                        reverseAssociationSetKey = relatedEnd.ResourceSet.Name + '_' + relatedEnd.ResourceProperty.ResourceType.FullName + '_' + relatedEnd.ResourceProperty.Name;
                    }
                    else
                    {
                        reverseAssociationSetKey = relatedEnd.ResourceSet.Name + "_Null_" + resourceType.FullName + '_' + navigationProperty.Name;
                    }

                    ResourceAssociationSet conflictingAssociationSet;
                    if (this.associationSets.TryGetValue(reverseAssociationSetKey, out conflictingAssociationSet))
                    {
                        // If only one of the ends is already in the cache, we know that the provider is giving us inconsistant metadata.
                        // Make sure that if two or more AssociationSets refer to the same AssociationType, the ends must not refer to the same EntitySet.
                        // For CLR context, this could happen if multiple entity sets have entity types that have a common ancestor and the ancestor has a property of derived entity types.
                        throw new InvalidOperationException(Strings.ResourceAssociationSet_MultipleAssociationSetsForTheSameAssociationTypeMustNotReferToSameEndSets(conflictingAssociationSet.Name, associationSet.Name, relatedEnd.ResourceSet.Name));
                    }

                    this.associationSets.Add(reverseAssociationSetKey, associationSet);
                    this.associationSets.Add(associationSetKey, associationSet);
                }

                return associationSet;
            }

            /// <summary>Populate associations for the given set and type</summary>
            /// <param name="resourceSet">resource type to inspect</param>
            /// <param name="resourceType">resource set to inspect</param>
            private void PopulateAssociationsForSetAndType(ResourceSetWrapper resourceSet, ResourceType resourceType)
            {
                Debug.Assert(resourceSet != null, "resourceSet != null");
                Debug.Assert(resourceType != null, "resourceType != null");

                foreach (ResourceProperty navigationProperty in resourceType.PropertiesDeclaredOnThisType.Where(p => p.TypeKind == ResourceTypeKind.EntityType))
                {
                    // For every nav property, add a new association set to the metadata
                    ResourceAssociationSet associationSet = this.GetAndValidateResourceAssociationSet(resourceSet, resourceType, navigationProperty);
                    if (associationSet != null)
                    {
                        associationSet.ResourceAssociationType = this.GetResourceAssociationType(associationSet, resourceSet, resourceType, navigationProperty);
                        Debug.Assert(associationSet.ResourceAssociationType != null, "associationSet.ResourceAssociationType != null");
                    }
                }
            }

            /// <summary>Populate associations from the given set</summary>
            /// <param name="resourceSet">resource set to inspect</param>
            private void PopulateAssociationsForSet(ResourceSetWrapper resourceSet)
            {
                Debug.Assert(resourceSet != null, "resourceSet != null");

                // Populate for derived types
                foreach (ResourceType derivedType in this.provider.GetDerivedTypes(resourceSet.ResourceType))
                {
                    this.PopulateAssociationsForSetAndType(resourceSet, derivedType);
                }

                // Populate for this type and its base types
                ResourceType resourceType = resourceSet.ResourceType;
                this.PopulateAssociationsForSetAndType(resourceSet, resourceType);
                while (resourceType != null)
                {
                    this.PopulateAssociationsForSetAndType(resourceSet, resourceType);
                    resourceType = resourceType.BaseType;
                }
            }

            /// <summary>Populate resource types returned by the given service operation.</summary>
            /// <param name="serviceOperation">Service operation to inspect</param>
            private void PopulateTypeForServiceOperation(ServiceOperationWrapper serviceOperation)
            {
                Debug.Assert(serviceOperation != null, "serviceOperation != null");

                ResourceType resultType = serviceOperation.ResultType;
                if (resultType != null && resultType.ResourceTypeKind == ResourceTypeKind.ComplexType)
                {
                    this.AddVisibleResourceType(resultType);
                    this.AddComplexPropertTypes(resultType);
                }

                Debug.Assert(
                    resultType == null || resultType.ResourceTypeKind != ResourceTypeKind.EntityType ||
                    (this.resourceTypes.ContainsKey(resultType.Namespace) && this.resourceTypes[resultType.Namespace].Contains(resultType)),
                    "If a result type is an entity type, it must be visible through an entity set.");
            }

            #endregion Methods
        }
    }
}
