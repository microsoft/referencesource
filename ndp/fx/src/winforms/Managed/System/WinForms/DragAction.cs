//------------------------------------------------------------------------------
// <copyright file="DragAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;


    /// <include file='doc\DragAction.uex' path='docs/doc[@for="DragAction"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies how and if a drag-and-drop operation should continue.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public enum DragAction {
        /// <include file='doc\DragAction.uex' path='docs/doc[@for="DragAction.Continue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The operation will continue.
        ///    </para>
        /// </devdoc>
        Continue = 0,
        /// <include file='doc\DragAction.uex' path='docs/doc[@for="DragAction.Drop"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The operation will stop with a drop.
        ///    </para>
        /// </devdoc>
        Drop = 1,
        /// <include file='doc\DragAction.uex' path='docs/doc[@for="DragAction.Cancel"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The operation is canceled with no
        ///       drop message.
        ///       
        ///    </para>
        /// </devdoc>
        Cancel = 2,

    }
}
