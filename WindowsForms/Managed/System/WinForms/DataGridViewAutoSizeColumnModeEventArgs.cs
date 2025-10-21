//------------------------------------------------------------------------------
// <copyright file="DataGridViewAutoSizeColumnModeEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics;

    /// <include file='doc\DataGridViewAutoSizeColumnModeEventArgs.uex' path='docs/doc[@for="DataGridViewAutoSizeColumnModeEventArgs"]/*' />
    public class DataGridViewAutoSizeColumnModeEventArgs : EventArgs
    {
        private DataGridViewAutoSizeColumnMode previousMode;
        private DataGridViewColumn dataGridViewColumn;

        /// <include file='doc\DataGridViewAutoSizeColumnModeEventArgs.uex' path='docs/doc[@for="DataGridViewAutoSizeColumnModeEventArgs.DataGridViewAutoSizeColumnModeEventArgs"]/*' />
        public DataGridViewAutoSizeColumnModeEventArgs(DataGridViewColumn dataGridViewColumn, DataGridViewAutoSizeColumnMode previousMode)
        {
            Debug.Assert(dataGridViewColumn != null);
            this.dataGridViewColumn = dataGridViewColumn;
            this.previousMode = previousMode;
        }

        /// <include file='doc\DataGridViewAutoSizeColumnModeEventArgs.uex' path='docs/doc[@for="DataGridViewAutoSizeColumnModeEventArgs.Column"]/*' />
        public DataGridViewColumn Column
        {
            get
            {
                return this.dataGridViewColumn;
            }
        }

        /// <include file='doc\DataGridViewAutoSizeColumnModeEventArgs.uex' path='docs/doc[@for="DataGridViewAutoSizeColumnModeEventArgs.PreviousMode"]/*' />
        public DataGridViewAutoSizeColumnMode PreviousMode
        {
            get
            {
                return this.previousMode;
            }
        }
    }
}
