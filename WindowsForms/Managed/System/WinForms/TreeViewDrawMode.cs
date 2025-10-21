//------------------------------------------------------------------------------
// <copyright file="TreeViewDrawMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
namespace System.Windows.Forms {

    /// <include file='doc\TreeViewDrawMode.uex' path='docs/doc[@for="TreeViewDrawMode"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies responsibility for drawing TreeView nodes.
    ///    </para>
    /// </devdoc>

    public enum TreeViewDrawMode {
        /// <include file='doc\TreeViewDrawMode.uex' path='docs/doc[@for="TreeViewDrawMode.Normal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The operating system paints the nodes of the TreeView.
        ///
        ///    </para>
        /// </devdoc>
        Normal = 0,

        /// <include file='doc\TreeViewDrawMode.uex' path='docs/doc[@for="TreeViewDrawMode.OwnerDrawText"]/*' />
        /// <devdoc>
        ///    <para>
        ///	The user needs to paint the text only.
        ///    </para>
        /// </devdoc>
        OwnerDrawText = 1,

        /// <include file='doc\TreeViewDrawMode.uex' path='docs/doc[@for="TreeViewDrawMode.OwnerDrawAll"]/*' />
        /// <devdoc>
        ///    <para>
        ///	The user paints the entire row corresponding to a node, including lines and boxes.
        ///    </para>
        /// </devdoc>
        OwnerDrawAll = 2,
    }
}
