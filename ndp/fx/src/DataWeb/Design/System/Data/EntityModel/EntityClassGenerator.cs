//---------------------------------------------------------------------
// <copyright file="EntityClassGenerator.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner       Microsoft
// @backupOwner Microsoft
//---------------------------------------------------------------------

using System.Collections.Generic;
using System.Data.EntityModel;
using System.Data.Metadata.Edm;
using System.Data.Services.Design.Common;
using System.Data.Services.Design.Xml;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Xml;
using System.Xml.Linq;
using System.Xml.Schema;
using System.Xml.XPath;

namespace System.Data.Common.Utils
{
    internal static class EntityUtil
    {
        static internal void CheckArgumentNull<T>(T value, string parameterName) where T : class
        {
            System.Data.Services.Design.EntityUtil.CheckArgumentNull<T>(value, parameterName);
        }
    }
}

namespace System.Data.Services.Design
{
    internal static class EntityUtil
    {
        static internal void CheckArgumentNull<T>(T value, string parameterName) where T : class
        {
            EDesignUtil.CheckArgumentNull<T>(value, parameterName);
        }

        static internal void CheckStringArgument(string value, string parameterName)
        {
            EDesignUtil.CheckStringArgument(value, parameterName);
        }
    }

    /// <summary>
    /// Summary description for CodeGenerator.
    /// </summary>
    public sealed class EntityClassGenerator
    {
        #region Instance Fields
        private LanguageOption _languageOption = LanguageOption.GenerateCSharpCode;
        private DataServiceCodeVersion _version = DataServiceCodeVersion.V1;
        private EdmToObjectNamespaceMap _edmToObjectNamespaceMap = new EdmToObjectNamespaceMap();
        private bool _useDataServiceCollection;
#if QFE_ENV
        private bool _useDataServiceCollectionExplicitlySet;
        private bool _versionExplicitlySet;
        const string UseDSC_EnvironmentVariable = "dscodegen_usedsc";
        const string Version_EnvironmentVariable = "dscodegen_version";
        const string Version2Dot0 = "2.0";
        const string UseDSCTrue = "1";
#endif

        #endregion

        #region Events

        /// <summary>
        /// The event that is raised when a type is generated
        /// </summary>
        public event EventHandler<TypeGeneratedEventArgs> OnTypeGenerated;

        /// <summary>
        /// The event that is raised when a property is generated
        /// </summary>
        public event EventHandler<PropertyGeneratedEventArgs> OnPropertyGenerated;

        #endregion

        #region Public Methods
        /// <summary>
        /// 
        /// </summary>
        public EntityClassGenerator()
        {
        }

        /// <summary>
        /// 
        /// </summary>
        public EntityClassGenerator(LanguageOption languageOption)
        {
            _languageOption = EDesignUtil.CheckLanguageOptionArgument(languageOption, "languageOption");
        }

        /// <summary>
        /// Get or set the flag that specifies if code generation emits the code necessary for data binding
        /// </summary>
        public bool UseDataServiceCollection
        {
            get
            {
                return _useDataServiceCollection;
            }

            set
            {
                _useDataServiceCollection = value;
#if QFE_ENV 
                _useDataServiceCollectionExplicitlySet = true;
#endif
            }
        }

        /// <summary>
        /// Gets and Sets the WCF Data Service version which the generated code will be compatible with.
        /// </summary>
        public DataServiceCodeVersion Version
        {
            get 
            { 
                return _version; 
            }

            set 
            { 
                _version = EDesignUtil.CheckDataServiceCodeVersionArgument(value, "value"); 
#if QFE_ENV
                _versionExplicitlySet = true; 
#endif
            }
        }

        /// <summary>
        /// Gets and Sets the Language to use for code generation.
        /// </summary>
        public LanguageOption LanguageOption
        {
            get { return _languageOption; }
            set { _languageOption = EDesignUtil.CheckLanguageOptionArgument(value, "value"); }
        }

        /// <summary>
        /// Gets the map entries use to customize the namespace of .net types that are generated
        /// and referenced by the generated code
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Edm")]
        public EdmToObjectNamespaceMap EdmToObjectNamespaceMap
        {
            get { return _edmToObjectNamespaceMap; }
        }

