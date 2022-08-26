// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  PipelineComponent
**
** Purpose: Base class representing parts of the add-in 
**     pipeline, from the host to the contract to the addin.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Reflection;
using System.Security;
using System.Security.Permissions;
using System.Text;
using System.Diagnostics;
using System.AddIn.MiniReflection;
using System.AddIn.Pipeline;

using System.Diagnostics.Contracts;
using TypeInfo=System.AddIn.MiniReflection.TypeInfo;

namespace System.AddIn
{
    [Serializable]
    internal enum PipelineComponentType
    {
        HostAdapter,
        Contract,
        AddInAdapter,
        AddInBase,
        AddIn
    }

    [Serializable]
    internal abstract class PipelineComponent
    {
        private TypeInfo _typeInfo;
        private String _location;         // fully qualified path, known after deserialization
        private String _relativeLocation; // relative to the add-in root or pipeline root.
        private bool _connectedToNeighbors;
        private bool _haveSetRootDirectory; // Whether the add-in root or pipeline root has been set.

        // Since these dictionaries are read-only after initialization and they are often empty,
        // a perf optimization is to have the empty ones be shared.
        private IDictionary<String, String> _qualificationData;

        internal static readonly IDictionary<String, String> s_emptyDictionary = 
            new ReadOnlyDictionary<String, String>(new Dictionary<String, String>());

        private static volatile Type s_IContractInReflectionLoaderContext;
        private static volatile Type s_ContractAttrInReflectionLoaderContext;
        private static volatile Type s_AddInAdapterAttrInReflectionLoaderContext;
        private static volatile Type s_AddInBaseAttrInReflectionLoaderContext;
        private static volatile Type s_AddInAttrInReflectionLoaderContext;
        private static volatile Type s_QualificationDataAttrInReflectionLoaderContext;


        internal PipelineComponent(TypeInfo typeInfo, String assemblyLocation)
        {
            if (typeInfo == null)
                throw new ArgumentNullException("typeInfo");
            if (assemblyLocation == null)
                throw new ArgumentNullException("assemblyLocation");
            System.Diagnostics.Contracts.Contract.EndContractBlock();
            
            _typeInfo = typeInfo;
            if (Path.IsPathRooted(assemblyLocation)) {
                // For FindAddIn case, we know the full location;
                _location = assemblyLocation;
                _haveSetRootDirectory = true;
            }
            else {
                _relativeLocation = assemblyLocation;
            }

            // load the qualification data, either from reflection or minireflection, as appropriate
            if (_typeInfo.HasReflectionType)
            {
                IList<CustomAttributeData> cas =CustomAttributeData.GetCustomAttributes(_typeInfo.ReflectionType); 
                Dictionary<String, String> dictionary = new Dictionary<String, String>();
                foreach (CustomAttributeData ca in cas)
                {
                    if (Object.ReferenceEquals(ca.Constructor.DeclaringType, s_QualificationDataAttrInReflectionLoaderContext))
                    {
                        IList<CustomAttributeTypedArgument> args = ca.ConstructorArguments;
                        String key = (String)args[0].Value;
                        String val = (String)args[1].Value;
                        dictionary[key] = val;
                    }
                }
                _qualificationData = dictionary.Count == 0 ? s_emptyDictionary : 
                    new ReadOnlyDictionary<String, String>(dictionary);
            }
            else
            {
                Type qualificationDataAttribute = typeof (QualificationDataAttribute);
                MiniCustomAttributeInfo[] cas = typeInfo.GetCustomAttributeInfos(qualificationDataAttribute);
                Dictionary<String, String> dictionary = new Dictionary<String, String>();
                if (cas != null && cas.Length > 0)
                {
                    foreach (MiniCustomAttributeInfo ca in cas)
                    {
                        MiniCustomAttributeFixedArgInfo[] fai = ca.FixedArgs;
                        String key = (String)fai[0].Value;
                        String val = (String)fai[1].Value;
                        dictionary[key] = val;
                    }
                }
                _qualificationData = dictionary.Count == 0 ? s_emptyDictionary : 
                    new ReadOnlyDictionary<String, String>(dictionary);
            }
        }

        internal IDictionary<String, String> QualificationData
        {
            get { return _qualificationData; }
        }

        protected static Type IContractInReflectionLoaderContext {
            get {
                System.Diagnostics.Contracts.Contract.Assert(s_IContractInReflectionLoaderContext != null);
                return s_IContractInReflectionLoaderContext;
            }
        }

        internal static Type ContractAttributeInReflectionLoaderContext {
            get {
                System.Diagnostics.Contracts.Contract.Assert(s_ContractAttrInReflectionLoaderContext != null);
                return s_ContractAttrInReflectionLoaderContext;
            }
        }

        internal static Type AddInAdapterAttributeInReflectionLoaderContext {
            get {
                System.Diagnostics.Contracts.Contract.Assert(s_AddInAdapterAttrInReflectionLoaderContext != null);
                return s_AddInAdapterAttrInReflectionLoaderContext;
            }
        }

        internal static Type AddInBaseAttributeInReflectionLoaderContext {
            get {
                System.Diagnostics.Contracts.Contract.Assert(s_AddInBaseAttrInReflectionLoaderContext != null);
                return s_AddInBaseAttrInReflectionLoaderContext;
            }
        }

        internal static Type AddInAttributeInReflectionLoaderContext {
            get {
                System.Diagnostics.Contracts.Contract.Assert(s_AddInAttrInReflectionLoaderContext != null);
                return s_AddInAttrInReflectionLoaderContext;
            }
        }

