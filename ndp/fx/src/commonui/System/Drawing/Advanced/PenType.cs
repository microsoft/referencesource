//------------------------------------------------------------------------------
// <copyright file="PenType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {

    using System;
    using System.Drawing;

    /**
     * PenType Type
     */
    /// <include file='doc\PenType.uex' path='docs/doc[@for="PenType"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the type of fill a <see cref='System.Drawing.Pen'/> uses to
    ///       fill lines.
    ///    </para>
    /// </devdoc>
    public enum PenType
    {
        /// <include file='doc\PenType.uex' path='docs/doc[@for="PenType.SolidColor"]/*' />
        /// <devdoc>
        ///    Specifies a solid fill.
        /// </devdoc>
        SolidColor       = BrushType.SolidColor,
        /// <include file='doc\PenType.uex' path='docs/doc[@for="PenType.HatchFill"]/*' />
        /// <devdoc>
        ///    Specifies a hatch fill.
        /// </devdoc>
        HatchFill        = BrushType.HatchFill,
        /// <include file='doc\PenType.uex' path='docs/doc[@for="PenType.TextureFill"]/*' />
        /// <devdoc>
        ///    Specifies a bitmap texture fill.
        /// </devdoc>
        TextureFill      = BrushType.TextureFill,
        /// <include file='doc\PenType.uex' path='docs/doc[@for="PenType.PathGradient"]/*' />
        /// <devdoc>
        ///    Specifies a path gradient fill.
        /// </devdoc>
        PathGradient     = BrushType.PathGradient,
        /// <include file='doc\PenType.uex' path='docs/doc[@for="PenType.LinearGradient"]/*' />
        /// <devdoc>
        ///    Specifies a linear gradient fill.
        /// </devdoc>
        LinearGradient   = BrushType.LinearGradient,
    }
}
