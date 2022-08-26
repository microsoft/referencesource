//------------------------------------------------------------------------------
// <copyright file="WrapMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System.Drawing;
    using System;

    /**
     * Various wrap modes for brushes
     */
    /// <include file='doc\WrapMode.uex' path='docs/doc[@for="WrapMode"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies how a texture or gradient is tiled when it is
    ///       larger than the area being filled.
    ///    </para>
    /// </devdoc>
    public enum WrapMode
    {
        /// <include file='doc\WrapMode.uex' path='docs/doc[@for="WrapMode.Tile"]/*' />
        /// <devdoc>
        ///    Tiles the gradient or texture.
        /// </devdoc>
        Tile = 0,
        /// <include file='doc\WrapMode.uex' path='docs/doc[@for="WrapMode.TileFlipX"]/*' />
        /// <devdoc>
        ///    Reverses the texture or gradient
        ///    horizontally and then tiles the texture or gradient.
        /// </devdoc>
        TileFlipX = 1,
        /// <include file='doc\WrapMode.uex' path='docs/doc[@for="WrapMode.TileFlipY"]/*' />
        /// <devdoc>
        ///    Reverses the texture or
        ///    gradient vertically and then tiles the texture or gradient.
        /// </devdoc>
        TileFlipY = 2,
        /// <include file='doc\WrapMode.uex' path='docs/doc[@for="WrapMode.TileFlipXY"]/*' />
        /// <devdoc>
        ///    Reverses the texture or gradient
        ///    horizontally and vertically and then tiles the texture or gradient.
        /// </devdoc>
        TileFlipXY = 3,
        /// <include file='doc\WrapMode.uex' path='docs/doc[@for="WrapMode.Clamp"]/*' />
        /// <devdoc>
        ///    Clamps the texture or gradient to the
        ///    object boundary.
        /// </devdoc>
        Clamp = 4
    }

}
