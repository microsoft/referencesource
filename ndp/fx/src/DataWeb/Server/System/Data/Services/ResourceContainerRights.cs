//---------------------------------------------------------------------
// <copyright file="ResourceContainerRights.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides an enumeration to describe the rights granted on a
//      resource set.
// </summary>
//
// @owner  mruiz
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System;

    /// <summary>
    /// Provides values to describe the kind of thing targetted by a 
    /// client request.
    /// </summary>
    [Flags]
    public enum EntitySetRights
    {
        /// <summary>Specifies no rights on this resource.</summary>
        None = 0,

        /// <summary>Specifies the right to read one resource per request.</summary>
        ReadSingle = 1,

        /// <summary>Specifies the right to read multiple resources per request.</summary>
        ReadMultiple = 2,

        /// <summary>Specifies the right to append new resources to the container.</summary>
        WriteAppend = 4,

        /// <summary>Specifies the right to update existing resource in the container.</summary>
        WriteReplace = 8,

        /// <summary>Specifies the right to delete existing resource in the container.</summary>
        WriteDelete = 16,

        /// <summary>Specifies the right to update existing resource in the container.</summary>
        WriteMerge = 32,

        /// <summary>Specifies the right to read single or multiple resources in a single request.</summary>
        AllRead = ReadSingle | ReadMultiple,

        /// <summary>Specifies the right to append, delete or update resources in the container.</summary>
        AllWrite = WriteAppend | WriteDelete | WriteReplace | WriteMerge,

        /// <summary>Specifies all rights to the container.</summary>
        All = AllRead | AllWrite,
    }
}
