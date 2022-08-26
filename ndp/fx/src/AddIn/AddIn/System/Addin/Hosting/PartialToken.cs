// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  PartialToken
**
** Purpose: Represents a valid combination of pipeline components
**          associated classes, like host adaptors, etc.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Text;
using System.AddIn.MiniReflection;
using System.Diagnostics.Contracts;

namespace System.AddIn.Hosting
{
    [Serializable]
    internal sealed class PartialToken
    {
        internal HostAdapter _hostAdapter;
        internal ContractComponent _contract;
        internal AddInAdapter _addinAdapter;
        internal AddInBase _addinBase;

        internal PartialToken(HostAdapter hostAdapter, ContractComponent contract,
            AddInAdapter addinAdapter, AddInBase addinBase)
        {
            System.Diagnostics.Contracts.Contract.Requires(hostAdapter != null);
            System.Diagnostics.Contracts.Contract.Requires(contract != null);
            System.Diagnostics.Contracts.Contract.Requires(addinAdapter != null);
            System.Diagnostics.Contracts.Contract.Requires(addinBase != null);

            _hostAdapter = hostAdapter;
            _contract = contract;
            _addinAdapter = addinAdapter;
            _addinBase = addinBase;
        }

        internal HostAdapter HostAdapter {
            get { return _hostAdapter; }
        }

        internal String PipelineRootDirectory {

            set {
                System.Diagnostics.Contracts.Contract.Requires(value != null);
                // Update the paths for each part of the pipeline (except the add-in, of course).
                _hostAdapter.SetRootDirectory(value);
                _contract.SetRootDirectory(value);
                _addinAdapter.SetRootDirectory(value);
                _addinBase.SetRootDirectory(value);
            }
        }
    }
}
