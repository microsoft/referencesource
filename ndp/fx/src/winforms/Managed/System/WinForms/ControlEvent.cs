//------------------------------------------------------------------------------
// <copyright file="ControlEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;


    /// <include file='doc\ControlEvent.uex' path='docs/doc[@for="ControlEventArgs"]/*' />
    /// <devdoc>
    ///      A ControlEventArgs is an event that has a control
    ///      as a property.
    /// </devdoc>
    public class ControlEventArgs : EventArgs {
        private Control control;

        /// <include file='doc\ControlEvent.uex' path='docs/doc[@for="ControlEventArgs.Control"]/*' />
        /// <devdoc>
        ///      Retrieves the control object stored in this event.
        /// </devdoc>
        public Control Control {
            get {
                return control;
            }
        }

        /// <include file='doc\ControlEvent.uex' path='docs/doc[@for="ControlEventArgs.ControlEventArgs"]/*' />
        /// <devdoc>
        ///      Creates a new ControlEventArgs.
        /// </devdoc>
        public ControlEventArgs(Control control) {
            this.control = control;
        }
    }
}

