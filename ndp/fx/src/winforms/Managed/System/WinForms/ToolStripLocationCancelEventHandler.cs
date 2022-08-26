//------------------------------------------------------------------------------
// <copyright file="ToolStripLocationCancelEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;

    /// <include file='doc\ToolStripLocationCancelEventHandler.uex' path='docs/doc[@for="ToolStripLocationCancelEventHandler"]/*' />
    /// <devdoc>
    ///    <para>Represents the method that will handle the event raised when canceling an
    ///       OnLocationChanging event for ToolStrips.</para>
    /// </devdoc>
    internal delegate void ToolStripLocationCancelEventHandler(object sender, ToolStripLocationCancelEventArgs e);
}

