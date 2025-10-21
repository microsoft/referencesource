//------------------------------------------------------------------------------
// <copyright file="StatusBarDrawItemEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;


    /// <include file='doc\StatusBarDrawItemEventHandler.uex' path='docs/doc[@for="StatusBarDrawItemEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='System.Windows.Forms.StatusBar.OnDrawItem'/>
    ///       event of a <see cref='System.Windows.Forms.StatusBar'/>.
    ///    </para>
    /// </devdoc>
    public delegate void StatusBarDrawItemEventHandler(object sender, StatusBarDrawItemEventArgs sbdevent);
}
