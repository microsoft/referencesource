//---------------------------------------------------------------------
// <copyright file="ItemTypeEmitter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner       Microsoft
// @backupOwner Microsoft
//---------------------------------------------------------------------

using System.CodeDom;
using System.Data.Metadata.Edm;

namespace System.Data.EntityModel.Emitters
{
    /// <summary>
    /// Summary description for ItemTypeEmitter.
    /// </summary>
    internal sealed class EntityTypeEmitter : StructuredTypeEmitter
    {
        #region Public Methods
        /// <summary>
        /// 
        /// </summary>
        /// <param name="generator"></param>
        /// <param name="itemType"></param>
        public EntityTypeEmitter(ClientApiGenerator generator, EntityType entity)
            : base(generator, entity)
        {
        }
        #endregion

        #region Protected Methods
        /// <summary>
        /// 
        /// </summary>
        /// <param name="typeDecl"></param>
        protected override void EmitProperties(CodeTypeDeclaration typeDecl)
        {
            base.EmitProperties(typeDecl);
            foreach (EdmMember member in Item.Members)
            {
                NavigationProperty navigationProperty = (member as NavigationProperty);
                if ((null != navigationProperty) && (navigationProperty.DeclaringType == Item))
                {
                    NavigationPropertyEmitter navigationPropertyEmitter = new NavigationPropertyEmitter(Generator, navigationProperty, UsingStandardBaseClass);
                    navigationPropertyEmitter.Emit(typeDecl);
                }
            }
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA2204:Literals should be spelled correctly", MessageId = "KeyProperties")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Globalization", "CA1303:Do not pass literals as localized parameters", MessageId = ".ctor")]
        public override CodeTypeDeclarationCollection EmitApiClass()
        {
            CodeTypeDeclarationCollection typeDecls = base.EmitApiClass();

            if (Item.KeyMembers.Count > 0 && typeDecls.Count == 1)
            {
                // generate xml comments for the key properties
                CodeTypeDeclaration typeDecl = typeDecls[0];
                typeDecl.Comments.Add(new CodeCommentStatement("<KeyProperties>", true));
                foreach (EdmMember keyProperty in Item.KeyMembers)
                {
                    string name = keyProperty.Name;
                    typeDecl.Comments.Add(new CodeCommentStatement(name, true));
                }
                typeDecl.Comments.Add(new CodeCommentStatement("</KeyProperties>", true));
            }

            return typeDecls;
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
        /// <returns></returns>
        protected override CodeTypeReference GetBaseType()
        {
            CodeTypeReference baseType = base.GetBaseType();
            return baseType;
        }

        protected override ReadOnlyMetadataCollection<EdmProperty> GetProperties()
        {
            return Item.Properties;
        }
        #endregion



        #region Public Properties
        #endregion

        #region Protected Properties
        #endregion

        #region Private Properties
        /// <summary>
        /// Gets the SchemaElement that this class is generating code for.
        /// </summary>
        /// <value></value>
        public new EntityType Item
        {
            get
            {
                return base.Item as EntityType;
            }
        }

        #endregion



    }
}
