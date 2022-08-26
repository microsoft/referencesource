//------------------------------------------------------------------------------
// <copyright file="Appearance.cs" company="Microsoft">
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


    /// <include file='doc\Appearance.uex' path='docs/doc[@for="Appearance"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies
    ///       the appearance of a control.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public enum Appearance {

        /// <include file='doc\Appearance.uex' path='docs/doc[@for="Appearance.Normal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The default appearance defined by the control
        ///       class.
        ///    </para>
        /// </devdoc>
        Normal              = 0,

        /// <include file='doc\Appearance.uex' path='docs/doc[@for="Appearance.Button"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The appearance of a Windows
        ///       button.
        ///    </para>
        /// </devdoc>
        Button              = 1,

    }
}
