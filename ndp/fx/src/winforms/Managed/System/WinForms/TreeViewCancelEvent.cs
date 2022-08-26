//------------------------------------------------------------------------------
// <copyright file="TreeViewCancelEvent.cs" company="Microsoft">
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


    /// <include file='doc\TreeViewCancelEvent.uex' path='docs/doc[@for="TreeViewCancelEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='System.Windows.Forms.TreeView.OnBeforeCheck'/>,
    ///    <see cref='System.Windows.Forms.TreeView.OnBeforeCollapse'/>,
    ///    <see cref='System.Windows.Forms.TreeView.OnBeforeExpand'/>,
    ///       or <see cref='System.Windows.Forms.TreeView.OnBeforeSelect'/> event.
    ///
    ///    </para>
    /// </devdoc>
    public class TreeViewCancelEventArgs : CancelEventArgs {
        private TreeNode node;
        private TreeViewAction action;

        /// <include file='doc\TreeViewCancelEvent.uex' path='docs/doc[@for="TreeViewCancelEventArgs.TreeViewCancelEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TreeViewCancelEventArgs(TreeNode node, bool cancel, TreeViewAction action)
        : base(cancel) {
            this.node = node;                                           
            this.action = action;
        }

        /// <include file='doc\TreeViewCancelEvent.uex' path='docs/doc[@for="TreeViewCancelEventArgs.Node"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TreeNode Node {
            get {
                return node;
            }
        }
        
        /// <include file='doc\TreeViewCancelEvent.uex' path='docs/doc[@for="TreeViewCancelEventArgs.Action"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TreeViewAction Action {
            get {
                return action;
            }
        }
    }
}
