//------------------------------------------------------------------------------
// <copyright file="PrintAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using System.Diagnostics;
    using System;
    using System.Runtime.InteropServices;
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\PrintAction.uex' path='docs/doc[@for="PrintAction"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the type of action for the <see cref='System.Drawing.Printing.PrintEventArgs'/>.
    ///    </para>
    /// </devdoc>
    public enum PrintAction {
        /// <include file='doc\PrintAction.uex' path='docs/doc[@for="PrintAction.PrintToFile"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Printing to a file.
        ///    </para>
        /// </devdoc>
        PrintToFile,
        /// <include file='doc\PrintAction.uex' path='docs/doc[@for="PrintAction.PrintToPreview"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Printing to a preview.
        ///    </para>
        /// </devdoc>
        PrintToPreview,
        /// <include file='doc\PrintAction.uex' path='docs/doc[@for="PrintAction.PrintToPrinter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Printing to a printer.
        ///    </para>
        /// </devdoc>
        PrintToPrinter
    }
}
