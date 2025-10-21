//------------------------------------------------------------------------------
// <copyright file="DataGridViewCellStyleContentChangedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    /// <include file='doc\DataGridViewCellStyleContentChangedEventArgs.uex' path='docs/doc[@for="DataGridViewCellStyleContentChangedEventArgs"]/*' />
    public class DataGridViewCellStyleContentChangedEventArgs : EventArgs
    {
        private DataGridViewCellStyle dataGridViewCellStyle;
        private bool changeAffectsPreferredSize;

        internal DataGridViewCellStyleContentChangedEventArgs(DataGridViewCellStyle dataGridViewCellStyle, bool changeAffectsPreferredSize)
        {
            this.dataGridViewCellStyle = dataGridViewCellStyle;
            this.changeAffectsPreferredSize = changeAffectsPreferredSize;
        }

        /// <include file='doc\DataGridViewCellStyleContentChangedEventArgs.uex' path='docs/doc[@for="DataGridViewCellStyleContentChangedEventArgs.CellStyle"]/*' />
        public DataGridViewCellStyle CellStyle
        {
            get
            {
                return this.dataGridViewCellStyle;
            }
        }

        /// <include file='doc\DataGridViewCellStyleContentChangedEventArgs.uex' path='docs/doc[@for="DataGridViewCellStyleContentChangedEventArgs.CellStyleScope"]/*' />
        public DataGridViewCellStyleScopes CellStyleScope
        {
            get
            {
                return this.dataGridViewCellStyle.Scope;
            }
        }

        internal bool ChangeAffectsPreferredSize
        {
            get
            {
                return this.changeAffectsPreferredSize;
            }
        }
    }
}
