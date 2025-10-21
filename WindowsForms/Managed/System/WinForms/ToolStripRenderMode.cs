//------------------------------------------------------------------------------
// <copyright file="ToolStripRenderMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using System.ComponentModel;
    
    /// <include file='doc\ToolStripRenderMode.uex' path='docs/doc[@for="ToolStripRenderMode"]/*' />
    public enum ToolStripRenderMode {
        /// <include file='doc\ToolStripRenderMode.uex' path='docs/doc[@for="ToolStripRenderMode.Custom"]/*' />
        [Browsable(false)]
        Custom,
        /// <include file='doc\ToolStripRenderMode.uex' path='docs/doc[@for="ToolStripRenderMode.System"]/*' />
        System,
        /// <include file='doc\ToolStripRenderMode.uex' path='docs/doc[@for="ToolStripRenderMode.Professional"]/*' />
        Professional,
        /// <include file='doc\ToolStripRenderMode.uex' path='docs/doc[@for="ToolStripRenderMode.ManagerRenderMode"]/*' />
        ManagerRenderMode
    }
   

}
