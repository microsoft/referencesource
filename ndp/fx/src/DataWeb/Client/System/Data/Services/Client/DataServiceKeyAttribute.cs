//---------------------------------------------------------------------
// <copyright file="DataServiceKeyAttribute.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Clr Attribute to be annotated on key properties
// </summary>
//
// @owner  Microsoft, Microsoft
//---------------------------------------------------------------------
namespace System.Data.Services.Common
{
    using System;
    using System.Collections.ObjectModel;
    using System.Data.Services.Client;
    using System.Linq;

    /// <summary>
    /// Attribute to be annotated on key properties
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1019:DefineAccessorsForAttributeArguments", Justification = "Accessors are available for processed input.")]
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false)]
    public sealed class DataServiceKeyAttribute : System.Attribute
    {
        /// <summary>Name of the properties that form the key.</summary>
        private readonly ReadOnlyCollection<string> keyNames;

        /// <summary>
        /// Initializes a new instance of DataServiceKey attribute with the property name
        /// that forms the Key.
        /// </summary>
        /// <param name='keyName'>Name of the property that form the key for the current type.</param>
        public DataServiceKeyAttribute(string keyName)
        {
            Util.CheckArgumentNull(keyName, "keyName");
            Util.CheckArgumentNotEmpty(keyName, "KeyName");
            this.keyNames = new ReadOnlyCollection<string>(new string[1] { keyName });
        }

        /// <summary>
        /// Initializes a new instance of DataServiceKey attribute with the list of property names
        /// that form the key.
        /// </summary>
        /// <param name='keyNames'>Name of the properties that form the key for the current type.</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1062:Validate arguments of public methods", MessageId = "0", Justification = "parameters are validated against null via CheckArgumentNull")]
        public DataServiceKeyAttribute(params string[] keyNames)
        {
            Util.CheckArgumentNull(keyNames, "keyNames");
            if (keyNames.Length == 0 || keyNames.Any(f => f == null || f.Length == 0))
            {
                throw Error.Argument(Strings.DSKAttribute_MustSpecifyAtleastOnePropertyName, "keyNames");
            }

            this.keyNames = new ReadOnlyCollection<string>(keyNames);
        }

        /// <summary>Name of the properties that form the key for the current type.</summary>
        public ReadOnlyCollection<string> KeyNames
        {
            get
            {
                return this.keyNames;
            }
        }
    }
}
