//------------------------------------------------------------------------------
// <copyright file="ToolStripItemEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    /// <include file='doc\ToolStripItemEventArgs.uex' path='docs/doc[@for="ToolStripItemEventArgs"]/*' />
    public class  ToolStripItemEventArgs : EventArgs {

        private ToolStripItem item;

        /// <include file='doc\ToolStripItemEventArgs.uex' path='docs/doc[@for="ToolStripItemEventArgs.ToolStripItemEventArgs"]/*' />
        public ToolStripItemEventArgs(ToolStripItem item) {
           this.item = item;
        }

        /// <include file='doc\ToolStripItemEventArgs.uex' path='docs/doc[@for="ToolStripItemEventArgs.Item"]/*' />
        public ToolStripItem Item {
            get {
                return item;
            }
        }
    }
}
