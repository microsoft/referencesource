//------------------------------------------------------------------------------
// <copyright file="BitmapSuffixInSameAssemblyAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing {
    using System;

    /// <summary>
    /// Opt-In flag to look for resources in the same assembly but with the "bitmapSuffix" config setting.
    /// i.e. System.Web.UI.WebControl.Button.bmp -> System.Web.UI.WebControl.Button.VisualStudio.11.0.bmp
    /// </summary>
    [AttributeUsage(AttributeTargets.Assembly)]
    public class BitmapSuffixInSameAssemblyAttribute : Attribute {
    }
}
