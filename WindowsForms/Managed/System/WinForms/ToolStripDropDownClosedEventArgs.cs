//------------------------------------------------------------------------------
// <copyright file="ToolStripDropDownClosedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using System.ComponentModel;
    
    public class ToolStripDropDownClosedEventArgs : EventArgs {

        ToolStripDropDownCloseReason closeReason;
        
        public ToolStripDropDownClosedEventArgs(ToolStripDropDownCloseReason reason) {
            closeReason = reason;
        }

        public ToolStripDropDownCloseReason CloseReason {
            get { return closeReason; }
        }

     }
}
