/*+==========================================================================
  File:     XSD.cs

  Summary:  Utility to support XSD schema files; imports data types, creates
            new schemas, and converts the XDR format to the XSD format
  Notes:    

----------------------------------------------------------------------------

  Copyright (C) 2000 Microsoft Corporation.  All rights reserved.
==========================================================================+*/

namespace XsdTool {

    using System;
    using System.Reflection;
    using System.Xml.Serialization;
    using System.Xml.Serialization.Advanced;
    using System.Xml.Schema;
    using System.IO;
    using System.CodeDom.Compiler;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Threading;
    using System.Xml;
    using System.Data;
    using System.Data.Design;
    using System.CodeDom;
    using System.Text;
    using Microsoft.DevApps.WebServices.XsdResources;
    using System.Globalization;
    using System.Configuration;
    
    /// <include file='doc\xsd.uex' path='docs/doc[@for="Xsd"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class Xsd {

        TextWriter CreateOutputWriter(string outputdir, string fileName, string newExtension) {
            string strippedName = Path.GetFileName(fileName);
            string newExtensionName = Path.ChangeExtension(strippedName, newExtension);
            string outputName = Path.Combine(outputdir, newExtensionName);

            Console.Out.WriteLine(Res.GetString(Res.InfoWrittingFile, outputName));
            return new StreamWriter(outputName, false, new System.Text.UTF8Encoding(true));
        }

        // Get a code generator for the specified language. language can either be a known abbreviation
        // for C#, VB or JScript. Or it can be a fully-qualified (with assembly) name for an CodeDomProvider
        // or a CodeDomProvider.
        void CreateCodeProvider(string language, ref CodeDomProvider codeProvider, ref string fileExtension) {
            if (CodeDomProvider.IsDefinedLanguage(language)) {
                try {
                    codeProvider = CodeDomProvider.CreateProvider(language);
                } 
                catch (Exception e) {
                    if (e is ThreadAbortException || e is StackOverflowException || e is OutOfMemoryException || e is ConfigurationException) {
                        throw;
                    }
                    throw new InvalidOperationException(Res.GetString(Res.ErrLanguage, language), e);
                } 
            }
            else {
                //try to reflect a custom code generator
                //ignore case when reflecting; language argument must specify the namespace
                Type t = Type.GetType(language, false, true);
                if (t == null)
                    throw new InvalidOperationException(Res.GetString(Res.ErrLanguage, language));
                object o = Activator.CreateInstance(t);
                if (o is CodeDomProvider)
                    codeProvider = (CodeDomProvider)o;
                else
                    throw new InvalidOperationException(Res.GetString(Res.ErrCodeDomProvider, language));
            }

            if (codeProvider != null) {
                fileExtension = codeProvider.FileExtension;
                if (fileExtension == null)
                    fileExtension = string.Empty;
                else if (fileExtension.Length > 0 && fileExtension[0] != '.')
                    fileExtension = "." + fileExtension;
            }
            else
                fileExtension = ".src";
        }

        private static XmlSchema ReadSchema(string location, bool throwOnAbsent) {
            if (!File.Exists(location)) {
                if (throwOnAbsent)
                    throw new FileNotFoundException(Res.GetString(Res.FileNotFound, location));
                else {
                    Console.WriteLine(Res.GetString(Res.SchemaValidationWarningDetails, Res.GetString(Res.FileNotFound, location)));
                    return null;
                }
            }
            XmlTextReader reader = new XmlTextReader(location, (new StreamReader(location)).BaseStream);
            reader.XmlResolver = null;
            schemaCompileErrors = false;
            XmlSchema schema = XmlSchema.Read(reader, new ValidationEventHandler(ValidationCallbackWithErrorCode));
            if (schemaCompileErrors)
                throw new InvalidOperationException(Res.GetString(Res.SchemaValidationError, location));
            return schema;
        }

        void ImportSchemaAsClasses(XmlSchema schema, string uri, IList elements, XmlSchemaImporter schemaImporter, XmlCodeExporter codeExporter) {
            if (schema == null)
                return;
            ArrayList mappings = new ArrayList();
            foreach (XmlSchemaElement element in schema.Elements.Values) {
                if (!element.IsAbstract) {
                    if (uri.Length == 0 ||
                        element.QualifiedName.Namespace == uri) {
                        bool found;
                        if (elements.Count == 0) {
                            found = true;
                        }
                        else {
                            found = false;
                            foreach (string e in elements) {
                                if (e == element.Name) {
                                    found = true;
                                    break;
                                }
                            }
                        }
                        if (found) {
                            mappings.Add(schemaImporter.ImportTypeMapping(element.QualifiedName));
                        }
                    }
                }
            }
            foreach (XmlTypeMapping xmlTypeMapping in mappings) {
                codeExporter.ExportTypeMapping(xmlTypeMapping);
            }
        }

        void ImportSchemasAsClasses(
            string outputdir,
            CodeDomProvider codeProvider,
            string fileExtension,
            IList fileNames, 
            string ns,
            string uri,
            CodeGenerationOptions options,
            IList elements, 
            StringCollection schemaImporterExtensions) {

            XmlSchemas schemasToCompile = new XmlSchemas();
            string outputSchemaName = "";

            Hashtable locations = new Hashtable();
            Hashtable uris = new Hashtable();
            foreach (string fileName in fileNames) {
                string trimmed = fileName.Trim();
                if (trimmed == null || trimmed.Length == 0)
                    continue;
                string path = Path.GetFullPath(trimmed).ToLower(CultureInfo.InvariantCulture);
                if (locations[path] == null) {
                    XmlSchema schema = ReadSchema(path, true);
                    locations.Add(path, schema);
                    Uri baseUri = new Uri(path);
                    uris.Add(schema, baseUri);
                    schemasToCompile.Add(schema , baseUri);
                    outputSchemaName += Path.ChangeExtension(trimmed,"").Replace('.','_');
                }
            }

            Hashtable includeSchemas = new Hashtable();
            Compile(schemasToCompile, uris, includeSchemas);
            try {
                outputSchemaName = outputSchemaName.Substring(0, outputSchemaName.Length - 1);
                CodeCompileUnit codeCompileUnit = new CodeCompileUnit();
                CodeNamespace codeNamespace = new CodeNamespace(ns);
                codeCompileUnit.Namespaces.Add(codeNamespace);
                GenerateVersionComment(codeNamespace);
                XmlCodeExporter codeExporter = new XmlCodeExporter(codeNamespace, codeCompileUnit, codeProvider, options, null);
                XmlSchemaImporter schemaImporter = new XmlSchemaImporter(schemasToCompile, options, codeProvider, new ImportContext(new CodeIdentifiers(), false));
                schemaImporter.Extensions.Add(new DataSetSchemaImporterExtension());
                foreach (string type in schemaImporterExtensions) {
                    Type t = Type.GetType(type.Trim(), true, false);
                    schemaImporter.Extensions.Add(t.FullName, t);
                }
                AddImports(codeNamespace, GetNamespacesForTypes(new Type[] { typeof(XmlAttributeAttribute) }));

                for (int i = 0; i < schemasToCompile.Count; i++) {
                    XmlSchema schema = schemasToCompile[i];
                    ImportSchemaAsClasses(schemasToCompile[i], uri, elements, schemaImporter, codeExporter);
                }
                foreach (XmlSchema s in includeSchemas.Values) {
                    ImportSchemaAsClasses(s, uri, elements, schemaImporter, codeExporter);
                }

                CodeTypeDeclarationCollection classes = codeNamespace.Types;
                if (classes == null || classes.Count == 0) {
                    Console.WriteLine(Res.GetString(Res.NoClassesGenerated));
                }
                else {
                    CodeGenerator.ValidateIdentifiers(codeNamespace);
                    TextWriter writer = CreateOutputWriter(outputdir, outputSchemaName, fileExtension);
                    codeProvider.GenerateCodeFromCompileUnit(codeCompileUnit, writer, null);
                    writer.Close();
                }
            }
            catch (Exception e) {
                if (e is ThreadAbortException || e is StackOverflowException || e is OutOfMemoryException) {
                    throw;
                }
                throw new InvalidOperationException(Res.GetString(Res.ErrGenerateClassesForSchema, outputSchemaName), e);
            }
        }

