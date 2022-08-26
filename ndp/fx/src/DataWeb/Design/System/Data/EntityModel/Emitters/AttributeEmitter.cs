//---------------------------------------------------------------------
// <copyright file="AttributeEmitter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner       Microsoft
// @backupOwner Microsoft
//---------------------------------------------------------------------

using System.CodeDom;
using System.Collections.Generic;
using System.Data.Metadata.Edm;
using System.Data.Services.Common;
using System.Data.Services.Design;
using System.Diagnostics;
using System.Globalization;
using System.Linq;

namespace System.Data.EntityModel.Emitters
{
    /// <summary>
    /// Summary description for AttributeEmitter.
    /// </summary>
    internal sealed partial class AttributeEmitter
    {
        private const string _generatorVersion = "1.0.0";
        private const string _generatorName = "System.Data.Services.Design";

        private static readonly CodeAttributeDeclaration _generatedCodeAttribute = new CodeAttributeDeclaration(
            new CodeTypeReference(typeof(System.CodeDom.Compiler.GeneratedCodeAttribute)),
            new CodeAttributeArgument(new CodePrimitiveExpression(_generatorName)),
            new CodeAttributeArgument(new CodePrimitiveExpression(_generatorVersion)));

        TypeReference _typeReference;

        internal TypeReference TypeReference
        {
            get { return _typeReference; }
        }

        internal AttributeEmitter(TypeReference typeReference)
        {
            _typeReference = typeReference;
        }

        /// <summary>
        /// The method to be called to create the type level attributes for the ItemTypeEmitter
        /// </summary>
        /// <param name="emitter">The strongly typed emitter</param>
        /// <param name="typeDecl">The type declaration to add the attribues to.</param>
        public void EmitTypeAttributes(EntityTypeEmitter emitter, CodeTypeDeclaration typeDecl)
        {
            Debug.Assert(emitter != null, "emitter should not be null");
            Debug.Assert(typeDecl != null, "typeDecl should not be null");

            if (emitter.Generator.Version != DataServiceCodeVersion.V1)
            {
                EmitEpmAttributesForEntityType(emitter.Generator.EdmItemCollection, emitter.Item, typeDecl);
                EmitStreamAttributesForEntityType(emitter.Item, typeDecl);
            }

            object[] keys = emitter.Item.KeyMembers.Select(km => (object) km.Name).ToArray();
            typeDecl.CustomAttributes.Add(EmitSimpleAttribute(Utils.WebFrameworkCommonNamespace + "." + "DataServiceKeyAttribute", keys));
        }


        /// <summary>
        /// The method to be called to create the type level attributes for the StructuredTypeEmitter
        /// </summary>
        /// <param name="emitter">The strongly typed emitter</param>
        /// <param name="typeDecl">The type declaration to add the attribues to.</param>
        public void EmitTypeAttributes(StructuredTypeEmitter emitter, CodeTypeDeclaration typeDecl)
        {
            Debug.Assert(emitter != null, "emitter should not be null");
            Debug.Assert(typeDecl != null, "typeDecl should not be null");

            // nothing to do here yet
        }

        /// <summary>
        /// The method to be called to create the type level attributes for the SchemaTypeEmitter
        /// </summary>
        /// <param name="emitter">The strongly typed emitter</param>
        /// <param name="typeDecl">The type declaration to add the attribues to.</param>
        public void EmitTypeAttributes(SchemaTypeEmitter emitter, CodeTypeDeclaration typeDecl)
        {
            Debug.Assert(emitter != null, "emitter should not be null");
            Debug.Assert(typeDecl != null, "typeDecl should not be null");
        }

