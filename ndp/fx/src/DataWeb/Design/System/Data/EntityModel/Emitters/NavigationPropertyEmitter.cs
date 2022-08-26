//---------------------------------------------------------------------
// <copyright file="NavigationPropertyEmitter.cs" company="Microsoft">
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
using System.Diagnostics;

namespace System.Data.EntityModel.Emitters
{
    /// <summary>
    /// Summary description for NavigationPropertyEmitter.
    /// </summary>
    internal sealed class NavigationPropertyEmitter : PropertyEmitterBase
    {
        private const string ValuePropertyName = "Value";

        /// <summary>
        /// 
        /// </summary>
        /// <param name="generator"></param>
        /// <param name="navigationProperty"></param>
        public NavigationPropertyEmitter(ClientApiGenerator generator, NavigationProperty navigationProperty, bool declaringTypeUsesStandardBaseType)
            : base(generator, navigationProperty, declaringTypeUsesStandardBaseType)
        {
        }

        /// <summary>
        /// Generate the navigation property
        /// </summary>
        /// <param name="typeDecl">The type to add the property to.</param>
        protected override void EmitProperty(CodeTypeDeclaration typeDecl)
        {
            EmitNavigationProperty(typeDecl);
        }

        /// <summary>
        /// Generate the navigation property specified 
        /// </summary>
        /// <param name="typeDecl">The type to add the property to.</param>
        private void EmitNavigationProperty(CodeTypeDeclaration typeDecl)
        {
            // create a regular property
            CodeMemberProperty property = EmitNavigationProperty(Item.ToEndMember);
            typeDecl.Members.Add(property);

            EmitField(typeDecl, GetReturnType(Item.ToEndMember), Item.ToEndMember.RelationshipMultiplicity == RelationshipMultiplicity.Many);
        }

        private void EmitField(CodeTypeDeclaration typeDecl, CodeTypeReference fieldType, bool hasDefault)
        {
            CodeMemberField memberField = new CodeMemberField(fieldType, Utils.FieldNameFromPropName(Item.Name));
            memberField.Attributes = MemberAttributes.Private;

            AttributeEmitter.AddGeneratedCodeAttribute(memberField);

            if (hasDefault)
            {
                if (this.Generator.UseDataServiceCollection)
                {
                    // new DataServiceCollection<T>(null, System.Data.Services.Client.TrackingMode.None, null, null, null);
                    // declare type is DataServiceCollection<T>
                    Debug.Assert(fieldType.TypeArguments.Count == 1, "Declare type is non generic.");

                    // new DataServiceCollection<[type]>(null, TrackingMode.None)
                    memberField.InitExpression = new CodeObjectCreateExpression(
                        fieldType,
                        new CodePrimitiveExpression(null),
                        new CodeFieldReferenceExpression(
                            new CodeTypeReferenceExpression(typeof(System.Data.Services.Client.TrackingMode)),
                            "None"));
                }
                else
                {
                    memberField.InitExpression = new CodeObjectCreateExpression(fieldType);
                }
            }

            typeDecl.Members.Add(memberField);
        }

