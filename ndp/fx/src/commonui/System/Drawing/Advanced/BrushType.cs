//------------------------------------------------------------------------------
// <copyright file="BrushType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {

    using System;
    using System.Drawing;

    /**
     * BrushType Type
     */
    internal enum BrushType
    {
        SolidColor     = 0,
        HatchFill      = 1,
        TextureFill    = 2,
        PathGradient   = 3,
        LinearGradient = 4
    }
}