        public IList<EdmSchemaError> GenerateCode(XmlReader sourceReader, string targetFilePath)
        {
            EntityUtil.CheckArgumentNull(sourceReader, "sourceReader");
            EntityUtil.CheckStringArgument(targetFilePath, "targetPath");
            using (LazyTextWriterCreator target = new LazyTextWriterCreator(targetFilePath))
            {
                // we do want to close the file
                return GenerateCode(sourceReader, target, null);
            }
        }

        /// <summary>
        /// Generate code by reading an EDMX schema from an XmlReader and outputting the code into a TextWriter
        /// </summary>
        /// <param name="sourceReader">Reader with a EDMX schema in it</param>
        /// <param name="targetWriter">Target writer for the generated code</param>
        /// <param name="namespacePrefix">Prefix to use for generated namespaces</param>
        /// <remarks>
        /// Note that the NamespacePrefix is used as the only namespace for types in the same namespace
        /// as the default container, and as a prefix for the server-provided namespace for everything else. If
        /// this argument is null, the server-provided namespaces are used for all types.
        /// </remarks>
        /// <returns></returns>
        public IList<EdmSchemaError> GenerateCode(XmlReader sourceReader, TextWriter targetWriter, string namespacePrefix)
        {
            EntityUtil.CheckArgumentNull(sourceReader, "sourceReader");
            EntityUtil.CheckArgumentNull(targetWriter, "targetWriter");
            using (LazyTextWriterCreator target = new LazyTextWriterCreator(targetWriter))
            {
                return GenerateCode(sourceReader, target, namespacePrefix);
            }   // does not actually close the targetWriter - that is the caller's responsibility
        }

        /// <summary>
        /// Given the specified element in a given namespace, remaps it into a 
        /// <paramref name="targetNamespace"/> and trims elements and attributes that
        /// don't conform to the schema.
        /// </summary>
        /// <param name="element">Element to fit.</param>
        /// <param name="schemaNamespace">Namespace of element.</param>
        /// <param name="targetNamespace">Target namespace.</param>
        /// <returns>A new <see cref="XElement"/> that fits the specified <paramref name="targetNamespace"/>.</returns>
        private static XElement FitElementToSchema(XElement element, string schemaNamespace, string targetNamespace)
        {
            Debug.Assert(element != null, "element != null");
            Debug.Assert(schemaNamespace != null, "schemaNamespace != null");
            Debug.Assert(targetNamespace != null, "targetNamespace != null");
            Debug.Assert(
                targetNamespace == XmlConstants.EdmV1dot1Namespace,
                "targetNamespace == XmlConstants.EdmV1dot1Namespace -- otherwise update CreateTargetSchemaSet to pull other schemas");

            XmlSchemaSet schemas = CreateTargetSchemaSet();
            XElement result = UpdateNamespaces(element, schemaNamespace, targetNamespace);
            XNodeSchemaApplier.Apply(schemas, result);
            return result;
        }

        /// <summary>
        /// Creates an <see cref="XmlReader"/> for the resource in the Entity Framework assembly
        /// specified by <paramref name="resourceName"/>.
        /// </summary>
        /// <param name="resourceName">Name of the resource to read.</param>
        /// <returns>A new <see cref="XmlReader"/> instance.</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "caller should Dispose the XmlReader")]
        private static XmlReader CreateEdmResourceXmlReader(string resourceName)
        {
            bool success = false;
            Stream stream = null;
            XmlReader result = null;
            try
            {
                stream = typeof(EdmItemCollection).Assembly.GetManifestResourceStream(resourceName);
                result = XmlReader.Create(stream);
                success = true;
                return result;
            }
            finally
            {
                if (!success)
                {
                    if (result != null)
                    {
                        result.Close();
                    }

                    if (stream != null)
                    {
                        stream.Dispose();
                    }
                }
            }
        }

        /// <summary>Creates the <see cref="XmlSchemaSet"/> that adjusted schemas should target.</summary>
        /// <returns>A new <see cref="XmlSchemaSet"/> instance.</returns>
        private static XmlSchemaSet CreateTargetSchemaSet()
        {
            XmlSchemaSet result = new XmlSchemaSet();
            using (XmlReader reader = CreateEdmResourceXmlReader("System.Data.Resources.CodeGenerationSchema.xsd"))
            {
                result.Add(null, reader);
            }

            using (XmlReader reader = CreateEdmResourceXmlReader("System.Data.Resources.CSDLSchema_1_1.xsd"))
            {
                XmlSchema schema = result.Add(null, reader);
                RemoveReferentialConstraint(schema);
                AddCustomAttributesToEntityContainer(schema);
            }

            XmlSchemaSet result2 = new XmlSchemaSet();
            foreach (XmlSchema s in result.Schemas())
            {
                result2.Add(s);
            }

            return result2;
        }

