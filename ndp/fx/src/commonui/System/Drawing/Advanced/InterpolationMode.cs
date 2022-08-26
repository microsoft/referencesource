//------------------------------------------------------------------------------
// <copyright file="InterpolationMode.cs" company="Microsoft">
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
    /// <include file='doc\InterpolationMode.uex' path='docs/doc[@for="InterpolationMode"]/*' />
    /// <devdoc>
    ///    Specifies how data is interpolated between
    ///    endpoints.
    /// </devdoc>
    public enum InterpolationMode
    {
        /// <include file='doc\InterpolationMode.uex' path='docs/doc[@for="InterpolationMode.Invalid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Equivalent to <see cref='System.Drawing.Drawing2D.QualityMode.Invalid'/>
        ///    </para>
        /// </devdoc>
        Invalid = QualityMode.Invalid,
        /// <include file='doc\InterpolationMode.uex' path='docs/doc[@for="InterpolationMode.Default"]/*' />
        /// <devdoc>
        ///    Specifies default mode.
        /// </devdoc>
        Default = QualityMode.Default,
        /// <include file='doc\InterpolationMode.uex' path='docs/doc[@for="InterpolationMode.Low"]/*' />
        /// <devdoc>
        ///    Specifies low quality.
        /// </devdoc>
        Low = QualityMode.Low,
        /// <include file='doc\InterpolationMode.uex' path='docs/doc[@for="InterpolationMode.High"]/*' />
        /// <devdoc>
        ///    Specifies high quality.
        /// </devdoc>
        High = QualityMode.High,
        /// <include file='doc\InterpolationMode.uex' path='docs/doc[@for="InterpolationMode.Bilinear"]/*' />
        /// <devdoc>
        ///    Specifies bilinear interpolation.
        /// </devdoc>
        Bilinear,
        /// <include file='doc\InterpolationMode.uex' path='docs/doc[@for="InterpolationMode.Bicubic"]/*' />
        /// <devdoc>
        ///    Specifies bicubic interpolation.
        /// </devdoc>
        Bicubic,
        /// <include file='doc\InterpolationMode.uex' path='docs/doc[@for="InterpolationMode.NearestNeighbor"]/*' />
        /// <devdoc>
        ///    Specifies nearest neighbor interpolation.
        /// </devdoc>
        NearestNeighbor,
        /// <include file='doc\InterpolationMode.uex' path='docs/doc[@for="InterpolationMode.HighQualityBilinear"]/*' />
        /// <devdoc>
        ///    Specifies high qulaity bilenear
        ///    interpolation.
        /// </devdoc>
        HighQualityBilinear,
        /// <include file='doc\InterpolationMode.uex' path='docs/doc[@for="InterpolationMode.HighQualityBicubic"]/*' />
        /// <devdoc>
        ///    Specifies high quality bicubic
        ///    interpolation.
        /// </devdoc>
        HighQualityBicubic
    }
}
