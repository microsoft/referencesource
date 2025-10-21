//------------------------------------------------------------------------------
// <copyright file="HelpEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.Drawing;

    /// <include file='doc\HelpEventHandler.uex' path='docs/doc[@for="HelpEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the Help event of a Control.
    ///    </para>
    /// </devdoc>
    public delegate void HelpEventHandler(object sender, HelpEventArgs hlpevent);
}