        internal class Namespace {
            internal const string SoapEncoding = "http://schemas.xmlsoap.org/soap/encoding/";
            internal const string Wsdl = "http://schemas.xmlsoap.org/wsdl/";
            internal const string ReservedXmlNs = "http://www.w3.org/XML/1998/namespace";
        }

        static bool schemaCompileErrors = false;
        internal static void ValidationCallbackWithErrorCode (object sender, ValidationEventArgs args) {
            string message;
            if (args.Exception.LineNumber == 0 && args.Exception.LinePosition == 0) {
                message = Res.GetString(Res.SchemaValidationWarningDetails, args.Message);
            }
            else {
                message = Res.GetString(Res.SchemaValidationWarningDetailsSource, args.Message, args.Exception.LineNumber.ToString(CultureInfo.InvariantCulture), args.Exception.LinePosition.ToString(CultureInfo.InvariantCulture));
            }
            if (args.Severity == XmlSeverityType.Error) {
                Console.WriteLine(message);
                schemaCompileErrors = true;
            }
        }

        internal static void XsdParametersValidationHandler(object sender, ValidationEventArgs args) {
            /*
            if (args.Severity != XmlSeverityType.Error)
                return;
            */
            string message = Res.GetString(Res.XsdParametersValidationError, XsdParameters.targetNamespace, args.Message);
            if (args.Exception.LineNumber != 0 || args.Exception.LinePosition != 0) {
                message += " " + Res.GetString(Res.ErrorPosition, args.Exception.LineNumber.ToString(CultureInfo.InvariantCulture), args.Exception.LinePosition.ToString(CultureInfo.InvariantCulture));
            }
            throw new InvalidOperationException(message);
        }
        
        private static void CollectIncludes(XmlSchema schema, Uri baseUri, Hashtable includeSchemas, string topUri) {
            if (schema == null)
                return;
            foreach (XmlSchemaExternal external in schema.Includes) {
                string schemaLocation = external.SchemaLocation;
                if (external is XmlSchemaImport) {
                    external.SchemaLocation = null;
                }
                else {
                    if (external.Schema == null && schemaLocation != null && schemaLocation.Length > 0) {
                        Uri includeUri = ResolveUri(baseUri, schemaLocation);
                        string uri = includeUri.ToString().ToLower(CultureInfo.InvariantCulture);
                        if (topUri == uri) {
                            external.Schema = new XmlSchema();
                            external.Schema.TargetNamespace = schema.TargetNamespace;
                            external.SchemaLocation = null;
                            return;
                        }
                        XmlSchema include = (XmlSchema)includeSchemas[uri];
                        if (include == null) {

                            // Compute include path the new way (using includeUri) as well the old way (using schemaLocation)
                            string includePath = schemaLocation;
                            string newIncludePath = GetPathFromUri(includeUri);
                            bool isNewIncludePathValid = (newIncludePath != null && File.Exists(newIncludePath));

                            // For backword compatibility, first check if file exists at schemaLocation (old way)
                            if (File.Exists(includePath))
                            {
                                // Print warning if a file also exists at include path (new way) 
                                if (isNewIncludePathValid)
                                {
                                    string oldIncludePath = Path.GetFullPath(includePath).ToLower(CultureInfo.InvariantCulture);
                                    if (oldIncludePath != newIncludePath)
                                        Warning(Res.GetString(Res.MultipleFilesFoundMatchingInclude4, schemaLocation,
                                            GetPathFromUri(baseUri), oldIncludePath, newIncludePath));
                                }
                            }
                            else if (isNewIncludePathValid)
                                includePath = newIncludePath;

                            include = ReadSchema(includePath, false);
                            includeSchemas[uri] = include;
                            CollectIncludes(include, includeUri, includeSchemas, topUri);
                        }
                        if (include != null) {
                            external.Schema = include;
                            external.SchemaLocation = null;
                        }
                    }
                }
            }
        }

        private static string GetPathFromUri(Uri uri)
        {
            if (uri != null)
            {
                try
                {
                    return Path.GetFullPath(uri.LocalPath).ToLower(CultureInfo.InvariantCulture);
                }
                catch (Exception e)
                {
                    if (e is ThreadAbortException || e is StackOverflowException || e is OutOfMemoryException || e is ConfigurationException)
                        throw;
                }
            }
            return null;
        }

        private static Uri ResolveUri(Uri baseUri, string relativeUri) {
            if (baseUri == null || (!baseUri.IsAbsoluteUri && baseUri.OriginalString.Length == 0)) {
                Uri uri = new Uri(relativeUri, UriKind.RelativeOrAbsolute);
                if ( !uri.IsAbsoluteUri ) {
                    uri = new Uri(Path.GetFullPath(relativeUri));
                }
                return uri;
            }
            else {
                if (relativeUri == null || relativeUri.Length == 0) {
                    return baseUri;
                }
                return new Uri(baseUri, relativeUri);
            }
        }

        private static void Compile(XmlSchemas userSchemas, Hashtable uris, Hashtable includeSchemas) {
            foreach (XmlSchema s in userSchemas) {
                if (s.TargetNamespace != null && s.TargetNamespace.Length == 0) {
                    s.TargetNamespace = null;
                }
                Uri uri = (Uri)uris[s];
                CollectIncludes(s, uri, includeSchemas, uri.ToString().ToLower(CultureInfo.InvariantCulture));
            }
            try {
                userSchemas.Compile(new ValidationEventHandler (ValidationCallbackWithErrorCode), true);
            }
            catch (Exception e) {
                if (e is ThreadAbortException || e is StackOverflowException || e is OutOfMemoryException) {
                    throw;
                }
                Console.WriteLine(Environment.NewLine + Res.GetString(Res.SchemaValidationWarning) + Environment.NewLine + e.Message + Environment.NewLine);
            }
            if (!userSchemas.IsCompiled) {
                Console.WriteLine(Environment.NewLine + Res.GetString(Res.SchemaValidationWarning) + Environment.NewLine);
            }
        }

        private static void GenerateVersionComment(CodeNamespace codeNamespace) {
            codeNamespace.Comments.Add(new CodeCommentStatement(""));
            AssemblyName assemblyName = Assembly.GetExecutingAssembly().GetName();
            codeNamespace.Comments.Add(new CodeCommentStatement(
                Res.GetString(Res.InfoVersionComment, assemblyName.Name, ThisAssembly.InformationalVersion)));
            codeNamespace.Comments.Add(new CodeCommentStatement(""));
        }


