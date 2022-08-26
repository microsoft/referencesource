//---------------------------------------------------------------------
// <copyright file="AssociationTypeEmitter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner       Microsoft
// @backupOwner Microsoft
//---------------------------------------------------------------------

using System.CodeDom;
using System.Data.Metadata.Edm;
using System.Diagnostics;

namespace System.Data.EntityModel.Emitters
{
    /// <summary>
    /// Summary description for NestedTypeEmitter.
    /// </summary>
    internal sealed class AssociationTypeEmitter : SchemaTypeEmitter
    {
        public AssociationTypeEmitter(ClientApiGenerator generator, AssociationType associationType)
            : base(generator, associationType)
        {
        }


        public override CodeTypeDeclarationCollection EmitApiClass()
        {
            Debug.Assert((base.Item as AssociationType).AssociationEndMembers.Count == 2, "must have exactly two ends");

            // this method doesn't actually create a new type, just a new assembly level attribute for each end
            return new CodeTypeDeclarationCollection();
        }
    }
}
