//------------------------------------------------------------------------------
// <copyright file="ToolStripDropDownCloseReason.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.Windows.Forms {
    using System;
	
    public enum ToolStripDropDownCloseReason {
          AppFocusChange,
          AppClicked,
          ItemClicked,
          Keyboard,
          CloseCalled
    }
}
