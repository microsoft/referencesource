//------------------------------------------------------------------------------
// <copyright file="RightToLeft.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using System.Diagnostics;

    using System.Drawing;

    /// <include file='doc\RightToLeft.uex' path='docs/doc[@for="RightToLeft"]/*' />
    /// <devdoc>
    ///    <para>Specifies a value indicating whether the text appears
    ///       from right to
    ///       left, as when using Hebrew or Arabic fonts.</para>
    /// </devdoc>
    public enum RightToLeft {

        /// <include file='doc\RightToLeft.uex' path='docs/doc[@for="RightToLeft.No"]/*' />
        /// <devdoc>
        ///    <para>
        ///       
        ///       The
        ///       
        ///       text reads
        ///       
        ///       from left to right. This is the default.
        ///       
        ///    </para>
        /// </devdoc>
        No = 0,

        /// <include file='doc\RightToLeft.uex' path='docs/doc[@for="RightToLeft.Yes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The text reads from
        ///       right to left.
        ///       
        ///    </para>
        /// </devdoc>
        Yes = 1,

        /// <include file='doc\RightToLeft.uex' path='docs/doc[@for="RightToLeft.Inherit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The direction the
        ///       text appears in is inherited from the parent control.
        ///       
        ///    </para>
        /// </devdoc>
        Inherit = 2
    }
}