        /// <include file='doc\xsd.uex' path='docs/doc[@for="Xsd.AddImports"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal static void AddImports(CodeNamespace codeNamespace, string[] namespaces) {
            foreach (string ns in namespaces)
                codeNamespace.Imports.Add(new CodeNamespaceImport(ns));
        }


        /// <include file='doc\xsd.uex' path='docs/doc[@for="Xsd.GetNamespacesForTypes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal static string[] GetNamespacesForTypes(Type[] types) {
            Hashtable names = new Hashtable();
            for (int i = 0; i < types.Length; i++) {
                string name = types[i].FullName;
                int dot = name.LastIndexOf('.');
                if (dot > 0)
                    names[name.Substring(0, dot)] = types[i];
            }
            string[] ns = new string[names.Keys.Count];
            names.Keys.CopyTo(ns, 0);
            return ns;
        }
       
        void ImportSchemasAsDataSets(
            string outputdir,
            CodeDomProvider codeProvider,
            string fileExtension,
            IList fileNames, 
            string ns, 
            string uri, 
            IList elements,
            bool enableLinqDataSets) {

            foreach (string fileName in fileNames) {
                try {                    
                    // REVIEW: (davidgut) what about culture?
                    StreamReader reader = File.OpenText(fileName);
                    TextWriter writer = CreateOutputWriter(outputdir, fileName, fileExtension);
                    string xsdContent = FixUpSchemaIncludes(reader.ReadToEnd(), fileName);
                    GenerateDataSetClasses(xsdContent, ns, writer, codeProvider, enableLinqDataSets);
                    writer.Close();
                }
                catch (Exception e) {
                    if (e is ThreadAbortException || e is StackOverflowException || e is OutOfMemoryException) {
                        throw;
                    }
                    throw new InvalidOperationException(Res.GetString(Res.ErrGeneral, fileName), e);
                }
            }
        }

        /// <include file='doc\xsd.uex' path='docs/doc[@for="Xsd.GenerateDataSetClasses"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal static void GenerateDataSetClasses(
            string schemaContent, 
            string namespaceName, 
            TextWriter outputWriter, 
            CodeDomProvider codeProvider,
            bool enableLinqDataSets) {

            string dsName = null;
            try {
                CodeCompileUnit codeCompileUnit = new CodeCompileUnit();
                CodeNamespace codeNamespace = new CodeNamespace(namespaceName);
                codeCompileUnit.Namespaces.Add(codeNamespace);
                GenerateVersionComment(codeNamespace);
                // UNDONE,yannc: remove this if data set Generate API is updated to take a CodeCompileUnit

                if (enableLinqDataSets)
                {
                    dsName = System.Data.Design.TypedDataSetGenerator.Generate(schemaContent, codeCompileUnit, codeNamespace, 
                        codeProvider, System.Data.Design.TypedDataSetGenerator.GenerateOption.LinqOverTypedDatasets);
                }
                else
                {
                    dsName = System.Data.Design.TypedDataSetGenerator.Generate(schemaContent, codeCompileUnit, codeNamespace, codeProvider);
                }
                if(namespaceName == null) {
                    codeNamespace.Name = dsName;
                }
                codeProvider.GenerateCodeFromCompileUnit(codeCompileUnit, outputWriter, null);
            }
            catch (Exception e) {
                throw new InvalidOperationException(Res.GetString(Res.ErrGenerateDataSetClass, dsName), e);
            }
        }

        void ExportSchemas(string outputdir, IList dlls, IList typeNames) {
            XmlReflectionImporter importer = new XmlReflectionImporter();
            XmlSchemas schemas = new XmlSchemas();
            XmlSchemaExporter exporter = new XmlSchemaExporter(schemas);

            foreach (string dll in dlls) {
                Assembly a = Assembly.LoadFrom(dll);
                if (a == null)
                    throw new InvalidOperationException(Res.GetString(Res.ErrLoadAssembly, dll));

                try {
                    foreach (Type type in a.GetTypes()) {
                        if (!type.IsPublic)
                            continue;
                        if (type.IsAbstract && type.IsSealed)
                            continue;
                        if (type.IsInterface)
                            continue;
                        if (type.ContainsGenericParameters)
                            continue;

                        bool found;
                        if (typeNames.Count == 0) {
                            found = true;
                        }
                        else {
                            found = false;
                            foreach (string typeName in typeNames) {
                                if (type.FullName == typeName ||
                                    type.Name == typeName ||
                                    (typeName.EndsWith(".*") && 
                                    type.FullName.StartsWith(typeName.Substring(0, typeName.Length - 2)))) {
                                    found = true;
                                    break;
                                }
                            }
                        }

                        if (found) {
                            XmlTypeMapping xmlTypeMapping = importer.ImportTypeMapping(type);
                            exporter.ExportTypeMapping(xmlTypeMapping);
                        }
                    }
                    // need to preprocess all exported schemas to make sure that IXmlSerializable schemas are Merged and the resulting set is valid
                    schemas.Compile(new ValidationEventHandler (ValidationCallbackWithErrorCode), false);
                }
                catch (Exception e) {
                    if (e is ThreadAbortException || e is StackOverflowException || e is OutOfMemoryException) {
                        throw;
                    }
                    throw new InvalidOperationException(Res.GetString(Res.ErrGeneral, dll), e);
                }
            }

            for (int i = 0; i < schemas.Count; i++) {
                XmlSchema schema = schemas[i];
                try {
                    TextWriter writer = CreateOutputWriter(outputdir, "schema" + i.ToString(), ".xsd");
                    schemas[i].Write(writer);
                    writer.Close();
                }
                catch (Exception e) {
                    if (e is ThreadAbortException || e is StackOverflowException || e is OutOfMemoryException) {
                        throw;
                    }
                    throw new InvalidOperationException(Res.GetString(Res.ErrGeneral, schema.TargetNamespace), e);
                }
            }
            if (schemas.Count == 0) {
                Console.WriteLine(Res.GetString(Res.NoTypesGenerated));
            }
        }

        void ConvertXDRSchemas(string outputdir, IList xdrSchemas) {
            foreach (string xdrSchema in xdrSchemas) {
                try {

                    // REVIEW: (davidgut) what about culture?
                    DataSet dataSet = new DataSet();
                    dataSet.ReadXmlSchema(xdrSchema);
                    TextWriter writer = CreateOutputWriter(outputdir, xdrSchema, ".xsd");
                    XmlTextWriter w =  new XmlTextWriter(writer);
                    w.Formatting = Formatting.Indented;
                    w.WriteStartDocument();
                    dataSet.WriteXmlSchema(w);
                    w.Close();
                    writer.Close();
                }
                catch (Exception e) {
                    if (e is ThreadAbortException || e is StackOverflowException || e is OutOfMemoryException) {
                        throw;
                    }
                    throw new InvalidOperationException(Res.GetString(Res.ErrGeneral, xdrSchema), e);
                }
            }
        }

        
        void InferSchemas(string outputdir, IList instances) {
            foreach (string instance in instances) {
                try {

                    // REVIEW: (davidgut) what about culture?
                    DataSet dataSet = new DataSet();
                    dataSet.ReadXml(instance);
                    TextWriter writer = CreateOutputWriter(outputdir, instance, ".xsd");
                    XmlTextWriter w =  new XmlTextWriter(writer);
                    w.Formatting = Formatting.Indented;
                    w.WriteStartDocument();
                    dataSet.WriteXmlSchema(w);
                    w.Close();
                    writer.Close();
                }
                catch (Exception e) {
                    if (e is ThreadAbortException || e is StackOverflowException || e is OutOfMemoryException) {
                        throw;
                    }
                    throw new InvalidOperationException(Res.GetString(Res.ErrGeneral, instance), e);
                }
            }
        }

        /// <include file='doc\xsd.uex' path='docs/doc[@for="Xsd.Main"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int Main(string[] args) {
            Xsd xsd = new Xsd();
            return xsd.Run(args);
        }

        //Function is used when an argument has 2+ letters short form.
        bool ArgumentMatchEx(string arg, string formal, int minletters) {
            //arg comes on '/xxx' form, formal in the 'xxx' one
            if (arg.Length > minletters + 1) {
                return ArgumentMatch(arg, formal);
            }
            else if (arg.Length == minletters + 1) {
                return (string.Compare(arg, 1, formal, 0, minletters, false, CultureInfo.InvariantCulture) == 0);
            }
            return false;
        }

        //Function is used when an argument has 2+ letters short form.
        bool ArgumentMatchEx(string arg, string formal, string shortForm) {
            //arg comes on '/xxx' form, formal in the 'xxx' one
            int shortFormLength = shortForm.Length + 1;
            if (arg.Length > shortFormLength) 
                return ArgumentMatch(arg, formal);
            else if (arg.Length == shortFormLength)
                return ArgumentMatch(arg, shortForm);
            return false;
        }

        // assumes all same case.        
        bool ArgumentMatch(string arg, string formal) {
            return ArgumentMatch(arg, formal, false);
        }

        // assumes all same case.        
        bool ArgumentMatch(string arg, string formal, bool exactMatch) {
            if (arg[0] != '/' && arg[0] != '-') {
                return false;
            }
            if (exactMatch) {
                arg = arg.Substring(1);
                return (arg == formal);
            }
            arg = arg.Substring(1);
            return (arg == formal || (arg.Length == 1 && arg[0] == formal[0]));
        }

        // For command line apps, cmd.exe cannot display right-to-left languages currently.  Change the
        // CurrentUICulture to meet their needs, falling back to our invariant resources.
        private static void SetConsoleUICulture()
        {
            Thread t = Thread.CurrentThread;
            t.CurrentUICulture = CultureInfo.CurrentUICulture.GetConsoleFallbackUICulture();

            if (Console.OutputEncoding.CodePage != Encoding.UTF8.CodePage &&
                Console.OutputEncoding.CodePage != t.CurrentUICulture.TextInfo.OEMCodePage &&
                Console.OutputEncoding.CodePage != t.CurrentUICulture.TextInfo.ANSICodePage)
            {
                t.CurrentUICulture = new CultureInfo("en-US");
            }
        }

        int Run(string[] args) {
            SetConsoleUICulture();
            try {
                XsdParameters xsdParameters = new XsdParameters();
                XsdParameters userParameters = null;
                    
                for (int i = 0; i < args.Length; i++) {
                    string arg = args[i];
                    string value = string.Empty;
                    bool argument = false;
                
                    if (arg.StartsWith("/") || arg.StartsWith("-")) {
                        argument = true;
                        int colonPos = arg.IndexOf(":");
                        if (colonPos != -1) {
                            value = arg.Substring(colonPos + 1);
                            arg = arg.Substring(0, colonPos);
                        }
                    }
                    arg = arg.ToLower(CultureInfo.InvariantCulture);

                    //the user may have wanted to provide an absolute path to the file, so detect that FIRST
                    //for example. "c:bob.xsd" will be (erroneously) split up into arg = "c" and value = "bob.xsd"
                    //however, "bob.xsd" will be "properly" split up into arg = "bob.xsd" and value = ""

                    if (!argument && arg.EndsWith(".xsd")) {
                        xsdParameters.XsdSchemas.Add(args[i]);
                    }
                    else if (!argument && arg.EndsWith(".xdr")) {
                        xsdParameters.XdrSchemas.Add(args[i]);
                    }
                    else if (!argument && arg.EndsWith(".xml")) {
                        xsdParameters.Instances.Add(args[i]);
                    }
                    else if (!argument && (arg.EndsWith(".dll") || arg.EndsWith(".exe"))) {
                        xsdParameters.Assemblies.Add(args[i]);
                    }
                    else if (ArgumentMatch(arg, "?") || ArgumentMatch(arg, "help")) {
                        WriteHeader();
                        WriteHelp();
                        return 0;
                    }
                    else if (ArgumentMatch(arg, "classes")) {
                        if (value.Length > 0) {
                            WriteHeader();
                            throw new InvalidOperationException(Res.GetString(Res.ErrInvalidArgument, arg + ":" + value));
                        }
                        xsdParameters.Classes = true;
                    }
                    else if (ArgumentMatch(arg, "dataset")) {
                        if (value.Length > 0) {
                            WriteHeader();
                            throw new InvalidOperationException(Res.GetString(Res.ErrInvalidArgument, arg + ":" + value));
                        }
                        xsdParameters.Dataset = true;
                    }
                    else if (ArgumentMatchEx(arg, "enablelinqdataset", "eld")) { // all lower-case used to match argument
                        if (value.Length > 0) 
                        {
                            WriteHeader();
                            throw new InvalidOperationException(Res.GetString(Res.ErrInvalidArgument, arg + ":" + value));
                        }
                        xsdParameters.EnableLinqDataSet = true;
                    }
                    else if (ArgumentMatch(arg, "element")) {
                        xsdParameters.Elements.Add(value);
                    }
                    else if (ArgumentMatch(arg, "fields")) {
                        xsdParameters.Options &= ~CodeGenerationOptions.GenerateProperties;
                    }
                    else if (ArgumentMatch(arg, "order", true)) {
                        xsdParameters.Options |= CodeGenerationOptions.GenerateOrder;
                    }
                    else if (ArgumentMatchEx(arg, "enabledatabinding", "edb")) { // all lower-case used to match argument
                        xsdParameters.Options |= CodeGenerationOptions.EnableDataBinding;
                    }
                    else if (ArgumentMatch(arg, "language")) {
                        xsdParameters.Language = value.ToLower(CultureInfo.InvariantCulture);
                    }
                    else if (ArgumentMatch(arg, "namespace")) {
                        xsdParameters.Namespace = value;
                    }
                    else if (ArgumentMatch(arg, "nologo")) {
                        if (value.Length > 0) {
                            WriteHeader();
                            throw new InvalidOperationException(Res.GetString(Res.ErrInvalidArgument, arg + ":" + value));
                        }
                        xsdParameters.Nologo = true;
                    }
                    else if (ArgumentMatch(arg, "out") || ArgumentMatch(arg, "outputdir")) {
                        xsdParameters.OutputDir = value;
                    }
                    else if (ArgumentMatch(arg, "type")) {
                        xsdParameters.Types.Add(value);
                    }
                    else if (ArgumentMatch(arg, "uri")) {
                        xsdParameters.Uri = value;
                    }
                    else if (ArgumentMatch(arg, "parameters")) {
                        userParameters = XsdParameters.Read(value);
                    }
                    else {
                        WriteHeader();
                        throw new InvalidOperationException(Res.GetString(Res.ErrInvalidArgument, args[i]));
                    }
                }

                if (userParameters != null) {
                    xsdParameters = userParameters.Merge(xsdParameters);
                }
                if (xsdParameters.Help) {
                    WriteHeader();
                    WriteHelp();
                    return 0;
                }
                if (!xsdParameters.Nologo)
                    WriteHeader();

                bool schemasFound = xsdParameters.XsdSchemas.Count > 0;
                bool xdrSchemasFound = xsdParameters.XdrSchemas.Count > 0;
                bool dllsFound = xsdParameters.Assemblies.Count > 0;
                bool instancesFound = xsdParameters.Instances.Count > 0;
            
                if (xsdParameters.OutputDir.Length == 0) {
                    xsdParameters.OutputDir = Directory.GetCurrentDirectory();
                }
                if (schemasFound) {
                    if (dllsFound || instancesFound || xdrSchemasFound) {
                        throw new InvalidOperationException(Res.GetString(Res.ErrInputFileTypes));
                    }
                    if (xsdParameters.Classes == xsdParameters.Dataset) {
                        throw new InvalidOperationException(Res.GetString(Res.ErrClassOrDataset));
                    }

                    CodeDomProvider codeProvider = null;
                    string fileExtension = string.Empty;

                    CreateCodeProvider(xsdParameters.Language, ref codeProvider, ref fileExtension);

                    if (xsdParameters.Classes)
                        ImportSchemasAsClasses(xsdParameters.OutputDir, codeProvider, fileExtension, xsdParameters.XsdSchemas, xsdParameters.Namespace, xsdParameters.Uri, xsdParameters.Options, xsdParameters.Elements, xsdParameters.SchemaImporterExtensions);
                    else
                        ImportSchemasAsDataSets(xsdParameters.OutputDir, codeProvider, fileExtension, xsdParameters.XsdSchemas, xsdParameters.Namespace, xsdParameters.Uri, xsdParameters.Elements, xsdParameters.EnableLinqDataSet);
                }
                else if (dllsFound) {
                    if (instancesFound || xdrSchemasFound)
                        throw new InvalidOperationException(Res.GetString(Res.ErrInputFileTypes));
                    ExportSchemas(xsdParameters.OutputDir, xsdParameters.Assemblies, xsdParameters.Types);
                }
                else if (xdrSchemasFound) {
                    if (instancesFound)
                        throw new InvalidOperationException(Res.GetString(Res.ErrInputFileTypes));
                    ConvertXDRSchemas(xsdParameters.OutputDir, xsdParameters.XdrSchemas);
                }
                else if (instancesFound) {
                    InferSchemas(xsdParameters.OutputDir, xsdParameters.Instances);
                }
                else {
                    WriteHelp();
                    return 0;
                }
            }
            catch (Exception e) {
                if (e is ThreadAbortException || e is StackOverflowException || e is OutOfMemoryException) {
                    throw;
                }
                Error(e, Res.GetString(Res.Error));
                return 1;
            }
            return 0;
        }

        static void Error(Exception e, string prefix) {
            Console.Error.WriteLine(prefix + e.Message);
            if (e is System.Data.Design.TypedDataSetGeneratorException) {
                foreach (string msg in ((System.Data.Design.TypedDataSetGeneratorException) e).ErrorList) {
                    Console.WriteLine(prefix + msg);
                }
            }
            if (e.InnerException != null) {
                Error(e.InnerException, "  - ");
            }
            else {
                Console.WriteLine(Res.GetString(Res.MoreHelp,"/?"));
            }
        }

        static void Warning(string message)
        {
            Console.WriteLine(Res.GetString(Res.Warning, message));
        }

        private void WriteHeader() {
            Console.WriteLine(Res.GetString(Res.Logo));
            // do not localize Copyright header
            Console.WriteLine(String.Format(CultureInfo.CurrentCulture, "[Microsoft (R) .NET Framework, Version {0}]", ThisAssembly.InformationalVersion));
            Console.WriteLine("Copyright (C) Microsoft Corporation. All rights reserved.");
        }

        void WriteHelp() {
            Console.WriteLine(Res.GetString(Res.HelpDescription, ThisAssembly.Title));
            Console.WriteLine(Res.GetString(Res.HelpUsage, ThisAssembly.Title));
            Console.WriteLine(Res.GetString(Res.HelpOptions));
            Console.WriteLine(Res.GetString(Res.HelpClasses, "/classes", "/c"));
            Console.WriteLine(Res.GetString(Res.HelpDataset, "/dataset", "/d"));
            Console.WriteLine(Res.GetString(Res.HelpEnableLinqDataSet, "/enableLinqDataSet", "/eld"));
            Console.WriteLine(Res.GetString(Res.HelpElement, "/element:", "/e:"));
            Console.WriteLine(Res.GetString(Res.HelpFields, "/fields", "/f"));
            Console.WriteLine(Res.GetString(Res.HelpOrder, "/order"));
            Console.WriteLine(Res.GetString(Res.HelpEnableDataBinding, "/enableDataBinding", "/edb"));
            Console.WriteLine(Res.GetString(Res.HelpLanguage, "/language:", "/l:"));
            Console.WriteLine(Res.GetString(Res.HelpNamespace, "/namespace:", "/n:"));
            Console.WriteLine(Res.GetString(Res.HelpNoLogo, "/nologo"));
            Console.WriteLine(Res.GetString(Res.HelpOut, "/out:", "/o:"));
            Console.WriteLine(Res.GetString(Res.HelpType,"/type:","/t:"));
            Console.WriteLine(Res.GetString(Res.HelpUri, "/uri:", "/u:"));
            Console.WriteLine(Res.GetString(Res.HelpAdvanced));
            Console.WriteLine(Res.GetString(Res.HelpParameters, "/parameters:", "/p:"));
            Console.WriteLine(Res.GetString(Res.HelpArguments));
            Console.WriteLine(Res.GetString(Res.HelpArgumentsDescription));
        }

        internal static string FixUpSchemaIncludes(string xsdContent, string xsdPath) {
            if (xsdContent == null || xsdPath == null) {
                return xsdContent;
            }

            try {
                bool madeChanges = false;
                XmlDocument xmlDoc = new XmlDocument();
                xmlDoc.Load(XmlReader.Create(new StringReader(xsdContent)));

                XmlNamespaceManager nsMgr = new XmlNamespaceManager(xmlDoc.NameTable);
                nsMgr.AddNamespace("xs", XmlSchema.Namespace);
                string expression = "//xs:schema/xs:include";

                XmlNodeList nodeList = xmlDoc.SelectNodes(expression, nsMgr);
                foreach (XmlNode node in nodeList) {
                    XmlElement xmlElement = node as XmlElement;
                    if (xmlElement == null) {
                        continue;
                    }
                    foreach (XmlAttribute attribute in xmlElement.Attributes) {
                        if (String.Compare(attribute.Name, "schemaLocation", StringComparison.OrdinalIgnoreCase) == 0) {
                            string replacement = attribute.Value;

                            if (!Uri.IsWellFormedUriString(replacement, UriKind.Absolute) && !Path.IsPathRooted(replacement)) {
                                replacement = Path.Combine(Path.GetDirectoryName(Path.GetFullPath(xsdPath)), replacement);
                                if (File.Exists(replacement)) {
                                    attribute.Value = replacement;
                                    madeChanges = true;
                                }
                            }
                        }
                    }
                }

                if (madeChanges) {
                    StringWriter stringWriter = new StringWriter();
                    xmlDoc.Save(XmlWriter.Create(stringWriter));
                    xsdContent = stringWriter.ToString();
                }
            }
            catch { }

            return xsdContent;
        }
    }

    public class XsdParameters {
        internal const string targetNamespace = "http://microsoft.com/dotnet/tools/xsd/";
        bool nologo;
        bool help;
        bool classes;
        bool dataset;
        bool enableLinqDataSet;
        string language;
        string ns;
        string outputdir;
        CodeGenerationOptions options = CodeGenerationOptions.GenerateProperties;
        bool optionsDefault = true;
        string uri;
        StringCollection xsdSchemas;
        StringCollection xdrSchemas;
        StringCollection instances;
        StringCollection dlls;
        StringCollection elements;
        StringCollection types;
        StringCollection schemaImporterExtensions;

        internal bool Classes {
            get { return classes; }
            set { classes = value; }
        }

        internal bool Dataset {
            get { return dataset; }
            set { dataset = value; }
        }

        internal bool EnableLinqDataSet {
            get { return enableLinqDataSet; }
            set { enableLinqDataSet = value; }
        }

        internal bool Help {
            get { return help; }
            set { help = value; }
        }

        internal string Language {
            get { return language == null ? "c#" : language; }
            set { language = value; }
        }

        internal string Namespace {
            get { return ns == null ? string.Empty : ns; }
            set { ns = value; }
        }

        internal bool Nologo {
            get { return nologo; }
            set { nologo = value; }
        }

        internal string OutputDir {
            get { return outputdir == null ? string.Empty : outputdir; }
            set { outputdir = value; }
        }

        internal CodeGenerationOptions Options {
            get { return options; }
            set { options = value; optionsDefault = false;}
        }

        internal string Uri {
            get { return uri == null ? string.Empty : uri; }
            set { uri = value; }
        }

        internal StringCollection XsdSchemas {
            get {
                if (xsdSchemas == null)
                    xsdSchemas = new StringCollection();
                return xsdSchemas; 
            }
        }

        internal StringCollection XdrSchemas {
            get {
                if (xdrSchemas == null)
                    xdrSchemas = new StringCollection();
                return xdrSchemas; 
            }
        }

        internal StringCollection Instances {
            get {
                if (instances == null)
                    instances = new StringCollection();
                return instances; 
            }
        }

        internal StringCollection Assemblies {
            get {
                if (dlls == null)
                    dlls = new StringCollection();
                return dlls; 
            }
        }

        internal StringCollection Elements {
            get {
                if (elements == null)
                    elements = new StringCollection();
                return elements; 
            }
        }

        internal StringCollection Types {
            get {
                if (types == null)
                    types = new StringCollection();
                return types; 
            }
        }

        internal StringCollection SchemaImporterExtensions {
            get {
                if (schemaImporterExtensions == null)
                    schemaImporterExtensions = new StringCollection();
                return schemaImporterExtensions; 
            }
        }

        internal XsdParameters() {
        }

        internal XsdParameters Merge(XsdParameters parameters) {
            if (parameters.classes)
                this.classes = parameters.classes;
            if (parameters.dataset)
                this.dataset = parameters.dataset;
            if (parameters.language != null)
                this.language = parameters.language;
            if (parameters.ns != null)
                this.ns = parameters.ns;
            if (parameters.nologo)
                this.nologo = parameters.nologo;
            if (parameters.outputdir != null)
                this.outputdir = parameters.outputdir;
            if (!parameters.optionsDefault)
                this.options = parameters.options;
            if (parameters.uri != null)
                this.uri = parameters.uri;

            foreach(string s in parameters.XsdSchemas)
                this.XsdSchemas.Add(s);
            foreach(string s in parameters.XdrSchemas)
                this.XdrSchemas.Add(s);
            foreach(string s in parameters.Instances)
                this.Instances.Add(s);
            foreach(string s in parameters.Assemblies)
                this.Assemblies.Add(s);
            foreach(string s in parameters.Elements)
                this.Elements.Add(s);
            foreach(string s in parameters.Types)
                this.Types.Add(s);
            foreach(string s in parameters.SchemaImporterExtensions)
                this.SchemaImporterExtensions.Add(s);

            return this;
        }

        internal static XsdParameters Read(string file) {
            if (file == null || file.Length == 0)
                return null;
            if (File.Exists(file))
                return Read(new XmlTextReader(file), new ValidationEventHandler(Xsd.XsdParametersValidationHandler));

            throw new FileNotFoundException(Res.GetString(Res.FileNotFound, file));
        }

        internal static XsdParameters Read(XmlReader xmlReader, ValidationEventHandler validationEventHandler) {
            XmlReaderSettings readerSettings = new XmlReaderSettings();
            readerSettings.ValidationType = ValidationType.Schema;
            readerSettings.ValidationFlags = XmlSchemaValidationFlags.ProcessIdentityConstraints;
            readerSettings.Schemas.Add(XsdParameters.Schema);

            if (validationEventHandler != null) {
                readerSettings.ValidationEventHandler += validationEventHandler;
            }
            else {
                readerSettings.ValidationEventHandler += new ValidationEventHandler(Xsd.XsdParametersValidationHandler);
            }
            XmlReader validatingReader = XmlReader.Create(xmlReader, readerSettings);
            XsdParametersSerializer ser = new XsdParametersSerializer();
            return (XsdParameters)ser.Deserialize(validatingReader);
        }
        
        static XmlSchema schema;
        public static XmlSchema Schema {
            get {
                if (schema == null) {
                    schema = XmlSchema.Read(new StringReader(xsdParametersSchema), null);
                }
                return schema;
            }
        }

        internal const string xsdParametersSchema = @"<?xml version='1.0' encoding='UTF-8' ?>
<xs:schema xmlns:tns='http://microsoft.com/dotnet/tools/xsd/' elementFormDefault='qualified' targetNamespace='http://microsoft.com/dotnet/tools/xsd/' xmlns:xs='http://www.w3.org/2001/XMLSchema'>
  <xs:simpleType name='options'>
    <xs:list>
      <xs:simpleType>
        <xs:restriction base='xs:string'>
          <xs:enumeration value='none' />
          <xs:enumeration value='properties' />
          <xs:enumeration value='order' />
          <xs:enumeration value='enableDataBinding' />
        </xs:restriction>
      </xs:simpleType>
    </xs:list>
  </xs:simpleType>
  
  <xs:complexType name='generateObjectModel'>
    <xs:sequence>
      <xs:element name='schema' minOccurs='0' maxOccurs='unbounded' type='xs:string'/>
    </xs:sequence>
    <xs:attribute name='language' default='cs' type='xs:string'/>
    <xs:attribute name='namespace' type='xs:string'/>
  </xs:complexType>

  <xs:complexType name='generateClasses'>
    <xs:complexContent mixed='false'>
      <xs:extension base='tns:generateObjectModel'>
        <xs:sequence>
          <xs:element name='element' minOccurs='0' maxOccurs='unbounded' type='xs:string'/>
          <xs:element minOccurs='0' name='schemaImporterExtensions'>
            <xs:complexType>
              <xs:sequence>
                <xs:element minOccurs='0' maxOccurs='unbounded' name='type' type='xs:string' />
              </xs:sequence>
            </xs:complexType>
          </xs:element>
        </xs:sequence>
        <xs:attribute name='options' default='properties' type='tns:options'/>
        <xs:attribute name='uri' type='xs:anyURI'/>
      </xs:extension>
    </xs:complexContent>
  </xs:complexType>

  <xs:complexType name='generateDataSet'>
    <xs:complexContent mixed='false'>
      <xs:extension base='tns:generateObjectModel'>
        <xs:attribute name='enableLinqDataSet' default='false' type='xs:boolean'/>
      </xs:extension>
    </xs:complexContent>
  </xs:complexType>

  <xs:complexType name='generateSchemas'>
    <xs:choice>
      <xs:element name='xdr' type='xs:string' maxOccurs='unbounded' />
      <xs:element name='xml' type='xs:string' maxOccurs='unbounded' />
      <xs:choice>
        <xs:element name='assembly' type='xs:string' maxOccurs='unbounded' />
        <xs:element name='type' type='xs:string' maxOccurs='unbounded' />
      </xs:choice>
    </xs:choice>
  </xs:complexType>

  <xs:complexType name='xsdParameters'>
    <xs:choice>
      <xs:element name='generateClasses' type='tns:generateClasses'/>
      <xs:element name='generateDataSet' type='tns:generateDataSet'/>
      <xs:element name='generateSchemas' type='tns:generateSchemas'/>
    </xs:choice>
    <xs:attribute name='output' type='xs:string'/>
    <xs:attribute name='nologo' type='xs:boolean'/>
    <xs:attribute name='help' type='xs:boolean'/>
  </xs:complexType>
  
  <xs:element name='xsd' type='tns:xsdParameters' />

</xs:schema>";

    }


    internal sealed class XsdParametersSerializer : XmlSerializer {

        protected override XmlSerializationReader CreateReader() {
            return new XmlSerializationReader1();
        }
        protected override XmlSerializationWriter CreateWriter() {
            throw new InvalidOperationException();
        }
        public override System.Boolean CanDeserialize(XmlReader xmlReader) {
            return true;
        }

        protected override void Serialize(System.Object objectToSerialize, XmlSerializationWriter writer) {
            throw new InvalidOperationException();
        }
        protected override System.Object Deserialize(XmlSerializationReader reader) {
            return ((XmlSerializationReader1)reader).Read7_xsd();
        }
    }

    internal class XmlSerializationReader1 : System.Xml.Serialization.XmlSerializationReader {

        internal object Read7_xsd() {
            object o = null;
            Reader.MoveToContent();
            if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                if (((object) Reader.LocalName == (object)id1_xsd && (object) Reader.NamespaceURI == (object)id2_Item)) {
                    o = Read6_XsdParameters(false, true);
                }
                else {
                    throw CreateUnknownNodeException();
                }
            }
            else {
                UnknownNode(null);
            }
            return (object)o;
        }

        XsdParameters Read5_GenerateClasses(XsdParameters xsd, bool isNullable, bool checkType) {
            xsd.Classes = true;
            System.Xml.XmlQualifiedName xsiType = checkType ? GetXsiType() : null;
            bool isNull = false;
            if (isNullable) isNull = ReadNull();
            if (checkType) {
                if (xsiType == null || ((object) ((System.Xml.XmlQualifiedName)xsiType).Name == (object)id6_GenerateClasses && (object) ((System.Xml.XmlQualifiedName)xsiType).Namespace == (object)id2_Item)) {
                }
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)xsiType);
            }
            if (isNull) return null;
            StringCollection a_0 = xsd.XsdSchemas;
            StringCollection a_3 = xsd.Elements;
            StringCollection a_4 = xsd.SchemaImporterExtensions;
            while (Reader.MoveToNextAttribute()) {
                if (((object) Reader.LocalName == (object)id7_language && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    xsd.Language = Reader.Value;
                }
                else if (((object) Reader.LocalName == (object)id8_namespace && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    xsd.Namespace = Reader.Value;
                }
                else if (((object) Reader.LocalName == (object)id9_options && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    xsd.Options = Read4_CodeGenerationOptions(Reader.Value);
                }
                else if (((object) Reader.LocalName == (object)id10_uri && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    xsd.Uri = CollapseWhitespace(Reader.Value);
                }
                else if (!IsXmlnsAttribute(Reader.Name)) {
                    UnknownNode((object)xsd);
                }
            }
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                return xsd;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            int whileIterations1 = 0;
            int readerCount1 = ReaderCount;
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement && Reader.NodeType != System.Xml.XmlNodeType.None) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (((object) Reader.LocalName == (object)id11_schema && (object) Reader.NamespaceURI == (object)id2_Item)) { {
                            a_0.Add(Reader.ReadElementString());
                        }
                    }
                    else if (((object) Reader.LocalName == (object)id12_element && (object) Reader.NamespaceURI == (object)id2_Item)) { {
                            a_3.Add(Reader.ReadElementString());
                        }
                    }
                    else if (((object) Reader.LocalName == (object)id13_schemaImporterExtensions && (object) Reader.NamespaceURI == (object)id2_Item)) {
                        if (!ReadNull()) {
                            StringCollection a_4_0 = xsd.@SchemaImporterExtensions;
                            if (Reader.IsEmptyElement) {
                                Reader.Skip();
                            }
                            else {
                                Reader.ReadStartElement();
                                Reader.MoveToContent();
                                int whileIterations2 = 0;
                                int readerCount2 = ReaderCount;
                                while (Reader.NodeType != System.Xml.XmlNodeType.EndElement && Reader.NodeType != System.Xml.XmlNodeType.None) {
                                    if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                                        if (((object) Reader.LocalName == (object)id14_type && (object) Reader.NamespaceURI == (object)id2_Item)) { {
                                                a_4_0.Add(Reader.ReadElementString());
                                            }
                                        }
                                        else {
                                            UnknownNode(null);
                                        }
                                    }
                                    else {
                                        UnknownNode(null);
                                    }
                                    Reader.MoveToContent();
                                    CheckReaderCount(ref whileIterations2, ref readerCount2);
                                }
                                ReadEndElement();
                            }
                        }
                    }
                    else {
                        UnknownNode((object)xsd);
                    }
                }
                else {
                    UnknownNode((object)xsd);
                }
                Reader.MoveToContent();
                CheckReaderCount(ref whileIterations1, ref readerCount1);
            }
            ReadEndElement();
            return xsd;
        }

        System.Collections.Hashtable _CodeGenerationOptionsValues;

        internal System.Collections.Hashtable CodeGenerationOptionsValues {
            get {
                if ((object)_CodeGenerationOptionsValues == null) {
                    System.Collections.Hashtable h = new System.Collections.Hashtable();
                    h.Add(@"properties", (System.Int64)System.Xml.Serialization.CodeGenerationOptions.@GenerateProperties);
                    h.Add(@"order", (System.Int64)System.Xml.Serialization.CodeGenerationOptions.@GenerateOrder);
                    h.Add(@"enableDataBinding", (System.Int64)System.Xml.Serialization.CodeGenerationOptions.@EnableDataBinding);
                    h.Add(@"none", (System.Int64)0);
                    _CodeGenerationOptionsValues = h;
                }
                return _CodeGenerationOptionsValues;
            }
        }

        System.Xml.Serialization.CodeGenerationOptions Read4_CodeGenerationOptions(string s) {
            return (System.Xml.Serialization.CodeGenerationOptions)ToEnum(s, CodeGenerationOptionsValues, @"System.Xml.Serialization.CodeGenerationOptions");
        }

        XsdParameters Read3_GenerateSchemas(XsdParameters xsd, bool isNullable, bool checkType) {
            System.Xml.XmlQualifiedName xsiType = checkType ? GetXsiType() : null;
            bool isNull = false;
            if (isNullable) isNull = ReadNull();
            if (checkType) {
                if (xsiType == null || ((object) ((System.Xml.XmlQualifiedName)xsiType).Name == (object)id5_generateSchemas && (object) ((System.Xml.XmlQualifiedName)xsiType).Namespace == (object)id2_Item)) {
                }
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)xsiType);
            }
            if (isNull) return null;
            StringCollection a_0 = xsd.Instances;
            StringCollection a_1 = xsd.XdrSchemas;
            StringCollection a_2 = xsd.Assemblies;
            StringCollection a_3 = xsd.Types;
            bool[] paramsRead = new bool[4];
            while (Reader.MoveToNextAttribute()) {
                if (!IsXmlnsAttribute(Reader.Name)) {
                    UnknownNode((object)xsd);
                }
            }
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                return xsd;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            int whileIterations1 = 0;
            int readerCount1 = ReaderCount;
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement && Reader.NodeType != System.Xml.XmlNodeType.None) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (((object) Reader.LocalName == (object)id15_xml && (object) Reader.NamespaceURI == (object)id2_Item)) { {
                            a_0.Add(Reader.ReadElementString());
                        }
                    }
                    else if (((object) Reader.LocalName == (object)id16_xdr && (object) Reader.NamespaceURI == (object)id2_Item)) { {
                            a_1.Add(Reader.ReadElementString());
                        }
                    }
                    else if (((object) Reader.LocalName == (object)id17_assembly && (object) Reader.NamespaceURI == (object)id2_Item)) { {
                            a_2.Add(Reader.ReadElementString());
                        }
                    }
                    else if (((object) Reader.LocalName == (object)id14_type && (object) Reader.NamespaceURI == (object)id2_Item)) { {
                            a_3.Add(Reader.ReadElementString());
                        }
                    }
                    else {
                        UnknownNode((object)xsd);
                    }
                }
                else {
                    UnknownNode((object)xsd);
                }
                Reader.MoveToContent();
                CheckReaderCount(ref whileIterations1, ref readerCount1);
            }
            ReadEndElement();
            return xsd;
        }

        XsdParameters Read2_GenerateDataset(XsdParameters xsd, bool isNullable, bool checkType) {
            xsd.Dataset = true;
            System.Xml.XmlQualifiedName xsiType = checkType ? GetXsiType() : null;
            bool isNull = false;
            if (isNullable) isNull = ReadNull();
            if (checkType) {
                if (xsiType == null || ((object) ((System.Xml.XmlQualifiedName)xsiType).Name == (object)id3_generateObjectModel && (object) ((System.Xml.XmlQualifiedName)xsiType).Namespace == (object)id2_Item)) {
                }
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)xsiType);
            }
            if (isNull) return null;
            StringCollection a_0 = xsd.XsdSchemas;
            while (Reader.MoveToNextAttribute()) {
                if (((object) Reader.LocalName == (object)id7_language && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    xsd.Language = Reader.Value;
                }
                else if (((object) Reader.LocalName == (object)id8_namespace && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    xsd.Namespace = Reader.Value;
                }
                else if (((object) Reader.LocalName == (object)id24_enableLinqDataSet && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    xsd.EnableLinqDataSet = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                }
                else if (!IsXmlnsAttribute(Reader.Name)) {
                    UnknownNode((object)xsd);
                }
            }
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                return xsd;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            int whileIterations1 = 0;
            int readerCount1 = ReaderCount;
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement && Reader.NodeType != System.Xml.XmlNodeType.None) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (((object) Reader.LocalName == (object)id11_schema && (object) Reader.NamespaceURI == (object)id2_Item)) { {
                            a_0.Add(Reader.ReadElementString());
                        }
                    }
                    else {
                        UnknownNode((object)xsd);
                    }
                }
                else {
                    UnknownNode((object)xsd);
                }
                Reader.MoveToContent();
                CheckReaderCount(ref whileIterations1, ref readerCount1);
            }
            ReadEndElement();
            return xsd;
        }

        XsdParameters Read6_XsdParameters(bool isNullable, bool checkType) {
            System.Xml.XmlQualifiedName xsiType = checkType ? GetXsiType() : null;
            bool isNull = false;
            if (isNullable) isNull = ReadNull();
            if (checkType) {
                if (xsiType == null || ((object) ((System.Xml.XmlQualifiedName)xsiType).Name == (object)id18_xsdParameters && (object) ((System.Xml.XmlQualifiedName)xsiType).Namespace == (object)id2_Item)) {
                }
                else
                    throw CreateUnknownTypeException((System.Xml.XmlQualifiedName)xsiType);
            }
            if (isNull) return null;
            XsdParameters o = new XsdParameters();
            bool[] paramsRead = new bool[6];
            while (Reader.MoveToNextAttribute()) {
                if (!paramsRead[3] && ((object) Reader.LocalName == (object)id19_output && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@OutputDir = Reader.Value;
                    paramsRead[3] = true;
                }
                else if (!paramsRead[4] && ((object) Reader.LocalName == (object)id20_nologo && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Nologo = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[4] = true;
                }
                else if (!paramsRead[5] && ((object) Reader.LocalName == (object)id21_help && (object) Reader.NamespaceURI == (object)id4_Item)) {
                    o.@Help = System.Xml.XmlConvert.ToBoolean(Reader.Value);
                    paramsRead[5] = true;
                }
                else if (!IsXmlnsAttribute(Reader.Name)) {
                    UnknownNode((object)o);
                }
            }
            Reader.MoveToElement();
            if (Reader.IsEmptyElement) {
                Reader.Skip();
                return o;
            }
            Reader.ReadStartElement();
            Reader.MoveToContent();
            int whileIterations1 = 0;
            int readerCount1 = ReaderCount;
            while (Reader.NodeType != System.Xml.XmlNodeType.EndElement && Reader.NodeType != System.Xml.XmlNodeType.None) {
                if (Reader.NodeType == System.Xml.XmlNodeType.Element) {
                    if (!paramsRead[0] && ((object) Reader.LocalName == (object)id22_generateDataSet && (object) Reader.NamespaceURI == (object)id2_Item)) {
                        Read2_GenerateDataset(o, false, true);
                        paramsRead[0] = true;
                    }
                    else if (!paramsRead[1] && ((object) Reader.LocalName == (object)id5_generateSchemas && (object) Reader.NamespaceURI == (object)id2_Item)) {
                        Read3_GenerateSchemas(o, false, true);
                        paramsRead[1] = true;
                    }
                    else if (!paramsRead[2] && ((object) Reader.LocalName == (object)id23_generateClasses && (object) Reader.NamespaceURI == (object)id2_Item)) {
                        Read5_GenerateClasses(o, false, true);
                        paramsRead[2] = true;
                    }
                    else {
                        UnknownNode((object)o);
                    }
                }
                else {
                    UnknownNode((object)o);
                }
                Reader.MoveToContent();
                CheckReaderCount(ref whileIterations1, ref readerCount1);
            }
            ReadEndElement();
            return o;
        }

        protected override void InitCallbacks() {
        }

        System.String id6_GenerateClasses;
        System.String id5_generateSchemas;
        System.String id14_type;
        System.String id2_Item;
        System.String id9_options;
        System.String id16_xdr;
        System.String id13_schemaImporterExtensions;
        System.String id17_assembly;
        System.String id23_generateClasses;
        System.String id1_xsd;
        System.String id12_element;
        System.String id11_schema;
        System.String id4_Item;
        System.String id20_nologo;
        System.String id15_xml;
        System.String id21_help;
        System.String id3_generateObjectModel;
        System.String id18_xsdParameters;
        System.String id10_uri;
        System.String id7_language;
        System.String id22_generateDataSet;
        System.String id8_namespace;
        System.String id19_output;
        System.String id24_enableLinqDataSet;

        protected override void InitIDs() {
            id6_GenerateClasses = Reader.NameTable.Add(@"GenerateClasses");
            id5_generateSchemas = Reader.NameTable.Add(@"generateSchemas");
            id14_type = Reader.NameTable.Add(@"type");
            id2_Item = Reader.NameTable.Add(@"http://microsoft.com/dotnet/tools/xsd/");
            id9_options = Reader.NameTable.Add(@"options");
            id16_xdr = Reader.NameTable.Add(@"xdr");
            id13_schemaImporterExtensions = Reader.NameTable.Add(@"schemaImporterExtensions");
            id17_assembly = Reader.NameTable.Add(@"assembly");
            id23_generateClasses = Reader.NameTable.Add(@"generateClasses");
            id1_xsd = Reader.NameTable.Add(@"xsd");
            id12_element = Reader.NameTable.Add(@"element");
            id11_schema = Reader.NameTable.Add(@"schema");
            id4_Item = Reader.NameTable.Add(@"");
            id20_nologo = Reader.NameTable.Add(@"nologo");
            id15_xml = Reader.NameTable.Add(@"xml");
            id21_help = Reader.NameTable.Add(@"help");
            id3_generateObjectModel = Reader.NameTable.Add(@"generateObjectModel");
            id18_xsdParameters = Reader.NameTable.Add(@"xsdParameters");
            id10_uri = Reader.NameTable.Add(@"uri");
            id7_language = Reader.NameTable.Add(@"language");
            id22_generateDataSet = Reader.NameTable.Add(@"generateDataSet");
            id8_namespace = Reader.NameTable.Add(@"namespace");
            id19_output = Reader.NameTable.Add(@"output");
            id24_enableLinqDataSet = Reader.NameTable.Add(@"enableLinqDataSet");
        }
    }
}

