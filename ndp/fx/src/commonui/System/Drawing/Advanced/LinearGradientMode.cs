//------------------------------------------------------------------------------
// <copyright file="LinearGradientMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System;
    using System.Drawing;

    /**
     * Linear Gradient mode constants
     */
    /// <include file='doc\LinearGradientMode.uex' path='docs/doc[@for="LinearGradientMode"]/*' />
    /// <devdoc>
    ///    Specifies the direction of a linear
    ///    gradient.
    /// </devdoc>
    public enum LinearGradientMode
    {
        /// <include file='doc\LinearGradientMode.uex' path='docs/doc[@for="LinearGradientMode.Horizontal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies a gradient from left to right.
        ///    </para>
        /// </devdoc>
        Horizontal = 0,
        /// <include file='doc\LinearGradientMode.uex' path='docs/doc[@for="LinearGradientMode.Vertical"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies a gradient from top to bottom.
        ///    </para>
        /// </devdoc>
        Vertical = 1,
        /// <include file='doc\LinearGradientMode.uex' path='docs/doc[@for="LinearGradientMode.ForwardDiagonal"]/*' />
        /// <devdoc>
        ///    Specifies a gradient from upper-left to
        ///    lower-right.
        /// </devdoc>
        ForwardDiagonal = 2,
        /// <include file='doc\LinearGradientMode.uex' path='docs/doc[@for="LinearGradientMode.BackwardDiagonal"]/*' />
        /// <devdoc>
        ///    Specifies a gradient from upper-right to
        ///    lower-left.
        /// </devdoc>
        BackwardDiagonal = 3
    }
}
