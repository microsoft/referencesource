//------------------------------------------------------------------------------
// <copyright file="InvalidateEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.Drawing;


    /// <include file='doc\InvalidateEventHandler.uex' path='docs/doc[@for="InvalidateEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='System.Windows.Forms.Control.Invalidate'/> event
    ///       of a <see cref='System.Windows.Forms.Control'/>.
    ///    </para>
    /// </devdoc>
    public delegate void InvalidateEventHandler(object sender, InvalidateEventArgs e);
}
