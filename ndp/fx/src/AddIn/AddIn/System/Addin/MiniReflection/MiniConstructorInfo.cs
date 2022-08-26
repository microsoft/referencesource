// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  MiniConstructorInfo
**
** Purpose: Represents a constructor.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Text;
using System.AddIn.MiniReflection.MetadataReader;
using System.Diagnostics.Contracts;

namespace System.AddIn.MiniReflection
{
    [Serializable]
    internal sealed class MiniConstructorInfo
    {
        private MiniParameterInfo[] _parameters;

        public MiniConstructorInfo(MiniParameterInfo[] parameters)
        {
            System.Diagnostics.Contracts.Contract.Requires(parameters != null);
            _parameters = parameters;
        }

        /*
        internal MiniConstructorInfo(System.Reflection.ConstructorInfo ctor)
        {
            System.Reflection.ParameterInfo[] parameters = ctor.GetParameters();
            _parameters = new MiniParameterInfo[parameters.Length];
            for (int i = 0; i < parameters.Length; i++)
                _parameters[i] = new MiniParameterInfo(new TypeInfo(parameters[i].ParameterType));
        }
         */

        public MiniParameterInfo[] GetParameters()
        {
            return _parameters;
        }
    }
}
