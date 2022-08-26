//---------------------------------------------------------------------
// <copyright file="IProjectedResult.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides the interface that supports getting projected values
//      for associated $select segments.
// </summary>
//
// @owner  vitkaras
//---------------------------------------------------------------------

namespace System.Data.Services
{
    /// <summary>
    /// This interface declares the methods required to support getting values
    /// from projected results for $select queries
    /// </summary>
    internal interface IProjectedResult
    {
        /// <summary>The full name of the <see cref="System.Data.Services.Providers.ResourceType"/> which represents the type
        /// of this result.</summary>
        string ResourceTypeName 
        { 
            get; 
        }

        /// <summary>Gets the value for named property for the result.</summary>
        /// <param name="propertyName">Name of property for which to get the value.</param>
        /// <returns>The value for the named property of the result.</returns>
        object GetProjectedPropertyValue(string propertyName);
    }
}
