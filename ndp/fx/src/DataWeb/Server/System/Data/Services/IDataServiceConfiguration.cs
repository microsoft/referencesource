//---------------------------------------------------------------------
// <copyright file="IDataServiceConfiguration.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides an interface for modifying the configuration of a
//      web data service.
// </summary>
//
// @owner  mruiz
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System;
    using System.Data.Services.Providers;

    /// <summary>
    /// Use this interface to modify the configuration of a web data service.
    /// </summary>
    public interface IDataServiceConfiguration
    {
        #region Properties.

        /// <summary>Maximum number of change sets and query operations in a batch.</summary>
        int MaxBatchCount
        {
            get;
            set;
        }

        /// <summary>Maximum number of changes in a change set.</summary>
        int MaxChangesetCount
        {
            get;
            set;
        }

        /// <summary>Maximum number of segments to be expanded allowed in a request.</summary>
        int MaxExpandCount
        {
            get;
            set;
        }

        /// <summary>Maximum number of segments in a single $expand path.</summary>
        int MaxExpandDepth
        {
            get;
            set;
        }

        /// <summary>Maximum number of elements in each returned collection (top-level or expanded).</summary>
        int MaxResultsPerCollection
        {
            get;
            set;
        }

        /// <summary>Maximum number of objects that can be referee in each returned collection (top-level or expanded).</summary>
        int MaxObjectCountOnInsert
        {
            get;
            set;
        }

        /// <summary>Gets or sets whether verbose errors should be used by default.</summary>
        /// <remarks>
        /// This property sets the default for the whole service; individual responses may behave differently
        /// depending on the value of the VerboseResponse property of the arguments to the HandleException
        /// method on the <see cref="DataService&lt;T&gt;"/> class.
        /// </remarks>
        bool UseVerboseErrors 
        { 
            get; 
            set;
        }

        #endregion Properties.

        #region Methods.

        /// <summary>Sets the access rights on the specified resource set.</summary>
        /// <param name="name">
        /// Name of resource set to set; '*' to indicate all 
        /// resource sets not otherwise specified.
        /// </param>
        /// <param name="rights">Rights to be granted to this resource.</param>
        void SetEntitySetAccessRule(string name, EntitySetRights rights);

        /// <summary>Sets the access rights on the specified service operation.</summary>
        /// <param name="name">
        /// Name of service operation to set; '*' to indicate all 
        /// service operations not otherwise specified.
        /// </param>
        /// <param name="rights">Rights to be granted to this operation.</param>
        void SetServiceOperationAccessRule(string name, ServiceOperationRights rights);

        /// <summary>
        /// Add the type to the list of known types for the data service
        /// </summary>
        /// <param name="type">interested type whose metadata needs to be considered</param>
        void RegisterKnownType(Type type);

        #endregion Methods.
    }
}
