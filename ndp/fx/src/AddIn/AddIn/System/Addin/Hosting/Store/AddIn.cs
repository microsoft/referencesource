// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  AddIn
**
** Purpose: Represents an add-in on disk
**
===========================================================*/
using System;
using System.Collections.ObjectModel;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Text;
using System.Threading;
using System.AddIn.MiniReflection;
using System.Diagnostics.Contracts;
using TypeInfo=System.AddIn.MiniReflection.TypeInfo;
namespace System.AddIn
{
    [Serializable]
    internal sealed class AddIn : PipelineComponent
    {
        private TypeInfo[] _potentialAddinBases;
        private String _version;
        private String _assemblyName;

        private ResourceState _unlocalized;  // hard-coded strings

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields", Justification="May be used for localization later")]
        [NonSerialized]
        private String _fullPathToAddIn;

        // For localization
#if LOCALIZABLE_ADDIN_ATTRIBUTE
        private String _resMgrBaseName;
        private String _nameResource;
        private String _publisherResource;
        private String _descriptionResource;
        private ResourceState _localizedResources;
#endif

        internal AddIn(TypeInfo typeInfo, String assemblyLocation, String fullPathToAddin, String assemblyName)
            : base(typeInfo, assemblyLocation)
        {
            System.Diagnostics.Contracts.Contract.Requires(Path.IsPathRooted(fullPathToAddin));

            _fullPathToAddIn = fullPathToAddin;
            _assemblyName = assemblyName;

            _unlocalized = new ResourceState();
        }

        // expose this internally so we can default the name in the case of FindAddIn
        public ResourceState UnlocalizedResourceState
        {
            get { return _unlocalized; }
        }

        public String AddInName {
            get {
#if LOCALIZABLE_ADDIN_ATTRIBUTE
                ResourceState resState = GetLocalizedResources();
                if (resState.Name == null)
                    return _unlocalized.Name;
                else
                    return resState.Name;
#else
                return _unlocalized.Name;
#endif
            }
        }

        public String Publisher {
            get {
#if LOCALIZABLE_ADDIN_ATTRIBUTE
                ResourceState resState = GetLocalizedResources();
                if (resState.Publisher == null)
                    return _unlocalized.Publisher;
                else
                    return resState.Publisher;
#else
                return _unlocalized.Publisher;
#endif
            }
        }

        public String Description {
            get { 
#if LOCALIZABLE_ADDIN_ATTRIBUTE
                ResourceState resState = GetLocalizedResources();
                if (resState.Description == null)
                    return _unlocalized.Description;
                else
                    return resState.Description;
#else
                return _unlocalized.Description;
#endif
            }
        }

        public AssemblyName AssemblyName {
            get {
                return new AssemblyName(_assemblyName);
            }
        }


#if LOCALIZABLE_ADDIN_ATTRIBUTE
        internal ResourceState GetLocalizedResources()
        {
            if (!ContainsLocalizableStrings)
                return _unlocalized;
            ResourceState resState = _localizedResources;
            if (Thread.CurrentThread.CurrentUICulture.Name == resState.CultureName)
                return resState;
            // Cache this set of resources, in case someone asks for another from
            // the same culture.
            _localizedResources = ResourceProvider.LookupResourcesInNewDomain(Location, 
                _resMgrBaseName, _nameResource, _publisherResource, _descriptionResource);
            return _localizedResources;
        }

        internal bool ContainsLocalizableStrings {
            get {
                return _resMgrBaseName != null && 
                    (_nameResource != null || _publisherResource != null || _descriptionResource != null);
            }
        }
#endif // LOCALIZABLE_ADDIN_ATTRIBUTE

        public String Version {
            get { return _version; }
        }

        internal TypeInfo[] AddInBaseTypeInfo {
            get { return _potentialAddinBases; }
        }

        public override string ToString()
        {
            return String.Format(CultureInfo.CurrentCulture, Res.AddInToString, Name, BestAvailableLocation);
        }

