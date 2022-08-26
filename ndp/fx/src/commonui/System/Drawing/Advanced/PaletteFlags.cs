//------------------------------------------------------------------------------
// <copyright file="PaletteFlags.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {

    using System.Drawing;
    using System;

    /// <include file='doc\PaletteFlags.uex' path='docs/doc[@for="PaletteFlags"]/*' />
    /// <devdoc>
    ///    Specifies the type of color data in the
    ///    system palette. The data can be color data with alpha, grayscale only, or
    ///    halftone data.
    /// </devdoc>
    [Flags]
    public enum PaletteFlags
    {
    
        /// <include file='doc\PaletteFlags.uex' path='docs/doc[@for="PaletteFlags.HasAlpha"]/*' />
        /// <devdoc>
        ///    Specifies alpha data.
        /// </devdoc>
        HasAlpha    = 0x0001,
        /// <include file='doc\PaletteFlags.uex' path='docs/doc[@for="PaletteFlags.GrayScale"]/*' />
        /// <devdoc>
        ///    Specifies grayscale data.
        /// </devdoc>
        GrayScale   = 0x0002,
        /// <include file='doc\PaletteFlags.uex' path='docs/doc[@for="PaletteFlags.Halftone"]/*' />
        /// <devdoc>
        ///    Specifies halftone data.
        /// </devdoc>
        Halftone    = 0x0004
    }
}

