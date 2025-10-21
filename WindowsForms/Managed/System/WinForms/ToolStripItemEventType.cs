//------------------------------------------------------------------------------
// <copyright file="ToolStripEnumerations.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System;


    /// <devdoc>
    /// These methods allow the ToolStrip to route events
    /// to the winbar item.  Since a ToolStrip is not a ToolStripItem,
    /// it cannot directly call OnPaint.
    /// </devdoc>
    internal enum ToolStripItemEventType {
        Paint,
        LocationChanged,
        MouseUp,
        MouseDown,
        MouseMove,
        MouseEnter,
        MouseLeave,
        MouseHover,
        Click,
        DoubleClick
    }
}
