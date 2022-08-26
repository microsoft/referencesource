//---------------------------------------------------------------------
// <copyright file="ClientApiGenerator.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner       Microsoft
// @backupOwner Microsoft
//---------------------------------------------------------------------

using System.CodeDom;
using System.CodeDom.Compiler;
using System.Collections;
using System.Collections.Generic;
using System.Data.EntityModel.Emitters;
using System.Data.Metadata.Edm;
using System.Data.Services.Design;
using System.Diagnostics;
using System.IO;
using System.Linq;

namespace System.Data.Metadata.Edm
{
    internal static partial class Helper
    {
        internal static bool IsCollectionType(GlobalItem item)
        {
            return (BuiltInTypeKind.CollectionType == item.BuiltInTypeKind);
        }
        internal static bool IsComplexType(EdmType type)
        {
            return (BuiltInTypeKind.ComplexType == type.BuiltInTypeKind);
        }
        internal static bool IsEntitySet(EntitySetBase entitySetBase)
        {
            return BuiltInTypeKind.EntitySet == entitySetBase.BuiltInTypeKind;
        }
        internal static bool IsPrimitiveType(EdmType type)
        {
            return (BuiltInTypeKind.PrimitiveType == type.BuiltInTypeKind);
        }
    }

    internal static class TypeSemantics
    {
        internal static bool IsComplexType(TypeUsage type)
        {
            return Helper.IsComplexType(type.EdmType);
        }
    }

    internal static class EdmProviderManifest
    {
        // System Facet Info
        /// <summary>
        /// Name of the MaxLength Facet
        /// </summary>
        internal const string MaxLengthFacetName = "MaxLength";

        /// <summary>
        /// Name of the Unicode Facet
        /// </summary>
        internal const string UnicodeFacetName = "Unicode";

        /// <summary>
        /// Name of the FixedLength Facet
        /// </summary>
        internal const string FixedLengthFacetName = "FixedLength";

        /// <summary>
        /// Name of the DateTimeKind Facet
        /// </summary>
        internal const string DateTimeKindFacetName = "DateTimeKind";

        /// <summary>
        /// Name of the PreserveSeconds Facet
        /// </summary>
        internal const string PreserveSecondsFacetName = "PreserveSeconds";

        /// <summary>
        /// Name of the Precision Facet
        /// </summary>
        internal const string PrecisionFacetName = "Precision";

        /// <summary>
        /// Name of the Scale Facet
        /// </summary>
        internal const string ScaleFacetName = "Scale";

        /// <summary>
        /// Name of the Nullable Facet
        /// </summary>
        internal const string NullableFacetName = "Nullable";

        /// <summary>
        /// Name of the DefaultValue Facet
        /// </summary>
        internal const string DefaultValueFacetName = "DefaultValue";

        ///// <summary>
        ///// Name of the DefaultType metadata property
        ///// </summary>
        //internal const string DefaultTypeMetadataPropertyName = "DefaultType";

        /// <summary>
        /// Name of the Collation Facet
        /// </summary>
        internal const string CollationFacetName = "Collation";
    }

    internal static class TypeSystem
    {
        private static readonly System.Reflection.MethodInfo s_getDefaultMethod = typeof(TypeSystem).GetMethod("GetDefault", System.Reflection.BindingFlags.Static | System.Reflection.BindingFlags.NonPublic);
        private static T GetDefault<T>() { return default(T); }
        
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        internal static object GetDefaultValue(Type type)
        {
            // null is always the default for non value types and Nullable<>
            if (!type.IsValueType ||
                (type.IsGenericType &&
                 typeof(Nullable<>) == type.GetGenericTypeDefinition()))
            {
                return null;
            }
            System.Reflection.MethodInfo getDefaultMethod = s_getDefaultMethod.MakeGenericMethod(type);
            object defaultValue = getDefaultMethod.Invoke(null, new object[] { });
            return defaultValue;
        }
    }

    internal static class Schema
    {
        public const string CodeGenerationSchemaNamespace = "http://schemas.microsoft.com/ado/2006/04/codegeneration";
    }
}

namespace System.Data.EntityModel
{
    /// <summary>
    /// Summary description for ClientApiGenerator.
    /// </summary>
    internal sealed class ClientApiGenerator : IDisposable
    {
        #region Instance Fields
        private CodeCompileUnit _compileUnit;
        private bool _isLanguageCaseSensitive = true;

