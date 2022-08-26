//---------------------------------------------------------------------
// <copyright file="MimeTypePropertyAttribute.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Attribute to denote entity types describing a media entry
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;

    /// <summary>
    /// This attribute indicates another property in the same type that
    /// contains the MIME type that should be used for the data contained
    /// in the property this attribute is applied to.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = true)]
    public sealed class MimeTypePropertyAttribute : Attribute
    {
        /// <summary>The name of the property that contains the data</summary>
        private readonly string dataPropertyName;

        /// <summary>The name of the property that contains the mime type</summary>
        private readonly string mimeTypePropertyName;

        /// <summary>
        /// Creates a new instance of this attribute pointing to a particular 
        /// property to be used for the MIME type
        /// </summary>
        /// <param name="dataPropertyName">Name of the property holding the data</param>
        /// <param name="mimeTypePropertyName">Name of the property holding the MIME type</param>
        public MimeTypePropertyAttribute(string dataPropertyName, string mimeTypePropertyName)
        {
            this.dataPropertyName = dataPropertyName;
            this.mimeTypePropertyName = mimeTypePropertyName;
        }

        /// <summary>The name of the property that contains the data.</summary>
        public string DataPropertyName
        {
            get { return this.dataPropertyName; }
        }

        /// <summary>The name of the property that contains the mime type</summary>
        public string MimeTypePropertyName
        {
            get { return this.mimeTypePropertyName; }
        }
    }
}
