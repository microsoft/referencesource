//------------------------------------------------------------------------------
// <copyright file="ScrollEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;


    /// <include file='doc\ScrollEventHandler.uex' path='docs/doc[@for="ScrollEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that handles the
    ///    <see langword='Scroll'/> event of a <see cref='System.Windows.Forms.ScrollBar'/>, <see cref='System.Windows.Forms.TrackBar'/>, or 
    ///    <see cref='System.Windows.Forms.DataGrid'/>.
    ///       
    ///    </para>
    /// </devdoc>
    public delegate void ScrollEventHandler(object sender, ScrollEventArgs e);
}
