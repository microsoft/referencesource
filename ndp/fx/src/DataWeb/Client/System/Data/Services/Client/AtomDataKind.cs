//---------------------------------------------------------------------
// <copyright file="AtomDataKind.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Provides an enumeration of kinds of ATOM data understood.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    /// <summary>
    /// Use this type to describe what kind of data is being processed.
    /// </summary>
    internal enum AtomDataKind
    {
        /// <summary>No data available.</summary>
        None = 0,

        /// <summary>Custom (non-ATOM) data.</summary>
        Custom,

        /// <summary>An ATOM entry.</summary>
        Entry,

        /// <summary>An ATOM feed.</summary>
        Feed,

        /// <summary>An ATOM feed count.</summary>
        FeedCount,

        /// <summary>An ATOM link element</summary>
        PagingLinks,

        /// <summary>All data already processed.</summary>
        Finished,
    }
}
