// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  AddInBase
**
** Purpose: Represents an add-in's base class on disk.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Text;
using System.AddIn.MiniReflection;
using System.Reflection;
using System.Diagnostics.Contracts;
using TypeInfo=System.AddIn.MiniReflection.TypeInfo;

namespace System.AddIn
{
    [Serializable]
    internal sealed class AddInBase : PipelineComponent
    {
        internal TypeInfo[] _activatableAs;
        internal String _assemblyName;

        public AddInBase(TypeInfo typeInfo, TypeInfo[] activatableAs, String assemblyLocation, String assemblyName) : base(typeInfo, assemblyLocation)
        {
            _activatableAs = activatableAs;
            _assemblyName = assemblyName;
        }

        public override string ToString()
        {
            return String.Format(CultureInfo.CurrentCulture, Res.AddInBaseToString, Name, BestAvailableLocation);
        }

        internal override bool Validate(Type type, Collection<String> warnings)
        {
            if (type.IsClass && type.IsSealed)
            {
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.AddInBaseMustBeSubclassable, Name));
                return false;
            }
            return base.Validate(type, warnings);
        }

        internal bool CanDirectConnectTo(TypeInfo havTypeInfo)
        {
            bool result = false;
            
            if(havTypeInfo.Equals(TypeInfo))
            {
                // Check the add-in base's type info.
                result = true;
            }
            else if(_activatableAs != null)  
            {
                // Check the ActivatableAs types.
                for(int i = 0; i < _activatableAs.Length && result == false; i++)
                {
                    if(_activatableAs[i].Equals(havTypeInfo))
                    {
                        result = true;
                    }
                }
            }
            
            return result;
        }
    }
}

