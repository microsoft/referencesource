// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  AddInDeploymentState
**
** Purpose: Represents the add-in pipeline components 
**          in a directory structure.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.IO;
using System.Text;
using System.Reflection;
using System.AddIn.MiniReflection;
using System.Diagnostics.Contracts;

namespace System.AddIn.Hosting
{
    [Serializable]
    internal sealed class AddInDeploymentState : DeploymentState
    {
        private List<AddIn> _addins;

        //this is automatically initialized to 0
        private int _fileCount;

        internal AddInDeploymentState()
        {
            _addins = new List<AddIn>();
        }

        internal List<AddIn> AddIns {
            get { return _addins; }
        }

        internal int FileCount
        {
            get { return _fileCount; }
            set { _fileCount = value; }
        }
    }
}
