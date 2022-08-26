//------------------------------------------------------------------------------
// <copyright file="TreeNodeMouseHoverDragEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using Microsoft.Win32;


    /// <include file='doc\TreeNodeMouseHoverEvent.uex' path='docs/doc[@for="TreeNodeMouseHoverEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='System.Windows.Forms.TreeView.OnNodeMouseHover'/> event.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public class TreeNodeMouseHoverEventArgs : EventArgs {
        readonly TreeNode node;

        /// <include file='doc\TreeNodeMouseHoverEvent.uex' path='docs/doc[@for="TreeNodeMouseHoverEventArgs.TreeNodeMouseHoverEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TreeNodeMouseHoverEventArgs(TreeNode node) {
            this.node = node;
        }
        
        /// <include file='doc\TreeNodeMouseHoverEvent.uex' path='docs/doc[@for="TreeNodeMouseHoverEventArgs.Node"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TreeNode Node {
            get { return node; }
        }
    }
}
