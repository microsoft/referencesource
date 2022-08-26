//------------------------------------------------------------------------------
// <copyright file="FrameStyle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;

    /// <include file='doc\FrameStyle.uex' path='docs/doc[@for="FrameStyle"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the frame style of the selected control.
    ///    </para>
    /// </devdoc>
    public enum FrameStyle {

        /// <include file='doc\FrameStyle.uex' path='docs/doc[@for="FrameStyle.Dashed"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A thin, dashed border.
        ///    </para>
        /// </devdoc>
        Dashed,

        /// <include file='doc\FrameStyle.uex' path='docs/doc[@for="FrameStyle.Thick"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A thick, solid border.
        ///    </para>
        /// </devdoc>
        Thick,
    }
}
