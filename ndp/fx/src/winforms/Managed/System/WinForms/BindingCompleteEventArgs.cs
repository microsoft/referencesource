//------------------------------------------------------------------------------
// <copyright file="BindingCompleteEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.ComponentModel;

    /// <include file='doc\BindingCompleteEventArgs.uex' path='docs/doc[@for="BindingCompleteEventArgs"]/*' />
    /// <devdoc>
    ///     Provides information about a Binding Completed event.
    /// </devdoc>
    public class BindingCompleteEventArgs : CancelEventArgs {
        private Binding binding;
        private BindingCompleteState state;
        private BindingCompleteContext context;
        private	string errorText;
        private	Exception exception;

        /// <include file='doc\BindingCompleteEventArgs.uex' path='docs/doc[@for="BindingCompleteEventArgs.BindingCompleteEventArgs"]/*' />
        /// <devdoc>
        ///    Constructor for BindingCompleteEventArgs.
        /// </devdoc>
        public BindingCompleteEventArgs(Binding binding,
                                        BindingCompleteState state,
                                        BindingCompleteContext context,
                                        string errorText,
                                        Exception exception,
                                        bool cancel) : base(cancel) {
            this.binding = binding;
            this.state = state;
            this.context = context;
            this.errorText = (errorText == null) ? string.Empty : errorText;
            this.exception = exception;
        }

        /// <include file='doc\BindingCompleteEventArgs.uex' path='docs/doc[@for="BindingCompleteEventArgs.BindingCompleteEventArgs1"]/*' />
        /// <devdoc>
        ///    Constructor for BindingCompleteEventArgs.
        /// </devdoc>
        public BindingCompleteEventArgs(Binding binding,
                                        BindingCompleteState state,
                                        BindingCompleteContext context,
                                        string errorText,
                                        Exception exception) : this(binding, state, context, errorText, exception, true) {
        }

        /// <include file='doc\BindingCompleteEventArgs.uex' path='docs/doc[@for="BindingCompleteEventArgs.BindingCompleteEventArgs2"]/*' />
        /// <devdoc>
        ///    Constructor for BindingCompleteEventArgs.
        /// </devdoc>
        public BindingCompleteEventArgs(Binding binding,
                                        BindingCompleteState state,
                                        BindingCompleteContext context,
                                        string errorText) : this(binding, state, context, errorText, null, true) {
        }

        /// <include file='doc\BindingCompleteEventArgs.uex' path='docs/doc[@for="BindingCompleteEventArgs.BindingCompleteEventArgs3"]/*' />
        /// <devdoc>
        ///    Constructor for BindingCompleteEventArgs.
        /// </devdoc>
        public BindingCompleteEventArgs(Binding binding,
                                        BindingCompleteState state,
                                        BindingCompleteContext context) : this(binding, state, context, string.Empty, null, false) {
        }

        /// <include file='doc\BindingCompleteEventArgs.uex' path='docs/doc[@for="BindingCompleteEventArgs.Binding"]/*' />
        /// <devdoc>
        /// </devdoc>
        public Binding Binding {
            get {
                return this.binding;
            }
        }

        /// <include file='doc\BindingCompleteEventArgs.uex' path='docs/doc[@for="BindingCompleteEventArgs.BindingCompleteState"]/*' />
        /// <devdoc>
        /// </devdoc>
        public BindingCompleteState BindingCompleteState {
            get {
                return this.state;
            }
        }

        /// <include file='doc\BindingCompleteEventArgs.uex' path='docs/doc[@for="BindingCompleteEventArgs.BindingCompleteContext"]/*' />
        /// <devdoc>
        /// </devdoc>
        public BindingCompleteContext BindingCompleteContext {
            get {
                return this.context;
            }
        }

        /// <include file='doc\BindingCompleteEventArgs.uex' path='docs/doc[@for="BindingCompleteEventArgs.ErrorText"]/*' />
        /// <devdoc>
        /// </devdoc>
        public string ErrorText {
            get {
                return this.errorText;
            }
        }

        /// <include file='doc\BindingCompleteEventArgs.uex' path='docs/doc[@for="BindingCompleteEventArgs.Exception"]/*' />
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
