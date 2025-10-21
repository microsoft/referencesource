//------------------------------------------------------------------------------
// <copyright file="StatusBarPanelStyle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;


    /// <include file='doc\StatusBarPanelStyle.uex' path='docs/doc[@for="StatusBarPanelStyle"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies whether a panel on
    ///       a status bar is owner drawn or system drawn.
    ///    </para>
    /// </devdoc>
    public enum StatusBarPanelStyle {

        /// <include file='doc\StatusBarPanelStyle.uex' path='docs/doc[@for="StatusBarPanelStyle.Text"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The panel is
        ///       drawn by the system.
        ///    </para>
        /// </devdoc>
        Text        = 1,

        /// <include file='doc\StatusBarPanelStyle.uex' path='docs/doc[@for="StatusBarPanelStyle.OwnerDraw"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The panel is
        ///       drawn by the owner.
        ///    </para>
        /// </devdoc>
        OwnerDraw   = 2,

    }
}
