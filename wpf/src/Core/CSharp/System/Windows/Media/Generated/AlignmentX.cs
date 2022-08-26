//---------------------------------------------------------------------------
//
// <copyright file="AlignmentX.cs" company="Microsoft">
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
    ///     AlignmentX - The AlignmentX enum is used to describe how content is positioned 
    ///     horizontally within a container.
    /// </summary>
    public enum AlignmentX
    {
        /// <summary>
        ///     Left - Align contents towards the left of a space.
        /// </summary>
        Left = 0,

        /// <summary>
        ///     Center - Center contents horizontally.
        /// </summary>
        Center = 1,

        /// <summary>
        ///     Right - Align contents towards the right of a space.
        /// </summary>
        Right = 2,
    }   
}
