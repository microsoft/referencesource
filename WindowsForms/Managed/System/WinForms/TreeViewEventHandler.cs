//------------------------------------------------------------------------------
// <copyright file="TreeViewEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;


    /// <include file='doc\TreeViewEventHandler.uex' path='docs/doc[@for="TreeViewEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='System.Windows.Forms.TreeView.OnAfterCheck'/>, <see cref='System.Windows.Forms.TreeView.OnAfterCollapse'/>, <see cref='System.Windows.Forms.TreeView.OnAfterExpand'/>, or <see cref='System.Windows.Forms.TreeView.OnAfterSelect'/>
    ///       event of a <see cref='System.Windows.Forms.TreeView'/>
    ///       .
    ///    </para>
    /// </devdoc>
    public delegate void TreeViewEventHandler(object sender, TreeViewEventArgs e);
}
