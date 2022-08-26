//------------------------------------------------------------------------------
// <copyright file="DataGridViewCellFormattingEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Drawing;
    using System.ComponentModel;

    /// <include file='doc\DataGridViewCellFormattingEventArgs.uex' path='docs/doc[@for="DataGridViewCellFormattingEventArgs"]/*' />
    public class DataGridViewCellFormattingEventArgs : ConvertEventArgs
    {
        private int columnIndex, rowIndex;
        private DataGridViewCellStyle cellStyle;
        private bool formattingApplied;
    
        /// <include file='doc\DataGridViewCellFormattingEventArgs.uex' path='docs/doc[@for="DataGridViewCellFormattingEventArgs.DataGridViewCellFormattingEventArgs"]/*' />
        public DataGridViewCellFormattingEventArgs(int columnIndex,
                                                   int rowIndex,
                                                   object value,
                                                   Type desiredType,
                                                   DataGridViewCellStyle cellStyle) : base(value, desiredType)
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
            this.cellStyle = cellStyle;
        }

        /// <include file='doc\DataGridViewCellFormattingEventArgs.uex' path='docs/doc[@for="DataGridViewCellFormattingEventArgs.CellStyle"]/*' />
        public DataGridViewCellStyle CellStyle
        {
            get
            {
                return this.cellStyle;
            }
            set
            {
                this.cellStyle = value;
            }
        }

        /// <include file='doc\DataGridViewCellFormattingEventArgs.uex' path='docs/doc[@for="DataGridViewCellFormattingEventArgs.ColumnIndex"]/*' />
        public int ColumnIndex
        {
            get
            {
                return this.columnIndex;
            }
        }

        /// <include file='doc\DataGridViewCellFormattingEventArgs.uex' path='docs/doc[@for="DataGridViewCellFormattingEventArgs.FormattingApplied"]/*' />
        public bool FormattingApplied
        {
            get
            {
                return this.formattingApplied;
            }
            set
            {
                this.formattingApplied = value;
            }
        }

        /// <include file='doc\DataGridViewCellFormattingEventArgs.uex' path='docs/doc[@for="DataGridViewCellFormattingEventArgs.RowIndex"]/*' />
        public int RowIndex
        {
            get
            {
                return this.rowIndex;
            }
        }
    }
}
