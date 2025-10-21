//------------------------------------------------------------------------------
// <copyright file="DataGridViewColumnStateChangedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;

    /// <include file='doc\DataGridViewColumnStateChangedEventArgs.uex' path='docs/doc[@for="DataGridViewColumnStateChangedEventArgs"]/*' />
    public class DataGridViewColumnStateChangedEventArgs : EventArgs
    {
        private DataGridViewColumn dataGridViewColumn;
        private DataGridViewElementStates stateChanged;
    
        /// <include file='doc\DataGridViewColumnStateChangedEventArgs.uex' path='docs/doc[@for="DataGridViewColumnStateChangedEventArgs.DataGridViewColumnStateChangedEventArgs"]/*' />
        public DataGridViewColumnStateChangedEventArgs(DataGridViewColumn dataGridViewColumn, DataGridViewElementStates stateChanged)
        {
            this.dataGridViewColumn = dataGridViewColumn;
            this.stateChanged = stateChanged;
        }

        /// <include file='doc\DataGridViewColumnStateChangedEventArgs.uex' path='docs/doc[@for="DataGridViewColumnStateChangedEventArgs.Column"]/*' />
        public DataGridViewColumn Column
        {
            get
            {
                return this.dataGridViewColumn;
            }
        }

        /// <include file='doc\DataGridViewColumnStateChangedEventArgs.uex' path='docs/doc[@for="DataGridViewColumnStateChangedEventArgs.StateChanged"]/*' />
        public DataGridViewElementStates StateChanged
        {
            get
            {
                return this.stateChanged;
            }
        }
    }
}