        /// <summary>
        /// The method to be called to create the property level attributes for the PropertyEmitter
        /// </summary>
        /// <param name="emitter">The strongly typed emitter</param>
        /// <param name="propertyDecl">The type declaration to add the attribues to.</param>
        /// <param name="additionalAttributes">Additional attributes to emit</param>
        public void EmitPropertyAttributes(PropertyEmitter emitter,
                                           CodeMemberProperty propertyDecl,
                                           List<CodeAttributeDeclaration> additionalAttributes)
        {
            if (additionalAttributes != null && additionalAttributes.Count > 0)
            {
                try
                {
                    propertyDecl.CustomAttributes.AddRange(additionalAttributes.ToArray());
                }
                catch (ArgumentNullException e)
                {
                    emitter.Generator.AddError(Strings.InvalidAttributeSuppliedForProperty(emitter.Item.Name),
                                               ModelBuilderErrorCode.InvalidAttributeSuppliedForProperty,
                                               EdmSchemaErrorSeverity.Error,
                                               e);
                }
            }
        }

        /// <summary>
        /// The method to be called to create the type level attributes for the NestedTypeEmitter
        /// </summary>
        /// <param name="emitter">The strongly typed emitter</param>
        /// <param name="typeDecl">The type declaration to add the attribues to.</param>
        public void EmitTypeAttributes(ComplexTypeEmitter emitter, CodeTypeDeclaration typeDecl)
        {
            Debug.Assert(emitter != null, "emitter should not be null");
            Debug.Assert(typeDecl != null, "typeDecl should not be null");

            // not emitting System.Runtime.Serializaton.DataContractAttribute
            // not emitting System.Serializable
        }

