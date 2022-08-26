//------------------------------------------------------------------------------
// <copyright file="DataGridViewRowStateChangedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;

    /// <include file='doc\DataGridViewRowStateChangedEventArgs.uex' path='docs/doc[@for="DataGridViewRowStateChangedEventArgs"]/*' />
    public class DataGridViewRowStateChangedEventArgs : EventArgs
    {
        private DataGridViewRow dataGridViewRow;
        private DataGridViewElementStates stateChanged;
    
        /// <include file='doc\DataGridViewRowStateChangedEventArgs.uex' path='docs/doc[@for="DataGridViewRowStateChangedEventArgs.DataGridViewRowStateChangedEventArgs"]/*' />
        public DataGridViewRowStateChangedEventArgs(DataGridViewRow dataGridViewRow, DataGridViewElementStates stateChanged)
        {
            this.dataGridViewRow = dataGridViewRow;
            this.stateChanged = stateChanged;
        }

        /// <include file='doc\DataGridViewRowStateChangedEventArgs.uex' path='docs/doc[@for="DataGridViewRowStateChangedEventArgs.Row"]/*' />
        public DataGridViewRow Row
        {
            get
            {
                return this.dataGridViewRow;
            }
        }

        /// <include file='doc\DataGridViewRowStateChangedEventArgs.uex' path='docs/doc[@for="DataGridViewRowStateChangedEventArgs.StateChanged"]/*' />
        public DataGridViewElementStates StateChanged
        {
            get
            {
                return this.stateChanged;
            }
        }
    }
}
