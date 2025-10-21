//------------------------------------------------------------------------------
// <copyright file="FormClosedEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;


    /// <include file='doc\FormClosedEvent.uex' path='docs/doc[@for="FormClosedEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///    Provides data for the <see cref='System.Windows.Forms.Form.OnClosed'/>,
    ///    <see cref='System.Windows.Forms.Form.OnClosed'/>
    ///    event.
    ///
    ///    </para>
    /// </devdoc>
    public class FormClosedEventArgs : EventArgs {
        private CloseReason closeReason;
        
        /// <include file='doc\FormClosedEvent.uex' path='docs/doc[@for="FormClosedEvent.FormClosedEventArgs"]/*' />
        public FormClosedEventArgs(CloseReason closeReason) {
            this.closeReason = closeReason;                                           
        }

        /// <include file='doc\FormClosedEvent.uex' path='docs/doc[@for="FormClosedEvent.CloseReason"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides the reason for the Form Close.
        ///    </para>
        /// </devdoc>
        public CloseReason CloseReason {
            get {
                return closeReason;
            }
        }
    }
}

