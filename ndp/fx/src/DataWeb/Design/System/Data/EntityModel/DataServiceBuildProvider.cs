//------------------------------------------------------------------------------
// <copyright file="DataServiceBuildProvider.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// Putting the build provider in the System.Data.Services.Design namespace forces DataSvcUtil.exe to also take dependency on System.Web.dll
// Hence putting this in a different namespace.
[module: System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1020:AvoidNamespacesWithFewTypes", Scope = "namespace", Target = "System.Data.Services.BuildProvider")]
namespace System.Data.Services.BuildProvider
{
    using System;
    using System.Collections;
    using System.Data.Services.Design;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Web;
    using System.Web.Compilation;
    using System.Web.Hosting;
    using System.Xml;
    using System.Xml.Schema;

    /// <summary>
    /// A build provider for data service references in website projects.
    /// Data services (Astoria): in order to support Astoria "data services" we added code
    /// to scan for "datasvcmap" files in addition to the existing "svcmap" files. For data services
    /// we call into the Astoria code-gen library to do the work instead of the regular indigo path.
    ///     
    /// </summary>
    [FolderLevelBuildProviderAppliesTo(FolderLevelBuildProviderAppliesTo.WebReferences)]
    [System.Security.SecurityCritical]
    public class DataServiceBuildProvider : BuildProvider
    {
        private const string WebRefDirectoryName = "App_WebReferences";
        private const string DataSvcMapExtension = ".datasvcmap";

        private const string UseCollectionParameterName = "UseDataServiceCollection";
        private const string VersionParameterName = "Version";
        private const string Version1Dot0 = "1.0";
        private const string Version2Dot0 = "2.0";

        /// <summary>
        /// Search through the folder represented by base.VirtualPath for .svcmap and .datasvcmap files.  
        /// If any .svcmap/.datasvcmap files are found, then generate proxy code for them into the 
        /// specified assemblyBuilder.
        /// </summary>
        /// <param name="assemblyBuilder">Where to generate the proxy code</param>
        /// <remarks>
        /// When this routine is called, it is expected that the protected VirtualPath property has
        ///   been set to the folder to scan for .svcmap/.datasvcmap files.
        /// </remarks>
        [System.Security.SecuritySafeCritical]
        public override void GenerateCode(AssemblyBuilder assemblyBuilder)
        {
            // Go through all the svcmap files in the directory
            VirtualDirectory vdir = GetVirtualDirectory(VirtualPath);
            foreach (VirtualFile child in vdir.Files)
            {
                string extension = IO.Path.GetExtension(child.VirtualPath);
                if (extension.Equals(DataSvcMapExtension, StringComparison.OrdinalIgnoreCase))
                {
                    // NOTE: the WebReferences code requires a physical path, so this feature
                    // cannot work with a non-file based VirtualPathProvider
                    string physicalPath = HostingEnvironment.MapPath(child.VirtualPath);

                    GenerateCodeFromDataServiceMapFile(physicalPath, assemblyBuilder);
                }
            }
        }

        /// <summary>
        /// Generate code for one .datasvcmap file
        /// </summary>
        /// <param name="mapFilePath">The physical path to the data service map file</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "XmlReader over StringReader doesn't need to be disposed")]
        private void GenerateCodeFromDataServiceMapFile(string mapFilePath, AssemblyBuilder assemblyBuilder)
        {
            try
            {
                MapFileLoader loader = new MapFileLoader(mapFilePath);
                loader.LoadDataSvcMapFile();
                MapFileLoader.AddAssemblyReferences(assemblyBuilder, loader.UseDataServiceCollection);
                string edmxContent = loader.GetEdmxContent();
                System.Data.Services.Design.EntityClassGenerator generator = new System.Data.Services.Design.EntityClassGenerator(LanguageOption.GenerateCSharpCode);

                // the EntityClassGenerator works on streams/writers, does not return a CodeDom
                // object, so we use CreateCodeFile instead of compile units.
                using (TextWriter writer = assemblyBuilder.CreateCodeFile(this))
                {
                    generator.UseDataServiceCollection = loader.UseDataServiceCollection;
                    generator.Version = loader.Version;

                    // Note: currently GenerateCode never actually returns values
                    // for the error case (even though it returns an IList of error
                    // objects). Instead it throws on error. This may need some tweaking 
                    // later on.
#if DEBUG
                    object errors =
#endif
                    generator.GenerateCode(
                            XmlReader.Create(new StringReader(edmxContent)),
                            writer,
                            GetGeneratedNamespace());

#if DEBUG
                Debug.Assert(
                    errors == null ||
                    !(errors is ICollection) ||
                    ((ICollection)errors).Count == 0,
                    "Errors reported through the return value. Expected an exception");
#endif
                    writer.Flush();
                }
            }
            catch (Exception ex)
            {
                string errorMessage = ex.Message;
                errorMessage = String.Format(CultureInfo.CurrentCulture, "{0}: {1}", IO.Path.GetFileName(mapFilePath), errorMessage);
                throw new InvalidOperationException(errorMessage, ex);
            }
        }

        /// <summary>
        ///  Retrieve a VirtualDirectory for the given virtual path
        /// </summary>
        /// <param name="virtualPath"></param>
        /// <returns></returns>
        private VirtualDirectory GetVirtualDirectory(string virtualPath)
        {
            return HostingEnvironment.VirtualPathProvider.GetDirectory(VirtualPath);
        }

        /// <summary>
        ///  Caculate our namespace for current VirtualPath...
        /// </summary>
        /// <return></return>
        /// <remarks></remarks>
        private string GetGeneratedNamespace()
        {
            // First, determine the namespace to use for code generation.  This is based on the
            //   relative path of the reference from its base App_WebReferences directory

            // ... Get the virtual path to the App_WebReferences folder, e.g "/MyApp/App_WebReferences"
            string rootWebRefDirVirtualPath = GetWebRefDirectoryVirtualPath();

            // ... Get the folder's directory path, e.g "/MyApp/Application_WebReferences/Foo/Bar",
            //     where we'll look for .svcmap files
            string currentSubfolderUnderWebReferences = this.VirtualPath;
            if (currentSubfolderUnderWebReferences == null)
            {
                Debug.Fail("Shouldn't be given a null virtual path");
                throw new InvalidOperationException();
            }

            return CalculateGeneratedNamespace(rootWebRefDirVirtualPath, currentSubfolderUnderWebReferences);
        }

        /// <summary>
        /// Determine the namespace to use for the proxy generation
        /// </summary>
        /// <param name="webReferencesRootVirtualPath">The path to the App_WebReferences folder</param>
        /// <param name="virtualPath">The path to the current folder</param>
        /// <returns></returns>
        private static string CalculateGeneratedNamespace(string webReferencesRootVirtualPath, string virtualPath)
        {
            // ... Ensure both folders have trailing slashes
            webReferencesRootVirtualPath = VirtualPathUtility.AppendTrailingSlash(webReferencesRootVirtualPath);
            virtualPath = VirtualPathUtility.AppendTrailingSlash(virtualPath);

            Debug.Assert(virtualPath.StartsWith(webReferencesRootVirtualPath, StringComparison.OrdinalIgnoreCase),
                "We expected to be inside the App_WebReferences folder");

            // ... Determine the namespace to use, based on the directory structure where the .svcmap file
            //     is found.
            if (webReferencesRootVirtualPath.Length == virtualPath.Length)
            {
                Debug.Assert(string.Equals(webReferencesRootVirtualPath, virtualPath, StringComparison.OrdinalIgnoreCase),
                    "We expected to be in the App_WebReferences directory");

                // If it's the root WebReferences dir, use the empty namespace
                return String.Empty;
            }
            else
            {
                // We're in a subdirectory of App_WebReferences.
                // Get the directory's relative path from App_WebReferences, e.g. "Foo/Bar"

                virtualPath = VirtualPathUtility.RemoveTrailingSlash(virtualPath).Substring(webReferencesRootVirtualPath.Length);

                // Split it into chunks separated by '/'
                string[] chunks = virtualPath.Split('/');

                // Turn all the relevant chunks into valid namespace chunks
                for (int i = 0; i < chunks.Length; i++)
                {
                    chunks[i] = MakeValidTypeNameFromString(chunks[i]);
                }

                // Put the relevant chunks back together to form the namespace
                return String.Join(".", chunks);
            }
        }

        /// <summary>
        /// Returns the app domain's application virtual path [from HttpRuntime.AppDomainAppVPath].
        /// Includes trailing slash, e.g. "/MyApp/"
        /// </summary>
        private static string GetAppDomainAppVirtualPath()
        {
            string appVirtualPath = HttpRuntime.AppDomainAppVirtualPath;
            if (appVirtualPath == null)
            {
                Debug.Fail("Shouldn't get a null app virtual path from the app domain");
                throw new InvalidOperationException();
            }

            return VirtualPathUtility.AppendTrailingSlash(VirtualPathUtility.ToAbsolute(appVirtualPath));
        }

        /// <summary>
        /// Gets the virtual path to the application's App_WebReferences directory, e.g. "/MyApp/App_WebReferences/"
        /// </summary>
        private static string GetWebRefDirectoryVirtualPath()
        {
            return VirtualPathUtility.Combine(GetAppDomainAppVirtualPath(), WebRefDirectoryName + @"\");
        }

        /// <summary>
        /// Return a valid type name from a string by changing any character
        ///   that's not a letter or a digit to an '_'.
        /// </summary>
        /// <param name="s"></param>
        /// <returns></returns>
        internal static string MakeValidTypeNameFromString(string typeName)
        {
            if (String.IsNullOrEmpty(typeName))
                throw new ArgumentNullException("typeName");

            StringBuilder sb = new StringBuilder();

            for (int i = 0; i < typeName.Length; i++)
            {
                // Make sure it doesn't start with a digit (ASURT 31134)
                if (i == 0 && Char.IsDigit(typeName[0]))
                    sb.Append('_');

                if (Char.IsLetterOrDigit(typeName[i]))
                    sb.Append(typeName[i]);
                else
                    sb.Append('_');
            }

            // Identifier can't be a single underscore character
            string validTypeName = sb.ToString();
            if (validTypeName.Equals("_", StringComparison.Ordinal))
            {
                validTypeName = "__";
            }

            return validTypeName;
        }

        private class MapFileLoader
        {
            /// <summary>Namespace for the dataSvcMap file.</summary>
            private const string dataSvcSchemaNamespace = "urn:schemas-microsoft-com:xml-dataservicemap";

            /// <summary>xsd Schema for the datasvcmap file</summary>
            private static XmlSchemaSet serviceMapSchemaSet;

            /// <summary>xsd Schema for the datasvcmap file</summary>
            private static XmlNamespaceManager namespaceManager;

            /// <summary>full path of the data svc map file.</summary>
            private string dataSvcMapFilePath;

            /// <summary>Name of the edmx schema file.</summary>
            private string edmxSchemaFileName;

            /// <summary>True if DataServiceCollection needs to be used, otherwise false.</summary>
            private bool useDataServiceCollection;

            /// <summary>Version as specified in the svcmap file.</summary>
            private DataServiceCodeVersion version;

            /// <summary>
            /// Creates the new instance of the dataSvcMapFileLoader.
            /// </summary>
            /// <param name="mapFilePath">filePath for the dataSvcMap file.</param>
            internal MapFileLoader(string mapFilePath)
            {
                Debug.Assert(!String.IsNullOrEmpty(mapFilePath), "mapFilePath cannot be null or empty");
                this.dataSvcMapFilePath = mapFilePath;
            }

            /// <summary>True if the binding code needs to be generated, otherwise false.</summary>
            internal bool UseDataServiceCollection
            {
                get { return this.useDataServiceCollection; }
            }

            /// <summary>Returns the version as specified in the svcmap file.</summary>
            internal DataServiceCodeVersion Version
            {
                get { return this.version; }
            }

            /// <summary>
            /// Returns the XMlSchemaSet against which the dataSvcMap needs to be validated against.
            /// </summary>
            private static XmlSchemaSet ServiceMapSchemaSet
            {
                get
                {
                    if (serviceMapSchemaSet == null)
                    {
                        XmlSchema serviceMapSchema;
                        System.IO.Stream fileStream = typeof(WCFBuildProvider).Assembly.GetManifestResourceStream(@"System.Web.Compilation.WCFModel.Schema.DataServiceMapSchema.xsd");
                        System.Diagnostics.Debug.Assert(fileStream != null, "Couldn't find ServiceMapSchema.xsd resource");

                        serviceMapSchema = XmlSchema.Read(fileStream, null);
                        serviceMapSchemaSet = new XmlSchemaSet();
                        serviceMapSchemaSet.Add(serviceMapSchema);
                    }

                    return serviceMapSchemaSet;
                }
            }

            /// <summary>
            /// Returns the namespace manager for the dataSvcMap file.
            /// </summary>
            private static XmlNamespaceManager NamespaceManager
            {
                get
                {
                    if (namespaceManager == null)
                    {
                        namespaceManager = new XmlNamespaceManager(new NameTable());
                        namespaceManager.AddNamespace("ds", dataSvcSchemaNamespace);
                    }

                    return namespaceManager;
                }
            }

            /// <summary>
            /// Read the contents from the edmx file.
            /// </summary>
            /// <returns>a string instance containing all the contents of the edmx file.</returns>
            internal string GetEdmxContent()
            {
                string edmxFilePath = IO.Path.Combine(IO.Path.GetDirectoryName(this.dataSvcMapFilePath), this.edmxSchemaFileName);
                return IO.File.ReadAllText(edmxFilePath);
            }

            /// <summary>
            /// Load the DataSvcMapFile from the given location
            /// </summary>
            internal void LoadDataSvcMapFile()
            {
                using (System.IO.TextReader mapFileReader = IO.File.OpenText(this.dataSvcMapFilePath))
                {
                    XmlDocument document = LoadAndValidateFile(mapFileReader);
                    this.edmxSchemaFileName = GetMetadataFileNameFromMapFile(document);
                    ValidateParametersInMapFile(document, out this.version, out this.useDataServiceCollection);
                }
            }

            /// <summary>
            /// Add the assembly references to the assembly builder depending on whether the databinding is enabled or not.
            /// </summary>
            /// <param name="assemblyBuilder">instance of assemblyBuilder where we need to add assembly references.</param>
            /// <param name="enableDataBinding">indicates whether databinding is enabled or not.</param>
            internal static void AddAssemblyReferences(AssemblyBuilder assemblyBuilder, bool useDataServiceCollection)
            {
                assemblyBuilder.AddAssemblyReference(typeof(System.Data.Services.Client.DataServiceContext).Assembly);

                if (useDataServiceCollection)
                {
                    // TODO: SQLBUDT 707098 - Use the helper method which returns the FrameworkName instance as defined in System.Runtime.Versioning namespace.
                    // When generating data binding code for .Net framework 3.5, we need to load the right version of windows base here.
                    // 3.5 Framework version: WindowsBase, Version=3.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35
                    // In 4.0, ObservableCollection<> class was moved into System.dll. Hence no need to do anything.
                    if (BuildManager.TargetFramework.Version.Major < 4)
                    {
                        assemblyBuilder.AddAssemblyReference(System.Reflection.Assembly.Load("WindowsBase, Version=3.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35"));
                    }
                }
            }

            /// <summary>
            /// Validates the given map file.
            /// </summary>
            /// <param name="mapFileReader">instance of text reader containing the map file.</param>
            /// <returns>returns an instance of XmlDocument containing the map file.</returns>
            private static XmlDocument LoadAndValidateFile(TextReader mapFileReader)
            {
                XmlReaderSettings readerSettings = new XmlReaderSettings();

                readerSettings.Schemas = ServiceMapSchemaSet;
                readerSettings.ValidationType = ValidationType.Schema;
                readerSettings.ValidationFlags = XmlSchemaValidationFlags.ReportValidationWarnings;

                // Validation Handler
                ValidationEventHandler handler = delegate(object sender, System.Xml.Schema.ValidationEventArgs e)
                {
                    if (e.Severity == XmlSeverityType.Error)
                    {
                        throw e.Exception;
                    }
                };

                // Create an xmlDocument and load the map file
                XmlDocument document = new XmlDocument();
                document.Load(mapFileReader);

                // validate the map file against the xsd
                document.Schemas = ServiceMapSchemaSet;
                document.Validate(handler);

                return document;
            }

            /// <summary>
            /// Gets the metadata file name from the map file.
            /// </summary>
            /// <param name="document">instance of xml document which refers to the map file.</param>
            /// <returns>returns the metadata file path as specified in the map file.</returns>
            private static string GetMetadataFileNameFromMapFile(XmlDocument document)
            {
                // The xsd already validates that there should be exactly one MetadataFile element present.
                XmlNode node = document.SelectSingleNode("/ds:ReferenceGroup/ds:Metadata/ds:MetadataFile", NamespaceManager);
                Debug.Assert(node != null, "MetadataFile node must be present");

                // Both the attributes FileName and MetadataType must be present,
                // since the xsd already does this validation. 
                // xsd also does the validation that these attributes cannot be empty.
                string edmxSchemaFileName = node.Attributes["FileName"].Value;
                string metadataType = node.Attributes["MetadataType"].Value;

                // Ideally, this check must be moved in the xsd, so that user gets all the information about where
                // exactly the error is - file information, line number etc. We currently only have file information
                // with us
                if (metadataType != "Edmx")
                {
                    throw new ArgumentException(Strings.InvalidMetadataType(metadataType, "MetadataType", "Edmx"));
                }

                return edmxSchemaFileName;
            }

            /// <summary>
            /// Validate the parameters list in the map file. Currently only EnableDataBinding parameter is supported.
            /// </summary>
            /// <param name="document">instance of xml documents which contains the map file.</param>
            /// <returns>returns true if the data binding parameter is specified and its value is true, otherwise returns false.</returns>
            private static void ValidateParametersInMapFile(XmlDocument document, out DataServiceCodeVersion version, out bool useDataServiceCollection)
            {
                bool isSentinalTypePresent = BuildManager.GetType("System.Data.Services.Client.DataServiceCollection`1", false /*throwOnError*/) != null;

                // Validate all the parameters specified in the map file
                XmlNodeList parameters = document.SelectNodes("/ds:ReferenceGroup/ds:Parameters/ds:Parameter", NamespaceManager);
                bool? useCollectionParamValue = null;
                DataServiceCodeVersion? versionParamValue = null;

                if (parameters != null && parameters.Count != 0)
                {
                    // currently only one parameter can be specified. EnableDataBinding - whose valid values are 'true and 'false'
                    foreach (XmlNode p in parameters)
                    {
                        // Name and value attributes must be present, since they are verified by the xsd
                        XmlAttribute nameAttribute = p.Attributes["Name"];
                        XmlAttribute valueAttribute = p.Attributes["Value"];
                        if (nameAttribute.Value == VersionParameterName)
                        {
                            ValidateVersionParameter(ref versionParamValue, valueAttribute.Value);
                        }
                        else if (nameAttribute.Value == UseCollectionParameterName)
                        {
                            ValidateBooleanParameter(ref useCollectionParamValue, valueAttribute.Value, UseCollectionParameterName);
                        }
                        else
                        {
                            throw new ArgumentException(
                                Strings.InvalidParameterName(
                                    nameAttribute.Value,
                                    String.Format(CultureInfo.InvariantCulture, "'{0}', '{1}'", VersionParameterName, UseCollectionParameterName)));
                        }
                    }
                }

                if (!isSentinalTypePresent && useCollectionParamValue.HasValue && useCollectionParamValue == true)
                {
                    throw new ArgumentException(Strings.UseDataServiceCollectionCannotBeSetToTrue(UseCollectionParameterName));
                }

                version = versionParamValue.HasValue ? versionParamValue.Value : DataServiceCodeVersion.V1;
                useDataServiceCollection = useCollectionParamValue.HasValue ? useCollectionParamValue.Value : false;
            }

            /// <summary>
            /// Validate the boolean parameter value as specified in the .svcmap file.
            /// </summary>
            /// <param name="value">parameter value as specified in the svcmap file.</param>
            /// <param name="parameterName">name of the parameter whose value is getting validated.</param>
            /// <returns>boolean value as specified in the svcmap file.</returns>
            private static void ValidateBooleanParameter(ref bool? paramValue, string value, string parameterName)
            {
                if (paramValue == null)
                {
                    try
                    {
                        paramValue = XmlConvert.ToBoolean(value);
                    }
                    catch (FormatException e)
                    {
                        throw new ArgumentException(Strings.InvalidBooleanParameterValue(value, parameterName), e);
                    }
                }
                else
                {
                    throw new ArgumentException(Strings.ParameterSpecifiedMultipleTimes(UseCollectionParameterName));
                }
            }

            /// <summary>
            /// Validate the version parameter value is valid.
            /// </summary>
            /// <param name="parameterValue">version parameter value as specified in the .svcmap file.</param>
            /// <returns>instance of DataServiceCodeVersion, which contains the value of the version parameter as specified in the svcmap file.</returns>
            private static void ValidateVersionParameter(ref DataServiceCodeVersion? version, string parameterValue)
            {
                if (version == null)
                {
                    if (parameterValue == Version1Dot0)
                    {
                        version = DataServiceCodeVersion.V1;
                    }
                    else if (parameterValue == Version2Dot0)
                    {
                        version = DataServiceCodeVersion.V2;
                    }
                    else
                    {
                        throw new ArgumentException(
                            Strings.InvalidVersionParameterValue(
                                parameterValue,
                                VersionParameterName,
                                String.Format(CultureInfo.InvariantCulture, "'{0}', '{1}'", Version1Dot0, Version2Dot0)));
                    }
                }
                else
                {
                    throw new ArgumentException(Strings.ParameterSpecifiedMultipleTimes(VersionParameterName));
                }
            }
        }
    }
}

