//------------------------------------------------------------------------------
// <copyright file="SmoothingMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System.Drawing;
    using System;

    /// <include file='doc\SmoothingMode.uex' path='docs/doc[@for="SmoothingMode"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the overall quality of rendering of graphics
    ///       shapes.
    ///    </para>
    /// </devdoc>
    public enum SmoothingMode
    {
        /// <include file='doc\SmoothingMode.uex' path='docs/doc[@for="SmoothingMode.Invalid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies an invalid mode.
        ///    </para>
        /// </devdoc>
        Invalid = QualityMode.Invalid,
        /// <include file='doc\SmoothingMode.uex' path='docs/doc[@for="SmoothingMode.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the default mode.
        ///    </para>
        /// </devdoc>
        Default = QualityMode.Default,
        /// <include file='doc\SmoothingMode.uex' path='docs/doc[@for="SmoothingMode.HighSpeed"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies low quality, high performance rendering.
        ///    </para>
        /// </devdoc>
        HighSpeed = QualityMode.Low,
        /// <include file='doc\SmoothingMode.uex' path='docs/doc[@for="SmoothingMode.HighQuality"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies high quality, lower performance rendering.
        ///    </para>
        /// </devdoc>
        HighQuality = QualityMode.High,
        /// <include file='doc\SmoothingMode.uex' path='docs/doc[@for="SmoothingMode.None"]/*' />
        /// <devdoc>
        ///    Specifies no anti-aliasing.
        /// </devdoc>
        None,
        /// <include file='doc\SmoothingMode.uex' path='docs/doc[@for="SmoothingMode.AntiAlias"]/*' />
        /// <devdoc>
        ///    Specifies anti-aliased rendering.
        /// </devdoc>
        AntiAlias
    }
}
