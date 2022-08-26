//---------------------------------------------------------------------------
//
// <copyright file="RenderingBias.cs" company="Microsoft">
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
    ///     RenderingBias - Type of bias to give rendering of the effect
    /// </summary>
    public enum RenderingBias
    {
        /// <summary>
        ///     Performance - Bias towards performance
        /// </summary>
        Performance = 0,

        /// <summary>
        ///     Quality - Bias towards quality
        /// </summary>
        Quality = 1,
    }   
}