        private EdmItemCollection _edmItemCollection;
        private FixUpCollection _fixUps;
        private AttributeEmitter _attributeEmitter;
        private EntityClassGenerator _generator;
        private List<EdmSchemaError> _errors;
        private TypeReference _typeReference = new TypeReference();
        private string _sourceEdmNamespaceName;
        private string _defaultContainerNamespace;
        private string _namespacePrefix;
        private Dictionary<string, string> _namespaceMap;
        private EventHandler<TypeGeneratedEventArgs> _onTypeGenerated;
        private EventHandler<PropertyGeneratedEventArgs> _onPropertyGenerated;
        #endregion

        #region Public Methods
        public ClientApiGenerator(object sourceSchema, EdmItemCollection edmItemCollection, EntityClassGenerator generator, List<EdmSchemaError> errors, string namespacePrefix)
        {
            Debug.Assert(edmItemCollection != null, "edmItemCollection is null");
            Debug.Assert(generator != null, "generator is null");
            Debug.Assert(errors != null, "errors is null");

            _edmItemCollection = edmItemCollection;
            _generator = generator;
            _errors = errors;
            _attributeEmitter = new AttributeEmitter(_typeReference);
            _namespacePrefix = namespacePrefix;

            // generate map for inherited types and prefixed types
            _namespaceMap = new Dictionary<string, string>();

            _onTypeGenerated = new EventHandler<TypeGeneratedEventArgs>(TypeGeneratedEventHandler);
            _onPropertyGenerated = new EventHandler<PropertyGeneratedEventArgs>(PropertyGeneratedEventHandler);

            //// This constructor can be called multiple times with the same EntityClassGenerator. That is, many instances
            //// of the ClientApiGenerator can add handlers to one instance of the EntityClassGenerator.
            //// Make sure the handlers are not left on the EntityClassGenerator after the ClientApiGenerator is no longer in use.
            //// See the Dispose method 

            _generator.OnTypeGenerated += _onTypeGenerated;
            _generator.OnPropertyGenerated += _onPropertyGenerated;
        }

        public void Dispose()
        {
            //// Make sure the handlers are not left on the EntityClassGenerator after the ClientApiGenerator is no longer in use.

            _generator.OnTypeGenerated -= _onTypeGenerated;
            _generator.OnPropertyGenerated -= _onPropertyGenerated;
        }

        internal EdmItemCollection EdmItemCollection
        {
            get { return this._edmItemCollection; }
        }

        public string NamespacePrefix
        {
            get { return this._namespacePrefix; }
        }

        public Dictionary<string, string> NamespaceMap
        {
            get { return this._namespaceMap; }
        }

        /// <summary>
        /// Does code generation emits collections inherited from DataServiceCollection
        /// </summary>
        internal bool UseDataServiceCollection
        {
            get { return _generator.UseDataServiceCollection; }
        }

        /// <summary>Version for which to generate code.</summary>
        internal DataServiceCodeVersion Version
        {
            get { return _generator.Version; }
        }