        //Validate the addin. Also fill in the base class and interfaces.
        internal override bool Validate(TypeInfo type, Collection<String> warnings)
        {
            Type addInAttributeType = typeof(AddInAttribute);
            // Get the AddInAttribute, if available.  It is not required in the FindAddIn case.
            MiniCustomAttributeInfo[] attributes = type.GetCustomAttributeInfos(addInAttributeType);
            if (attributes.Length > 0)
            {
                MiniCustomAttributeInfo addInAttribute = attributes[0];

                _unlocalized.Name = (String)addInAttribute.FixedArgs[0].Value;
                foreach (MiniCustomAttributeNamedArgInfo namedArg in addInAttribute.NamedArgs)
                {
                    switch (namedArg.Name)
                    {
                        case "Description":
                            _unlocalized.Description = (String)namedArg.Value;
                        break;
                        case "Version":
                            _version = (String)namedArg.Value;
                        break;
                        case "Publisher":
                            _unlocalized.Publisher = (String)namedArg.Value;
                        break;
                        default:
                            warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.UnknownNamedAddInAttributeParameter, namedArg.Name, type.FullName, type.Assembly.ModuleName));
                        break;
                    }
                }
            }
            
            if (String.IsNullOrEmpty(_unlocalized.Name))
            {
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.AddInMustSpecifyName, type.FullName, type.Assembly.ModuleName));
                return false;
            }

            /* 
            // Parse and validate the custom attribute on this type.
            foreach (CustomAttributeData attr in CustomAttributeData.GetCustomAttributes(type)) {
                if (attr.Constructor.DeclaringType == PipelineComponent.AddInAttributeInReflectionLoaderContext) {
                    if (attr.ConstructorArguments.Count == 1) {
                        _unlocalized.Name = (String)attr.ConstructorArguments[0].Value;
                        if (String.IsNullOrEmpty(_unlocalized.Name)) {
                            warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.AddInMustSpecifyName, type.FullName, type.Assembly.Location));
                            return false;
                        }
                    }
#if LOCALIZABLE_ADDIN_ATTRIBUTE
                    else if (attr.ConstructorArguments.Count == 2) {
                        _resMgrBaseName = (String)attr.ConstructorArguments[0].Value;
                        _nameResource = (String)attr.ConstructorArguments[1].Value;
                        if (String.IsNullOrEmpty(_resMgrBaseName)) {
                            warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.MustSpecifyResMgrBaseName, type.FullName, type.Assembly.Location));
                            return false;
                        }
                        if (String.IsNullOrEmpty(_nameResource)) {
                            warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.MustSpecifyResourceName, type.FullName, type.Assembly.Location));
                            return false;
                        }
                    }
#endif  // LOCALIZABLE_ADDIN_ATTRIBUTE
                    else {
                        System.Diagnostics.Contracts.Contract.Assert(false, "Unknown number of custom attribute constructor parameters");
                    }
                    foreach (CustomAttributeNamedArgument arg in attr.NamedArguments) {
                        if (arg.MemberInfo.Name == "Publisher")
                            _unlocalized.Publisher = (String)arg.TypedValue.Value;
                        else if (arg.MemberInfo.Name == "Version")
                            _version = (String)arg.TypedValue.Value;
                        else if (arg.MemberInfo.Name == "Description")
                            _unlocalized.Description = (String)arg.TypedValue.Value;
#if LOCALIZABLE_ADDIN_ATTRIBUTE
                        else if (arg.MemberInfo.Name == "PublisherResourceName")
                            _publisherResource = (String)arg.TypedValue.Value;
                        else if (arg.MemberInfo.Name == "DescriptionResourceName")
                            _descriptionResource = (String)arg.TypedValue.Value;
#endif
                        else {
                            System.Diagnostics.Contracts.Contract.Assert(false, "Unknown named parameter to AddInAttribute: " + arg.MemberInfo.Name);
                            warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.UnknownNamedAddInAttributeParameter, arg.MemberInfo.Name, type.FullName, type.Assembly.Location));
                            // Let's ignore this - this shouldn't be fatal.
                        }
                    }
                    break;
                }
            }
            */

            // Check for a public default constructor
            bool found = false;
            foreach (MiniConstructorInfo ci in type.GetConstructors()) {
                MiniParameterInfo[] pars = ci.GetParameters();
                if (pars.Length == 0) {
                    found = true;
                    break;
                }
            }
            
            if (!found)
            {
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.NoDefaultConstructor, type.FullName, type.Assembly.ModuleName));
                return false;
            }

            _potentialAddinBases = FindBaseTypesAndInterfaces(type);
            
            if(_potentialAddinBases.Length == 0)
            {
                return false;
            }


#if LOCALIZABLE_ADDIN_ATTRIBUTE
            if (ContainsLocalizableStrings) {
                _localizedResources = ResourceProvider.LookupResourcesInCurrentDomain(_fullPathToAddIn,
                    _resMgrBaseName, _nameResource, _publisherResource, _descriptionResource);
            }
#endif
            return base.Validate(type, warnings);
        }
    }
}

