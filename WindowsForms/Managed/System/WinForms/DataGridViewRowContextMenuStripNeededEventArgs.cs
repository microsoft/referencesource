//------------------------------------------------------------------------------
// <copyright file="DataGridViewRowContextMenuStripNeededEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics;

    /// <include file='doc\DataGridViewRowContextMenuStripNeededEventArgs.uex' path='docs/doc[@for="DataGridViewRowContextMenuStripNeededEventArgs"]/*' />
    public class DataGridViewRowContextMenuStripNeededEventArgs : EventArgs
    {
        private int rowIndex;
        private ContextMenuStrip contextMenuStrip;

        /// <include file='doc\DataGridViewRowContextMenuStripNeededEventArgs.uex' path='docs/doc[@for="DataGridViewRowContextMenuStripNeededEventArgs.DataGridViewRowContextMenuStripNeededEventArgs"]/*' />
        public DataGridViewRowContextMenuStripNeededEventArgs(int rowIndex)
        {
            if (rowIndex < -1)
            {
                throw new ArgumentOutOfRangeException("rowIndex");
            }

            this.rowIndex = rowIndex;
        }

        internal DataGridViewRowContextMenuStripNeededEventArgs(int rowIndex, ContextMenuStrip contextMenuStrip) : this(rowIndex)
        {
            this.contextMenuStrip = contextMenuStrip;
        }

        /// <include file='doc\DataGridViewRowContextMenuStripNeededEventArgs.uex' path='docs/doc[@for="DataGridViewRowContextMenuStripNeededEventArgs.RowIndex"]/*' />
        public int RowIndex
        {
            get
            {
                return this.rowIndex;
            }
        }

        /// <include file='doc\DataGridViewRowContextMenuStripNeededEventArgs.uex' path='docs/doc[@for="DataGridViewRowContextMenuStripNeededEventArgs.ContextMenuStrip"]/*' />
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