        internal static void SetTypesFromReflectionLoaderContext(System.Reflection.Assembly systemAddInInReflLoaderContext,
                                                                 System.Reflection.Assembly systemAddInContractsInReflLoaderContext)
        {
            System.Diagnostics.Contracts.Contract.Requires(systemAddInInReflLoaderContext != null);
            System.Diagnostics.Contracts.Contract.Requires(systemAddInInReflLoaderContext.ReflectionOnly);
            System.Diagnostics.Contracts.Contract.Requires(systemAddInContractsInReflLoaderContext != null);
            System.Diagnostics.Contracts.Contract.Requires(systemAddInContractsInReflLoaderContext.ReflectionOnly);

            s_IContractInReflectionLoaderContext = systemAddInContractsInReflLoaderContext.GetType(typeof(System.AddIn.Contract.IContract).FullName, true);
            s_ContractAttrInReflectionLoaderContext =systemAddInContractsInReflLoaderContext.GetType("System.AddIn.Pipeline.AddInContractAttribute", true);
            s_QualificationDataAttrInReflectionLoaderContext = systemAddInContractsInReflLoaderContext.GetType("System.AddIn.Pipeline.QualificationDataAttribute", true);
            s_AddInAdapterAttrInReflectionLoaderContext = systemAddInInReflLoaderContext.GetType("System.AddIn.Pipeline.AddInAdapterAttribute", true);
            s_AddInBaseAttrInReflectionLoaderContext = systemAddInInReflLoaderContext.GetType("System.AddIn.Pipeline.AddInBaseAttribute", true);
            s_AddInAttrInReflectionLoaderContext = systemAddInInReflLoaderContext.GetType("System.AddIn.AddInAttribute", true);
        }

        internal void SetRootDirectory(String root)
        {
            System.Diagnostics.Contracts.Contract.Requires(Path.IsPathRooted(root));
            // Note that the same pipeline component may show up in multiple pipelines,
            // and we can't do this operation twice.
            if (!_haveSetRootDirectory) {
                System.Diagnostics.Contracts.Contract.Assert(!Path.IsPathRooted(_relativeLocation));

                _location = Path.Combine(root, _relativeLocation);
                _haveSetRootDirectory = true;
            }
        }

        public String Location {
            get {
                System.Diagnostics.Contracts.Contract.Assert(_haveSetRootDirectory, "You must set this component's root directory before getting an absolute path.  Either call SetRootDirectory or use RelativeLocation instead.");
                return _location;
            }
        }

        internal String RelativeLocation {
            get {
                return _relativeLocation;
            }
        }

        protected String BestAvailableLocation {
            get {
                System.Diagnostics.Contracts.Contract.Ensures(System.Diagnostics.Contracts.Contract.Result<String>() != null);
                if (_haveSetRootDirectory)
                    return _location;
                return _relativeLocation;
            }
        }

        public String Name {
            get {
                System.Diagnostics.Contracts.Contract.Ensures(System.Diagnostics.Contracts.Contract.Result<String>() != null);
                return _typeInfo.Name;
            }
        }

        public TypeInfo TypeInfo {
            get { return _typeInfo; }
        }

        public String FullName {
            get {
                return _typeInfo.FullName;
            }
        }

        public bool ConnectedToNeighbors {
            get { return _connectedToNeighbors; }
            set { _connectedToNeighbors = value; }
        }

        // @






        internal virtual bool Validate(Type type, Collection<String> warnings)
        {
            return Validate(new TypeInfo(type), warnings);
        }

        // Finish the initialization, and ensure this type is a valid component
        // for use in the add-in model.  On failure, log a message & return false.
        // Derived classes should do their own validation and then call this one.
        internal virtual bool Validate(TypeInfo type, Collection<String> warnings)
        {
            if (type.IsGeneric)
            {
                warnings.Add(String.Format(System.Globalization.CultureInfo.CurrentCulture, Res.ComponentUnusableBecauseItIsGeneric, type.Name));
                return false;
            }
            return true;
        }

        // used in AddIn class and HostAdapter class
        // Since MiniReflection doesn't support generics, we do a "best effort" while generating this list.
        [System.Security.SecuritySafeCritical]
        internal static TypeInfo[] FindBaseTypesAndInterfaces(TypeInfo type)
        {
            TypeInfo currentTypeInfo = type;
            TypeInfo objectType = new TypeInfo(typeof(Object));
            List<TypeInfo> infos = new List<TypeInfo>();

            try
            {
                //we walk up the hierarchy as far as we can until we can't
                //get the types anymore...
                while (!currentTypeInfo.Equals(objectType))
                {
                    // To get the base type and interfaces, we will need to resolve a TypeRef to a TypeDef first
                    currentTypeInfo = currentTypeInfo.TryGetTypeDef();
                    if (currentTypeInfo != null)
                    {
                        TypeInfo[] interfaces = currentTypeInfo.GetInterfaces();
                        if (interfaces != null)
                            infos.AddRange(interfaces);

                        TypeInfo baseType = currentTypeInfo.BaseType;
                        infos.Add(baseType);
                        currentTypeInfo = baseType;
                    }
                    else
                    {
                        // we have reached the limit of what we can resolve in the addin/hostadapter's folder
                        break;
                    }
                }
            }
            catch (GenericsNotImplementedException) 
            {
                // Since GetInterfaces() ignores all generic interfaces, we'll only come here if we encounter a generic base class.
                // We'll ignore the generic base class and return whatever bases we found earlier.
            }

            return infos.ToArray();
        }

    }
}
