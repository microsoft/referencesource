//---------------------------------------------------------------------
// <copyright file="EntityContainerEmitter.cs" company="Microsoft">
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
using System.Diagnostics;
using System.Linq;

namespace System.Data.EntityModel.Emitters
{
    /// <summary>
    /// This class is responsible for emiting the code for the EntityContainer schema element
    /// </summary>
    internal sealed class EntityContainerEmitter : SchemaTypeEmitter
    {
        #region Fields

        private const string _onContextCreatedString = "OnContextCreated";

        #endregion

        #region Constructors

        /// <summary>
        /// 
        /// </summary>
        /// <param name="generator"></param>
        /// <param name="entityContainer"></param>
        public EntityContainerEmitter(ClientApiGenerator generator, EntityContainer entityContainer)
            : base(generator, entityContainer)
        {
        }

        #endregion

        #region Properties, Methods, Events & Delegates
        /// <summary>
        /// Creates the CodeTypeDeclarations necessary to generate the code for the EntityContainer schema element
        /// </summary>
        /// <returns></returns>
        public override CodeTypeDeclarationCollection EmitApiClass()
        {
            Validate(); // emitter-specific validation

            // declare the new class
            // public partial class LOBScenario : ObjectContext
            CodeTypeDeclaration typeDecl = new CodeTypeDeclaration(Item.Name);
            typeDecl.IsPartial = true;

            // raise the TypeGenerated event
            CodeTypeReference objectContextTypeRef = TypeReference.ObjectContext;
            TypeGeneratedEventArgs eventArgs = new TypeGeneratedEventArgs(Item, objectContextTypeRef);
            Generator.RaiseTypeGeneratedEvent(eventArgs);

            if (eventArgs.BaseType != null && !eventArgs.BaseType.Equals(objectContextTypeRef))
            {
                typeDecl.BaseTypes.Add(eventArgs.BaseType);
            }
            else
            {
                typeDecl.BaseTypes.Add(TypeReference.ObjectContext);
            }

            AddInterfaces(Item.Name, typeDecl, eventArgs.AdditionalInterfaces);

            CommentEmitter.EmitSummaryComments(Item, typeDecl.Comments);
            EmitTypeAttributes(Item.Name, typeDecl, eventArgs.AdditionalAttributes);

            bool needTypeMapper = (0 < this.Generator.NamespaceMap.Count);

            var q = from a in this.Generator.EdmItemCollection.GetItems<StructuralType>()
                    where (a.BaseType != null) &&
                          (a.BuiltInTypeKind == BuiltInTypeKind.ComplexType || a.BuiltInTypeKind == BuiltInTypeKind.EntityType)
                    select a;
            bool hasInheritance = (null != q.FirstOrDefault());

            CreateConstructors(typeDecl, needTypeMapper, hasInheritance);
            // adding partial OnContextCreated method 
            CreateContextPartialMethods(typeDecl);

            if (needTypeMapper || hasInheritance)
            {
                CreateTypeMappingMethods(typeDecl, needTypeMapper, hasInheritance);
            }

            foreach (EntitySetBase entitySetBase in Item.BaseEntitySets)
            {
                if (Helper.IsEntitySet(entitySetBase))
                {
                    EntitySet set = (EntitySet)entitySetBase;
                    CodeMemberProperty codeProperty = CreateEntitySetProperty(set);
                    typeDecl.Members.Add(codeProperty);

                    CodeMemberField codeField = CreateEntitySetField(set);
                    typeDecl.Members.Add(codeField);
                }
            }

            foreach (EntitySetBase entitySetBase in Item.BaseEntitySets)
            {
                if (Helper.IsEntitySet(entitySetBase))
                {
                    EntitySet set = (EntitySet)entitySetBase;
                    CodeMemberMethod codeProperty = CreateEntitySetAddObjectMethod(set);
                    typeDecl.Members.Add(codeProperty);
                }
            }

            // additional members, if provided by the event subscriber
            AddMembers(Item.Name, typeDecl, eventArgs.AdditionalMembers);

            CodeTypeDeclarationCollection typeDecls = new CodeTypeDeclarationCollection();
            typeDecls.Add(typeDecl);
            return typeDecls;
        }

