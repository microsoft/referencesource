//------------------------------------------------------------------------------
// <copyright file="AssemblyAttributes.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

// Always hard bind to System.dll

using System.Runtime.CompilerServices;

[assembly:DependencyAttribute("System,", LoadHint.Always)]
// This is now trun on by default, use source file NO_RUNTIMECOMPATIBILITY_ATTRIBUTE flag to control this
// [assembly:RuntimeCompatibility(WrapNonExceptionThrows = true)
[assembly:System.Runtime.InteropServices.TypeLibVersion(2, 4)]

// Opts into the VS loading icons from the Icon Satellite assembly
[assembly: System.Drawing.BitmapSuffixInSatelliteAssemblyAttribute()]