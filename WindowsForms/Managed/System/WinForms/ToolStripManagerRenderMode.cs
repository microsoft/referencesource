//------------------------------------------------------------------------------
// <copyright file="ToolStripManagerRenderMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using System.ComponentModel;
    
    /// <include file='doc\ToolStripManagerRenderMode.uex' path='docs/doc[@for="ToolStripManagerRenderMode"]/*' />
    public enum ToolStripManagerRenderMode {
        /// <include file='doc\ToolStripManagerRenderMode.uex' path='docs/doc[@for="ToolStripManagerRenderMode.Custom"]/*' />
        [Browsable(false)]
        Custom = ToolStripRenderMode.Custom, 
        /// <include file='doc\ToolStripManagerRenderMode.uex' path='docs/doc[@for="ToolStripManagerRenderMode.System"]/*' />
        System = ToolStripRenderMode.System,
        /// <include file='doc\ToolStripManagerRenderMode.uex' path='docs/doc[@for="ToolStripManagerRenderMode.Professional"]/*' />
        Professional = ToolStripRenderMode.Professional
    }
   

}
