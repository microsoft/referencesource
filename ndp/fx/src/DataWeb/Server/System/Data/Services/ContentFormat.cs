//---------------------------------------------------------------------
// <copyright file="ContentFormat.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      List of content formats that data web service supports
// </summary>
//
// @owner  pratikp
//---------------------------------------------------------------------

namespace System.Data.Services
{
    /// <summary>This enumeration provides values for the content format.</summary>
    internal enum ContentFormat
    {
        /// <summary>A binary format with no additional modifications.</summary>
        Binary,

        /// <summary>The application/json format.</summary>
        Json,

        /// <summary>A text-based format with no additional markup.</summary>
        Text,

        /// <summary>The application/atom+xml format.</summary>
        Atom,

        /// <summary>An XML document for CSDL.</summary>
        MetadataDocument,

        /// <summary>An XML document for primitive and complex types.</summary>
        PlainXml,

        /// <summary>An as-yet-undetermined format.</summary>
        Unknown,

        /// <summary>An unsupported format.</summary>
        Unsupported
    }
}
