//---------------------------------------------------------------------------
//
// <copyright file="PenLineCap.cs" company="Microsoft">
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
    ///     PenLineCap - Enum which descibes the drawing of the ends of a line.
    /// </summary>
    public enum PenLineCap
    {
        /// <summary>
        ///     Flat - Flat line cap.
        /// </summary>
        Flat = 0,

        /// <summary>
        ///     Square - Square line cap.
        /// </summary>
        Square = 1,

        /// <summary>
        ///     Round - Round line cap.
        /// </summary>
        Round = 2,

        /// <summary>
        ///     Triangle - Triangle line cap.
        /// </summary>
        Triangle = 3,
    }   
}
