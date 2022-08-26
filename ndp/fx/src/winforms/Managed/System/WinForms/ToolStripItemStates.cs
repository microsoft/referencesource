//------------------------------------------------------------------------------
// <copyright file="ToolStripItemStates.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.Windows.Forms {
    using System;
	
  
    internal enum ToolStripItemStates {
        None     = 0x00000000,
        Selected = 0x00000001, 
        Focused  = 0x00000002,
        Hot      = 0x00000004,
        Pressed  = 0x00000008,
        Disabled = 0x00000010
    }
}