        /// <summary>
        /// Removes the ReferentialConstraint element from the list of valid children for an association type.
        /// </summary>
        /// <param name="csdlSchema">Loaded <see cref="XmlSchema"/> for the 1.1 Entity Framework CSDL XSD.</param>
        private static void RemoveReferentialConstraint(XmlSchema csdlSchema)
        {
            Debug.Assert(csdlSchema != null, "csdlSchema != null");

            XmlSchemaComplexType associationType = csdlSchema.SchemaTypes[new XmlQualifiedName("TAssociation", csdlSchema.TargetNamespace)] as XmlSchemaComplexType;
            Debug.Assert(associationType != null, "associationType != null -- otherwise can't find TAssociation - CSDL resource has changed?");

            XmlSchemaSequence sequence = associationType.Particle as XmlSchemaSequence;
            XmlSchemaObject referentialConstraint = null;
            foreach (XmlSchemaObject item in sequence.Items)
            {
                XmlSchemaElement e = item as XmlSchemaElement;
                if (e.QualifiedName == new XmlQualifiedName("ReferentialConstraint", csdlSchema.TargetNamespace))
                {
                    referentialConstraint = e;
                    break;
                }
            }

            Debug.Assert(referentialConstraint != null, "referentialConstraint != null");
            sequence.Items.Remove(referentialConstraint);
        }

        /// <summary>
        /// Add any attribute to the entity container element in the schema. We need to do
        /// this, since this was missing from the xsd that got shipped in System.Data.Entity.dll
        /// in 3.5 SP1. Hence we need to make compensating changes now here. The reason why validating
        /// against this csd works in edmitemcollection is that they ignore all errors due to elements/attributes
        /// not in the edm namespace.
        /// </summary>
        /// <param name="csdlSchema">Loaded <see cref="XmlSchema"/> for the 1.1 Entity Framework CSDL XSD.</param>
        private static void AddCustomAttributesToEntityContainer(XmlSchema csdlSchema)
        {
            Debug.Assert(csdlSchema != null, "csdlSchema != null");

            XmlSchemaElement entityContainerElement = csdlSchema.Elements[new XmlQualifiedName("EntityContainer", csdlSchema.TargetNamespace)] as XmlSchemaElement;
            Debug.Assert(entityContainerElement != null, "entityContainerElement != null -- otherwise can't find EntityContainer element- CSDL resource has changed?");

            XmlSchemaComplexType complexType = entityContainerElement.SchemaType as XmlSchemaComplexType;
            complexType.AnyAttribute = new XmlSchemaAnyAttribute();
            complexType.AnyAttribute.Namespace = "##other";
            complexType.AnyAttribute.ProcessContents = XmlSchemaContentProcessing.Lax;
        }