        /// <summary>
        /// Parses a source Schema and outputs client-side generated code to
        /// the output TextWriter.
        /// </summary>
        /// <param name="schema">The source Schema</param>
        /// <param name="output">The TextWriter in which to write the output</param>
        /// <param name="outputUri">The Uri for the output. Can be null.</param>
        /// <returns>A list of GeneratorErrors.</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification="Reviewed")]
        internal void GenerateCode(LazyTextWriterCreator target)
        {
            Debug.Assert(target != null, "target parameter is null");

            IndentedTextWriter indentedTextWriter = null;
            System.IO.Stream tempFileStream = null;
            System.IO.StreamReader reader = null;
            System.IO.StreamWriter writer = null;
            TempFileCollection tempFiles = null;
            try
            {
                CodeDomProvider provider = null;
                switch (Language)
                {
                    case LanguageOption.GenerateCSharpCode:
                        provider = new Microsoft.CSharp.CSharpCodeProvider();
                        break;

                    case LanguageOption.GenerateVBCode:
                        provider = new Microsoft.VisualBasic.VBCodeProvider();
                        break;
                }

                _isLanguageCaseSensitive = (provider.LanguageOptions & LanguageOptions.CaseInsensitive) == 0;

                new NamespaceEmitter(this, this.NamespacePrefix, target.TargetFilePath).Emit();

                // if there were errors we don't need the output file
                if (RealErrorsExist)
                {
                    return;
                }

                if (FixUps.Count == 0 || !FixUpCollection.IsLanguageSupported(Language))
                {
                    indentedTextWriter = new IndentedTextWriter(target.GetOrCreateTextWriter(), "\t");
                }
                else
                {
                    // need to write to a temporary file so we can do fixups...
                    tempFiles = new TempFileCollection(Path.GetTempPath());
                    string filename = Path.Combine(tempFiles.TempDir, "EdmCodeGenFixup-" + Guid.NewGuid().ToString() + ".tmp");
                    tempFiles.AddFile(filename, false);
                    tempFileStream = new System.IO.FileStream(filename, System.IO.FileMode.CreateNew, System.IO.FileAccess.ReadWrite,
                        System.IO.FileShare.None);
                    indentedTextWriter = new IndentedTextWriter(new System.IO.StreamWriter(tempFileStream), "\t");
                }

                CodeGeneratorOptions styleOptions = new CodeGeneratorOptions();
                styleOptions.BracingStyle = "C";
                styleOptions.BlankLinesBetweenMembers = false;
                styleOptions.VerbatimOrder = true;
                provider.GenerateCodeFromCompileUnit(CompileUnit, indentedTextWriter, styleOptions);

                // if we wrote to a temp file need to post process the file...
                if (tempFileStream != null)
                {
                    indentedTextWriter.Flush();
                    tempFileStream.Seek(0, System.IO.SeekOrigin.Begin);
                    reader = new System.IO.StreamReader(tempFileStream);
                    FixUps.Do(reader, target.GetOrCreateTextWriter(), Language, !String.IsNullOrEmpty(SourceObjectNamespaceName));
                }
            }
            catch (System.UnauthorizedAccessException ex)
            {
                AddError(ModelBuilderErrorCode.SecurityError, EdmSchemaErrorSeverity.Error, ex);
            }
            catch (System.IO.FileNotFoundException ex)
            {
                AddError(ModelBuilderErrorCode.FileNotFound, EdmSchemaErrorSeverity.Error, ex);
            }
            catch (System.Security.SecurityException ex)
            {
                AddError(ModelBuilderErrorCode.SecurityError, EdmSchemaErrorSeverity.Error, ex);
            }
            catch (System.IO.DirectoryNotFoundException ex)
            {
                AddError(ModelBuilderErrorCode.DirectoryNotFound, EdmSchemaErrorSeverity.Error, ex);
            }
            catch (System.IO.IOException ex)
            {
                AddError(ModelBuilderErrorCode.IOException, EdmSchemaErrorSeverity.Error, ex);
            }
            finally
            {
                if (indentedTextWriter != null)
                {
                    indentedTextWriter.Close();
                }
                if (tempFileStream != null)
                {
                    tempFileStream.Close();
                }
                if (tempFiles != null)
                {
                    tempFiles.Delete();
                    ((IDisposable)tempFiles).Dispose();
                }
                if (reader != null)
                {
                    reader.Close();
                }
                if (writer != null)
                {
                    writer.Close();
                }
            }
        }

        /// <summary>
        /// Verification code invoked for types
        /// </summary>
        /// <param name="item">The type being generated</param>
        internal void VerifyLanguageCaseSensitiveCompatibilityForType(GlobalItem item)
        {
            if (_isLanguageCaseSensitive)
            {
                return; // no validation necessary
            }

            try
            {
                _edmItemCollection.GetItem<GlobalItem>(
                                                        Identity(item),
                                                        true   // ignore case
                                                    );
            }
            catch (InvalidOperationException ex)
            {
                AddError(ModelBuilderErrorCode.IncompatibleSettingForCaseSensitiveOption, EdmSchemaErrorSeverity.Error, ex);
            }
        }

        /// <summary>
        /// Verification code invoked for properties
        /// </summary>
        /// <param name="item">The property or navigation property being generated</param>
        internal void VerifyLanguageCaseSensitiveCompatibilityForProperty(EdmMember item)
        {
            if (_isLanguageCaseSensitive)
            {
                return; // no validation necessary
            }

            Debug.Assert(item != null);

            ReadOnlyMetadataCollection<EdmMember> members = item.DeclaringType.Members;
            HashSet<string> set = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

            foreach (EdmMember member in members)
            {
                if (set.Contains(Identity(member)) &&
                    String.Equals(Identity(item), Identity(member), StringComparison.OrdinalIgnoreCase))
                {
                    AddError(ModelBuilderErrorCode.IncompatibleSettingForCaseSensitiveOption, EdmSchemaErrorSeverity.Error,
                                new InvalidOperationException(Strings.PropertyExistsWithDifferentCase(Identity(item))));
                }
                else
                {
                    set.Add(Identity(member));
                }
            }
        }

