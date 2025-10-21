//------------------------------------------------------------------------------
// <copyright file="DataGridViewCellEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics;

    /// <include file='doc\DataGridViewCellEventArgs.uex' path='docs/doc[@for="DataGridViewCellEventArgs"]/*' />
    public class DataGridViewCellEventArgs : EventArgs
    {
        private int columnIndex;
        private int rowIndex;
    
        internal DataGridViewCellEventArgs(DataGridViewCell dataGridViewCell) : this(dataGridViewCell.ColumnIndex, dataGridViewCell.RowIndex)
        {
        }

        /// <include file='doc\DataGridViewCellEventArgs.uex' path='docs/doc[@for="DataGridViewCellEventArgs.DataGridViewCellEventArgs"]/*' />
        public DataGridViewCellEventArgs(int columnIndex, int rowIndex)
        {
            if (columnIndex < -1)
            {
                throw new ArgumentOutOfRangeException("columnIndex");
            }
            if (rowIndex < -1)
            {
                throw new ArgumentOutOfRangeException("rowIndex");
            }
            this.columnIndex = columnIndex;
            this.rowIndex = rowIndex;
        }

        /// <include file='doc\DataGridViewCellEventArgs.uex' path='docs/doc[@for="DataGridViewCellEventArgs.ColumnIndex"]/*' />
        public int ColumnIndex
        {
            get
            {
                return this.columnIndex;
            }
        }

        /// <include file='doc\DataGridViewCellEventArgs.uex' path='docs/doc[@for="DataGridViewCellEventArgs.RowIndex"]/*' />
        public int RowIndex
        {
            get
            {
                return this.rowIndex;
            }
        }
    }
}
