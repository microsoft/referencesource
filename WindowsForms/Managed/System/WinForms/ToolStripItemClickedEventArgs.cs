//------------------------------------------------------------------------------
// <copyright file="ToolStripItemClickedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    /// <include file='doc\ToolStripItemClickedEventArgs.uex' path='docs/doc[@for="ToolStripItemClickedEventArgs"]/*' />
    public class ToolStripItemClickedEventArgs : EventArgs {
        private ToolStripItem  clickedItem = null;

        /// <include file='doc\ToolStripItemClickedEventArgs.uex' path='docs/doc[@for="ToolStripItemClickedEventArgs.ToolStripItemClickedEventArgs"]/*' />
        /// <devdoc>
        /// This class represents event args a ToolStrip can use when an item has been clicked.
        /// </devdoc>
        public ToolStripItemClickedEventArgs(ToolStripItem clickedItem) {

            this.clickedItem = clickedItem;
        }


        /// <include file='doc\ToolStripItemClickedEventArgs.uex' path='docs/doc[@for="ToolStripItemClickedEventArgs.ClickedItem"]/*' />
        /// <devdoc>
        /// Represents the item that was clicked on the toolStrip.
        /// </devdoc>
        public ToolStripItem ClickedItem {
            get {
                return clickedItem;    
            }
        }

    }
}
