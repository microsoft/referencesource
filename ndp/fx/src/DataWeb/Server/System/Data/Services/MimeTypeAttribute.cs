//---------------------------------------------------------------------
// <copyright file="MimeTypeAttribute.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a class to decorate properties and custom service
//      operations with a MIME type.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System;
    using System.Diagnostics;
    using System.Linq;
    using System.Reflection;

    /// <summary>
    /// Use this attribute on a DataService service operation method
    /// or a data object property to indicate than the type returned is 
    /// of a specific MIME type.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = true)]
    public sealed class MimeTypeAttribute : Attribute
    {
        /// <summary>Name of the attributed method or property.</summary>
        private readonly string memberName;

        /// <summary>MIME type for the attributed method or property.</summary>
        private readonly string mimeType;

        /// <summary>
        /// Initializes a new <see cref="MimeTypeAttribute"/> instance with
        /// the specified MIME type.
        /// </summary>
        /// <param name="memberName">Name of the attributed method or property.</param>
        /// <param name="mimeType">MIME type for the attributed method or property.</param>
        public MimeTypeAttribute(string memberName, string mimeType)
        {
            this.memberName = memberName;
            this.mimeType = mimeType;
        }

        /// <summary>Name of the attributed method or property.</summary>
        public string MemberName
        {
            get { return this.memberName; }
        }

        /// <summary>
        /// MIME type for the attributed method or property.
        /// </summary>
        public string MimeType
        {
            get { return this.mimeType; }
        }

        /// <summary>
        /// Gets the MIME type declared on the specified <paramref name="member"/>.
        /// </summary>
        /// <param name="member">Member to check.</param>
        /// <returns>
        /// The MIME type declared on the specified <paramref name="member"/>; null
        /// if no attribute is declared.
        /// </returns>
        internal static MimeTypeAttribute GetMimeTypeAttribute(MemberInfo member)
        {
            Debug.Assert(member != null, "member != null");

            return member.ReflectedType.GetCustomAttributes(typeof(MimeTypeAttribute), true)
                .Cast<MimeTypeAttribute>()
                .FirstOrDefault(o => o.MemberName == member.Name);
        }
    }
}
