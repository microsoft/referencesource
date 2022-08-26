//---------------------------------------------------------------------
// <copyright file="IDataServiceQueryProvider.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides the query interface definition for web data service
//      data sources.
// </summary>
//
// @owner  pratikp
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    /// <summary>
    /// Provides a metadata and query source abstraction for a 
    /// web data service's store.
    /// </summary>
    public interface IDataServiceQueryProvider
    {
        /// <summary>The data source from which data is provided.</summary>
        object CurrentDataSource
        {
            get;
            set;
        }

        /// <summary>Gets a value indicating whether null propagation is required in expression trees.</summary>
        bool IsNullPropagationRequired
        {
            get;
        }

        /// <summary>
        /// Returns the IQueryable that represents the resource set.
        /// </summary>
        /// <param name="resourceSet">resource set representing the entity set.</param>
        /// <returns>
        /// An IQueryable that represents the set; null if there is 
        /// no set for the specified name.
        /// </returns>
        IQueryable GetQueryRootForResourceSet(ResourceSet resourceSet);

        /// <summary>Gets the <see cref="ResourceType"/> for the specified <paramref name="target"/>.</summary>
        /// <param name="target">Target instance to extract a <see cref="ResourceType"/> from.</param>
        /// <returns>The <see cref="ResourceType"/> that describes this <paramref name="target"/> in this provider.</returns>
        ResourceType GetResourceType(object target);

        /// <summary>
        /// Get the value of the strongly typed property.
        /// </summary>
        /// <param name="target">instance of the type declaring the property.</param>
        /// <param name="resourceProperty">resource property describing the property.</param>
        /// <returns>value for the property.</returns>
        object GetPropertyValue(object target, ResourceProperty resourceProperty);

        /// <summary>
        /// Get the value of the open property.
        /// </summary>
        /// <param name="target">instance of the type declaring the open property.</param>
        /// <param name="propertyName">name of the open property.</param>
        /// <returns>value for the open property.</returns>
        object GetOpenPropertyValue(object target, string propertyName);

        /// <summary>
        /// Get the name and values of all the properties defined in the given instance of an open type.
        /// </summary>
        /// <param name="target">instance of a open type.</param>
        /// <returns>collection of name and values of all the open properties.</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1006:DoNotNestGenericTypesInMemberSignatures", Justification = "need to return a collection of key value pair")]
        IEnumerable<KeyValuePair<string, object>> GetOpenPropertyValues(object target);

        /// <summary>
        /// Invoke the given service operation and returns the results.
        /// </summary>
        /// <param name="serviceOperation">service operation to invoke.</param>
        /// <param name="parameters">value of parameters to pass to the service operation.</param>
        /// <returns>returns the result of the service operation. If the service operation returns void, then this should return null.</returns>
        object InvokeServiceOperation(ServiceOperation serviceOperation, object[] parameters);
    }
}
