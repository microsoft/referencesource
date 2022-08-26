//---------------------------------------------------------------------
// <copyright file="MergeOption.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Used to specify a value synchronization strategy. 
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    /// <summary>
    /// Used to specify a value synchronization strategy. 
    /// </summary>
    /// <remarks>
    /// Equivalent to System.Data.dll!System.Data.LoadOption
    /// Equivalent to System.Data.Linq.dll!System.Data.Linq.RefreshMode
    /// Equivalent to System.Data.Entity.dll!System.Data.Objects.MergeOption
    /// </remarks>
    public enum MergeOption
    {
        /// <summary>
        /// No current values are modified.
        /// </summary>
        /// <remarks>
        /// Equivalent to System.Data.Objects.MergeOption.AppendOnly
        /// Equivalent to System.Data.Linq.RefreshMode.KeepCurrentValues
        /// </remarks>
        AppendOnly = 0,

        /// <summary>
        /// All current values are overwritten with current store values,
        /// regardless of whether they have been changed.
        /// </summary>
        /// <remarks>
        /// Equivalent to System.Data.LoadOption.OverwriteChanges
        /// Equivalent to System.Data.Objects.MergeOption.OverwriteChanges
        /// Equivalent to System.Data.Linq.RefreshMode.OverwriteCurrentValues
        /// </remarks>
        OverwriteChanges = 1,

        /// <summary>
        /// Current values that have been changed are not modified, but
        /// any unchanged values are updated with the current store
        /// values.  No changes are lost in this merge.
        /// </summary>
        /// <remarks>
        /// Equivalent to System.Data.LoadOption.PreserveChanges
        /// Equivalent to System.Data.Objects.MergeOption.PreserveChanges
        /// Equivalent to System.Data.Linq.RefreshMode.KeepChanges
        /// </remarks>
        PreserveChanges = 2,

        /// <summary>
        /// Equivalent to System.Data.Objects.MergeOption.NoTracking
        /// </summary>
        NoTracking = 3,
    }
}
