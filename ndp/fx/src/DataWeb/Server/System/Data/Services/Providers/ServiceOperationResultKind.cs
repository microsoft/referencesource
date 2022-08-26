//---------------------------------------------------------------------
// <copyright file="ServiceOperationResultKind.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides an enumeration that describes the kind of result
//      that a service operation provides.
// </summary>
//
// @owner  mruiz
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    /// <summary>
    /// Use this type to describe the kind of results returned by a service
    /// operation.
    /// </summary>
    public enum ServiceOperationResultKind
    {
        /// <summary>A single direct value which cannot be further composed.</summary>
        DirectValue,

        /// <summary>An enumeration of values which cannot be further composed.</summary>
        Enumeration,

        /// <summary>A queryable object which returns multiple elements.</summary>
        QueryWithMultipleResults,

        /// <summary>A queryable object which returns a single element.</summary>
        QueryWithSingleResult,

        /// <summary>No result return.</summary>
        Void,
    }
}
