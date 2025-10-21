//------------------------------------------------------------------------------
// <copyright file="ItemCheckedEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\ItemCheckedEvent.uex' path='docs/doc[@for="ItemCheckedEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='System.Windows.Forms.CheckedListBox.ItemCheck'/>
    ///       event.
    ///
    ///    </para>
    /// </devdoc>
    
    public class ItemCheckedEventArgs : EventArgs {
        private ListViewItem lvi;


        /// <include file='doc\ItemCheckEvent.uex' path='docs/doc[@for="ItemCheckedEventArgs.ItemCheckedEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ItemCheckedEventArgs(ListViewItem item) {
            this.lvi = item;
        }
        /// <include file='doc\ItemCheckEvent.uex' path='docs/doc[@for="ItemCheckedEventArgs.Item"]/*' />
        /// <devdoc>
        ///     The index of the item that is about to change.
        /// </devdoc>
        public ListViewItem Item {
            get { return lvi; }
        }
    }
}
