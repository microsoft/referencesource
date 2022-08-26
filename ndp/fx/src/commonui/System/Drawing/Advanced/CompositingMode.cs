//------------------------------------------------------------------------------
// <copyright file="CompositingMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System.Drawing;
    using System;

    /*
     * Alpha compositing mode constants
     *
     * @notes Should we scrap this for the first version
     *  and support only SrcOver instead?
     */

    /// <include file='doc\CompositingMode.uex' path='docs/doc[@for="CompositingMode"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines how the source image is composited with the background image.
    ///    </para>
    /// </devdoc>
    public enum CompositingMode
    {
        /// <include file='doc\CompositingMode.uex' path='docs/doc[@for="CompositingMode.SourceOver"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The source pixels overwrite the background pixels.
        ///    </para>
        /// </devdoc>
        SourceOver = 0,
        /// <include file='doc\CompositingMode.uex' path='docs/doc[@for="CompositingMode.SourceCopy"]/*' />
        /// <devdoc>
        ///    The source pixels are combined with the
        ///    background pixels.
        /// </devdoc>
        SourceCopy = 1
    }

}
