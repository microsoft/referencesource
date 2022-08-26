//---------------------------------------------------------------------
// <copyright file="OpenObjectAttribute.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Attribute to be annotated on class to designate the name of the instance property to store name-value pairs.
// </summary>
//---------------------------------------------------------------------

#if ASTORIA_OPEN_OBJECT
namespace System.Data.Services.Client
{
    using System;

    /// <summary>
    /// Attribute to be annotated on class to designate the name of the instance property to store name-value pairs.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = true)]
    public sealed class OpenObjectAttribute : System.Attribute
    {
        /// <summary>
        /// The name of the instance property returning an IDictionary&lt;string,object&gt;.
        /// </summary>
        private readonly string openObjectPropertyName;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="openObjectPropertyName">The name of the instance property returning an IDictionary&lt;string,object&gt;.</param>
        public OpenObjectAttribute(string openObjectPropertyName)
        {
            Util.CheckArgumentNotEmpty(openObjectPropertyName, "openObjectPropertyName");
            this.openObjectPropertyName = openObjectPropertyName;
        }

        /// <summary>
        /// The name of the instance property returning an IDictionary&lt;string,object&gt;.
        /// </summary>
        public string OpenObjectPropertyName
        {
            get { return this.openObjectPropertyName; }
        }
    }
}
#endif