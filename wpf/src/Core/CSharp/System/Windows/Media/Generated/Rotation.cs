//---------------------------------------------------------------------------
//
// <copyright file="Rotation.cs" company="Microsoft">
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

namespace System.Windows.Media.Imaging
{
    /// <summary>
    ///     Rotation - The rotation to be applied; only multiples of 90 degrees is supported.
    /// </summary>
    public enum Rotation
    {
        /// <summary>
        ///     Rotate0 - Do not rotate
        /// </summary>
        Rotate0 = 0,

        /// <summary>
        ///     Rotate90 - Rotate 90 degress
        /// </summary>
        Rotate90 = 1,

        /// <summary>
        ///     Rotate180 - Rotate 180 degrees
        /// </summary>
        Rotate180 = 2,

        /// <summary>
        ///     Rotate270 - Rotate 270 degrees
        /// </summary>
        Rotate270 = 3,
    }   
}