        /// <summary>
        /// Creates a list of readers for adjusted schemas in the specifed <paramref name="sourceReader"/>/
        /// </summary>
        /// <param name="sourceReader">Input source for metadata.</param>
        /// <returns>A list of readers for Schema elements.</returns>
        /// <remarks>
        /// These are some processing differences between V1 and V2 metadata.
        /// 
        /// - V1 processes all Schema elements at any depth.
        /// - V2 processes only Schema elements that are at the root or under *:Edmx/*:DataServices[1] nodes
        ///
        /// - V1 processes Schema elements from known namespaces
        /// - V2 processes Schema elements from the first Schema's namespace
        /// 
        /// - V1 ignores the DataServiceVersion attribute on the DataServices element
        /// - V2 rejects DataServiceVersion attributes with a version that is not 1.0 or 2.0
        /// 
        /// The detection hinges on whether all Schema elements belong to 1.1 and 1.0 namespaces;
        /// if so, we use V1 rules.
        /// </remarks>
        private static List<XmlReader> CreateReaders(XmlReader sourceReader)
        {
            Debug.Assert(sourceReader != null, "sourceReader != null");

            NameTable nameTable = new NameTable();
            XmlNamespaceManager namespaceManager = new XmlNamespaceManager(nameTable);
            namespaceManager.AddNamespace("m", XmlConstants.DataWebMetadataNamespace);
            namespaceManager.AddNamespace("edmx", XmlConstants.EdmxNamespace);
            namespaceManager.AddNamespace("edmv1", XmlConstants.EdmV1Namespace);
            namespaceManager.AddNamespace("edmv1_1", XmlConstants.EdmV1dot1Namespace);

            XDocument sourceDocument = XDocument.Load(sourceReader);
            List<XElement> schemaElements = new List<XElement>();
            List<XmlReader> result = new List<XmlReader>();

            // Check for V2 schemas.
            if (TryCreateReadersV2(sourceDocument, schemaElements))
            {
                Debug.Assert(schemaElements.Count > 0, "schemaElements.Count > 0 -- otherwise TryCreateReadersV2 should have returned false");
                string schemaNamespace = null;
                for (int i = 0; i < schemaElements.Count; i++)
                {
                    if (schemaNamespace == null)
                    {
                        schemaNamespace = schemaElements[i].Name.NamespaceName;
                    }
                    else
                    {
                        if (schemaNamespace != schemaElements[i].Name.NamespaceName)
                        {
                            throw new NotSupportedException(Strings.InvalidMetadataMultipleNamespaces(schemaNamespace, schemaElements[i].Name.NamespaceName));
                        }
                    }

                    XElement fitted = FitElementToSchema(schemaElements[i], schemaNamespace, XmlConstants.EdmV1dot1Namespace);
                    result.Add(fitted.CreateReader());
                }
            }
            else
            {
                Debug.Assert(result.Count == 0, "result.Count == 0 -- otherwise TryCreateReadesV2 should have returned true");
                CreateReadersV1(sourceDocument, namespaceManager, result);
            }

            return result;
        }

        /// <summary>Creates readers as per V1 rules.</summary>
        /// <param name="sourceDocument">Parsed source document.</param>
        /// <param name="namespaceManager">Namespace manager with 'edmv1' and 'edmv1_1' defined.</param>
        /// <param name="readers">List of XmlReaders to populate.</param>
        /// <remarks>See comments on CreateReaders for details.</remarks>
        private static void CreateReadersV1(XDocument sourceDocument, XmlNamespaceManager namespaceManager, List<XmlReader> readers)
        {
            Debug.Assert(sourceDocument != null, "sourceDocument != null");
            Debug.Assert(namespaceManager != null, "namespaceManager != null");
            Debug.Assert(readers != null, "readers != null");
            Debug.Assert(namespaceManager.HasNamespace("edmv1"), "namespaceManager.HasNamespace('edmv1')");
            Debug.Assert(namespaceManager.HasNamespace("edmv1_1"), "namespaceManager.HasNamespace('edmv1_1')");

            // Look for 1.0 schema elements
            foreach (var element in sourceDocument.XPathSelectElements("//edmv1:Schema", namespaceManager))
            {
                readers.Add(element.CreateReader());
            }

            // Look for 1.1 schema elements
            foreach (var element in sourceDocument.XPathSelectElements("//edmv1_1:Schema", namespaceManager))
            {
                readers.Add(element.CreateReader());
            }
        }

        /// <summary>
        /// Determines whether the specified <paramref name="namespaceName"/> is a known Schema element.
        /// </summary>
        /// <param name="namespaceName">Name of namespace to check.</param>
        /// <returns>true if <paramref name="namespaceName"/> is a known Schema element; false otherwise.</returns>
        private static bool IsKnownSchemaNamespace(string namespaceName)
        {
            return namespaceName == XmlConstants.EdmV1Namespace || namespaceName == XmlConstants.EdmV1dot1Namespace;
        }

        /// <summary>Determines whether the specified <paramref name="element"/> is a V2 Schema element.</summary>
        /// <param name="element">Element to check.</param>
        /// <returns>true if <paramref name="element"/> is a V2 Schema element.</returns>
        /// <remarks>
        /// A V2 Schema is an element with a local name of 'Schema', within a namespace that doesn'
        /// t belong in the known V1 namespaces.
        /// </remarks>
        private static bool IsSchemaV2(XElement element)
        {
            Debug.Assert(element != null, "element != null");
            var name = element.Name;
            return name.LocalName == "Schema" && !IsKnownSchemaNamespace(name.NamespaceName);
        }