        /// <summary>
        /// Verification code invoked for entity sets
        /// </summary>
        /// <param name="item">The entity container being generated</param>
        internal void VerifyLanguageCaseSensitiveCompatibilityForEntitySet(System.Data.Metadata.Edm.EntityContainer item)
        {
            if (_isLanguageCaseSensitive)
            {
                return; // no validation necessary
            }

            Debug.Assert(item != null);

            HashSet<string> set = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

            foreach (EntitySetBase entitySetBase in item.BaseEntitySets)
            {
                if (Helper.IsEntitySet(entitySetBase))
                {
                    EntitySet entitySet = (EntitySet)entitySetBase;
                    if (set.Contains(Identity(entitySet)))
                    {
                        AddError(ModelBuilderErrorCode.IncompatibleSettingForCaseSensitiveOption, EdmSchemaErrorSeverity.Error,
                                    new InvalidOperationException(Strings.EntitySetExistsWithDifferentCase(Identity(entitySet))));
                    }
                    else
                    {
                        set.Add(Identity(entitySet));
                    }
                }
            }
        }

        #endregion

        #region Internal Properties

        internal LanguageOption Language
        {
            get
            {
                return _generator.LanguageOption;
            }
        }

        internal TypeReference TypeReference
        {
            get { return _typeReference; }
        }

        internal CodeCompileUnit CompileUnit
        {
            get
            {
                if (_compileUnit == null)
                    _compileUnit = new CodeCompileUnit();

                return _compileUnit;
            }
        }

        public void AddError(string message, ModelBuilderErrorCode errorCode, EdmSchemaErrorSeverity severity)
        {
        }

        public void AddError(ModelBuilderErrorCode errorCode, EdmSchemaErrorSeverity severity, Exception ex)
        {
        }

        internal void AddError(string message, ModelBuilderErrorCode errorCode, EdmSchemaErrorSeverity severity, Exception ex)
        {
        }

        /// <summary>
        /// Check collection for any real errors (Severity != Warning)
        /// </summary>
        public bool RealErrorsExist
        {
            get
            {
                foreach (EdmSchemaError error in _errors)
                {
                    if (error.Severity != EdmSchemaErrorSeverity.Warning)
                        return true;
                }
                return false;
            }
        }

        public IEnumerable<GlobalItem> GetSourceTypes()
        {
            foreach (EntityContainer container in _edmItemCollection.GetItems<EntityContainer>())
            {
                BuiltInTypeKind kind = container.BuiltInTypeKind;
                yield return container;
            }

            foreach (EdmType item in _edmItemCollection.GetItems<EdmType>())
            {
                switch (item.BuiltInTypeKind)
                {
                    case BuiltInTypeKind.AssociationType:
                    case BuiltInTypeKind.ComplexType:
                    case BuiltInTypeKind.EntityType:
                        yield return item;
                        break;
                    case BuiltInTypeKind.EdmFunction:
                    case BuiltInTypeKind.PrimitiveType:
                        break;
                    default:
                        Debug.Assert(false, item.BuiltInTypeKind.ToString());
                        break;
                }
            }
        }

        public string GetClientTypeNamespace(string serviceTypeNamespace)
        {
            if (string.IsNullOrEmpty(this.NamespacePrefix)) return serviceTypeNamespace;

            if (string.IsNullOrEmpty(serviceTypeNamespace) ||
                ((this.DefaultContainerNamespace != null) && (this.DefaultContainerNamespace == serviceTypeNamespace)))
            {
                return this.NamespacePrefix;
            }
            else
            {
                return this.NamespacePrefix + "." + serviceTypeNamespace;
            }
        }

        public string GetContainerNamespace(EntityContainer container)
        {
            if (container == null) return null;

            string namespaceName = null;
            EntitySetBase baseEntitySet = container.BaseEntitySets.FirstOrDefault();
            if (null != baseEntitySet)
            {
                namespaceName = baseEntitySet.ElementType.NamespaceName;
            }

            return namespaceName;
        }

