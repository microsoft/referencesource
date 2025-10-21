//------------------------------------------------------------------------------
// <copyright file="DataGridViewCellParsingEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Drawing;
    using System.ComponentModel;

    /// <include file='doc\DataGridViewCellParsingEventArgs.uex' path='docs/doc[@for="DataGridViewCellParsingEventArgs"]/*' />
    public class DataGridViewCellParsingEventArgs : ConvertEventArgs
    {
        private int rowIndex, columnIndex;
        private DataGridViewCellStyle inheritedCellStyle;
        private bool parsingApplied;
    
        /// <include file='doc\DataGridViewCellParsingEventArgs.uex' path='docs/doc[@for="DataGridViewCellParsingEventArgs.DataGridViewCellParsingEventArgs"]/*' />
        public DataGridViewCellParsingEventArgs(int rowIndex,
                                         int columnIndex,
                                         object value,
                                         Type desiredType,
                                         DataGridViewCellStyle inheritedCellStyle) : base(value, desiredType)
        {
            this.rowIndex = rowIndex;
            this.columnIndex = columnIndex;
            this.inheritedCellStyle = inheritedCellStyle;
        }

        /// <include file='doc\DataGridViewCellParsingEventArgs.uex' path='docs/doc[@for="DataGridViewCellParsingEventArgs.RowIndex"]/*' />
        public int RowIndex
        {
            get
            {
                return this.rowIndex;
            }
        }

        /// <include file='doc\DataGridViewCellParsingEventArgs.uex' path='docs/doc[@for="DataGridViewCellParsingEventArgs.ColumnIndex"]/*' />
        public int ColumnIndex
        {
            get
            {
                return this.columnIndex;
            }
        }

        /// <include file='doc\DataGridViewCellParsingEventArgs.uex' path='docs/doc[@for="DataGridViewCellParsingEventArgs.InheritedCellStyle"]/*' />
        public DataGridViewCellStyle InheritedCellStyle
        {
            get
            {
                return this.inheritedCellStyle;
            }
            set
            {
                this.inheritedCellStyle = value;
            }
        }

        /// <include file='doc\DataGridViewCellParsingEventArgs.uex' path='docs/doc[@for="DataGridViewCellParsingEventArgs.ParsingApplied"]/*' />
        public bool ParsingApplied
        {
            get
            {
                return this.parsingApplied;
            }
            set
            {
                this.parsingApplied = value;
            }
        }
    }
}
