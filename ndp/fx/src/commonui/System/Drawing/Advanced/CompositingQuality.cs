//------------------------------------------------------------------------------
// <copyright file="CompositingQuality.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System.Drawing;
    using System;

    /// <include file='doc\CompositingQuality.uex' path='docs/doc[@for="CompositingQuality"]/*' />
    /// <devdoc>
    ///    Specifies the quality level to use during
    ///    compositing.
    /// </devdoc>
    public enum CompositingQuality
    {
        /// <include file='doc\CompositingQuality.uex' path='docs/doc[@for="CompositingQuality.Invalid"]/*' />
        /// <devdoc>
        ///    Invalid quality.
        /// </devdoc>
        Invalid = QualityMode.Invalid,
        /// <include file='doc\CompositingQuality.uex' path='docs/doc[@for="CompositingQuality.Default"]/*' />
        /// <devdoc>
        ///    Default quality.
        /// </devdoc>
        Default = QualityMode.Default,
        /// <include file='doc\CompositingQuality.uex' path='docs/doc[@for="CompositingQuality.HighSpeed"]/*' />
        /// <devdoc>
        ///    Low quality, high speed.
        /// </devdoc>
        HighSpeed = QualityMode.Low,
        /// <include file='doc\CompositingQuality.uex' path='docs/doc[@for="CompositingQuality.HighQuality"]/*' />
        /// <devdoc>
        ///    High quality, low speed.
        /// </devdoc>
        HighQuality = QualityMode.High,
        /// <include file='doc\CompositingQuality.uex' path='docs/doc[@for="CompositingQuality.GammaCorrected"]/*' />
        /// <devdoc>
        ///    Gamma correction is used.
        /// </devdoc>
        GammaCorrected,
        /// <include file='doc\CompositingQuality.uex' path='docs/doc[@for="CompositingQuality.AssumeLinear"]/*' />
        /// <devdoc>
        ///    Assume linear values.
        /// </devdoc>
        AssumeLinear
    }

}
