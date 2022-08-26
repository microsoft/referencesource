//------------------------------------------------------------------------------
// <copyright file="ToolStripPanelRenderEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Drawing;

    
    /// <devdoc>
    ///   ToolStripPanelRenderEventArgs
    /// </devdoc>
    public class ToolStripPanelRenderEventArgs : EventArgs {

        private ToolStripPanel      toolStripPanel         = null;
        private Graphics              graphics                 = null;
        private bool handled = false;

        /// <devdoc>
        ///  This class represents all the information to render the toolStrip
        /// </devdoc>        
        public ToolStripPanelRenderEventArgs(Graphics g, ToolStripPanel toolStripPanel) {
            this.toolStripPanel = toolStripPanel;
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

        /// <devdoc>
        ///  Represents which toolStrip was affected by the click
        /// </devdoc>
        public ToolStripPanel ToolStripPanel {
            get {
                return toolStripPanel;
            }
        }

        public bool Handled {
            get { return handled; }
            set { handled = value; }
        }

    }
}