        public CodeTypeReference GetLeastPossibleQualifiedTypeReference(EdmType type)
        {
            string typeRef;
            string clientNamespace = GetClientTypeNamespace(type.NamespaceName);
            if (clientNamespace == SourceEdmNamespaceName)
            {
                // we are already generating in this namespace, no need to qualify it
                typeRef = type.Name;
            }
            else
            {
                typeRef = GetObjectNamespace(clientNamespace) + "." + type.Name;
            }

            return TypeReference.FromString(typeRef);
        }

        public string SourceEdmNamespaceName
        {
            get
            {
                if (null != _sourceEdmNamespaceName)
                {
                    return _sourceEdmNamespaceName;
                }

                // 
                foreach (GlobalItem item in GetSourceTypes())
                {
                    EdmType edm = item as EdmType;
                    if (null != edm)
                    {
                        return edm.NamespaceName;
                    }
                }

                return null;
            }
            set
            {
                _sourceEdmNamespaceName = value;
            }
        }

        public string DefaultContainerNamespace
        {
            get { return _defaultContainerNamespace; }
            set { _defaultContainerNamespace = value; }
        }

        public string SourceObjectNamespaceName
        {
            get
            {
                string sourceEdmNamespaceName = SourceEdmNamespaceName;
                if (!String.IsNullOrEmpty(sourceEdmNamespaceName))
                {
                    return GetObjectNamespace(sourceEdmNamespaceName);
                }

                return null;
            }
        }

        private string GetObjectNamespace(string csdlNamespaceName)
        {
            Debug.Assert(csdlNamespaceName != null, "csdlNamespaceName is null");

            string objectNamespace;
            if (_generator.EdmToObjectNamespaceMap.TryGetObjectNamespace(csdlNamespaceName, out objectNamespace))
            {
                return objectNamespace;
            }

            return csdlNamespaceName;
        }


        /// <summary>
        /// 
        /// </summary>
        /// <value></value>
        internal FixUpCollection FixUps
        {
            get
            {
                if (_fixUps == null)
                    _fixUps = new FixUpCollection();

                return _fixUps;
            }
        }

        internal AttributeEmitter AttributeEmitter
        {
            get { return _attributeEmitter; }
        }

        internal bool IsLanguageCaseSensitive
        {
            get { return _isLanguageCaseSensitive; }
        }

        internal StringComparison LanguageAppropriateStringComparer
        {
            get
            {
                if (IsLanguageCaseSensitive)
                {
                    return StringComparison.Ordinal;
                }
                else
                {
                    return StringComparison.OrdinalIgnoreCase;
                }
            }
        }

        /// <summary>
        /// Helper method that raises the TypeGenerated event
        /// </summary>
        /// <param name="eventArgs">The event arguments passed to the subscriber</param>
        internal void RaiseTypeGeneratedEvent(TypeGeneratedEventArgs eventArgs)
        {
            _generator.RaiseTypeGeneratedEvent(eventArgs);
        }

        /// <summary>
        /// Helper method that raises the PropertyGenerated event
        /// </summary>
        /// <param name="eventArgs">The event arguments passed to the subscriber</param>
        internal void RaisePropertyGeneratedEvent(PropertyGeneratedEventArgs eventArgs)
        {
            _generator.RaisePropertyGeneratedEvent(eventArgs);
        }

        /// <summary>
        /// Gets the collection type to be returned for a multi-valued navigation property.
        /// </summary>
        /// <returns>Type name which is decided based on UseDataServiceCollection setting.</returns>
        internal string GetRelationshipMultiplicityManyCollectionTypeName()
        {
            return this.UseDataServiceCollection ? "System.Data.Services.Client.DataServiceCollection" : "System.Collections.ObjectModel.Collection";
        }

        #endregion

        private static string Identity(EdmMember member)
        {
            return member.ToString();
        }
        private static string Identity(EntitySetBase entitySet)
        {
            return entitySet.ToString();
        }
        private static string Identity(MetadataItem item)
        {
            return item.ToString();
        }

