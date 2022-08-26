// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  HostAdapter
**
** Purpose: Represents an host adapter class on disk.
**
===========================================================*/
using System;
using System.IO;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Reflection;
using System.Text;
using System.AddIn.Contract;
using System.AddIn.MiniReflection;
using System.Diagnostics.Contracts;
using TypeInfo=System.AddIn.MiniReflection.TypeInfo;

namespace System.AddIn
{
    [Serializable]
    internal sealed class HostAdapter : PipelineComponent
    {
        private static readonly TypeInfo typeofIContract = new TypeInfo(typeof(System.AddIn.Contract.IContract));

        private TypeInfo[] _potentialHAVs;
        private List<TypeInfo> _constructors;

        public HostAdapter(TypeInfo typeInfo, String assemblyLocation)
            : base(typeInfo, assemblyLocation)
        {
            _constructors = new List<TypeInfo>();
        }

        public List<TypeInfo> Constructors
        {
            get { return _constructors; }
        }

        public TypeInfo[] HostAddinViews
        {
            get { return _potentialHAVs; }
        }

        public override string ToString()
        {
            return String.Format(CultureInfo.CurrentCulture, Res.HostAdapterToString, Name, BestAvailableLocation);
        }

        internal override bool Validate(TypeInfo type, Collection<String> warnings)
        {
            _potentialHAVs = FindBaseTypesAndInterfaces(type);
            
            if (_potentialHAVs.Length == 0)
            {
                return false;
            }

            foreach (MiniConstructorInfo ci in type.GetConstructors(true))
            {
                MiniParameterInfo[] pars = ci.GetParameters();
                if (pars.Length != 1)
                {
                    warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.HostAdapterUnusableCtorMultipleParams, type.AssemblyQualifiedName));
                    continue;
                }

                TypeInfo paramType = pars[0].ParameterType;
                bool success = false;
                try
                {
                    if (paramType.IsInterface && (paramType.Implements(typeofIContract)))
                    {
                        _constructors.Add(paramType);
                        success = true;
                    }
                }
                catch (FileNotFoundException) { }
                catch (FileLoadException)
                {
                    // Can happen for a constructor taking a type that isn't in mscorlib nor System.AddIn.Contract, and
                    // also isn't in any of the directories that we would probe.
                }
                if (!success)
                {
                    warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.HostAdapterUnusableCtorBadParam, type.AssemblyQualifiedName, paramType.FullName));
                    continue;
                }
            }
            if (_constructors.Count == 0)
            {
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.HostAdapterNoValidCtors, type.AssemblyQualifiedName));
                return false;
            }

            return base.Validate(type, warnings);
        }
    }
}