        /// <summary>
        /// Generate a navigation property
        /// </summary>
        /// <param name="target">the other end</param>
        /// <param name="referenceProperty">True to emit Reference navigation property</param>
        /// <returns>the generated property</returns>
        private CodeMemberProperty EmitNavigationProperty(RelationshipEndMember target)
        {
            CodeTypeReference typeRef = GetReturnType(target);

            // raise the PropertyGenerated event
            PropertyGeneratedEventArgs eventArgs = new PropertyGeneratedEventArgs(Item,
                                                                                  null, // no backing field
                                                                                  typeRef);
            this.Generator.RaisePropertyGeneratedEvent(eventArgs);

            // [System.ComponentModel.Browsable(false)]
            // public TargetType TargetName
            // public EntityReference<TargetType> TargetName
            // or
            // public EntityCollection<targetType> TargetNames
            CodeMemberProperty property = new CodeMemberProperty();

            // Only reference navigation properties are currently currently supported with XML serialization
            // and thus we should use the XmlIgnore and SoapIgnore attributes on other property types.
            AttributeEmitter.AddIgnoreAttributes(property);

            AttributeEmitter.AddBrowsableAttribute(property);

            AttributeEmitter.AddGeneratedCodeAttribute(property);

            CommentEmitter.EmitSummaryComments(Item, property.Comments);

            property.Name = Item.Name;

            if (eventArgs.ReturnType != null && !eventArgs.ReturnType.Equals(typeRef))
            {
                property.Type = eventArgs.ReturnType;
            }
            else
            {
                property.Type = typeRef;
            }

            property.Attributes = MemberAttributes.Final;

            CodeExpression getMethod = EmitGetMethod(target);
            CodeExpression getReturnExpression;

            if (target.RelationshipMultiplicity != RelationshipMultiplicity.Many)
            {
                property.Attributes |= AccessibilityFromGettersAndSetters(Item);

                // insert user-supplied Set code here, before the assignment
                //
                List<CodeStatement> additionalSetStatements = eventArgs.AdditionalSetStatements;
                if (additionalSetStatements != null && additionalSetStatements.Count > 0)
                {
                    try
                    {
                        property.SetStatements.AddRange(additionalSetStatements.ToArray());
                    }
                    catch (ArgumentNullException e)
                    {
                        Generator.AddError(Strings.InvalidSetStatementSuppliedForProperty(Item.Name),
                                           ModelBuilderErrorCode.InvalidSetStatementSuppliedForProperty,
                                           EdmSchemaErrorSeverity.Error,
                                           e);
                    }
                }

                CodeExpression valueRef = new CodePropertySetValueReferenceExpression();
                if (typeRef != eventArgs.ReturnType)
                {
                    // we need to cast to the actual type
                    valueRef = new CodeCastExpression(typeRef, valueRef);
                }


                CodeExpression valueProperty = getMethod;

                // get                
                //     return ((IEntityWithRelationships)this).RelationshipManager.GetRelatedReference<TTargetEntity>("CSpaceQualifiedRelationshipName", "TargetRoleName").Value;
                getReturnExpression = valueProperty;

                // set
                //     ((IEntityWithRelationships)this).RelationshipManager.GetRelatedReference<TTargetEntity>("CSpaceQualifiedRelationshipName", "TargetRoleName").Value = value;
                property.SetStatements.Add(
                    new CodeAssignStatement(valueProperty, valueRef));

                // setup the accessibility of the navigation property setter and getter
                MemberAttributes propertyAccessibility = property.Attributes & MemberAttributes.AccessMask;
                PropertyEmitter.AddGetterSetterFixUp(Generator.FixUps, GetFullyQualifiedPropertyName(property.Name),
                    PropertyEmitter.GetGetterAccessibility(Item), propertyAccessibility, true);
                PropertyEmitter.AddGetterSetterFixUp(Generator.FixUps, GetFullyQualifiedPropertyName(property.Name),
                    PropertyEmitter.GetSetterAccessibility(Item), propertyAccessibility, false);

                List<CodeStatement> additionalAfterSetStatements = eventArgs.AdditionalAfterSetStatements;
                if (additionalAfterSetStatements != null && additionalAfterSetStatements.Count > 0)
                {
                    try
                    {
                        property.SetStatements.AddRange(additionalAfterSetStatements.ToArray());
                    }
                    catch (ArgumentNullException e)
                    {
                        Generator.AddError(Strings.InvalidSetStatementSuppliedForProperty(Item.Name),
                                           ModelBuilderErrorCode.InvalidSetStatementSuppliedForProperty,
                                           EdmSchemaErrorSeverity.Error,
                                           e);
                    }
                }
            }
            else
            {
                property.Attributes |= PropertyEmitter.GetGetterAccessibility(Item);
                // get
                //     return ((IEntityWithRelationships)this).RelationshipManager.GetRelatedCollection<TTargetEntity>("CSpaceQualifiedRelationshipName", "TargetRoleName");
                getReturnExpression = getMethod;

                // set
                // if (value != null) ==> Only for non-binding scenario
                // {
                //    this = 
                //    this.OnPropertyChanged("")
                // }  
                
                CodeExpression valueRef = new CodePropertySetValueReferenceExpression();

                CodeStatementCollection csc = null;

                if (this.Generator.UseDataServiceCollection == true)
                {
                    csc = property.SetStatements;
                }
                else
                {
                    CodeConditionStatement ccs = new CodeConditionStatement(EmitExpressionDoesNotEqualNull(valueRef));
                    property.SetStatements.Add(ccs);

                    csc = ccs.TrueStatements;
                }

                csc.Add(new CodeAssignStatement(getMethod, valueRef));

                if (eventArgs.AdditionalAfterSetStatements != null)
                {
                    try
                    {
                        foreach (CodeStatement s in eventArgs.AdditionalAfterSetStatements)
                        {
                            csc.Add(s);
                        }
                    }
                    catch (ArgumentNullException e)
                    {
                        Generator.AddError(Strings.InvalidSetStatementSuppliedForProperty(Item.Name),
                                           ModelBuilderErrorCode.InvalidSetStatementSuppliedForProperty,
                                           EdmSchemaErrorSeverity.Error,
                                           e);
                    }
                }
            }

            // if additional Get statements were specified by the event subscriber, insert them now
            //
            List<CodeStatement> additionalGetStatements = eventArgs.AdditionalGetStatements;
            if (additionalGetStatements != null && additionalGetStatements.Count > 0)
            {
                try
                {
                    property.GetStatements.AddRange(additionalGetStatements.ToArray());
                }
                catch (ArgumentNullException ex)
                {
                    Generator.AddError(Strings.InvalidGetStatementSuppliedForProperty(Item.Name),
                                       ModelBuilderErrorCode.InvalidGetStatementSuppliedForProperty,
                                       EdmSchemaErrorSeverity.Error,
                                       ex);
                }
            }

            property.GetStatements.Add(new CodeMethodReturnStatement(getReturnExpression));

            return property;
        }

