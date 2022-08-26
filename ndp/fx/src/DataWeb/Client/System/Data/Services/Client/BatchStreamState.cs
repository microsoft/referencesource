//---------------------------------------------------------------------
// <copyright file="BatchStreamState.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// states of the batch stream
// </summary>
//---------------------------------------------------------------------

#if ASTORIA_CLIENT
namespace System.Data.Services.Client
#else
namespace System.Data.Services
#endif
{
    /// <summary>
    /// states of the batch stream
    /// </summary>
    internal enum BatchStreamState
    {
        /// <summary>EndBatch</summary>
        EndBatch = 0,

        /// <summary>StartBatch</summary>
        StartBatch = 1,

        /// <summary>BeginChangeSet</summary>
        BeginChangeSet = 2,

        /// <summary>EndChangeSet</summary>
        EndChangeSet = 3,

        /// <summary>request Post</summary>
        Post = 4,

        /// <summary>request Put</summary>
        Put = 5,

        /// <summary>request Delete</summary>
        Delete = 6,

        /// <summary>request Get</summary>
        Get = 7,

        /// <summary>request Merge</summary>
        Merge = 8,

        /// <summary>response to a Get</summary>
        GetResponse = 9,

        /// <summary>response to a Post/Put/Merge/Delete</summary>
        ChangeResponse = 10,
    }
}
