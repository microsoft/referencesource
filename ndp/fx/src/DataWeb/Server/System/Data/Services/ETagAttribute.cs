//---------------------------------------------------------------------
// <copyright file="ETagAttribute.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      CLR attribute to be annotated on types which indicate the list of properties
//      form the ETag.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;

    /// <summary>Attribute to be annotated on types with ETags.</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1019:DefineAccessorsForAttributeArguments", Justification = "Processed value is available")]
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = true)]
    public sealed class ETagAttribute : System.Attribute
    {
        /// <summary>Name of the properties that form the ETag.</summary>
        private readonly ReadOnlyCollection<string> propertyNames;

        // This constructor was added since string[] is not a CLS-compliant type and
        // compiler gives a warning as error saying this attribute doesn't have any
        // constructor that takes CLS-compliant type

        /// <summary>
        /// Initializes a new instance of ETag attribute with the property name
        /// that forms the ETag.
        /// </summary>
        /// <param name='propertyName'>Name of the property that form the ETag for the current type.</param>
        public ETagAttribute(string propertyName)
        {
            WebUtil.CheckArgumentNull(propertyName, "propertyName");
            this.propertyNames = new ReadOnlyCollection<string>(new List<string>(new string[1] { propertyName }));
        }

        /// <summary>
        /// Initializes a new instance of ETag attribute with the list of property names
        /// that form the ETag.
        /// </summary>
        /// <param name='propertyNames'>Name of the properties that form the ETag for the current type.</param>
        public ETagAttribute(params string[] propertyNames)
        {
            WebUtil.CheckArgumentNull(propertyNames, "propertyNames");
            if (propertyNames.Length == 0)
            {
                throw new ArgumentException(Strings.ETagAttribute_MustSpecifyAtleastOnePropertyName, "propertyNames");
            }

            this.propertyNames = new ReadOnlyCollection<string>(new List<string>(propertyNames));
        }

        /// <summary>Name of the properties that form the ETag for the current type.</summary>
        public ReadOnlyCollection<string> PropertyNames
        {
            get
            {
                return this.propertyNames;
            }
        }
    }
}
