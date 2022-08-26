//---------------------------------------------------------------------------
//
// <copyright file="ShaderRenderMode.cs" company="Microsoft">
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
    ///     ShaderRenderMode - Policy for rendering the shader in software.
    /// </summary>
    public enum ShaderRenderMode
    {
        /// <summary>
        ///     Auto - Allow hardware and software
        /// </summary>
        Auto = 0,

        /// <summary>
        ///     SoftwareOnly - Force software rendering
        /// </summary>
        SoftwareOnly = 1,

        /// <summary>
        ///     HardwareOnly - Require hardware rendering, ignore otherwise
        /// </summary>
        HardwareOnly = 2,
    }   
}