        private void TypeGeneratedEventHandler(object sender, TypeGeneratedEventArgs eventArgs)
        {
            if (!this.UseDataServiceCollection)
            {
                return;
            }

            if (eventArgs.TypeSource.BuiltInTypeKind != BuiltInTypeKind.EntityType && 
                eventArgs.TypeSource.BuiltInTypeKind != BuiltInTypeKind.ComplexType)
            {
                return;
            }

            if (eventArgs.TypeSource.BuiltInTypeKind == BuiltInTypeKind.EntityType)
            {
                // Generate EntitySetAttribute only if there is exactly one entity set associated 
                // with the entity type. The DataServiceEntitySetAttribute is not generated for ComplexType(s).
                EntitySetBase entitySet = this.GetUniqueEntitySetForType((EntityType)eventArgs.TypeSource);
                if (entitySet != null)
                {
                    List<CodeAttributeDeclaration> additionalAttributes = eventArgs.AdditionalAttributes;
                    CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(
                        new CodeTypeReference(typeof(System.Data.Services.Common.EntitySetAttribute),
                            CodeTypeReferenceOptions.GlobalReference), new CodeAttributeArgument(new CodePrimitiveExpression(entitySet.Name)));
                    additionalAttributes.Add(attribute);
                }
            }

            //// Determine if type being generated has a base type
            if (eventArgs.BaseType != null && !String.IsNullOrEmpty(eventArgs.BaseType.BaseType))
            {
                if (this.GetSourceTypes().Where(x => x.BuiltInTypeKind == BuiltInTypeKind.EntityType).Where(x => ((EntityType)x).Name == eventArgs.BaseType.BaseType).Count() != 0)
                {
                    //// Don't generate the PropertyChanged event and OnPropertyChanged method for derived type classes
                    return;
                }
            }

            //// Add the INotifyPropertyChanged interface

            List<Type> additionalInterfaces = eventArgs.AdditionalInterfaces;

            additionalInterfaces.Add(typeof(System.ComponentModel.INotifyPropertyChanged));
            
            //// Add the implementation of the INotifyPropertyChanged interface

            //// Generate this code:
            ////
            //// CSharp:
            //// 
            //// public event global::System.ComponentModel.PropertyChangedEventHandler PropertyChanged;
            //// 
            //// protected virtual void OnPropertyChanged(string property) {
            ////     if ((this.PropertyChanged != null)) {
            ////         this.PropertyChanged(this, new global::System.ComponentModel.PropertyChangedEventArgs(property));
            ////     }
            //// }
            //// 
            //// Visual Basic:
            ////
            //// Public Event PropertyChanged As Global.System.ComponentModel.PropertyChangedEventHandler Implements System.ComponentModel.INotifyPropertyChanged.PropertyChanged
            //// Protected Overridable Sub OnPropertyChanged(ByVal [property] As String)
            ////     If (Not (Me.PropertyChangedEvent) Is Nothing) Then
            ////         RaiseEvent PropertyChanged(Me, New Global.System.ComponentModel.PropertyChangedEventArgs([property]))
            ////     End If
            //// End Sub


            CodeMemberEvent propertyChangedEvent = new CodeMemberEvent();
            propertyChangedEvent.Type = new CodeTypeReference(typeof(System.ComponentModel.PropertyChangedEventHandler), CodeTypeReferenceOptions.GlobalReference);
            propertyChangedEvent.Name = "PropertyChanged";
            propertyChangedEvent.Attributes = MemberAttributes.Public;
            propertyChangedEvent.ImplementationTypes.Add(typeof(System.ComponentModel.INotifyPropertyChanged));

            AttributeEmitter.AddGeneratedCodeAttribute(propertyChangedEvent);
            
            eventArgs.AdditionalMembers.Add(propertyChangedEvent);

            CodeMemberMethod propertyChangedMethod = new CodeMemberMethod();
            propertyChangedMethod.Name = "OnPropertyChanged";
            propertyChangedMethod.Parameters.Add(new CodeParameterDeclarationExpression(new CodeTypeReference(typeof(System.String), CodeTypeReferenceOptions.GlobalReference), "property"));
            propertyChangedMethod.ReturnType = new CodeTypeReference(typeof(void));

            AttributeEmitter.AddGeneratedCodeAttribute(propertyChangedMethod);

            propertyChangedMethod.Statements.Add(
                    new CodeConditionStatement(
                        new CodeBinaryOperatorExpression(
                            new CodeEventReferenceExpression(new CodeThisReferenceExpression(), "PropertyChanged"),
                                CodeBinaryOperatorType.IdentityInequality,
                                new CodePrimitiveExpression(null)),
                            new CodeExpressionStatement(
                            new CodeDelegateInvokeExpression(
                                new CodeEventReferenceExpression(new CodeThisReferenceExpression(), "PropertyChanged"),
                                    new CodeExpression[] { 
                                    new CodeThisReferenceExpression(), 
                                    new CodeObjectCreateExpression(new CodeTypeReference(typeof(System.ComponentModel.PropertyChangedEventArgs), CodeTypeReferenceOptions.GlobalReference), new CodeArgumentReferenceExpression("property"))
                                }))));
            propertyChangedMethod.Attributes = MemberAttributes.Family;
            eventArgs.AdditionalMembers.Add(propertyChangedMethod);
        }

