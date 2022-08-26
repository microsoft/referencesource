//---------------------------------------------------------------------
// <copyright file="DataServiceEntityAttribute.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Clr Attribute mark a class as entity
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------
namespace System.Data.Services.Common
{
    using System;
    using System.Collections.ObjectModel;
    using System.Data.Services.Client;
    using System.Linq;

    /// <summary>
    /// Attribute to mark a class as entity
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false)]
    public sealed class DataServiceEntityAttribute : System.Attribute
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public DataServiceEntityAttribute()
        {
        }
    }
}
