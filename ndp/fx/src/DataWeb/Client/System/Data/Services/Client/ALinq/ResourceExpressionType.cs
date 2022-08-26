//---------------------------------------------------------------------
// <copyright file="ResourceExpressionType.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Resource expression type enum
// </summary>
//
// @owner  aconrad
//---------------------------------------------------------------------
namespace System.Data.Services.Client
{
    /// <summary>Enum for resource expression types</summary>
    internal enum ResourceExpressionType
    {
        /// <summary>ResourceSet Expression</summary>
        RootResourceSet = 10000,

        /// <summary>Resource Navigation Expression</summary>
        ResourceNavigationProperty,

        /// <summary>Resource Navigation Expression to Singleton</summary>
        ResourceNavigationPropertySingleton,

        /// <summary>Take Query Option Expression</summary>
        TakeQueryOption,

        /// <summary>Skip Query Option Expression</summary>
        SkipQueryOption,

        /// <summary>OrderBy Query Option Expression</summary>
        OrderByQueryOption,

        /// <summary>Filter Query Option Expression</summary>
        FilterQueryOption,

        /// <summary>Reference to a bound component of the resource set path</summary>
        InputReference,

        /// <summary>Projection Query Option Expression</summary>
        ProjectionQueryOption
    }
}
