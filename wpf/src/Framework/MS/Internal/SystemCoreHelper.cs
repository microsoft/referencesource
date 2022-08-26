//---------------------------------------------------------------------------
//
// <copyright file="SystemCoreHelper.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: Helper methods for code that uses types from System.Core.
//
//---------------------------------------------------------------------------

using System;

namespace MS.Internal
{
    internal static class SystemCoreHelper
    {
        // return true if the item implements IDynamicMetaObjectProvider
        internal static bool IsIDynamicMetaObjectProvider(object item)
        {
            SystemCoreExtensionMethods extensions = AssemblyHelper.ExtensionsForSystemCore();
            return (extensions != null) ? extensions.IsIDynamicMetaObjectProvider(item) : false;
        }

        // return a new DynamicPropertyAccessor
        internal static object NewDynamicPropertyAccessor(Type ownerType, string propertyName)
        {
            SystemCoreExtensionMethods extensions = AssemblyHelper.ExtensionsForSystemCore();
            return (extensions != null) ? extensions.NewDynamicPropertyAccessor(ownerType, propertyName) : null;
        }

        // return a DynamicIndexerAccessor with the given number of arguments
        internal static object GetIndexerAccessor(int rank)
        {
            SystemCoreExtensionMethods extensions = AssemblyHelper.ExtensionsForSystemCore();
            return (extensions != null) ? extensions.GetIndexerAccessor(rank) : null;
        }
    }
}
