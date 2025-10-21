//------------------------------------------------------------------------------
// <copyright file="ItemDragEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;


    /// <include file='doc\ItemDragEvent.uex' path='docs/doc[@for="ItemDragEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='System.Windows.Forms.ListView.OnItemDrag'/> event.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public class ItemDragEventArgs : EventArgs {
        readonly MouseButtons button;
        readonly object item;

        /// <include file='doc\ItemDragEvent.uex' path='docs/doc[@for="ItemDragEventArgs.ItemDragEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ItemDragEventArgs(MouseButtons button) {
            this.button = button;
            this.item = null;
        }
        
        /// <include file='doc\ItemDragEvent.uex' path='docs/doc[@for="ItemDragEventArgs.ItemDragEventArgs1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ItemDragEventArgs(MouseButtons button, object item) {
            this.button = button;
            this.item = item;
        }
        
        /// <include file='doc\ItemDragEvent.uex' path='docs/doc[@for="ItemDragEventArgs.Button"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MouseButtons Button {
            get { return button; }
        }

        /// <include file='doc\ItemDragEvent.uex' path='docs/doc[@for="ItemDragEventArgs.Item"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object Item {
            get { return item; }
        }
    }
}
