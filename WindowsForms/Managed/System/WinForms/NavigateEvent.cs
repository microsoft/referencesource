//------------------------------------------------------------------------------
// <copyright file="NavigateEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;
    using System;
    using System.ComponentModel;

    /// <include file='doc\NavigateEvent.uex' path='docs/doc[@for="NavigateEventArgs"]/*' />
    /// <devdoc>
    ///
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public class NavigateEventArgs : EventArgs {
        private bool isForward = true;

        /// <include file='doc\NavigateEvent.uex' path='docs/doc[@for="NavigateEventArgs.Forward"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Forward {
            get {
                return isForward;
            }
        }

        /// <include file='doc\NavigateEvent.uex' path='docs/doc[@for="NavigateEventArgs.NavigateEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public NavigateEventArgs(bool isForward) {
            this.isForward = isForward;
        }
    }
}
