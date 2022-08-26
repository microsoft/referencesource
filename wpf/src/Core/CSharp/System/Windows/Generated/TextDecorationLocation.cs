//---------------------------------------------------------------------------
//
// <copyright file="TextDecorationLocation.cs" company="Microsoft">
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
    ///     TextDecorationLocation - Referenced localization of the text decoration
    /// </summary>
    public enum TextDecorationLocation
    {
        /// <summary>
        ///     Underline - Underline position
        /// </summary>
        Underline = 0,

        /// <summary>
        ///     OverLine - OverLine position
        /// </summary>
        OverLine = 1,

        /// <summary>
        ///     Strikethrough - Strikethrough position
        /// </summary>
        Strikethrough = 2,

        /// <summary>
        ///     Baseline - Baseline position
        /// </summary>
        Baseline = 3,
    }   
}