        /// <summary>Tries to create readers according to V2 rules.</summary>
        /// <param name="sourceDocument">Source document for metadata.</param>
        /// <param name="schemaElements">List of elements to populate with Schema nodes.</param>
        /// <returns>true if schema elements were found according to V2 rules; false otherwise.</returns>
        /// <remarks>See comments on CreateReaders for details.</remarks>
        private static bool TryCreateReadersV2(XDocument sourceDocument, List<XElement> schemaElements)
        {
            Debug.Assert(sourceDocument != null, "sourceDocument != null");
            Debug.Assert(schemaElements != null, "schemaElements != null");

            bool result = false;

            // Check for a root Schema element in a new namespace.
            var root = sourceDocument.Root;
            if (IsSchemaV2(root))
            {
                schemaElements.Add(root);
                result = true;
            }
            else
            {
                if (root.Name.LocalName == XmlConstants.EdmxElement)
                {
                    XElement dataServiceElement = root.Elements().Where(e => e.Name.LocalName == XmlConstants.EdmxDataServicesElement).FirstOrDefault();
                    if (dataServiceElement != null)
                    {
                        XNamespace metadataNs = XmlConstants.DataWebMetadataNamespace;
                        XAttribute version = dataServiceElement.Attributes(metadataNs + XmlConstants.HttpDataServiceVersion).FirstOrDefault();
                        if (version != null && version.Value != XmlConstants.DataServiceVersion1Dot0 && version.Value != XmlConstants.DataServiceVersion2Dot0)
                        {
                            throw new InvalidOperationException(Strings.InvalidMetadataDataServiceVersion(version.Value));
                        }

                        schemaElements.AddRange(dataServiceElement.Elements().Where(IsSchemaV2));
                        result = schemaElements.Count > 0;
                    }
                }
            }

            return result;
        }

        /// <summary>
        /// Updates the namespaces under the specified <paramref name="element"/>, changing <paramref name="oldNamespaceName"/> 
        /// into <paramref name="newNamespaceName"/>.
        /// </summary>
        /// <param name="element">Element to update (recursively).</param>
        /// <param name="oldNamespaceName">Old namespace.</param>
        /// <param name="newNamespaceName">New namespace.</param>
        /// <returns>The updated element.</returns>
        /// <remarks>
        /// Currently, the updates are in-place, so the returned element is always <paramref name="element"/>.
        /// 
        /// For valid cases, there will be no qualified CSDL attributes, so those code paths are
        /// not likely to hit - however the code preserves the namespaces so it can fail
        /// correctly later on.
        /// </remarks>
        private static XElement UpdateNamespaces(XElement element, string oldNamespaceName, string newNamespaceName)
        {
            Debug.Assert(element != null, "element != null");
            Debug.Assert(oldNamespaceName != null, "oldNamespaceName != null");
            Debug.Assert(newNamespaceName != null, "newNamespaceName != null");

            XNamespace oldNamespace = XNamespace.Get(oldNamespaceName);
            XNamespace newNamespace = XNamespace.Get(newNamespaceName);

            Stack<XElement> pending = new Stack<XElement>();
            pending.Push(element);
            do
            {
                XElement e = pending.Pop();
                if (e.Name.Namespace == oldNamespace)
                {
                    e.Name = newNamespace.GetName(e.Name.LocalName);
                }

                List<XAttribute> attributesToReplace = null;
                foreach (XAttribute attribute in e.Attributes())
                {
                    if (attribute.IsNamespaceDeclaration)
                    {
                        if (attribute.Value == oldNamespaceName)
                        {
                            attribute.Value = newNamespaceName;
                        }
                    }
                    else if (attribute.Name.Namespace == oldNamespace || IsOpenTypeAttribute(attribute))
                    {
                        XNodeSchemaApplier.AppendWithCreation(ref attributesToReplace, attribute);
                    }
                }

                if (attributesToReplace != null)
                {
                    attributesToReplace.Remove();
                    foreach (XAttribute attribute in attributesToReplace)
                    {
                        if (IsOpenTypeAttribute(attribute))
                        {
                            XAttribute existingAttribute = e.Attributes().SingleOrDefault(a => a.Name.NamespaceName == XmlConstants.EdmV1dot2Namespace && a.Name.LocalName == attribute.Name.LocalName);

                            if (existingAttribute == null)
                            {
                                e.Add(new XAttribute(XNamespace.Get(XmlConstants.EdmV1dot2Namespace) + attribute.Name.LocalName, attribute.Value));
                            }
                            else
                            {
                                existingAttribute.Value = attribute.Value;
                            }
                        }
                        else
                        {
                            e.Add(new XAttribute(newNamespace.GetName(attribute.Name.LocalName), attribute.Value));
                        }
                    }
                }

                foreach (var child in e.Elements())
                {
                    pending.Push(child);
                }
            }
            while (pending.Count > 0);

            return element;
        }

