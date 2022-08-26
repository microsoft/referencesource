//------------------------------------------------------------------------------
// <copyright file="DataGridViewCellContextMenuStripNeededEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics;

    /// <include file='doc\DataGridViewCellContextMenuStripNeededEventArgs.uex' path='docs/doc[@for="DataGridViewCellContextMenuStripNeededEventArgs"]/*' />
    public class DataGridViewCellContextMenuStripNeededEventArgs : DataGridViewCellEventArgs
    {
        private ContextMenuStrip contextMenuStrip;

        /// <include file='doc\DataGridViewCellContextMenuStripNeededEventArgs.uex' path='docs/doc[@for="DataGridViewCellContextMenuStripNeededEventArgs.DataGridViewContextMenuStripNeededEventArgs"]/*' />
        public DataGridViewCellContextMenuStripNeededEventArgs(int columnIndex, int rowIndex) : base(columnIndex, rowIndex)
        {
        }

        internal DataGridViewCellContextMenuStripNeededEventArgs(
            int columnIndex,
            int rowIndex,
            ContextMenuStrip contextMenuStrip) : base(columnIndex, rowIndex)
        {
            this.contextMenuStrip = contextMenuStrip;
        }

        /// <include file='doc\DataGridViewCellContextMenuStripNeededEventArgs.uex' path='docs/doc[@for="DataGridViewCellContextMenuStripNeededEventArgs.ContextMenuStrip"]/*' />
        public ContextMenuStrip ContextMenuStrip
        {
            get
            {
                return this.contextMenuStrip;
            }
            set
            {
                this.contextMenuStrip = value;
            }
        }
    }
}
