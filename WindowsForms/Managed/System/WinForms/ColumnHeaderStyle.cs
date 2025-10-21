//------------------------------------------------------------------------------
// <copyright file="ColumnHeaderStyle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Runtime.Remoting;

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\ColumnHeaderStyle.uex' path='docs/doc[@for="ColumnHeaderStyle"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies how <see cref='System.Windows.Forms.ListView'/> column headers behave.
    ///    </para>
    /// </devdoc>
    public enum ColumnHeaderStyle {

        /// <include file='doc\ColumnHeaderStyle.uex' path='docs/doc[@for="ColumnHeaderStyle.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No visible column header.
        ///    </para>
        /// </devdoc>
        None         = 0,
        /// <include file='doc\ColumnHeaderStyle.uex' path='docs/doc[@for="ColumnHeaderStyle.Nonclickable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Visible column header that does not respond to clicking.
        ///    </para>
        /// </devdoc>
        Nonclickable = 1,
        /// <include file='doc\ColumnHeaderStyle.uex' path='docs/doc[@for="ColumnHeaderStyle.Clickable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Visible column header that responds to clicking.
        ///    </para>
        /// </devdoc>
        Clickable    = 2,

    }
}
