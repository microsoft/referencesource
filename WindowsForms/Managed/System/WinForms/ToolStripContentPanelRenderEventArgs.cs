//------------------------------------------------------------------------------
// <copyright file="ToolStripContentPanelRenderEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Drawing;

    
    /// <devdoc>
    ///   ToolStripContentPanelRenderEventArgs
    /// </devdoc>
    public class ToolStripContentPanelRenderEventArgs : EventArgs {

        private ToolStripContentPanel      contentPanel         = null;
        private Graphics                   graphics                 = null;
        private bool handled = false;

        /// <devdoc>
        ///  This class represents all the information to render the toolStrip
        /// </devdoc>        
        public ToolStripContentPanelRenderEventArgs(Graphics g, ToolStripContentPanel contentPanel) {
            this.contentPanel = contentPanel;
            this.graphics = g;
        }


        /// <devdoc>
        ///  the graphics object to draw with
        /// </devdoc>
        public Graphics Graphics {
            get {
                return graphics;    
            }
        }

        public bool Handled {
            get { return handled; }
            set { handled = value; }
        }

        /// <devdoc>
        ///  Represents which toolStrip was affected by the click
        /// </devdoc>
        public ToolStripContentPanel ToolStripContentPanel {
            get {
                return contentPanel;
            }
        }
        
    }
}
