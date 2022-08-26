//---------------------------------------------------------------------
// <copyright file="ResourceTypeKind.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides an enumeration for the kind of resource types that
//      can be present
// </summary>
//
// @owner  pratikp
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    /// <summary>
    /// Enumeration for the kind of resource types
    /// </summary>
    public enum ResourceTypeKind
    {
        /// <summary> Resource type with keys </summary>
        EntityType,

        /// <summary> Resource type without keys </summary>
        ComplexType,

        /// <summary>A resource type without keys and with no properties.</summary>
        Primitive,
    }
}