        internal static bool IsNameAlreadyAMemberName(StructuralType type, string generatedPropertyName, StringComparison comparison)
        {
            foreach (EdmMember member in type.Members)
            {
                if (member.DeclaringType == type &&
                    member.Name.Equals(generatedPropertyName, comparison))
                {
                    return true;
                }
            }

            return false;
        }

        private string GetFullyQualifiedPropertyName(string propertyName)
        {
            return Item.DeclaringType.FullName + "." + propertyName;
        }

        /// <summary>
        /// Gives the SchemaElement back cast to the most
        /// appropriate type
        /// </summary>
        private new NavigationProperty Item
        {
            get
            {
                return base.Item as NavigationProperty;
            }
        }

        /// <summary>
        /// Get the return type for the get method, given the target end
        /// </summary>
        /// <param name="target"></param>
        /// <param name="referenceMethod">true if the is the return type for a reference property</param>
        /// <returns>the return type for a target</returns>
        private CodeTypeReference GetReturnType(RelationshipEndMember target)
        {
            CodeTypeReference returnType = Generator.GetLeastPossibleQualifiedTypeReference(GetEntityType(target));

            if (target.RelationshipMultiplicity == RelationshipMultiplicity.Many)
            {
                returnType = TypeReference.FrameworkGenericClass(this.Generator.GetRelationshipMultiplicityManyCollectionTypeName(), returnType);
            }

            return returnType;
        }

        private static EntityTypeBase GetEntityType(RelationshipEndMember endMember)
        {
            Debug.Assert((BuiltInTypeKind.RefType == endMember.TypeUsage.EdmType.BuiltInTypeKind), "not a reference type");
            EntityTypeBase type = ((RefType)endMember.TypeUsage.EdmType).ElementType;
            return type;
        }

        /// <summary>
        /// Emit the GetRelatedCollection or GetRelatedReference methods
        /// </summary>
        /// <param name="target">Target end of the relationship</param>        
        /// <returns>Expression to invoke the appropriate method</returns>
        private CodeExpression EmitGetMethod(RelationshipEndMember target)
        {
            return new CodeFieldReferenceExpression(ThisRef, Utils.FieldNameFromPropName(Item.Name));
        }
    }
}
