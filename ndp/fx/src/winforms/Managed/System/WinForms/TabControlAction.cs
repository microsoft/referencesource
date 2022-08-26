//------------------------------------------------------------------------------
// <copyright file="TabControlAction.cs" company="Microsoft">
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


    /// <include file='doc\TabControlAction.uex' path='docs/doc[@for="TabControlAction"]/*' />
    /// <devdoc>
    ///     This enum is used to specify the action that caused a TreeViewEventArgs.
    /// </devdoc>
    public enum TabControlAction {

        /// <include file='doc\TabControlAction.uex' path='docs/doc[@for="TabControlAction.Selecting"]/*' />
        Selecting,
        /// <include file='doc\TabControlAction.uex' path='docs/doc[@for="TabControlAction.Selected"]/*' />
        Selected,
        /// <include file='doc\TabControlAction.uex' path='docs/doc[@for="TabControlAction.Deselecting"]/*' />
        Deselecting,
        /// <include file='doc\TabControlAction.uex' path='docs/doc[@for="TabControlAction.Deselected"]/*' />
        Deselected
    }
}
