//---------------------------------------------------------------------
// <copyright file="StructuredTypeEmitter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner       Microsoft
// @backupOwner Microsoft
//---------------------------------------------------------------------

using System.CodeDom;
using System.Collections.Generic;
using System.Data.Metadata.Edm;
using System.Data.Services.Design;
using System.Data.Services.Design.Common;

namespace System.Data.EntityModel.Emitters
{
    /// <summary>
    /// Summary description for StructuredTypeEmitter.
    /// </summary>
    internal abstract class StructuredTypeEmitter : SchemaTypeEmitter
    {
        #region Public Methods
        private bool _usingStandardBaseClass = true;



        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public override CodeTypeDeclarationCollection EmitApiClass()
        {
            Validate(); // emitter-specific validation

            CodeTypeReference baseType = this.GetBaseType();

            // raise the TypeGenerated event
            TypeGeneratedEventArgs eventArgs = new TypeGeneratedEventArgs(Item, baseType);
            this.Generator.RaiseTypeGeneratedEvent(eventArgs);

            // public [abstract] partial class ClassName
            CodeTypeDeclaration typeDecl = new CodeTypeDeclaration(Item.Name);
            typeDecl.IsPartial = true;
            typeDecl.TypeAttributes = System.Reflection.TypeAttributes.Class;
            if (Item.Abstract)
            {
                typeDecl.TypeAttributes |= System.Reflection.TypeAttributes.Abstract;
            }

            SetTypeVisibility(typeDecl);

            EmitTypeAttributes(Item.Name, typeDecl, eventArgs.AdditionalAttributes);

            // : baseclass
            AssignBaseType(typeDecl, baseType, eventArgs.BaseType);

            AddInterfaces(Item.Name, typeDecl, eventArgs.AdditionalInterfaces);

            CommentEmitter.EmitSummaryComments(Item, typeDecl.Comments);

            // Since abstract types cannot be instantiated, skip the factory method for abstract types
            if ((typeDecl.TypeAttributes & System.Reflection.TypeAttributes.Abstract) == 0)
                EmitFactoryMethod(typeDecl);

            EmitProperties(typeDecl);

            // additional members, if provided by the event subscriber
            this.AddMembers(Item.Name, typeDecl, eventArgs.AdditionalMembers);

            CodeTypeDeclarationCollection typeDecls = new CodeTypeDeclarationCollection();
            typeDecls.Add(typeDecl);
            return typeDecls;
        }

        #endregion

