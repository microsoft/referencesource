//------------------------------------------------------------------------------
// <copyright file="MouseEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.Drawing;


    /// <include file='doc\MouseEventHandler.uex' path='docs/doc[@for="MouseEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the
    ///    <see langword='MouseDown'/>, <see langword='MouseUp'/>, or <see langword='MouseMove '/>event of a form, control, or other component.
    ///    </para>
    /// </devdoc>
    public delegate void MouseEventHandler(object sender, MouseEventArgs e);
}
