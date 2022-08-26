//------------------------------------------------------------------------------
// <copyright file="BindingManagerDataErrorEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.ComponentModel;

    /// <include file='doc\BindingManagerDataErrorEventArgs.uex' path='docs/doc[@for="BindingManagerDataErrorEventArgs"]/*' />
    /// <devdoc>
    /// </devdoc>
    public class BindingManagerDataErrorEventArgs : EventArgs {
        private	Exception exception;

        /// <include file='doc\BindingManagerDataErrorEventArgs.uex' path='docs/doc[@for="BindingManagerDataErrorEventArgs.BindingManagerDataErrorEventArgs"]/*' />
        /// <devdoc>
        /// </devdoc>
        public BindingManagerDataErrorEventArgs(Exception exception) {
            this.exception = exception;
        }

        /// <include file='doc\BindingManagerDataErrorEventArgs.uex' path='docs/doc[@for="BindingManagerDataErrorEventArgs.Exception"]/*' />
        /// <devdoc>
        /// </devdoc>
        public Exception Exception
        {
            get {
                return this.exception;
            }
        }
    }
}