        #region Protected Methods

        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        protected virtual CodeTypeReference GetBaseType()
        {
            if (Item.BaseType == null)
                return null;

            return Generator.GetLeastPossibleQualifiedTypeReference(Item.BaseType);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="generator"></param>
        /// <param name="structuredType"></param>
        protected StructuredTypeEmitter(ClientApiGenerator generator, StructuralType structuralType)
            : base(generator, structuralType)
        {
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="typeDecl"></param>
        protected override void EmitTypeAttributes(CodeTypeDeclaration typeDecl)
        {
            Generator.AttributeEmitter.EmitTypeAttributes(this, typeDecl);
            base.EmitTypeAttributes(typeDecl);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="typeDecl"></param>
        protected virtual void EmitProperties(CodeTypeDeclaration typeDecl)
        {
            foreach (EdmMember member in Item.Members)
            {
                EdmProperty property = (member as EdmProperty);
                if ((null != property) && (property.DeclaringType == Item))
                {
                    PropertyEmitter propertyEmitter = new PropertyEmitter(Generator, property, _usingStandardBaseClass);
                    propertyEmitter.Emit(typeDecl);
                }
            }
        }

        protected abstract ReadOnlyMetadataCollection<EdmProperty> GetProperties();

        /// <summary>
        /// Emit static factory method which creates an instance of the class and initializes
        /// non-nullable properties (taken as arguments)
        /// </summary>
        /// <param name="typeDecl"></param>
        protected virtual void EmitFactoryMethod(CodeTypeDeclaration typeDecl)
        {
            // build list of non-nullable properties
            ReadOnlyMetadataCollection<EdmProperty> properties = GetProperties();
            List<EdmProperty> parameters = new List<EdmProperty>(properties.Count);
            foreach (EdmProperty property in properties)
            {
                bool include = IncludeFieldInFactoryMethod(property);
                if (include)
                {
                    parameters.Add(property);
                }
            }

            // if there are no parameters, we don't emit anything (1 is for the null element)
            // nor do we emit everything if this is the Ref propertied ctor and the parameter list is the same as the many parametered ctor
            if (parameters.Count < 1)
            {
                return;
            }

            CodeMemberMethod method = new CodeMemberMethod();
            CodeTypeReference typeRef = TypeReference.FromString(Item.Name);
            UniqueIdentifierService uniqueIdentifierService = new UniqueIdentifierService(Generator.IsLanguageCaseSensitive);
            string instanceName = uniqueIdentifierService.AdjustIdentifier(Utils.CamelCase(Item.Name));

            // public static Class CreateClass(...)
            method.Attributes = MemberAttributes.Static | MemberAttributes.Public;
            method.Name = "Create" + Item.Name;
            if (NavigationPropertyEmitter.IsNameAlreadyAMemberName(Item, method.Name, Generator.LanguageAppropriateStringComparer))
            {
                Generator.AddError(Strings.GeneratedFactoryMethodNameConflict(method.Name, Item.Name),
                    ModelBuilderErrorCode.GeneratedFactoryMethodNameConflict,
                    EdmSchemaErrorSeverity.Error);
            }

            method.ReturnType = typeRef;
            AttributeEmitter.AddGeneratedCodeAttribute(method);

            // output method summary comments 
            CommentEmitter.EmitSummaryComments(Strings.FactoryMethodSummaryComment(Item.Name), method.Comments);


            // Class class = new Class();
            CodeVariableDeclarationStatement createNewInstance = new CodeVariableDeclarationStatement(
                typeRef, instanceName, new CodeObjectCreateExpression(typeRef));
            method.Statements.Add(createNewInstance);
            CodeVariableReferenceExpression instanceRef = new CodeVariableReferenceExpression(instanceName);

            // iterate over the properties figuring out which need included in the factory method
            foreach (EdmProperty property in parameters)
            {
                // CreateClass( ... , propType propName ...)
                PropertyEmitter propertyEmitter = new PropertyEmitter(Generator, property, UsingStandardBaseClass);
                CodeTypeReference propertyTypeReference = propertyEmitter.PropertyType;
                String parameterName = uniqueIdentifierService.AdjustIdentifier(Utils.FixParameterName(propertyEmitter.PropertyName, "argument"));
                parameterName = Utils.SetSpecialCaseForFxCopOnPropertyName(parameterName);
                CodeParameterDeclarationExpression paramDecl = new CodeParameterDeclarationExpression(
                    propertyTypeReference, parameterName);
                CodeArgumentReferenceExpression paramRef = new CodeArgumentReferenceExpression(paramDecl.Name);
                method.Parameters.Add(paramDecl);

                // add comment describing the parameter
                CommentEmitter.EmitParamComments(paramDecl, Strings.FactoryParamCommentGeneral(propertyEmitter.PropertyName), method.Comments);

                CodeExpression newPropertyValue;
                if (TypeSemantics.IsComplexType(propertyEmitter.Item.TypeUsage))
                {
                    List<CodeExpression> complexVerifyParameters = new List<CodeExpression>();
                    complexVerifyParameters.Add(paramRef);
                    complexVerifyParameters.Add(new CodePrimitiveExpression(propertyEmitter.PropertyName));

                    // if (null == param) { throw new ArgumentNullException("PropertyName"); }
                    method.Statements.Add(
                        new CodeConditionStatement(
                            EmitExpressionEqualsNull(paramRef),
                            new CodeThrowExceptionStatement(
                                new CodeObjectCreateExpression(
                                    TypeReference.ForType(typeof(ArgumentNullException)),
                                    new CodePrimitiveExpression(parameterName)
                                )
                            )
                        )
                    );

                    newPropertyValue = paramRef;
                }
                else
                {
                    newPropertyValue = paramRef;
                }

                // Scalar property:
                //     Property = param;
                // Complex property:
                //     Property = StructuralObject.VerifyComplexObjectIsNotNull(param, propertyName);

                method.Statements.Add(new CodeAssignStatement(new CodePropertyReferenceExpression(instanceRef, propertyEmitter.PropertyName), newPropertyValue));
            }

            // return class;
            method.Statements.Add(new CodeMethodReturnStatement(instanceRef));

            // actually add the method to the class
            typeDecl.Members.Add(method);
        }


        #endregion

        #region Protected Properties

        internal new StructuralType Item
        {
            get
            {
                return base.Item as StructuralType;
            }
        }

        protected bool UsingStandardBaseClass
        {
            get { return _usingStandardBaseClass; }
        }

        #endregion

        #region Private Methods

        /// <summary>
        /// 
        /// </summary>
        /// <param name="property"></param>
        /// <returns></returns>
        private bool IncludeFieldInFactoryMethod(EdmProperty property)
        {
            if (property.Nullable)
            {
                return false;
            }

            if (PropertyEmitter.HasDefault(property))
            {
                return false;
            }

            if (PropertyEmitter.GetGetterAccessibility(property) != MemberAttributes.Public &&
                PropertyEmitter.GetSetterAccessibility(property) != MemberAttributes.Public)
            {
                return false;
            }

            return true;
        }

        private void AssignBaseType(CodeTypeDeclaration typeDecl,
                                    CodeTypeReference baseType,
                                    CodeTypeReference eventReturnedBaseType)
        {
            if (eventReturnedBaseType != null && !eventReturnedBaseType.Equals(baseType))
            {
                _usingStandardBaseClass = false;
                typeDecl.BaseTypes.Add(eventReturnedBaseType);
            }
            else
            {
                if (baseType != null)
                {
                    typeDecl.BaseTypes.Add(baseType);
                }
            }
        }
        #endregion
    }
}
