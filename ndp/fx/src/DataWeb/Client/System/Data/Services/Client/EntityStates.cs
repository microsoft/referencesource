//---------------------------------------------------------------------
// <copyright file="EntityStates.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// <summary>
// describes the insert/update/delete state of a resource.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    /// <summary>
    /// Describes the insert/update/delete state of an entity or link.
    /// </summary>
    /// <remarks>
    /// Deleting an inserted resource will detach it.
    /// After SaveChanges, deleted resources will become detached and Added &amp; Modified resources will become unchanged.
    /// </remarks>
    [System.Flags()] 
    public enum EntityStates
    {
        /// <summary>
        /// The resource is not tracked by the context.
        /// </summary>
        Detached = 0x00000001,

        /// <summary>
        /// The resource is tracked by a context with no changes.
        /// </summary>
        Unchanged = 0x00000002,

        /// <summary>
        /// The resource is tracked by a context for insert.
        /// </summary>
        Added = 0x00000004,

        /// <summary>
        /// The resource is tracked by a context for deletion.
        /// </summary>
        Deleted = 0x00000008,

        /// <summary>
        /// The resource is tracked by a context for update.
        /// </summary>
        Modified = 0x00000010
    }

    /// <summary>
    /// Describes the insert/update state of a Media Resource stream.
    /// </summary>
    [System.Flags()] 
    internal enum StreamStates
    {
        /// <summary>
        /// Either the entity has no stream or the stream is unchanged
        /// </summary>
        NoStream = 0,

        /// <summary>
        /// The stream is tracked by a context for insert
        /// </summary>
        Added,

        /// <summary>
        /// The stream is tracked by a context for update
        /// </summary>
        Modified
    }
}
