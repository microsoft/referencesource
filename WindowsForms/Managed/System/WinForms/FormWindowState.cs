//------------------------------------------------------------------------------
// <copyright file="FormWindowState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Drawing;



    /// <include file='doc\FormWindowState.uex' path='docs/doc[@for="FormWindowState"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies how a form window
    ///       is displayed.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public enum FormWindowState {

        /// <include file='doc\FormWindowState.uex' path='docs/doc[@for="FormWindowState.Normal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A default sized window.
        ///       
        ///    </para>
        /// </devdoc>
        Normal = 0,

        /// <include file='doc\FormWindowState.uex' path='docs/doc[@for="FormWindowState.Minimized"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A minimized window.
        ///
        ///    </para>
        /// </devdoc>
        Minimized = 1,

        /// <include file='doc\FormWindowState.uex' path='docs/doc[@for="FormWindowState.Maximized"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A maximized window.
        ///    </para>
        /// </devdoc>
        Maximized = 2,


    }
}
