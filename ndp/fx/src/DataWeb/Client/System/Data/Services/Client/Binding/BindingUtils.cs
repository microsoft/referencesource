//---------------------------------------------------------------------
// <copyright file="BindingUtils.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//   Utilities for binding related operations
// </summary>
//
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
#region Namespaces
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Reflection;
    using System.Collections;
#endregion

    /// <summary>Utilities for binding related operations</summary>
    internal static class BindingUtils
    {
        /// <summary>
        /// Throw if the entity set name is null or empty
        /// </summary>
        /// <param name="entitySetName">entity set name.</param>
        /// <param name="entity">entity instance for which the entity set name is generated.</param>
        internal static void ValidateEntitySetName(string entitySetName, object entity)
        {
            if (String.IsNullOrEmpty(entitySetName))
            {
                throw new InvalidOperationException(Strings.DataBinding_Util_UnknownEntitySetName(entity.GetType().FullName));
            }
        }
        
        /// <summary>
        /// Given a collection type, gets it's entity type
        /// </summary>
        /// <param name="collectionType">Input collection type</param>
        /// <returns>Generic type argument for the collection</returns>
        internal static Type GetCollectionEntityType(Type collectionType)
        {
            while (collectionType != null)
            {
                if (collectionType.IsGenericType && WebUtil.IsDataServiceCollectionType(collectionType.GetGenericTypeDefinition()))
                {
                    return collectionType.GetGenericArguments()[0];
                }

                collectionType = collectionType.BaseType;
            }

            return null;
        }

        /// <summary>Verifies the absence of observer for an DataServiceCollection</summary>
        /// <typeparam name="T">Type of DataServiceCollection</typeparam>
        /// <param name="oec">Non-typed collection object</param>
        /// <param name="sourceProperty">Collection property of the source object which is being assigned to</param>
        /// <param name="sourceType">Type of the source object</param>
        internal static void VerifyObserverNotPresent<T>(object oec, string sourceProperty, Type sourceType)
        {
            Debug.Assert(BindingEntityInfo.IsDataServiceCollection(oec.GetType()), "Must be an DataServiceCollection.");
            
            DataServiceCollection<T> typedCollection = oec as DataServiceCollection<T>;
            
            if (typedCollection.Observer != null)
            {
                throw new InvalidOperationException(Strings.DataBinding_CollectionPropertySetterValueHasObserver(sourceProperty, sourceType));
            }
        }
    }
}
