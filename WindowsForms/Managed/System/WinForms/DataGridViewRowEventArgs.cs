//------------------------------------------------------------------------------
// <copyright file="DataGridViewRowEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics;

    /// <include file='doc\DataGridViewRowEventArgs.uex' path='docs/doc[@for="DataGridViewRowEventArgs"]/*' />
    public class DataGridViewRowEventArgs : EventArgs
    {
        private DataGridViewRow dataGridViewRow;

        /// <include file='doc\DataGridViewRowEventArgs.uex' path='docs/doc[@for="DataGridViewRowEventArgs.DataGridViewRowEventArgs"]/*' />
        public DataGridViewRowEventArgs(DataGridViewRow dataGridViewRow)
        {
            if (dataGridViewRow == null)
            {
                throw new ArgumentNullException("dataGridViewRow");
            }
            Debug.Assert(dataGridViewRow.Index >= -1);
            this.dataGridViewRow = dataGridViewRow;
        }

        /// <include file='doc\DataGridViewRowEventArgs.uex' path='docs/doc[@for="DataGridViewRowEventArgs.Row"]/*' />
        public DataGridViewRow Row
        {
            get
            {
                return this.dataGridViewRow;
            }
        }
    }
}
