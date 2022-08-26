//------------------------------------------------------------------------------
// <copyright file="DataGridViewRowCancelEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics;
    using System.ComponentModel;

    /// <include file='doc\DataGridViewRowCancelEventArgs.uex' path='docs/doc[@for="DataGridViewRowCancelEventArgs"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class DataGridViewRowCancelEventArgs : CancelEventArgs
    {
        private DataGridViewRow dataGridViewRow;
    
        /// <include file='doc\DataGridViewRowCancelEventArgs.uex' path='docs/doc[@for="DataGridViewRowCancelEventArgs.DataGridViewRowCancelEventArgs"]/*' />
        public DataGridViewRowCancelEventArgs(DataGridViewRow dataGridViewRow)
        {
            Debug.Assert(dataGridViewRow != null);
            Debug.Assert(dataGridViewRow.Index >= 0);
            this.dataGridViewRow = dataGridViewRow;
        }

        /// <include file='doc\DataGridViewRowCancelEventArgs.uex' path='docs/doc[@for="DataGridViewRowCancelEventArgs.Row"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataGridViewRow Row
        {
            get
            {
                return this.dataGridViewRow;
            }
        }
    }
}
