//------------------------------------------------------------------------------
// <copyright file="PrintPageEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\PrintPageEventHandler.uex' path='docs/doc[@for="PrintPageEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='E:System.Drawing.Printing.PrintDocument.PrintPage'/> event of a <see cref='System.Drawing.Printing.PrintDocument'/>.
    ///    </para>
    /// </devdoc>
    public delegate void PrintPageEventHandler(object sender, PrintPageEventArgs e);
}