        /// <summary>
        /// Emitter-specific validation: check if there exist entity containers and
        /// entity sets that have the same name but differ in case
        /// </summary>
        protected override void Validate()
        {
            base.Validate();
            Generator.VerifyLanguageCaseSensitiveCompatibilityForEntitySet(Item);
            VerifyEntityTypeAndSetAccessibilityCompatability();
        }

        /// <summary>
        /// Verify that Entity Set and Type have compatible accessibilty.
        /// They are compatible if the generated code will compile.
        /// </summary>
        private void VerifyEntityTypeAndSetAccessibilityCompatability()
        {
            foreach (EntitySetBase entitySetBase in Item.BaseEntitySets)
            {
                if (Helper.IsEntitySet(entitySetBase))
                {
                    EntitySet set = (EntitySet)entitySetBase;
                    if (!AreTypeAndSetAccessCompatible(GetEntityTypeAccessibility(set.ElementType), GetEntitySetPropertyAccessibility(set)))
                    {
                        Generator.AddError(
                            Strings.EntityTypeAndSetAccessibilityConflict(
                                set.ElementType.Name, GetAccessibilityCsdlStringFromMemberAttribute(GetEntityTypeAccessibility(set.ElementType)), set.Name, GetAccessibilityCsdlStringFromMemberAttribute(GetEntitySetPropertyAccessibility(set))),
                            ModelBuilderErrorCode.EntityTypeAndSetAccessibilityConflict,
                            EdmSchemaErrorSeverity.Error);
                    }
                }
            }
        }


        /// <summary>
        /// Tells whether Entity Type's specified accessibility and Entity Set Property's specified Accessibility will work together (compile) when codegen'd.
        /// False if (Type is internal and Set's Property is Public OR, type is internal and Set's property is protected).
        /// True otherwise
        /// </summary>
        private bool AreTypeAndSetAccessCompatible(MemberAttributes typeAccess, MemberAttributes setAccess)
        {
            return !(typeAccess == MemberAttributes.Assembly && (setAccess == MemberAttributes.Public || setAccess == MemberAttributes.Family));
        }

        /// <summary>
        /// Creates the necessary constructors for the entity container.
        /// </summary>
        private void CreateConstructors(CodeTypeDeclaration typeDecl, bool setupTypeMapper, bool hasInheritance)
        {
            // Constructor that takes a uri
            //
            // public ctor(System.Uri serviceRoot)
            //    : base(serviceRoot)
            // {
            //      this.OnContextCreated();
            // }
            CodeConstructor connectionWorkspaceCtor = new CodeConstructor();
            connectionWorkspaceCtor.Attributes = MemberAttributes.Public;
            CodeParameterDeclarationExpression connectionParam = new CodeParameterDeclarationExpression(TypeReference.FromString("System.Uri", true), "serviceRoot");
            connectionWorkspaceCtor.Parameters.Add(connectionParam);
            connectionWorkspaceCtor.BaseConstructorArgs.Add(new CodeArgumentReferenceExpression(connectionParam.Name));
            
            AttributeEmitter.AddGeneratedCodeAttribute(connectionWorkspaceCtor);

            CommentEmitter.EmitSummaryComments(Strings.CtorSummaryComment(Item.Name), connectionWorkspaceCtor.Comments);

            // If we have an externally provided namespage (e.g. from Visual Studio), we need to
            // inject a type-mapper because type names won't match between client and server
            if (setupTypeMapper || hasInheritance)
            {
                connectionWorkspaceCtor.Statements.Add(
                    new CodeAssignStatement(
                        new CodeFieldReferenceExpression(ThisRef, "ResolveName"),
                        new CodeDelegateCreateExpression(
                            TypeReference.ForType(typeof(Func<,>), TypeReference.ForType(typeof(Type)), TypeReference.ForType(typeof(String))),
                             ThisRef,
                             "ResolveNameFromType"
                         )
                    )
                );
            }

            if (setupTypeMapper)
            {
                connectionWorkspaceCtor.Statements.Add(
                    new CodeAssignStatement(
                        new CodeFieldReferenceExpression(ThisRef, "ResolveType"),
                        new CodeDelegateCreateExpression(
                            TypeReference.ForType(typeof(Func<,>), TypeReference.ForType(typeof(String)), TypeReference.ForType(typeof(Type))),
                             ThisRef,
                             "ResolveTypeFromName"
                         )
                    )
                );
            }

            connectionWorkspaceCtor.Statements.Add(OnContextCreatedCodeMethodInvokeExpression());

            typeDecl.Members.Add(connectionWorkspaceCtor);
        }

