//---------------------------------------------------------------------
// <copyright file="DataServiceSaveChangesEventArgs.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//   DataServiceSaveChangesEventArgs class
// </summary>
//
//---------------------------------------------------------------------
namespace System.Data.Services.Client
{
    /// <summary>
    /// Encapsulates the arguments for the DataServiceContext ChangesSaved event
    /// </summary>
    internal class SaveChangesEventArgs : EventArgs
    {
        /// <summary>
        /// DataServiceContext SaveChanges response
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1823", Justification = "No upstream callers.")]
        private DataServiceResponse response;

        /// <summary>
        /// Construct a DataServiceSaveChangesEventArgs object.
        /// </summary>
        /// <param name="response">DataServiceContext SaveChanges response</param>
        public SaveChangesEventArgs(DataServiceResponse response)
        {
            this.response = response;
        }
    }  
}
