//------------------------------------------------------------------------------
// <copyright file="ToolTipIcon.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;


    /// <include file='doc\ToolTipIcon.uex' path='docs/doc[@for="ToolTipIcon"]/*' />
    public enum ToolTipIcon {

        /// <include file='doc\ToolTipIcon.uex' path='docs/doc[@for="ToolTipIcon.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No Icon.
        ///    </para>
        /// </devdoc>
        None = 0,

        /// <include file='doc\ToolTipIcon.uex' path='docs/doc[@for="ToolTipIcon.InfoIcon"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A Information Icon.
        ///    </para>
        /// </devdoc>
        Info = 1,

        /// <include file='doc\ToolTipIcon.uex' path='docs/doc[@for="ToolTipIcon.WarningIcon"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A Warning Icon.
        ///    </para>
        /// </devdoc>
        Warning = 2,


        /// <include file='doc\ToolTipIcon.uex' path='docs/doc[@for="ToolTipIcon.ErrorIcon"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A Error Icon.
        ///    </para>
        /// </devdoc>
        Error = 3

    }
}

