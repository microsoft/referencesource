//------------------------------------------------------------------------------
// <copyright file="CombineMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System.Drawing;
    using System;

    /**
     * Combine mode constants
     */
    /// <include file='doc\CombineMode.uex' path='docs/doc[@for="CombineMode"]/*' />
    /// <devdoc>
    ///    Defines how different clipping regions can
    ///    be combined.
    /// </devdoc>
    public enum CombineMode
    {
        /// <include file='doc\CombineMode.uex' path='docs/doc[@for="CombineMode.Replace"]/*' />
        /// <devdoc>
        ///    One clipping region is replaced by another.
        /// </devdoc>
        Replace     = 0,
        /// <include file='doc\CombineMode.uex' path='docs/doc[@for="CombineMode.Intersect"]/*' />
        /// <devdoc>
        ///    The two clipping regions are combined by
        ///    taking their intersection.
        /// </devdoc>
        Intersect   = 1,
        /// <include file='doc\CombineMode.uex' path='docs/doc[@for="CombineMode.Union"]/*' />
        /// <devdoc>
        ///    The two clipping regions are combined by
        ///    taking the union of both.
        /// </devdoc>
        Union       = 2,
        /// <include file='doc\CombineMode.uex' path='docs/doc[@for="CombineMode.Xor"]/*' />
        /// <devdoc>
        ///    The two clipping regions are combined by
        ///    taking only the area enclosed by one or the other regions, but not both.
        /// </devdoc>
        Xor         = 3,
        /// <include file='doc\CombineMode.uex' path='docs/doc[@for="CombineMode.Exclude"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Two clipping regions are combined by taking
        ///       the area of the first region that does not intersect with the second.
        ///    </para>
        /// </devdoc>
        Exclude     = 4,
        /// <include file='doc\CombineMode.uex' path='docs/doc[@for="CombineMode.Complement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Two clipping regions are combined by taking
        ///       the area of the second region that does not intersect with the first.
        ///    </para>
        /// </devdoc>
        Complement  = 5
    }
}
