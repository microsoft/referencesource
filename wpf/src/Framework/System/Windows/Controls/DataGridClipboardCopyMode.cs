//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

namespace System.Windows.Controls
{
    /// <summary>
    /// Defines modes that indicate how DataGrid content is copied to the Clipboard. 
    /// </summary>
    public enum DataGridClipboardCopyMode
    {
        /// <summary>
        /// Copying to the Clipboard is disabled.
        /// </summary>
        None,

        /// <summary>
        /// The text values of selected cells can be copied to the Clipboard. Column header is not included. 
        /// </summary>
        ExcludeHeader,

        /// <summary>
        /// The text values of selected cells can be copied to the Clipboard. Column header is included for columns that contain selected cells.  
        /// </summary>
        IncludeHeader,
    }
}