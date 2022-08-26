//------------------------------------------------------------------------------
// <copyright file="PrintEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using System.Diagnostics;
    using System;
    using System.Drawing;
    using Microsoft.Win32;
    using System.ComponentModel;

    /// <include file='doc\PrintEvent.uex' path='docs/doc[@for="PrintEventArgs"]/*' />    
    /// <devdoc>
    /// <para>Provides data for the <see cref='E:System.Drawing.Printing.PrintDocument.BeginPrint'/> and
    /// <see cref='E:System.Drawing.Printing.PrintDocument.EndPrint'/> events.</para>
    /// </devdoc>
    public class PrintEventArgs : CancelEventArgs {
        private PrintAction printAction;
        
        /// <include file='doc\PrintEvent.uex' path='docs/doc[@for="PrintEventArgs.PrintEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Printing.PrintEventArgs'/> class.
        ///    </para>
        /// </devdoc>
        public PrintEventArgs() {
        }

        /// <include file='doc\PrintEvent.uex' path='docs/doc[@for="PrintEventArgs.PrintEventArgs1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Printing.PrintEventArgs'/> class.
        ///    </para>
        /// </devdoc>
        internal PrintEventArgs(PrintAction action) {
            this.printAction = action;
        }

        /// <include file='doc\PrintEvent.uex' path='docs/doc[@for="PrintEventArgs.PrintAction"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies which <see cref='System.Drawing.Printing.PrintAction'/> is causing this event.
        ///    </para>
        /// </devdoc>
        public PrintAction PrintAction {
            get {
                return printAction;
            }
        }
    }
}

