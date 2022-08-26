//------------------------------------------------------------------------------
// <copyright file="FormClosingEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;


    /// <include file='doc\FormClosingEvent.uex' path='docs/doc[@for="FormClosingEvent"]/*' />
    /// <devdoc>
    ///    <para>
    ///    Provides data for the <see cref='System.Windows.Forms.Form.OnClosing'/>,
    ///    <see cref='System.Windows.Forms.Form.OnClosing'/>
    ///    event.
    ///    </para>
    /// </devdoc>
    public class FormClosingEventArgs : CancelEventArgs {
        private CloseReason closeReason;
        
        /// <include file='doc\FormClosingEvent.uex' path='docs/doc[@for="FormClosingEventArgs.FormClosingEventArgs"]/*' />
        public FormClosingEventArgs(CloseReason closeReason, bool cancel)
        : base(cancel) {
            this.closeReason = closeReason;                                           
        }

        /// <include file='doc\FormClosingEvent.uex' path='docs/doc[@for="FormClosingEvent.CloseReason"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides the reason for the Form close.
        ///    </para>
        /// </devdoc>
        public CloseReason CloseReason {
            get {
                return closeReason;
            }
        }
    }
}