        /// <summary>
        /// Adds the OnContextCreated partial method for the entity container.
        /// </summary>
        private void CreateContextPartialMethods(CodeTypeDeclaration typeDecl)
        {
            CodeMemberMethod onContextCreatedPartialMethod = new CodeMemberMethod();
            onContextCreatedPartialMethod.Name = _onContextCreatedString;
            onContextCreatedPartialMethod.ReturnType = new CodeTypeReference(typeof(void));
            onContextCreatedPartialMethod.Attributes = MemberAttributes.Abstract | MemberAttributes.Public;
            typeDecl.Members.Add(onContextCreatedPartialMethod);

            Generator.FixUps.Add(new FixUp(Item.Name + "." + _onContextCreatedString, FixUpType.MarkAbstractMethodAsPartial));
        }

        private void CreateTypeMappingMethods(CodeTypeDeclaration typeDecl, bool needTypeMapper, bool hasInheritance)
        {
            // Special case to compensate for VB's "root namespace" feature.
            if (this.Generator.Language == LanguageOption.GenerateVBCode)
            {
                AddRootNamespaceField(typeDecl);
            }

            CodeExpression comparisonExpression = new CodePropertyReferenceExpression(
                    new CodeTypeReferenceExpression(TypeReference.ForType(typeof(StringComparison))),
                    Enum.GetName(typeof(StringComparison), this.Generator.LanguageAppropriateStringComparer));

            if (needTypeMapper)
            {
                CodeMemberMethod resolveTypeFromName = new CodeMemberMethod();
                resolveTypeFromName.Name = "ResolveTypeFromName";
                resolveTypeFromName.Attributes = MemberAttributes.Final | MemberAttributes.Family;
                resolveTypeFromName.Parameters.Add(new CodeParameterDeclarationExpression(TypeReference.ForType(typeof(string)), "typeName"));
                resolveTypeFromName.ReturnType = TypeReference.ForType(typeof(Type));
                AttributeEmitter.AddGeneratedCodeAttribute(resolveTypeFromName);
                CommentEmitter.EmitSummaryComments(Strings.TypeMapperDescription, resolveTypeFromName.Comments);

                // NOTE: since multiple namespaces can have the same prefix and match the namespace
                // prefix condition, it's important that the prefix check is done is prefix-length
                // order, starting with the longest prefix.
                var pairs = this.Generator.NamespaceMap.OrderByDescending(p => p.Key.Length).ThenBy(p => p.Key);

                foreach (var pair in pairs)
                {
                    // Assuming pair.Key is "abc" and pair.Value is "def" and len(def)=3, generate:
                    // if (typeName.StartsWith("abc", StringComparison.Ordinal)) 
                    //     return this.GetType().Assembly.GetType(string.Concat("def", typeName.Substring(3)), false)
                    // DEVNOTE(Microsoft): we should use GetType(xxx, FALSE) here so it will not throw and fall back to null
                    // GetType(type, bool throw, bool ignoreCase) does not exist in SL, do not use!
                    resolveTypeFromName.Statements.Add(
                        new CodeConditionStatement(
                            new CodeMethodInvokeExpression(
                                new CodeVariableReferenceExpression("typeName"),
                                "StartsWith",
                                new CodePrimitiveExpression(pair.Key),
                                comparisonExpression
                            ),
                            new CodeMethodReturnStatement(
                                new CodeMethodInvokeExpression(
                                    new CodePropertyReferenceExpression(
                                        new CodeMethodInvokeExpression(
                                            ThisRef,
                                            "GetType"
                                        ),
                                        "Assembly"
                                    ),
                                    "GetType",
                                    new CodeMethodInvokeExpression(
                                        new CodeTypeReferenceExpression(TypeReference.ForType(typeof(string))),
                                        "Concat",
                                        this.LanguageSpecificNamespace(pair.Value),
                                        new CodeMethodInvokeExpression(
                                            new CodeVariableReferenceExpression("typeName"),
                                            "Substring",
                                            new CodePrimitiveExpression(pair.Key.Length)
                                        )
                                    ),
                                    new CodePrimitiveExpression(false)
                                )
                            )
                        )
                    );
                }

                resolveTypeFromName.Statements.Add(
                    new CodeMethodReturnStatement(
                        new CodePrimitiveExpression(null)));

                typeDecl.Members.Add(resolveTypeFromName);
            }

            CodeMemberMethod resolveNameFromType = new CodeMemberMethod();
            resolveNameFromType.Name = "ResolveNameFromType";
            resolveNameFromType.Attributes = MemberAttributes.Final | MemberAttributes.Family;
            resolveNameFromType.Parameters.Add(new CodeParameterDeclarationExpression(TypeReference.ForType(typeof(Type)), "clientType"));
            resolveNameFromType.ReturnType = TypeReference.ForType(typeof(String));
            AttributeEmitter.AddGeneratedCodeAttribute(resolveNameFromType);
            CommentEmitter.EmitSummaryComments(Strings.TypeMapperDescription, resolveNameFromType.Comments);

            // NOTE: in this case order also matters, but the length of the CLR
            // namespace is what needs to be considered.
            var reversePairs = Generator.NamespaceMap.OrderByDescending(p => p.Value.Length).ThenBy(p => p.Key);

            foreach (var pair in reversePairs)
            {
                // Assuming pair.Key is "abc" and pair.Value is "def", generate:
                // if (t.Namespace.Equals("def", StringComparison.Ordinal)) return string.Concat("abc.", t.Name);

                resolveNameFromType.Statements.Add(
                    new CodeConditionStatement(
                        new CodeMethodInvokeExpression(
                            new CodePropertyReferenceExpression(
                                new CodeVariableReferenceExpression("clientType"),
                                "Namespace"
                            ),
                            "Equals",
                            this.LanguageSpecificNamespace(pair.Value),
                            comparisonExpression
                        ),
                        new CodeMethodReturnStatement(
                            new CodeMethodInvokeExpression(
                                new CodeTypeReferenceExpression(TypeReference.ForType(typeof(string))),
                                "Concat",
                                new CodePrimitiveExpression(pair.Key + "."),
                                new CodePropertyReferenceExpression(
                                    new CodeVariableReferenceExpression("clientType"),
                                    "Name"
                                )
                            )
                        )
                    )
                );
            }

            if (hasInheritance)
            {
                CodeExpression clientTypeFullName = new CodePropertyReferenceExpression(
                                                        new CodeVariableReferenceExpression("clientType"),
                                                        "FullName");

                if (this.Generator.Language == LanguageOption.GenerateVBCode)
                {
                    // return clientType.FullName.Substring(ROOTNAMESPACE.Length);
                    resolveNameFromType.Statements.Add(
                        new CodeMethodReturnStatement(
                            new CodeMethodInvokeExpression(
                                clientTypeFullName,
                                "Substring",
                                new CodePropertyReferenceExpression(
                                    new CodeVariableReferenceExpression("ROOTNAMESPACE"),
                                    "Length"
                                )
                            )
                        )
                    );
                }
                else
                {
                    // return clientType.FullName;
                    resolveNameFromType.Statements.Add(new CodeMethodReturnStatement(clientTypeFullName));
                }
            }
            else
            {
                resolveNameFromType.Statements.Add(
                    new CodeMethodReturnStatement(
                        NullExpression
                    )
                );
            }

            typeDecl.Members.Add(resolveNameFromType);
        }

