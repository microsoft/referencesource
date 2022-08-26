//------------------------------------------------------------------------------
// <copyright file="ToolStripLayoutStyle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.Windows.Forms {
    using System;
    using System.ComponentModel;
    
    /// <include file='doc\ToolStripLayoutStyle.uex' path='docs/doc[@for="ToolStripLayoutStyle"]/*' />
    public enum ToolStripLayoutStyle {
        StackWithOverflow = 0x0,
        HorizontalStackWithOverflow = 0x1,
        VerticalStackWithOverflow =  0x2,
        Flow = 0x3,
        Table = 0x4    
    }
}
