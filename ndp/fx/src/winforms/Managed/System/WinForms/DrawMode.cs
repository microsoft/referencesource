//------------------------------------------------------------------------------
// <copyright file="DrawMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;


    /// <include file='doc\DrawMode.uex' path='docs/doc[@for="DrawMode"]/*' />
    /// <devdoc>
    ///    <para>
    ///
    ///       Specifies responsibility for drawing a control or portion of a control.
    ///
    ///    </para>
    /// </devdoc>
    public enum DrawMode {
        /// <include file='doc\DrawMode.uex' path='docs/doc[@for="DrawMode.Normal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The
        ///       operating system paints the items in the control, and the items are each the
        ///       same height.
        ///
        ///    </para>
        /// </devdoc>
        Normal = 0,

        /// <include file='doc\DrawMode.uex' path='docs/doc[@for="DrawMode.OwnerDrawFixed"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The
        ///       programmer explicitly paints the items in the control, and the items are
        ///       each the same height.
        ///
        ///    </para>
        /// </devdoc>
        OwnerDrawFixed = 1,

        /// <include file='doc\DrawMode.uex' path='docs/doc[@for="DrawMode.OwnerDrawVariable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The programmer explicitly paints the items in the control manually, and they
        ///       may be different heights.
        ///    </para>
        /// </devdoc>
        OwnerDrawVariable = 2,
    }
}
