
//------------------------------------------------------------------------------
// <copyright file="Orientation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;



    /// <include file='doc\Orientation.uex' path='docs/doc[@for="Orientation"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the Fixed Panel in the SplitContainer Control.
    ///       
    ///    </para>
    /// </devdoc>
    public enum FixedPanel {
        /// <include file='doc\FixedPanel.uex' path='docs/doc[@for="FixedPanel.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No panel is fixed. Resize causes the Resize of both the panels.
        ///    </para>
        /// </devdoc>
        None = 0,

        /// <include file='doc\FixedPanel.uex' path='docs/doc[@for="FixedPanel.Panel1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Panel1 is Fixed. The resize will increase the size of second panel.
        ///    </para>
        /// </devdoc>
        Panel1 = 1,

        /// <include file='doc\FixedPanel.uex' path='docs/doc[@for="FixedPanel.Panel2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Panel2 is Fixed. The resize will increase the size of first panel.
        ///    </para>
        /// </devdoc>
        Panel2 = 2,

    }
}
