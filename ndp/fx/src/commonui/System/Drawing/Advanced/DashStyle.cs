//------------------------------------------------------------------------------
// <copyright file="DashStyle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System.Drawing;
    using System;

    /**
     * Dash style constants (sdkinc\GDIplusEnums.h)
     */
    /// <include file='doc\DashStyle.uex' path='docs/doc[@for="DashStyle"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the style of dashed lines drawn with a <see cref='System.Drawing.Pen'/> .
    ///    </para>
    /// </devdoc>
    public enum DashStyle
    {
        /// <include file='doc\DashStyle.uex' path='docs/doc[@for="DashStyle.Solid"]/*' />
        /// <devdoc>
        ///    Specifies a solid line.
        /// </devdoc>
        Solid = 0,
        /// <include file='doc\DashStyle.uex' path='docs/doc[@for="DashStyle.Dash"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies a line comprised of dashes.
        ///    </para>
        /// </devdoc>
        Dash = 1,
        /// <include file='doc\DashStyle.uex' path='docs/doc[@for="DashStyle.Dot"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies a line comprised of dots.
        ///    </para>
        /// </devdoc>
        Dot = 2,
        /// <include file='doc\DashStyle.uex' path='docs/doc[@for="DashStyle.DashDot"]/*' />
        /// <devdoc>
        ///    Specifies a line comprised of an alternating
        ///    pattern of dash-dot-dash-dot.
        /// </devdoc>
        DashDot = 3,
        /// <include file='doc\DashStyle.uex' path='docs/doc[@for="DashStyle.DashDotDot"]/*' />
        /// <devdoc>
        ///    Specifies a line comprised of an alternating
        ///    pattern of dash-dot-dot-dash-dot-dot.
        /// </devdoc>
        DashDotDot = 4,
        /// <include file='doc\DashStyle.uex' path='docs/doc[@for="DashStyle.Custom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies a user-defined custom dash
        ///       style.
        ///    </para>
        /// </devdoc>
        Custom = 5
    }

}
