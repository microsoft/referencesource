//------------------------------------------------------------------------------
// <copyright file="BitmapSuffixInSameAssemblyAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing {
    using System;

    /// <summary>
    /// Opt-In flag to look for resources in the another assembly with the "bitmapSuffix" config setting
    /// i.e. System.Web.dll -> System.Web<.VisualStudio.11.0>.dll
    /// </summary>
    [AttributeUsage(AttributeTargets.Assembly)]
    public class BitmapSuffixInSatelliteAssemblyAttribute : Attribute {
    }
}
