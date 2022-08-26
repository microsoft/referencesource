//---------------------------------------------------------------------
// <copyright file="ResourcePropertyKind.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides an enumeration for the kinds of properties that a
//      resource can have.
// </summary>
//
// @owner  mruiz
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    using System;
    
    /// <summary>
    /// Enumeration for the kinds of property a resource can have.
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1714", Justification = "Avoid unnecesarily changing existing code")]
    [Flags]
    public enum ResourcePropertyKind
    {
        /// <summary>A primitive type property.</summary>
        Primitive = 1,

        /// <summary>A property that is part of the key.</summary>
        Key = 2,

        /// <summary>A complex (compound) property.</summary>
        ComplexType = 4,

        /// <summary>A reference to another resource.</summary>
        ResourceReference = 8,

        /// <summary>A reference to a resource set.</summary>
        ResourceSetReference = 16,

        /// <summary>Whether this property is a etag property.</summary>
        ETag = 32,
    }
}
