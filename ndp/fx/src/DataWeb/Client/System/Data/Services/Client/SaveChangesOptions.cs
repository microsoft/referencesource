//---------------------------------------------------------------------
// <copyright file="SaveChangesOptions.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// options during save changes
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    /// <summary>
    /// options when saving changes
    /// </summary>
    [Flags]
    public enum SaveChangesOptions
    {
        /// <summary>default option, using multiple requests to the server stopping on the first failure</summary>
        None = 0,

        /// <summary>save the changes in a single batch request</summary>
        Batch = 1,

        /// <summary>save all the changes using multiple requests</summary>
        ContinueOnError = 2,

        /// <summary>Use replace semantics when doing update.</summary>
        ReplaceOnUpdate = 4,
    }
}
