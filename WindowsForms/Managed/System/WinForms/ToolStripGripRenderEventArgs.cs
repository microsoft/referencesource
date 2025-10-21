//------------------------------------------------------------------------------
// <copyright file="ToolStripGripRenderEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Drawing;

    /// <include file='doc\ToolStripGripRenderEventArgs.uex' path='docs/doc[@for="ToolStripGripRenderEventArgs"]/*' />
    /// <devdoc/>
    public class ToolStripGripRenderEventArgs : ToolStripRenderEventArgs {

        /// <include file='doc\ToolStripGripRenderEventArgs.uex' path='docs/doc[@for="ToolStripGripRenderEventArgs.ToolStripGripRenderEventArgs"]/*' />
        /// <devdoc>
        /// This class represents all the information to render the toolStrip
        /// </devdoc>
        public ToolStripGripRenderEventArgs(Graphics g, ToolStrip toolStrip) : base(g, toolStrip) {

        }

        /// <include file='doc\ToolStripGripRenderEventArgs.uex' path='docs/doc[@for="ToolStripGripRenderEventArgs.GripBounds"]/*' />
        /// <devdoc>
        /// the graphics object to draw with
        /// </devdoc>
        public Rectangle GripBounds  {
            get {
                return ToolStrip.GripRectangle;    
            }
        }


        /// <include file='doc\ToolStripGripRenderEventArgs.uex' path='docs/doc[@for="ToolStripGripRenderEventArgs.GripDisplayStyle"]/*' />
        /// <devdoc>
        /// vertical or horizontal
        /// </devdoc>
        public ToolStripGripDisplayStyle GripDisplayStyle {
            get {
                return ToolStrip.GripDisplayStyle;
            }
        }
        
        /// <include file='doc\ToolStripGripRenderEventArgs.uex' path='docs/doc[@for="ToolStripGripRenderEventArgs.GripStyle"]/*' />
        /// <devdoc>
        /// visible or not
        /// </devdoc>
        public ToolStripGripStyle GripStyle {
            get {
                return ToolStrip.GripStyle;
            }
        }

    }
}