        private void AddRootNamespaceField(CodeTypeDeclaration typeDecl)
        {
            // Add this field (VB declaration), where known client type name is ClientNS.BikesEntities
            // ROOTNAMESPACE = GetType(ClientNS.BikesEntities).Namespace.Remove(GetType(ClientNS.BikesEntities).Namespace.LastIndexOf("ClientNS"))

            EntityContainer container = (EntityContainer)this.Item;
            string containerNamespace = Generator.GetClientTypeNamespace(this.Generator.GetContainerNamespace(container));
            // VB-ism: if namespace and type names are the same (Blah.Blah), GetType(Blah.Blah) fails...
            string vbScopedcontainerName = (string.IsNullOrEmpty(containerNamespace) || container.Name.Equals(containerNamespace, StringComparison.OrdinalIgnoreCase))
                                           ? container.Name : (containerNamespace + "." + container.Name);


            CodeExpression namespaceAccess = new CodePropertyReferenceExpression(
                                                new CodeTypeOfExpression(vbScopedcontainerName),
                                                "Namespace");

            CodeMemberField rootNamespaceField = new CodeMemberField(
                                                    TypeReference.ForType(typeof(string)),
                                                    "ROOTNAMESPACE");
            rootNamespaceField.Attributes = MemberAttributes.Static | MemberAttributes.Private;
            rootNamespaceField.InitExpression = new CodeMethodInvokeExpression(
                                                    namespaceAccess,
                                                    "Remove",
                                                    new CodeMethodInvokeExpression(
                                                        namespaceAccess,
                                                        "LastIndexOf",
                                                        new CodePrimitiveExpression(containerNamespace)));
            AttributeEmitter.AddGeneratedCodeAttribute(rootNamespaceField);
            typeDecl.Members.Add(rootNamespaceField);
        }