        private void PropertyGeneratedEventHandler(object sender, PropertyGeneratedEventArgs eventArgs)
        {
            if (!this.UseDataServiceCollection)
            {
                return;
            }

            if (eventArgs.PropertySource.BuiltInTypeKind != BuiltInTypeKind.EdmProperty &&
                eventArgs.PropertySource.BuiltInTypeKind != BuiltInTypeKind.NavigationProperty)
            {
                return;
            }

            if (((EdmMember)eventArgs.PropertySource).DeclaringType.BuiltInTypeKind != BuiltInTypeKind.EntityType &&
                ((EdmMember)eventArgs.PropertySource).DeclaringType.BuiltInTypeKind != BuiltInTypeKind.ComplexType)
            {
                return;
            }

            string name = eventArgs.PropertySource.BuiltInTypeKind == BuiltInTypeKind.EdmProperty ? ((EdmProperty)eventArgs.PropertySource).Name : ((NavigationProperty)eventArgs.PropertySource).Name;
            
            // Add call to the OnPropertyChanged method
            eventArgs.AdditionalAfterSetStatements.Add(new CodeExpressionStatement(new CodeMethodInvokeExpression(
                new CodeThisReferenceExpression(), "OnPropertyChanged",
                new CodeExpression[] { new CodePrimitiveExpression(name) }
                )));
        }

        /// <summary>Given an entity type, returns the corresponding entity set if it's unique.</summary>
        /// <param name="entityType">Given entity type.</param>
        /// <returns>Corresponding entity set if it's unique, null otherwise.</returns>
        private EntitySetBase GetUniqueEntitySetForType(EntityType entityType)
        {
            HashSet<EntitySetBase> entitySets = new HashSet<EntitySetBase>(EqualityComparerEntitySet.Default);

            foreach (EntityContainer container in this.EdmItemCollection.GetItems<EntityContainer>())
            {
                bool alreadyAdded = false;
                foreach (EntitySetBase es in container.BaseEntitySets
                                                      .Where(x => x.BuiltInTypeKind == BuiltInTypeKind.EntitySet &&
                                                                  x.ElementType == entityType))
                {
                    if (alreadyAdded == true) 
                    { 
                        return null; 
                    }

                    alreadyAdded = true;
                    entitySets.Add(es);
                }
            }

            if (entitySets.Count == 1)
            {
                return entitySets.Single();
            }
            else
            {
                return null;
            }
        }

        /// <summary>Equality comparer used for deciding whether to add EntitySet attribute to an entity type.</summary>
        public class EqualityComparerEntitySet : IEqualityComparer<EntitySetBase>
        {
            /// <summary>Cached singleton comparer.</summary>
            private static EqualityComparerEntitySet _comparer = new EqualityComparerEntitySet();

            /// <summary>Gets the singleton comparer.</summary>
            public static EqualityComparerEntitySet Default
            {
                get
                {
                    return _comparer;
                }
            }

            #region IEqualityComparer<EntitySetBase> Members

            /// <summary>Equality check.</summary>
            /// <param name="x">Left.</param>
            /// <param name="y">Right.</param>
            /// <returns>true if names are same, false otherwise.</returns>
            public bool Equals(EntitySetBase x, EntitySetBase y)
            {
                return (x == null && y == null) ||
                       (x != null && y != null && x.Name == y.Name);
            }

            /// <summary>Gets hash code for EntitySetBase.</summary>
            /// <param name="obj">Object for which to get hash code.</param>
            /// <returns>Hash code for the name.</returns>
            public int GetHashCode(EntitySetBase obj)
            {
                return (null != obj) ? obj.Name.GetHashCode() : 0;
            }

            #endregion
        }
    }
}
