//------------------------------------------------------------------------------
// <copyright file="ListViewItemMouseHoverEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using Microsoft.Win32;


    /// <include file='doc\ListViewMouseHoverEvent.uex' path='docs/doc[@for="ListViewMouseHoverEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='System.Windows.Forms.ListView.OnItemMouseHover'/> event.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public class ListViewItemMouseHoverEventArgs : EventArgs {
        readonly ListViewItem item;

        /// <include file='doc\ItemMouseHoverEvent.uex' path='docs/doc[@for="ListViewItemMouseHoverEventArgs.ItemMouseHoverEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListViewItemMouseHoverEventArgs(ListViewItem item) {
            this.item = item;
        }
        
        /// <include file='doc\ItemMouseHoverEvent.uex' path='docs/doc[@for="ListViewItemMouseHoverEventArgs.Item"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListViewItem Item {
            get { return item; }
        }
    }
}
