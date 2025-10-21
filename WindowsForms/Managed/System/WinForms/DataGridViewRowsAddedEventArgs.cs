//------------------------------------------------------------------------------
// <copyright file="DataGridViewRowsAddedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics;

    /// <include file='doc\DataGridViewRowsAddedEventArgs.uex' path='docs/doc[@for="DataGridViewRowsAddedEventArgs"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class DataGridViewRowsAddedEventArgs : EventArgs
    {
        private int rowIndex, rowCount;
    
        /// <include file='doc\DataGridViewRowsAddedEventArgs.uex' path='docs/doc[@for="DataGridViewRowsAddedEventArgs.DataGridViewRowsAddedEventArgs"]/*' />
        public DataGridViewRowsAddedEventArgs(int rowIndex, int rowCount)
        {
            Debug.Assert(rowIndex >= 0);
            Debug.Assert(rowCount >= 1);
            this.rowIndex = rowIndex;
            this.rowCount = rowCount;
        }

        /// <include file='doc\DataGridViewRowsAddedEventArgs.uex' path='docs/doc[@for="DataGridViewRowsAddedEventArgs.RowIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int RowIndex
        {
            get
            {
                return this.rowIndex;
            }
        }

        /// <include file='doc\DataGridViewRowsAddedEventArgs.uex' path='docs/doc[@for="DataGridViewRowsAddedEventArgs.RowCount"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int RowCount
        {
            get
            {
                return this.rowCount;
            }
        }
    }
}
