//---------------------------------------------------------------------
// <copyright file="ResourceTypeKeyKind.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides an enumeration for the kind of key based on the heuristic
// </summary>
//
// @owner  pratikp
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    /// <summary>
    /// Enumeration for the kind of resource key kind
    /// </summary>
    internal enum ResourceKeyKind
    {
        /// <summary> if the key property was attributed </summary>
        AttributedKey,

        /// <summary> If the key property name was equal to TypeName+ID </summary>
        TypeNameId,

        /// <summary> If the key property name was equal to ID </summary>
        Id,
    }
}
