//------------------------------------------------------------------------------
// <copyright file="DataGridViewCellValidatingEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.ComponentModel;

    /// <include file='doc\DataGridViewCellValidatingEventArgs.uex' path='docs/doc[@for="DataGridViewCellValidatingEventArgs"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class DataGridViewCellValidatingEventArgs : CancelEventArgs
    {
        private int rowIndex, columnIndex;
        private object formattedValue;

        internal DataGridViewCellValidatingEventArgs(int columnIndex, int rowIndex, object formattedValue)
        {
            this.rowIndex = rowIndex;
            this.columnIndex = columnIndex;
            this.formattedValue = formattedValue;
        }

        /// <include file='doc\DataGridViewCellValidatingEventArgs.uex' path='docs/doc[@for="DataGridViewCellValidatingEventArgs.ColumnIndex"]/*' />
        public int ColumnIndex
        {
            get
            {
                return this.columnIndex;
            }
        }

        /// <include file='doc\DataGridViewCellValidatingEventArgs.uex' path='docs/doc[@for="DataGridViewCellValidatingEventArgs.FormattedValue"]/*' />
        public object FormattedValue
        {
            get
            {
                return this.formattedValue;
            }
        }

        /// <include file='doc\DataGridViewCellValidatingEventArgs.uex' path='docs/doc[@for="DataGridViewCellValidatingEventArgs.RowIndex"]/*' />
        public int RowIndex
        {
            get
            {
                return this.rowIndex;
            }
        }
    }
}
