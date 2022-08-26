//------------------------------------------------------------------------------
// <copyright file="PrintEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\PrintEventHandler.uex' path='docs/doc[@for="PrintEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='E:System.Drawing.Printing.PrintDocument.BeginPrint'/>,
    ///    <see cref='E:System.Drawing.Printing.PrintDocument.EndPrint'/>, or <see cref='E:System.Drawing.Printing.PrintDocument.QueryPageSettings'/> event of a <see cref='System.Drawing.Printing.PrintDocument'/>.
    ///    </para>
    /// </devdoc>
    public delegate void PrintEventHandler(object sender, PrintEventArgs e);
}

