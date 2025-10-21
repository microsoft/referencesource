//------------------------------------------------------------------------------
// <copyright file="TreeViewAction.cs" company="Microsoft">
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


    /// <include file='doc\TreeViewAction.uex' path='docs/doc[@for="TreeViewAction"]/*' />
    /// <devdoc>
    ///     This enum is used to specify the action that caused a TreeViewEventArgs.
    /// </devdoc>
    public enum TreeViewAction {

        /// <include file='doc\TreeViewAction.uex' path='docs/doc[@for="TreeViewAction.Unknown"]/*' />
        /// <devdoc>
        ///     The action is unknown.
        /// </devdoc>
        Unknown = 0,

        /// <include file='doc\TreeViewAction.uex' path='docs/doc[@for="TreeViewAction.ByKeyboard"]/*' />
        /// <devdoc>
        ///     The event was caused by a keystroke.
        /// </devdoc>
        ByKeyboard = 1,

        /// <include file='doc\TreeViewAction.uex' path='docs/doc[@for="TreeViewAction.ByMouse"]/*' />
        /// <devdoc>
        ///     The event was caused by a mouse click.
        /// </devdoc>
        ByMouse = 2,
        
        /// <include file='doc\TreeViewAction.uex' path='docs/doc[@for="TreeViewAction.Collapse"]/*' />
        /// <devdoc>
        ///     The tree node is collapsing.
        /// </devdoc>
        Collapse = 3,
        
        /// <include file='doc\TreeViewAction.uex' path='docs/doc[@for="TreeViewAction.Expand"]/*' />
        /// <devdoc>
        ///     The tree node is expanding.
        /// </devdoc>
        Expand = 4,
    }
}
