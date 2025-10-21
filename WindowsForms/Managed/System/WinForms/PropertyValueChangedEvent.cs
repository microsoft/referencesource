//------------------------------------------------------------------------------
// <copyright file="PropertyValueChangedEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using System.Diagnostics;
    using System;
    using System.ComponentModel;

    /// <include file='doc\PropertyValueChangedEvent.uex' path='docs/doc[@for="PropertyValueChangedEventArgs"]/*' />
    /// <devdoc>
    /// The event class that is created when a property
    /// in the grid is modified by the user.
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public class PropertyValueChangedEventArgs : EventArgs {
        private readonly GridItem changedItem;
        private object oldValue;
                /// <include file='doc\PropertyValueChangedEvent.uex' path='docs/doc[@for="PropertyValueChangedEventArgs.PropertyValueChangedEventArgs"]/*' />
                /// <devdoc>
        /// Constructor
        /// </devdoc>
        public PropertyValueChangedEventArgs(GridItem changedItem, object oldValue) {
            this.changedItem = changedItem;
            this.oldValue = oldValue;
        }

        /// <include file='doc\PropertyValueChangedEvent.uex' path='docs/doc[@for="PropertyValueChangedEventArgs.ChangedItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public GridItem ChangedItem {
            get {
                return changedItem;
            }
        }
        
        /// <include file='doc\PropertyValueChangedEvent.uex' path='docs/doc[@for="PropertyValueChangedEventArgs.OldValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object OldValue {
            get {
                return oldValue;
            }
        }
    }
}
