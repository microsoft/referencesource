//------------------------------------------------------------------------------
// <copyright file="MenuGlyph.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;



    /// <include file='doc\MenuGlyph.uex' path='docs/doc[@for="MenuGlyph"]/*' />
    /// <devdoc>
    ///     Enum to be used with the drawMenuGlyph function.
    ///
    /// </devdoc>
    public enum MenuGlyph {

        /// <include file='doc\MenuGlyph.uex' path='docs/doc[@for="MenuGlyph.Arrow"]/*' />
        /// <devdoc>
        ///     Draws a submenu arrow.
        /// </devdoc>
        Arrow = NativeMethods.DFCS_MENUARROW,

        /// <include file='doc\MenuGlyph.uex' path='docs/doc[@for="MenuGlyph.Checkmark"]/*' />
        /// <devdoc>
        ///     Draws a menu checkmark.
        /// </devdoc>
        Checkmark = NativeMethods.DFCS_MENUCHECK,

        /// <include file='doc\MenuGlyph.uex' path='docs/doc[@for="MenuGlyph.Bullet"]/*' />
        /// <devdoc>
        ///     Draws a menu bullet.
        /// </devdoc>
        Bullet = NativeMethods.DFCS_MENUBULLET,

        /// <include file='doc\MenuGlyph.uex' path='docs/doc[@for="MenuGlyph.Min"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Min = NativeMethods.DFCS_MENUARROW,
        /// <include file='doc\MenuGlyph.uex' path='docs/doc[@for="MenuGlyph.Max"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Max = NativeMethods.DFCS_MENUBULLET,

    }
}
