// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ContractComponent
**
** Purpose: Represents a class that implements IContract on 
**     disk, for the add-in model.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Text;
using System.AddIn.MiniReflection;
using System.Diagnostics.Contracts;

namespace System.AddIn
{
    [Serializable]
    internal sealed class ContractComponent : PipelineComponent
    {
        public ContractComponent(TypeInfo typeInfo, String assemblyLocation) : base(typeInfo, assemblyLocation)
        {
        }

        public override string ToString()
        {
            return String.Format(CultureInfo.CurrentCulture, Res.ContractToString, Name, BestAvailableLocation);
        }

        internal override bool Validate(Type type, Collection<String> warnings)
        {
            //if (!type.Implements(new TypeInfo(typeof(IContract))))
            if (!IContractInReflectionLoaderContext.IsAssignableFrom(type))
            {
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.ContractMustImplementIContract, Name));
                return false;
            }

            if (!type.IsInterface)
            {
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.ContractMustBeInterface, Name));
                return false;
            }

            return base.Validate(type, warnings);
        }
    }
}
