//---------------------------------------------------------------------------
//
// <copyright file="PenLineJoin.cs" company="Microsoft">
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
    ///     PenLineJoin - Enum which descibes the drawing of the corners on the line.
    /// </summary>
    public enum PenLineJoin
    {
        /// <summary>
        ///     Miter - Miter join.
        /// </summary>
        Miter = 0,

        /// <summary>
        ///     Bevel - Bevel join.
        /// </summary>
        Bevel = 1,

        /// <summary>
        ///     Round - Round join.
        /// </summary>
        Round = 2,
    }   
}
