//------------------------------------------------------------------------------
// <copyright file="PaintEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.Drawing;


    /// <include file='doc\PaintEventHandler.uex' path='docs/doc[@for="PaintEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='System.Windows.Forms.Control.Paint'/>event of a <see cref='System.Windows.Forms.Control'/>class.
    ///    </para>
    /// </devdoc>
    public delegate void PaintEventHandler(object sender, PaintEventArgs e);
}
