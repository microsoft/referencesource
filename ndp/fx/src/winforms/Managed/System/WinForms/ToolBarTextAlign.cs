//------------------------------------------------------------------------------
// <copyright file="ToolBarTextAlign.cs" company="Microsoft">
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



    /// <include file='doc\ToolBarTextAlign.uex' path='docs/doc[@for="ToolBarTextAlign"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies
    ///       the alignment of text on the toolbar button control.
    ///    </para>
    /// </devdoc>
    public enum ToolBarTextAlign {

        /// <include file='doc\ToolBarTextAlign.uex' path='docs/doc[@for="ToolBarTextAlign.Underneath"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The text
        ///       is aligned underneath the toolbar button image.
        ///    </para>
        /// </devdoc>
        Underneath = 0,

        /// <include file='doc\ToolBarTextAlign.uex' path='docs/doc[@for="ToolBarTextAlign.Right"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The text
        ///       is aligned to the right of the toolbar button image.
        ///    </para>
        /// </devdoc>
        Right = 1,

    }
}
