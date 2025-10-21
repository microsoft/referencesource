//------------------------------------------------------------------------------
// <copyright file="TreeViewCancelEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;


    /// <include file='doc\TreeViewCancelEventHandler.uex' path='docs/doc[@for="TreeViewCancelEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will
    ///       handle the <see cref='System.Windows.Forms.TreeView.OnBeforeCheck'/>, <see cref='System.Windows.Forms.TreeView.OnBeforeCollapse'/>, <see cref='System.Windows.Forms.TreeView.BeforeExpand'/>, or <see cref='System.Windows.Forms.TreeView.BeforeSelect'/> event of a <see cref='System.Windows.Forms.TreeView'/>
    ///       .
    ///       
    ///    </para>
    /// </devdoc>
    public delegate void TreeViewCancelEventHandler(object sender, TreeViewCancelEventArgs e);
}
