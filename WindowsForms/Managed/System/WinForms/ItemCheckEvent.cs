//------------------------------------------------------------------------------
// <copyright file="ItemCheckEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\ItemCheckEvent.uex' path='docs/doc[@for="ItemCheckEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='System.Windows.Forms.CheckedListBox.ItemCheck'/>
    ///       event.
    ///
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public class ItemCheckEventArgs : EventArgs {

        readonly int index;
        CheckState newValue;
        readonly CheckState currentValue;

        /// <include file='doc\ItemCheckEvent.uex' path='docs/doc[@for="ItemCheckEventArgs.ItemCheckEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ItemCheckEventArgs(int index, CheckState newCheckValue, CheckState currentValue) {
            this.index = index;
            this.newValue = newCheckValue;
            this.currentValue = currentValue;
        }
        /// <include file='doc\ItemCheckEvent.uex' path='docs/doc[@for="ItemCheckEventArgs.Index"]/*' />
        /// <devdoc>
        ///     The index of the item that is about to change.
        /// </devdoc>
        public int Index {
            get { return index; }
        }

        /// <include file='doc\ItemCheckEvent.uex' path='docs/doc[@for="ItemCheckEventArgs.NewValue"]/*' />
        /// <devdoc>
        ///     The proposed new value of the CheckBox.
        /// </devdoc>
        public CheckState NewValue {
            get { return newValue; }
            set { newValue = value; }
        }

        /// <include file='doc\ItemCheckEvent.uex' path='docs/doc[@for="ItemCheckEventArgs.CurrentValue"]/*' />
        /// <devdoc>
        ///     The current state of the CheckBox.
        /// </devdoc>
        public CheckState CurrentValue {
            get { return currentValue; }
        }

    }
}
