//------------------------------------------------------------------------------
// <copyright file="StatusBarPanelClickEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;


    /// <include file='doc\StatusBarPanelClickEventHandler.uex' path='docs/doc[@for="StatusBarPanelClickEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='System.Windows.Forms.StatusBar.OnPanelClick'/>
    ///       event of a <see cref='System.Windows.Forms.StatusBar'/>.
    ///    </para>
    /// </devdoc>
    public delegate void StatusBarPanelClickEventHandler(object sender, StatusBarPanelClickEventArgs e);
}
