// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  AddInAdapter
**
** Purpose: Represents an add-in adapter on disk
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Reflection;
using System.Text;
using System.AddIn.MiniReflection;
using System.Diagnostics.Contracts;
using TypeInfo=System.AddIn.MiniReflection.TypeInfo;

namespace System.AddIn
{
    [Serializable]
    internal sealed class AddInAdapter : PipelineComponent
    {
        private List<TypeInfo> _contracts;
        private List<TypeInfo> _constructors;

        public AddInAdapter(TypeInfo typeInfo, String assemblyLocation) : base(typeInfo, assemblyLocation)
        {
            _contracts = new List<TypeInfo>();
            _constructors = new List<TypeInfo>();
        }

        public List<TypeInfo> Constructors {
            get { return _constructors; }
        }

        public List<TypeInfo> Contracts {
            get { return _contracts; }
        }

        public override String ToString()
        {
            return String.Format(CultureInfo.CurrentCulture, Res.AddInAdapterToString, Name, BestAvailableLocation);
        }

        internal override bool Validate(Type type, Collection<String> warnings)
        {
            System.Diagnostics.Contracts.Contract.Assert(type.Assembly.ReflectionOnly && IContractInReflectionLoaderContext.Assembly.ReflectionOnly,
                "Both the type and IContract should be in the ReflectionOnly loader context");

            if (!type.IsMarshalByRef)
            {
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.AddInAdapterMustBeMBRO, type.AssemblyQualifiedName));
                return false;
            }

            //if (!type.Implements(typeofIContract))
            if (!IContractInReflectionLoaderContext.IsAssignableFrom(type))
            {
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.AddInAdapterMustImplementAnAddInContract, type.AssemblyQualifiedName));
                return false;
            }

            foreach (Type contractInterface in type.GetInterfaces())
            {
                //if (contractInterface.Implements(typeofIContract))
                if (IContractInReflectionLoaderContext.IsAssignableFrom(contractInterface))
                    _contracts.Add(new TypeInfo(contractInterface));
            }
            if (_contracts.Count == 0)
            {
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.AddInAdapterMustImplementAnAddInContract, type.AssemblyQualifiedName));
                return false;
            }

            foreach (ConstructorInfo ctor in type.GetConstructors(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance))
            {
                ParameterInfo[] pars = ctor.GetParameters();
                if (pars.Length != 1)
                {
                    warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.AddInAdapterOneUnusableConstructor, type.AssemblyQualifiedName));
                    continue;
                }
                _constructors.Add(new TypeInfo(pars[0].ParameterType));
            }
            if (_constructors.Count == 0)
            {
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.AddInAdapterNoUsableConstructors, type.AssemblyQualifiedName));
                return false;
            }
            return base.Validate(type, warnings);
        }


        // Imagine a generic AddInBase (AB<T>), and an AddInAdapter with a 
        // constructor taking in AB<Int32>.  If we have IntAddIn : AB<Int32>, 
        // then we should be able to hook this up.
        internal bool CanConnectTo(AddInBase addInBase)
        {
            System.Diagnostics.Contracts.Contract.Requires(addInBase != null);

            if (!addInBase.TypeInfo.IsGeneric)
            {
                if (this.Constructors.Contains(addInBase.TypeInfo))
                    return true;
                
                // return true if we have a constructor that accepts one of addinBase's ActivatableAs base classes
                if (addInBase._activatableAs != null)
                {
                    foreach (TypeInfo activatableAsTypeInfo in addInBase._activatableAs)
                    {
                        if (this.Constructors.Contains(activatableAsTypeInfo))
                            return true;
                    }
                }
            }
            else
            {
                return false;
            }
            return false;
        }
    }
}
