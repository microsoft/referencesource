//---------------------------------------------------------------------------
//
// <copyright file="ColorInterpolationMode.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// This file was generated, please do not edit it directly.
//
// Please see http://wiki/default.aspx/Microsoft.Projects.Avalon/MilCodeGen.html for more information.
//
//---------------------------------------------------------------------------

using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using MS.Internal.PresentationCore;
#if PRESENTATION_CORE
using SR=MS.Internal.PresentationCore.SR;
using SRID=MS.Internal.PresentationCore.SRID;
#else
using SR=System.Windows.SR;
using SRID=System.Windows.SRID;
#endif

namespace System.Windows.Media
{
    /// <summary>
    ///     ColorInterpolationMode - This determines how the colors in a gradient are 
    ///     interpolated.
    /// </summary>
    public enum ColorInterpolationMode
    {
        /// <summary>
        ///     ScRgbLinearInterpolation - Colors are interpolated in the scRGB color space
        /// </summary>
        ScRgbLinearInterpolation = 0,

        /// <summary>
        ///     SRgbLinearInterpolation - Colors are interpolated in the sRGB color space
        /// </summary>
        SRgbLinearInterpolation = 1,
    }   
}
