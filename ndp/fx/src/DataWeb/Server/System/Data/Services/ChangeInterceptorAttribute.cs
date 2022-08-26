//---------------------------------------------------------------------
// <copyright file="ChangeInterceptorAttribute.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a class to decorate change callback methods.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System;
    using System.Diagnostics;

    /// <summary>
    /// Use this attribute on a DataService method to indicate that
    /// this method should be invoked with data changes.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = true, Inherited = true)]
    public sealed class ChangeInterceptorAttribute : Attribute
    {
        /// <summary>Container name that the method filters.</summary>
        private readonly string entitySetName;

        /// <summary>Declares a new <see cref="ChangeInterceptorAttribute"/> instance.</summary>
        /// <param name="entitySetName">Name of entity set that the method intercepts changes to.</param>
        public ChangeInterceptorAttribute(string entitySetName)
        {
            if (entitySetName == null)
            {
                throw Error.ArgumentNull("entitySetName");
            }

            this.entitySetName = entitySetName;
        }

        /// <summary>Entity set name that the method intercepts changes to.</summary>
        public string EntitySetName
        {
            [DebuggerStepThrough]
            get { return this.entitySetName; }
        }
    }
}