        private CodeExpression LanguageSpecificNamespace(string ns)
        {
            if (this.Generator.Language == LanguageOption.GenerateVBCode)
            {
                return new CodeMethodInvokeExpression(
                    new CodeTypeReferenceExpression(TypeReference.ForType(typeof(string))),
                    "Concat",
                    new CodeVariableReferenceExpression("ROOTNAMESPACE"),
                    new CodePrimitiveExpression(ns));
            }
            else
            {
                return new CodePrimitiveExpression(ns);
            }
        }

        private CodeMemberField CreateEntitySetField(EntitySet set)
        {
            Debug.Assert(set != null, "Field is Null");

            // trying to get
            //
            // private ObjectQuery<SpanTestsModel.Customer> _Customers = null;

            CodeMemberField codeField = new CodeMemberField();
            codeField.Attributes = MemberAttributes.Final | MemberAttributes.Private;
            codeField.Name = Utils.FieldNameFromPropName(set.Name);
            AttributeEmitter.AddGeneratedCodeAttribute(codeField);
            
            CodeTypeReference genericParameter = Generator.GetLeastPossibleQualifiedTypeReference(set.ElementType);
            codeField.Type = TypeReference.AdoFrameworkGenericClass("DataServiceQuery", genericParameter);
            return codeField;
        }

        private CodeMemberProperty CreateEntitySetProperty(EntitySet set)
        {
            Debug.Assert(set != null, "Property is Null");

            // trying to get
            //
            // [System.ComponentModel.Browsable(false)]
            // public ObjectQuery<Customer> Customers
            // {
            //      get
            //      {
            //          if ((this._Customers == null))
            //          {
            //              this._Customers = base.CreateQuery<Customer>("[Customers]");
            //          }
            //          return this._Customers;
            //      }
            // }
            CodeMemberProperty codeProperty = new CodeMemberProperty();
            codeProperty.Attributes = MemberAttributes.Final | GetEntitySetPropertyAccessibility(set);
            codeProperty.Name = set.Name;
            codeProperty.HasGet = true;
            codeProperty.HasSet = false;

            // SQLBUDT 598300: property to get query hides a property on base Context?
            if (null != TypeReference.ObjectContextBaseClassType.GetProperty(set.Name))
            {
                codeProperty.Attributes |= MemberAttributes.New;
            }

            AttributeEmitter.AddBrowsableAttribute(codeProperty);
            AttributeEmitter.AddGeneratedCodeAttribute(codeProperty);

            CodeTypeReference genericParameter = Generator.GetLeastPossibleQualifiedTypeReference(set.ElementType);
            codeProperty.Type = TypeReference.AdoFrameworkGenericClass("DataServiceQuery", genericParameter);
            string fieldName = Utils.FieldNameFromPropName(set.Name);

            // raise the PropertyGenerated event before proceeding further
            PropertyGeneratedEventArgs eventArgs = new PropertyGeneratedEventArgs(set, fieldName, codeProperty.Type);
            Generator.RaisePropertyGeneratedEvent(eventArgs);

            if (eventArgs.ReturnType == null || !eventArgs.ReturnType.Equals(codeProperty.Type))
            {
                throw EDesignUtil.InvalidOperation(Strings.CannotChangePropertyReturnType(set.Name, Item.Name));
            }

            List<CodeAttributeDeclaration> additionalAttributes = eventArgs.AdditionalAttributes;
            if (additionalAttributes != null && additionalAttributes.Count > 0)
            {
                try
                {
                    codeProperty.CustomAttributes.AddRange(additionalAttributes.ToArray());
                }
                catch (ArgumentNullException e)
                {
                    Generator.AddError(Strings.InvalidAttributeSuppliedForProperty(Item.Name),
                                               ModelBuilderErrorCode.InvalidAttributeSuppliedForProperty,
                                               EdmSchemaErrorSeverity.Error,
                                               e);
                }
            }

            // we need to insert user-specified code before other/existing code, including
            // the return statement
            List<CodeStatement> additionalGetStatements = eventArgs.AdditionalGetStatements;

            if (additionalGetStatements != null && additionalGetStatements.Count > 0)
            {
                try
                {
                    codeProperty.GetStatements.AddRange(additionalGetStatements.ToArray());
                }
                catch (ArgumentNullException e)
                {
                    Generator.AddError(Strings.InvalidGetStatementSuppliedForProperty(Item.Name),
                                       ModelBuilderErrorCode.InvalidGetStatementSuppliedForProperty,
                                       EdmSchemaErrorSeverity.Error,
                                       e);
                }
            }

            codeProperty.GetStatements.Add(
                new CodeConditionStatement(
                    EmitExpressionEqualsNull(new CodeFieldReferenceExpression(ThisRef, fieldName)),
                    new CodeAssignStatement(
                        new CodeFieldReferenceExpression(ThisRef, fieldName),
                        new CodeMethodInvokeExpression(
                            new CodeMethodReferenceExpression(
                                new CodeBaseReferenceExpression(),
                                "CreateQuery",
                                new CodeTypeReference[] { genericParameter }
                            ),
                            new CodePrimitiveExpression(set.Name)
                        )
                    )
                )
            );

            codeProperty.GetStatements.Add(
                new CodeMethodReturnStatement(
                    new CodeFieldReferenceExpression(
                        ThisRef,
                        fieldName
                    )
                )
            );

            // property summary
            CommentEmitter.EmitSummaryComments(set, codeProperty.Comments);

            return codeProperty;
        }

