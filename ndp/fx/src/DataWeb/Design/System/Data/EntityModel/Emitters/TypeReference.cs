//---------------------------------------------------------------------
// <copyright file="TypeReference.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner       Microsoft
// @backupOwner Microsoft
//---------------------------------------------------------------------

using System.CodeDom;
using System.Collections.Generic;
using System.Data.Common.Utils;

namespace System.Data.EntityModel.Emitters
{
    /// <summary>
    /// Summary description for TypeReferences.
    /// </summary>
    internal class TypeReference
    {
        #region Fields
        internal static readonly Type ObjectContextBaseClassType = typeof(System.Data.Services.Client.DataServiceContext);

        public const string FQMetaDataWorkspaceTypeName = "System.Data.Metadata.Edm.MetadataWorkspace";

        private static CodeTypeReference _byteArray;
        private static CodeTypeReference _dateTime;
        private static CodeTypeReference _guid;
        private static CodeTypeReference _objectContext;

        private readonly Memoizer<Type, CodeTypeReference> _forTypeMemoizer;
        private readonly Memoizer<Type, CodeTypeReference> _nullableForTypeMemoizer;
        private readonly Memoizer<KeyValuePair<string, bool>, CodeTypeReference> _fromStringMemoizer;
        private readonly Memoizer<KeyValuePair<string, CodeTypeReference>, CodeTypeReference> _fromStringGenericMemoizer;
        #endregion

        #region Constructors
        internal TypeReference()
        {
            _forTypeMemoizer = new Memoizer<Type, CodeTypeReference>(ComputeForType, null);
            _fromStringMemoizer = new Memoizer<KeyValuePair<string, bool>, CodeTypeReference>(ComputeFromString, null);
            _nullableForTypeMemoizer = new Memoizer<Type, CodeTypeReference>(ComputeNullableForType, null);
            _fromStringGenericMemoizer = new Memoizer<KeyValuePair<string, CodeTypeReference>, CodeTypeReference>(ComputeFromStringGeneric, null);
        }
        #endregion

        #region Public Methods
        /// <summary>
        /// Get TypeReference for a type represented by a Type object
        /// </summary>
        /// <param name="type">the type object</param>
        /// <returns>the associated TypeReference object</returns>
        public CodeTypeReference ForType(Type type)
        {
            return _forTypeMemoizer.Evaluate(type);
        }

        private CodeTypeReference ComputeForType(Type type)
        {
            // we know that we can safely global:: qualify this because it was already 
            // compiled before we are emitting or else we wouldn't have a Type object
            CodeTypeReference value = new CodeTypeReference(type, CodeTypeReferenceOptions.GlobalReference);
            return value;
        }

        /// <summary>
        /// Get TypeReference for a type represented by a Generic Type object.
        /// We don't cache the TypeReference for generic type object since the type would be the same
        /// irresepective of the generic arguments. We could potentially cache it using both the type name
        /// and generic type arguments.
        /// </summary>
        /// <param name="type">the generic type object</param>
        /// <returns>the associated TypeReference object</returns>
        public CodeTypeReference ForType(Type generic, params CodeTypeReference[] argument)
        {
            // we know that we can safely global:: qualify this because it was already 
            // compiled before we are emitting or else we wouldn't have a Type object
            CodeTypeReference typeRef = new CodeTypeReference(generic, CodeTypeReferenceOptions.GlobalReference);
            if ((null != argument) && (0 < argument.Length))
            {
                typeRef.TypeArguments.AddRange(argument);
            }
            return typeRef;
        }


        /// <summary>
        /// Get TypeReference for a type represented by a namespace quailifed string 
        /// </summary>
        /// <param name="type">namespace qualified string</param>
        /// <returns>the TypeReference</returns>
        public CodeTypeReference FromString(string type)
        {
            return FromString(type, false);
        }

        /// <summary>
        /// Get TypeReference for a type represented by a namespace quailifed string,
        /// with optional global qualifier
        /// </summary>
        /// <param name="type">namespace qualified string</param>
        /// <param name="addGlobalQualifier">indicates whether the global qualifier should be added</param>
        /// <returns>the TypeReference</returns>
        public CodeTypeReference FromString(string type, bool addGlobalQualifier)
        {
            return _fromStringMemoizer.Evaluate(new KeyValuePair<string, bool>(type, addGlobalQualifier));
        }

        private CodeTypeReference ComputeFromString(KeyValuePair<string, bool> arguments)
        {
            string type = arguments.Key;
            bool addGlobalQualifier = arguments.Value;
            CodeTypeReference value;
            if (addGlobalQualifier)
            {
                value = new CodeTypeReference(type, CodeTypeReferenceOptions.GlobalReference);
            }
            else
            {
                value = new CodeTypeReference(type);
            }
            return value;
        }

