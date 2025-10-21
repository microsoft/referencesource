//------------------------------------------------------------------------------
// <copyright file="ToolStripSeparatorRenderEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Drawing;
    /// <include file='doc\ToolStripSeparatorRenderEventArgs.uex' path='docs/doc[@for="ToolStripSeparatorRenderEventArgs"]/*' />
    /// <devdoc>
    /// This class represents all the information to render the winbar
    /// </devdoc>
    public class ToolStripSeparatorRenderEventArgs : ToolStripItemRenderEventArgs {

        private bool vertical = false;

        /// <include file='doc\ToolStripSeparatorRenderEventArgs.uex' path='docs/doc[@for="ToolStripSeparatorRenderEventArgs.ToolStripSeparatorRenderEventArgs"]/*' />
        /// <devdoc>
        /// This class represents all the information to render the winbar
        /// </devdoc>
        public ToolStripSeparatorRenderEventArgs(Graphics g, ToolStripSeparator separator, bool vertical) : base(g, separator) {
            this.vertical = vertical;
        }

        /// <include file='doc\ToolStripSeparatorRenderEventArgs.uex' path='docs/doc[@for="ToolStripSeparatorRenderEventArgs.Vertical"]/*' />
        /// <devdoc>
        /// the graphics object to draw with
        /// </devdoc>
        public bool Vertical  {
            get {
                return vertical;    
            }
        }
        
    }
}
