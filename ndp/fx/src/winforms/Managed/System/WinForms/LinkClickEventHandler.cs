//------------------------------------------------------------------------------
// <copyright file="LinkClickEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;


    /// <include file='doc\LinkClickEventHandler.uex' path='docs/doc[@for="LinkClickedEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle 
    ///       the <see cref='System.Windows.Forms.RichTextBox.LinkClicked'/> event of 
    ///       a <see cref='System.Windows.Forms.RichTextBox'/>.
    ///    </para>
    /// </devdoc>
    public delegate void LinkClickedEventHandler(object sender, LinkClickedEventArgs e);
}

