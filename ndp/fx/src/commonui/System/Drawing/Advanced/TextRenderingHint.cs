//------------------------------------------------------------------------------
// <copyright file="TextRenderingHint.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Text {

    using System.Drawing;
    using System;

    /// <include file='doc\TextRenderingHint.uex' path='docs/doc[@for="TextRenderingHint"]/*' />
    /// <devdoc>
    ///    Specifies the quality of text rendering.
    /// </devdoc>
    public enum TextRenderingHint
    {
            /// <include file='doc\TextRenderingHint.uex' path='docs/doc[@for="TextRenderingHint.SystemDefault"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            SystemDefault = 0,        // Glyph with system default rendering hint
        /// <include file='doc\TextRenderingHint.uex' path='docs/doc[@for="TextRenderingHint.SingleBitPerPixelGridFit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SingleBitPerPixelGridFit, // Glyph bitmap with hinting
        /// <include file='doc\TextRenderingHint.uex' path='docs/doc[@for="TextRenderingHint.SingleBitPerPixel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SingleBitPerPixel,        // Glyph bitmap without hinting
        /// <include file='doc\TextRenderingHint.uex' path='docs/doc[@for="TextRenderingHint.AntiAliasGridFit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        AntiAliasGridFit,         //Anti-aliasing with hinting
        /// <include file='doc\TextRenderingHint.uex' path='docs/doc[@for="TextRenderingHint.AntiAlias"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        AntiAlias,                // Glyph anti-alias bitmap without hinting
        // Glyph anti-alias bitmap without hinting  
        /// <include file='doc\TextRenderingHint.uex' path='docs/doc[@for="TextRenderingHint.ClearTypeGridFit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ClearTypeGridFit          // Glyph CT bitmap with hinting
    }

}

