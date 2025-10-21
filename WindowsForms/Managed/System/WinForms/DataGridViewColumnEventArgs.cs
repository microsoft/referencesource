//------------------------------------------------------------------------------
// <copyright file="DataGridViewColumnEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics;

    /// <include file='doc\DataGridViewColumnEventArgs.uex' path='docs/doc[@for="DataGridViewColumnEventArgs"]/*' />
    public class DataGridViewColumnEventArgs : EventArgs
    {
        private DataGridViewColumn dataGridViewColumn;

        /// <include file='doc\DataGridViewColumnEventArgs.uex' path='docs/doc[@for="DataGridViewColumnEventArgs.DataGridViewColumnEventArgs"]/*' />
        public DataGridViewColumnEventArgs(DataGridViewColumn dataGridViewColumn)
        {
            if (dataGridViewColumn == null)
            {
                throw new ArgumentNullException("dataGridViewColumn");
            }
            Debug.Assert(dataGridViewColumn.Index >= -1);
            this.dataGridViewColumn = dataGridViewColumn;
        }

        /// <include file='doc\DataGridViewColumnEventArgs.uex' path='docs/doc[@for="DataGridViewColumnEventArgs.Column"]/*' />
        public DataGridViewColumn Column
        {
            get
            {
                return this.dataGridViewColumn;
            }
        }
    }
}
