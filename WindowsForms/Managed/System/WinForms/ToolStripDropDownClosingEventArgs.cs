//------------------------------------------------------------------------------
// <copyright file="ToolStripDropDownClosingEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using System.ComponentModel;

    // SECREVIEW - for the "Cancel" to be honored, you must have AllWindowsPermission
    //   i.e. ToolStripDropDown.IsRestrictedWindow MUST be false. 
    //   for more details see ToolStripDropDown.SetVisibleCore
        
    public class ToolStripDropDownClosingEventArgs : CancelEventArgs {

        ToolStripDropDownCloseReason closeReason;
    
        public ToolStripDropDownClosingEventArgs(ToolStripDropDownCloseReason reason) {
            closeReason = reason;
        }
        
 
        public ToolStripDropDownCloseReason CloseReason {
            get { return closeReason; }
        }
        // TBD: CloseReason
     }
}
