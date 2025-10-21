//------------------------------------------------------------------------------
// <copyright file="ISupportToolStripPanel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System;
    
    internal interface ISupportToolStripPanel {
        ToolStripPanelRow ToolStripPanelRow {
            get; set;
        }
        
        ToolStripPanelCell ToolStripPanelCell {
            get;
        }
        
        
        bool Stretch {
            get; set;
        }

        bool IsCurrentlyDragging {
            get;
        }

        void BeginDrag();
        
        void EndDrag();
        
    }
}


