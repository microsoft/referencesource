// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  DeploymentState
**
** Purpose: Abstract base class for pipeline and addin caches
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
    internal abstract class DeploymentState
    {
    }
}
