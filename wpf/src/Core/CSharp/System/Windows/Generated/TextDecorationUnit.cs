//---------------------------------------------------------------------------
//
// <copyright file="TextDecorationUnit.cs" company="Microsoft">
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

namespace System.Windows
{
    /// <summary>
    ///     TextDecorationUnit - The unit type of text decoration value
    /// </summary>
    public enum TextDecorationUnit
    {
        /// <summary>
        ///     FontRecommended - The unit is the calculated value by layout system
        /// </summary>
        FontRecommended = 0,

        /// <summary>
        ///     FontRenderingEmSize - The unit is the rendering Em size
        /// </summary>
        FontRenderingEmSize = 1,

        /// <summary>
        ///     Pixel - The unit is one pixel
        /// </summary>
        Pixel = 2,
    }   
}