        #region Static Methods
        /// <summary>
        /// 
        /// </summary>
        /// <param name="attributeType"></param>
        /// <param name="arguments"></param>
        /// <returns></returns>
        public CodeAttributeDeclaration EmitSimpleAttribute(string attributeType, params object[] arguments)
        {
            CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(TypeReference.FromString(attributeType, true));

            AddAttributeArguments(attribute, arguments);

            return attribute;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="attribute"></param>
        /// <param name="arguments"></param>
        public static void AddAttributeArguments(CodeAttributeDeclaration attribute, object[] arguments)
        {
            foreach (object argument in arguments)
            {
                CodeExpression expression = argument as CodeExpression;
                if (expression == null)
                    expression = new CodePrimitiveExpression(argument);
                attribute.Arguments.Add(new CodeAttributeArgument(expression));
            }
        }

        /// <summary>
        /// Adds an XmlIgnore attribute to the given property declaration.  This is 
        /// used to explicitly skip certain properties during XML serialization.
        /// </summary>
        /// <param name="propertyDecl">the property to mark with XmlIgnore</param>
        public void AddIgnoreAttributes(CodeMemberProperty propertyDecl)
        {
            // not emitting System.Xml.Serialization.XmlIgnoreAttribute
            // not emitting System.Xml.Serialization.SoapIgnoreAttribute
        }

        /// <summary>
        /// Adds an Browsable(false) attribute to the given property declaration.
        /// This is used to explicitly avoid display property in the PropertyGrid.
        /// </summary>
        /// <param name="propertyDecl">the property to mark with XmlIgnore</param>
        public void AddBrowsableAttribute(CodeMemberProperty propertyDecl)
        {
            // not emitting System.ComponentModel.BrowsableAttribute
        }

        #endregion

        /// <summary>
        /// Add the GeneratedCode attribute to the code type member
        /// </summary>
        /// <param name="ctm">the code type member</param>
        public static void AddGeneratedCodeAttribute(CodeTypeMember ctm)
        {
            ctm.CustomAttributes.Add(_generatedCodeAttribute);
        }

        /// <summary>Given a type detects if it is an open type or not</summary>
        /// <param name="entityType">Input type</param>
        /// <returns>true if it is an open type, false otherwise</returns>
        private static bool IsOpenType(StructuralType entityType)
        {
            MetadataProperty isOpenTypeProperty = entityType.MetadataProperties.FirstOrDefault(x => x.Name == System.Data.Services.XmlConstants.EdmV1dot2Namespace + ":" + System.Data.Services.XmlConstants.DataWebOpenTypeAttributeName);
            if (isOpenTypeProperty != null)
            {
                bool isOpenType;
                if (!Boolean.TryParse(Convert.ToString(isOpenTypeProperty.Value, CultureInfo.InvariantCulture), out isOpenType))
                {
                    throw new InvalidOperationException(Strings.ObjectContext_OpenTypePropertyValueIsNotCorrect(System.Data.Services.XmlConstants.DataWebOpenTypeAttributeName, entityType.Name));
                }
                
                return isOpenType;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Checks if the given path corresponds to some open property on the <paramref name="baseEntityType"/>
        /// </summary>
        /// <param name="baseEntityType">Type to check the path for</param>
        /// <param name="sourcePath">Input property path</param>
        /// <returns>true if there is some open property corresponding to the path, false otherwise</returns>
        bool IsOpenPropertyOnPath(StructuralType baseEntityType, String sourcePath)
        {
            Debug.Assert(baseEntityType != null, "Expecting non-null entity type");
            if (String.IsNullOrEmpty(sourcePath))
            {
                return false;
            }
            
            String[] propertyPath = sourcePath.Split('/');
            EdmMember entityProperty = baseEntityType.Members.SingleOrDefault(p => p.Name == propertyPath[0]);

            if (entityProperty == null)
            {
                if (baseEntityType.BaseType != null)
                {
                    return IsOpenPropertyOnPath(baseEntityType.BaseType as StructuralType, sourcePath);
                }
                else
                {
                    return IsOpenType(baseEntityType);
                }
            }
            else
            {
                StructuralType entityPropertyType = entityProperty.TypeUsage.EdmType as StructuralType;
                if (entityPropertyType != null)
                {
                    return IsOpenPropertyOnPath(entityPropertyType, String.Join("/", propertyPath, 1, propertyPath.Length - 1));
                }
                else
                {
                    return false;
                }
            }        
        }

        /// <summary>
        /// Obtains the entity property corresponding to a given sourcePath
        /// </summary>
        /// <param name="baseEntityType">Entity type in which to look for property</param>
        /// <param name="sourcePath">Source Path</param>
        /// <returns>EdmMember object corresponding to the property given through source path</returns>
        private static EdmMember GetEntityPropertyFromEpmPath(StructuralType baseEntityType, String sourcePath)
        {
            Debug.Assert(baseEntityType != null, "Expecting non-null entity type");

            String[] propertyPath = sourcePath.Split('/');

            if (!baseEntityType.Members.Any(p => p.Name == propertyPath[0]))
            {
                return baseEntityType.BaseType != null ? GetEntityPropertyFromEpmPath(baseEntityType.BaseType as StructuralType, sourcePath) : null;
            }
            else
            {
                EdmMember entityProperty = null;
                foreach (var pathSegment in propertyPath)
                {
                    if (baseEntityType == null)
                    {
                        return null;
                    }
                    
                    entityProperty = baseEntityType.Members.SingleOrDefault(p => p.Name == pathSegment);
                    
                    if (entityProperty == null)
                    {
                        return null;
                    }
                    
                    baseEntityType = entityProperty.TypeUsage.EdmType as StructuralType;
                }

                return entityProperty;
            }
        }

        /// <summary>
        /// Returns a sequence of attributes corresponding to a complex type with recursion
        /// </summary>
        /// <param name="complexProperty">Complex typed property</param>
        /// <param name="epmSourcePath">Source path</param>
        /// <param name="epmTargetPath">Target path</param>
        /// <param name="epmNsPrefix">Namespace prefix</param>
        /// <param name="epmNsUri">Namespace Uri</param>
        /// <param name="epmKeepContent">KeepInContent setting</param>
        /// <returns>Sequence of entity property mapping information for complex type properties</returns>
        private static IEnumerable<EntityPropertyMappingAttribute> GetEpmAttrsFromComplexProperty(
            EdmMember complexProperty,
            String epmSourcePath,
            String epmTargetPath,
            String epmNsPrefix,
            String epmNsUri,
            bool epmKeepContent)
        {
            Debug.Assert(complexProperty != null, "Expecting non-null complex property");

            ComplexType complexType = complexProperty.TypeUsage.EdmType as ComplexType;
            if (complexType == null)
            {
                throw new ArgumentException(Strings.ExpectingComplexTypeForMember(complexProperty.Name, complexProperty.DeclaringType.Name));
            }

            foreach (EdmMember subProperty in complexType.Properties)
            {
                String sourcePath = epmSourcePath + "/" + subProperty.Name;
                String targetPath = epmTargetPath + "/" + subProperty.Name;

                if (subProperty.TypeUsage.EdmType.BuiltInTypeKind == BuiltInTypeKind.ComplexType)
                {
                    foreach (EntityPropertyMappingAttribute epmAttr in GetEpmAttrsFromComplexProperty(subProperty, sourcePath, targetPath, epmNsPrefix, epmNsUri, epmKeepContent))
                    {
                        yield return epmAttr;
                    }
                }
                else
                {
                    yield return new EntityPropertyMappingAttribute(
                                        sourcePath,
                                        targetPath,
                                        epmNsPrefix,
                                        epmNsUri,
                                        epmKeepContent);
                }
            }
        }

        /// <summary>
        /// Given a resource type, builds the EntityPropertyMappingInfo for each EntityPropertyMappingAttribute on it
        /// </summary>
        /// <param name="itemCollection">EFx metadata item collection that has been loaded</param>
        /// <param name="entityType">Entity type for which EntityPropertyMappingAttribute discovery is happening</param>
        /// <param name="typeDecl">Type declaration to add the attributes to</param>
        private void EmitEpmAttributesForEntityType(EdmItemCollection itemCollection, EntityType entityType, CodeTypeDeclaration typeDecl)
        {
            // Get epm information provided at the entity type declaration level
            IEnumerable<MetadataProperty> extendedProperties = entityType.MetadataProperties.Where(mp => mp.PropertyKind == PropertyKind.Extended);

            foreach (EpmPropertyInformation propertyInformation in GetEpmPropertyInformation(extendedProperties, entityType.Name, null))
            {
                EdmMember redefinedProperty = GetEntityPropertyFromEpmPath(entityType, propertyInformation.SourcePath);
                if (redefinedProperty == null)
                {
                    if (IsOpenPropertyOnPath(entityType, propertyInformation.SourcePath))
                    {
                        EmitEpmAttributeForEntityProperty(
                            propertyInformation,
                            new EdmInfo { IsComplex = false, Member = null },
                            typeDecl);
                    }
                    else
                    {
                        throw new InvalidOperationException(Strings.ObjectContext_UnknownPropertyNameInEpmAttributesType(propertyInformation.SourcePath, entityType.Name));
                    }
                }
                else
                {
                    EmitEpmAttributeForEntityProperty(
                        propertyInformation,
                        new EdmInfo { IsComplex = redefinedProperty.TypeUsage.EdmType.BuiltInTypeKind == BuiltInTypeKind.ComplexType, Member = redefinedProperty }, 
                        typeDecl);
                }
            }

            // Get epm information provided at the entity type property level
            foreach (EdmMember member in entityType.Members.Where(m => m.DeclaringType == entityType))
            {
                EdmMember entityProperty = entityType.Properties.SingleOrDefault(p => p.DeclaringType == entityType && p.Name == member.Name);
                IEnumerable<MetadataProperty> extendedMemberProperties = member.MetadataProperties.Where(mdp => mdp.PropertyKind == PropertyKind.Extended);

                foreach (EpmPropertyInformation propertyInformation in GetEpmPropertyInformation(extendedMemberProperties, entityType.Name, member.Name))
                {
                    EdmMember entityPropertyCurrent = entityProperty;
                    if (entityProperty.TypeUsage.EdmType.BuiltInTypeKind == BuiltInTypeKind.ComplexType && propertyInformation.PathGiven)
                    {
                        String originalPath = propertyInformation.SourcePath;
                        propertyInformation.SourcePath = entityProperty.Name + "/" + propertyInformation.SourcePath;
                        entityPropertyCurrent = GetEntityPropertyFromEpmPath(entityType, propertyInformation.SourcePath);
                        if (entityPropertyCurrent == null)
                        {
                            if (IsOpenPropertyOnPath(entityProperty.TypeUsage.EdmType as StructuralType, originalPath))
                            {
                                EmitEpmAttributeForEntityProperty(
                                    propertyInformation,
                                    new EdmInfo { IsComplex = false, Member = null },
                                    typeDecl);
                                continue;                                    
                            }
                            else
                            {
                                throw new InvalidOperationException(Strings.ObjectContext_UnknownPropertyNameInEpmAttributesMember(originalPath, member.Name, entityType.Name));
                            }
                        }
                    }

                    EmitEpmAttributeForEntityProperty(
                        propertyInformation, 
                        new EdmInfo { IsComplex = entityPropertyCurrent.TypeUsage.EdmType.BuiltInTypeKind == BuiltInTypeKind.ComplexType, Member = entityPropertyCurrent }, 
                        typeDecl);
                }
            }
        }

        /// <summary>
        /// Given a resource type and its resource proeperty builds the EntityPropertyMappingInfo for the EntityPropertyMappingAttribute on it
        /// </summary>
        /// <param name="propertyInformation">EPM information for current property</param>
        /// <param name="entityProperty">Property for which to get the information</param>
        /// <param name="typeDecl">Type declaration to add the attributes to</param>
        private void EmitEpmAttributeForEntityProperty(
            EpmPropertyInformation propertyInformation, 
            EdmInfo entityProperty, 
            CodeTypeDeclaration typeDecl)
        {
            if (propertyInformation.IsAtom)
            {
                if (entityProperty.IsComplex)
                {
                    throw new InvalidOperationException(Strings.ObjectContext_SyndicationMappingForComplexPropertiesNotAllowed);
                }
                else
                {
                    EntityPropertyMappingAttribute epmAttr = new EntityPropertyMappingAttribute(
                                        propertyInformation.SourcePath,
                                        propertyInformation.SyndicationItem,
                                        propertyInformation.ContentKind,
                                        propertyInformation.KeepInContent);

                    this.AddEpmAttributeToTypeDeclaration(epmAttr, typeDecl);
                }
            }
            else
            {
                if (entityProperty.IsComplex)
                {
                    foreach (EntityPropertyMappingAttribute epmAttr in GetEpmAttrsFromComplexProperty(
                                                                        entityProperty.Member,
                                                                        propertyInformation.SourcePath,
                                                                        propertyInformation.TargetPath,
                                                                        propertyInformation.NsPrefix,
                                                                        propertyInformation.NsUri,
                                                                        propertyInformation.KeepInContent))
                    {
                        this.AddEpmAttributeToTypeDeclaration(epmAttr, typeDecl);
                    }
                }
                else
                {
                    EntityPropertyMappingAttribute epmAttr = new EntityPropertyMappingAttribute(
                                        propertyInformation.SourcePath,
                                        propertyInformation.TargetPath,
                                        propertyInformation.NsPrefix,
                                        propertyInformation.NsUri,
                                        propertyInformation.KeepInContent);

                    this.AddEpmAttributeToTypeDeclaration(epmAttr, typeDecl);
                }
            }
        }

        /// <summary>Creates an EntityPropertyMappingAttribute and adds it to the <paramref name="typeDecl"/></summary>
        /// <param name="epmAttr">Attribute to add</param>
        /// <param name="typeDecl">Type declaration for which the attribute is generated</param>
        private void AddEpmAttributeToTypeDeclaration(EntityPropertyMappingAttribute epmAttr, CodeTypeDeclaration typeDecl)
        {
            if (epmAttr.TargetSyndicationItem != SyndicationItemProperty.CustomProperty)
            {
                var syndicationItem = new CodeFieldReferenceExpression(
                                        new CodeTypeReferenceExpression(typeof(SyndicationItemProperty)),
                                        epmAttr.TargetSyndicationItem.ToString());
                var contentKind = new CodeFieldReferenceExpression(
                                        new CodeTypeReferenceExpression(typeof(SyndicationTextContentKind)),
                                        epmAttr.TargetTextContentKind.ToString());

                CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(
                                                            TypeReference.FromString(
                                                                    Utils.WebFrameworkCommonNamespace + "." + "EntityPropertyMappingAttribute",
                                                                    true));
                AddAttributeArguments(attribute, new object[] { epmAttr.SourcePath, syndicationItem, contentKind, epmAttr.KeepInContent });
                typeDecl.CustomAttributes.Add(attribute);
            }
            else
            {
                CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(
                                                            TypeReference.FromString(
                                                                    Utils.WebFrameworkCommonNamespace + "." + "EntityPropertyMappingAttribute",
                                                                    true));
                AddAttributeArguments(attribute, new object[] { epmAttr.SourcePath, epmAttr.TargetPath, epmAttr.TargetNamespacePrefix, epmAttr.TargetNamespaceUri, epmAttr.KeepInContent });
                typeDecl.CustomAttributes.Add(attribute);
            }
        }
        
        /// <summary>
        /// Given a resource type, generates the HasStreamAttribute for it
        /// </summary>
        /// <param name="entityType">Entity type for which HasStreamAttribute discovery is happening</param>
        /// <param name="typeDecl">Type declaration to add the attributes to</param>
        private void EmitStreamAttributesForEntityType(EntityType entityType, CodeTypeDeclaration typeDecl)
        {
            IEnumerable<MetadataProperty> hasStreamMetadataProperties =
                entityType.MetadataProperties.Where(mp => mp.PropertyKind == PropertyKind.Extended &&
                    mp.Name == System.Data.Services.XmlConstants.DataWebMetadataNamespace + ":" + System.Data.Services.XmlConstants.DataWebAccessHasStreamAttribute);
            MetadataProperty hasStreamMetadataProperty = null;
            foreach (MetadataProperty p in hasStreamMetadataProperties)
            {
                if (hasStreamMetadataProperty != null)
                {
                    throw new InvalidOperationException(
                        Strings.ObjectContext_MultipleValuesForSameExtendedAttributeType(
                            System.Data.Services.XmlConstants.DataWebAccessHasStreamAttribute, 
                            entityType.Name));
                }
                hasStreamMetadataProperty = p;
            }
            if (hasStreamMetadataProperty == null)
            {
                return;
            }
            if (!String.Equals(Convert.ToString(hasStreamMetadataProperty.Value, CultureInfo.InvariantCulture), 
                    System.Data.Services.XmlConstants.DataWebAccessDefaultStreamPropertyValue, StringComparison.Ordinal))
            {
                return;
            }

            CodeAttributeDeclaration attribute = new CodeAttributeDeclaration(
                                                        TypeReference.FromString(
                                                            Utils.WebFrameworkCommonNamespace + "." + "HasStreamAttribute",
                                                            true));
            typeDecl.CustomAttributes.Add(attribute);
        }

        /// <summary>Edm Member information used for generating attribute, necessary for supporting 
        /// open types which can potentioall not have any member so EdmMember property can be null
        /// </summary>
        private sealed class EdmInfo
        {
            /// <summary>Is the given type a complex type</summary>
            public bool IsComplex 
            { 
                get; 
                set; 
            }
            
            /// <summary>Corresponding EdmMember</summary>
            public EdmMember Member 
            { 
                get; 
                set; 
            }
        }
        
    }
}
