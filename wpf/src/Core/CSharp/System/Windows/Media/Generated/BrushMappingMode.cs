//---------------------------------------------------------------------------
//
// <copyright file="BrushMappingMode.cs" company="Microsoft">
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
    ///     BrushMappingMode - Enum which describes whether certain values should be considered 
    ///     as absolute local coordinates or whether they should be considered multiples of a 
    ///     bounding box's size.
    /// </summary>
    public enum BrushMappingMode
    {
        /// <summary>
        ///     Absolute - Absolute means that the values in question will be interpreted directly 
        ///     in local space.
        /// </summary>
        Absolute = 0,

        /// <summary>
        ///     RelativeToBoundingBox - RelativeToBoundingBox means that the values will be 
        ///     interpreted as a multiples of a bounding box, where 1.0 is considered 100% of the 
        ///     bounding box measure.
        /// </summary>
        RelativeToBoundingBox = 1,
    }   
}