        private IList<EdmSchemaError> GenerateCode(XmlReader sourceReader, LazyTextWriterCreator target, string namespacePrefix)
        {
            List<XmlReader> readers = CreateReaders(sourceReader);
            List<EdmSchemaError> errors = new List<EdmSchemaError>();
            EdmItemCollection itemCollection = new EdmItemCollection(readers);

#if QFE_ENV
            _version = _versionExplicitlySet ? _version : GetDataServiceCodeVersionFromEnvironment();
            _useDataServiceCollection = _useDataServiceCollectionExplicitlySet ? _useDataServiceCollection : GetUseDataServiceCollectionFromEnvironment();
#endif
            if (_useDataServiceCollection && _version == DataServiceCodeVersion.V1)
            {
                throw new InvalidOperationException(Strings.VersionV1RequiresUseDataServiceCollectionFalse);
            }

            // generate code
            using (ClientApiGenerator generator = new ClientApiGenerator(null, itemCollection, this, errors, namespacePrefix))
            {
                generator.GenerateCode(target);
            }

            return errors;
        }

#if QFE_ENV
        //[System.Security.Permissions.EnvironmentPermission(System.Security.Permissions.SecurityAction.Assert, Read = EntityClassGenerator.Version_EnvironmentVariable)]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2106")]
        private static DataServiceCodeVersion GetDataServiceCodeVersionFromEnvironment()
        {
            return Environment.GetEnvironmentVariable(EntityClassGenerator.Version_EnvironmentVariable) == EntityClassGenerator.Version2Dot0 ?
                                DataServiceCodeVersion.V2 : 
                                DataServiceCodeVersion.V1;
        }

        //[System.Security.Permissions.EnvironmentPermission(System.Security.Permissions.SecurityAction.Assert, Read = EntityClassGenerator.UseDSC_EnvironmentVariable)]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2106")]
        private static bool GetUseDataServiceCollectionFromEnvironment()
        {
            return Environment.GetEnvironmentVariable(EntityClassGenerator.UseDSC_EnvironmentVariable) == EntityClassGenerator.UseDSCTrue;
        }
#endif

        /// <summary>Checks if the given attribute refers to OpenType attribute.</summary>
        /// <param name="attribute">Input attribute.</param>
        /// <returns>true if the attribute is OpenType attribute, false otherwise.</returns>
        private static bool IsOpenTypeAttribute(XAttribute attribute)
        {
            return attribute.Name.LocalName == XmlConstants.DataWebOpenTypeAttributeName && attribute.Name.Namespace == XNamespace.None;
        }

        #endregion

        #region Event Helpers

        /// <summary>
        /// Helper method that raises the TypeGenerated event
        /// </summary>
        /// <param name="eventArgs">The event arguments passed to the subscriber</param>
        internal void RaiseTypeGeneratedEvent(TypeGeneratedEventArgs eventArgs)
        {
            if (this.OnTypeGenerated != null)
            {
                this.OnTypeGenerated(this, eventArgs);
            }
        }

        /// <summary>
        /// Helper method that raises the PropertyGenerated event
        /// </summary>
        /// <param name="eventArgs">The event arguments passed to the subscriber</param>
        internal void RaisePropertyGeneratedEvent(PropertyGeneratedEventArgs eventArgs)
        {
            if (this.OnPropertyGenerated != null)
            {
                this.OnPropertyGenerated(this, eventArgs);
            }
        }

        #endregion
    }
}