        /// <summary>
        /// Create an AddTo-EntitysetName methiod for each entityset in the context.
        /// </summary>
        /// <param name="set">EntityContainerEntitySet that we will go over to get the existing entitysets.</param>
        /// <returns> Method definition </returns>

        private CodeMemberMethod CreateEntitySetAddObjectMethod(EntitySet set)
        {
            Debug.Assert(set != null, "Property is Null");

            // trying to get
            //
            // public void AddToCustomer(Customer customer)
            // {
            //      base.AddObject("Customer", customer);
            // }
            CodeMemberMethod codeMethod = new CodeMemberMethod();
            codeMethod.Attributes = MemberAttributes.Final | GetEntityTypeAccessibility(set.ElementType);
            codeMethod.Name = ("AddTo" + set.Name);

            CodeParameterDeclarationExpression parameter = new CodeParameterDeclarationExpression();

            parameter.Type = Generator.GetLeastPossibleQualifiedTypeReference(set.ElementType);
            parameter.Name = Utils.CamelCase(set.ElementType.Name);
            parameter.Name = Utils.SetSpecialCaseForFxCopOnPropertyName(parameter.Name);
            codeMethod.Parameters.Add(parameter);

            codeMethod.ReturnType = new CodeTypeReference(typeof(void));

            AttributeEmitter.AddGeneratedCodeAttribute(codeMethod);

            codeMethod.Statements.Add(
                new CodeMethodInvokeExpression(
                    new CodeBaseReferenceExpression(),
                    "AddObject",
                    new CodePrimitiveExpression(set.Name),
                    new CodeFieldReferenceExpression(null, parameter.Name)
                )
            );

            // method summary
            CommentEmitter.EmitSummaryComments(set, codeMethod.Comments);
            return codeMethod;
        }

        /// <summary>
        /// return a code expression for invoking OnContextCreated partial method
        /// </summary>
        private CodeMethodInvokeExpression OnContextCreatedCodeMethodInvokeExpression()
        {
            return (new CodeMethodInvokeExpression(new CodeThisReferenceExpression(), _onContextCreatedString, new CodeExpression[] { }));
        }

        /// <summary>
        /// Returns the type specific SchemaElement
        /// </summary>
        private new EntityContainer Item
        {
            get
            {
                return base.Item as EntityContainer;
            }
        }

        #endregion
    }
}
