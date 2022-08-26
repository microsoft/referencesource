//---------------------------------------------------------------------
// <copyright file="SingleResultAttribute.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a class to decorate custom service operations with
//      a single-result attribute.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System;
    using System.Diagnostics;
    using System.Reflection;

    /// <summary>
    /// Use this attribute on a DataService service operation method 
    /// to indicate than the IQueryable returned should contain a single element.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = true)]
    public sealed class SingleResultAttribute : Attribute
    {
        /// <summary>Declares a new <see cref="SingleResultAttribute"/> instance.</summary>
        public SingleResultAttribute()
        {
        }

        /// <summary>Checks whether the specified method has a SingleResultAttribute declared on it.</summary>
        /// <param name="method">Method to check.</param>
        /// <returns>
        /// true if the specified method (in its declared type or in an 
        /// ancestor declaring the type) has the SingleResultAttribute set.
        /// </returns>
        internal static bool MethodHasSingleResult(MethodInfo method)
        {
            Debug.Assert(method != null, "method != null");
            return method.GetCustomAttributes(typeof(SingleResultAttribute), true).Length > 0;
        }
    }
}
