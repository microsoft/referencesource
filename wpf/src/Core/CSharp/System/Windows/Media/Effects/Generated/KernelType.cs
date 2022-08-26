//---------------------------------------------------------------------------
//
// <copyright file="KernelType.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// This file was generated, please do not edit it directly.
//
// Please see http://wiki/default.aspx/Microsoft.Projects.Avalon/MilCodeGen.html for more information.
//
//---------------------------------------------------------------------------

#if PRESENTATION_CORE
using SR=MS.Internal.PresentationCore.SR;
using SRID=MS.Internal.PresentationCore.SRID;
#else
using SR=System.Windows.SR;
using SRID=System.Windows.SRID;
#endif

namespace System.Windows.Media.Effects
{
    /// <summary>
    ///     KernelType - Type of blur kernel to use.
    /// </summary>
    public enum KernelType
    {
        /// <summary>
        ///     Gaussian - Use a Guassian filter
        /// </summary>
        Gaussian = 0,

        /// <summary>
        ///     Box - Use a Box filter
        /// </summary>
        Box = 1,
    }   
}
