//------------------------------------------------------------------------------
// <copyright file="KeyPressEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.Drawing;


    /// <include file='doc\KeyPressEventHandler.uex' path='docs/doc[@for="KeyPressEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a method that will handle the <see cref='System.Windows.Forms.Control.KeyPress'/> event of a
    ///    <see cref='System.Windows.Forms.Control'/>.
    ///    </para>
    /// </devdoc>
    public delegate void KeyPressEventHandler(object sender, KeyPressEventArgs e);
}