        /// <summary>
        /// Get TypeReference for a framework type
        /// </summary>
        /// <param name="name">unqualified name of the framework type</param>
        /// <returns>the TypeReference</returns>
        public CodeTypeReference AdoFrameworkType(string name)
        {
            return FromString(Utils.WebFrameworkNamespace + "." + name, true);
        }

        /// <summary>
        /// Get TypeReference for a bound generic framework class
        /// </summary>
        /// <param name="name">the name of the generic framework class</param>
        /// <param name="typeParameter">the type parameter for the framework class</param>
        /// <returns>TypeReference for the bound framework class</returns>
        public CodeTypeReference AdoFrameworkGenericClass(string name, CodeTypeReference typeParameter)
        {
            return FrameworkGenericClass(Utils.WebFrameworkNamespace, name, typeParameter);
        }

        /// <summary>
        /// Get TypeReference for a bound generic framework class
        /// </summary>
        /// <param name="namespaceName">the namespace of the generic framework class</param>
        /// <param name="name">the name of the generic framework class</param>
        /// <param name="typeParameter">the type parameter for the framework class</param>
        /// <returns>TypeReference for the bound framework class</returns>
        public CodeTypeReference FrameworkGenericClass(string namespaceName, string name, CodeTypeReference typeParameter)
        {
            return FrameworkGenericClass(namespaceName + "." + name, typeParameter);
        }

        /// <summary>
        /// Get TypeReference for a bound generic framework class
        /// </summary>
        /// <param name="fullname">the fully qualified name of the generic framework class</param>
        /// <param name="typeParameter">the type parameter for the framework class</param>
        /// <returns>TypeReference for the bound framework class</returns>
        public CodeTypeReference FrameworkGenericClass(string fullname, CodeTypeReference typeParameter)
        {
            return _fromStringGenericMemoizer.Evaluate(new KeyValuePair<string, CodeTypeReference>(fullname, typeParameter));
        }

        private CodeTypeReference ComputeFromStringGeneric(KeyValuePair<string, CodeTypeReference> arguments)
        {
            string name = arguments.Key;
            CodeTypeReference typeParameter = arguments.Value;
            CodeTypeReference typeRef = ComputeFromString(new KeyValuePair<string, bool>(name, true));
            typeRef.TypeArguments.Add(typeParameter);
            return typeRef;
        }

        /// <summary>
        /// Get TypeReference for a bound Nullable&lt;T&gt;
        /// </summary>
        /// <param name="innerType">Type of the Nullable&lt;T&gt; type parameter</param>
        /// <returns>TypeReference for a bound Nullable&lt;T&gt;</returns>
        public CodeTypeReference NullableForType(Type innerType)
        {
            return _nullableForTypeMemoizer.Evaluate(innerType);
        }

        private CodeTypeReference ComputeNullableForType(Type innerType)
        {
            // can't use FromString because it will return the same Generic type reference
            // but it will already have a previous type parameter (because of caching)
            CodeTypeReference typeRef = new CodeTypeReference(typeof(System.Nullable<>), CodeTypeReferenceOptions.GlobalReference);
            typeRef.TypeArguments.Add(ForType(innerType));
            return typeRef;
        }

        #endregion

        #region Public Properties
        /// <summary>
        /// Gets a CodeTypeReference to the System.Byte[] type.
        /// </summary>
        /// <value></value>
        public CodeTypeReference ByteArray
        {
            get
            {
                if (_byteArray == null)
                    _byteArray = ForType(typeof(byte[]));

                return _byteArray;
            }
        }

        /// <summary>
        /// Gets a CodeTypeReference object for the System.DateTime type.
        /// </summary>
        public CodeTypeReference DateTime
        {
            get
            {
                if (_dateTime == null)
                    _dateTime = ForType(typeof(System.DateTime));
                return _dateTime;
            }
        }

        /// <summary>
        /// Gets a CodeTypeReference object for the System.Guid type.
        /// </summary>
        public CodeTypeReference Guid
        {
            get
            {
                if (_guid == null)
                    _guid = ForType(typeof(System.Guid));

                return _guid;
            }
        }

        /// <summary>
        /// TypeReference for the Framework's ObjectContext class
        /// </summary>
        public CodeTypeReference ObjectContext
        {
            get
            {
                if (_objectContext == null)
                {
                    _objectContext = AdoFrameworkType("DataServiceContext");
                }

                return _objectContext;
            }
        }

        #endregion
    }
}
