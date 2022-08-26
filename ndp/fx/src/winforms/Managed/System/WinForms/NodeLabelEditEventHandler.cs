//------------------------------------------------------------------------------
// <copyright file="NodeLabelEditEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;


    /// <include file='doc\NodeLabelEditEventHandler.uex' path='docs/doc[@for="NodeLabelEditEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='System.Windows.Forms.TreeView.OnBeforeLabelEdit'/> or <see cref='System.Windows.Forms.TreeView.OnAfterLabelEdit'/>
    ///       
    ///       event of a <see cref='System.Windows.Forms.TreeView'/>.
    ///       
    ///    </para>
    /// </devdoc>
    public delegate void NodeLabelEditEventHandler(object sender, NodeLabelEditEventArgs e);
}
