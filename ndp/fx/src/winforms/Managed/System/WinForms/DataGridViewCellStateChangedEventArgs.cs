//------------------------------------------------------------------------------
// <copyright file="DataGridViewCellStateChangedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;

    /// <include file='doc\DataGridViewCellStateChangedEventArgs.uex' path='docs/doc[@for="DataGridViewCellStateChangedEventArgs"]/*' />
    public class DataGridViewCellStateChangedEventArgs : EventArgs
    {
        private DataGridViewCell dataGridViewCell;
        private DataGridViewElementStates stateChanged;

        /// <include file='doc\DataGridViewCellStateChangedEventArgs.uex' path='docs/doc[@for="DataGridViewCellStateChangedEventArgs.DataGridViewCellStateChangedEventArgs"]/*' />
        public DataGridViewCellStateChangedEventArgs(DataGridViewCell dataGridViewCell, DataGridViewElementStates stateChanged)
        {
            if (dataGridViewCell == null)
            {
                throw new ArgumentNullException("dataGridViewCell");
            }
            this.dataGridViewCell = dataGridViewCell;
            this.stateChanged = stateChanged;
        }

        /// <include file='doc\DataGridViewCellStateChangedEventArgs.uex' path='docs/doc[@for="DataGridViewCellStateChangedEventArgs.Cell"]/*' />
        public DataGridViewCell Cell
        {
            get
            {
                return this.dataGridViewCell;
            }
        }

        /// <include file='doc\DataGridViewCellStateChangedEventArgs.uex' path='docs/doc[@for="DataGridViewCellStateChangedEventArgs.StateChanged"]/*' />
        public DataGridViewElementStates StateChanged
        {
            get
            {
                return this.stateChanged;
            }
        }
    }
}
