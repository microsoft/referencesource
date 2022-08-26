//------------------------------------------------------------------------------
// <copyright file="ColorChannelFlags.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {

    using System.Diagnostics;

    using System.Drawing;
    using System;

    /**
     * Color channel flag constants
     */
    /// <include file='doc\ColorChannelFlags.uex' path='docs/doc[@for="ColorChannelFlag"]/*' />
    /// <devdoc>
    ///    Specifies a range of CMYK channels.
    /// </devdoc>
    public enum ColorChannelFlag {
        /// <include file='doc\ColorChannelFlags.uex' path='docs/doc[@for="ColorChannelFlag.ColorChannelC"]/*' />
        /// <devdoc>
        ///    Specifies the Cyan color channel.
        /// </devdoc>
        ColorChannelC = 0,
        /// <include file='doc\ColorChannelFlags.uex' path='docs/doc[@for="ColorChannelFlag.ColorChannelM"]/*' />
        /// <devdoc>
        ///    Specifies the Magenta color channel.
        /// </devdoc>
        ColorChannelM,
        /// <include file='doc\ColorChannelFlags.uex' path='docs/doc[@for="ColorChannelFlag.ColorChannelY"]/*' />
        /// <devdoc>
        ///    Specifies the Yellow color channel.
        /// </devdoc>
        ColorChannelY,
        /// <include file='doc\ColorChannelFlags.uex' path='docs/doc[@for="ColorChannelFlag.ColorChannelK"]/*' />
        /// <devdoc>
        ///    Specifies the Black color channel.
        /// </devdoc>
        ColorChannelK,
        /// <include file='doc\ColorChannelFlags.uex' path='docs/doc[@for="ColorChannelFlag.ColorChannelLast"]/*' />
        /// <devdoc>
        ///    <para>
        ///       This element specifies to leave the color
        ///       channel unchanged from the last selected channel.
        ///    </para>
        /// </devdoc>
        ColorChannelLast
    }
}
