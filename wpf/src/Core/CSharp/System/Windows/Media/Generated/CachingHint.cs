//---------------------------------------------------------------------------
//
// <copyright file="CachingHint.cs" company="Microsoft">
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
    ///     CachingHint - Enum used for hinting the rendering engine that rendered content can 
    ///     be cached
    /// </summary>
    public enum CachingHint
    {
        /// <summary>
        ///     Unspecified - Rendering engine will choose algorithm.
        /// </summary>
        Unspecified = 0,

        /// <summary>
        ///     Cache - Cache rendered content when possible.
        /// </summary>
        Cache = 1,
    }   
}
