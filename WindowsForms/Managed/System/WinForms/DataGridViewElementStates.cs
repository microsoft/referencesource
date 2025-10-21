//------------------------------------------------------------------------------
// <copyright file="DataGridViewElementStates.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    /// <include file='doc\DataGridViewElementStates.uex' path='docs/doc[@for="DataGridViewElementStates"]/*' />
    [
        Flags,
        System.Runtime.InteropServices.ComVisible(true)
    ]
    public enum DataGridViewElementStates
    {
        /// <include file='doc\DataGridViewElementStates.uex' path='docs/doc[@for="DataGridViewElementStates.None"]/*' />
        None            = 0x0000,
        /// <include file='doc\DataGridViewElementStates.uex' path='docs/doc[@for="DataGridViewElementStates.Displayed"]/*' />
        Displayed       = 0x0001,
        /// <include file='doc\DataGridViewElementStates.uex' path='docs/doc[@for="DataGridViewElementStates.Frozen"]/*' />
        Frozen          = 0x0002,
        /// <include file='doc\DataGridViewElementStates.uex' path='docs/doc[@for="DataGridViewElementStates.ReadOnly"]/*' />
        ReadOnly        = 0x0004,
        /// <include file='doc\DataGridViewElementStates.uex' path='docs/doc[@for="DataGridViewElementStates.Resizable"]/*' />
        Resizable       = 0x0008,
        /// <include file='doc\DataGridViewElementStates.uex' path='docs/doc[@for="DataGridViewElementStates.ResizableSet"]/*' />
        ResizableSet    = 0x0010,
        /// <include file='doc\DataGridViewElementStates.uex' path='docs/doc[@for="DataGridViewElementStates.Selected"]/*' />
        Selected        = 0x0020,
        /// <include file='doc\DataGridViewElementStates.uex' path='docs/doc[@for="DataGridViewElementStates.Visible"]/*' />
        Visible         = 0x0040
    }
}
