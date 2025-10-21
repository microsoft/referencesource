//---------------------------------------------------------------------------------------
// <copyright file="DataGridViewColumnDividerDoubleClickEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//---------------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    /// <include file='doc\DataGridViewColumnDividerDoubleClickEventArgs.uex' path='docs/doc[@for="DataGridViewColumnDividerDoubleClickEventArgs"]/*' />
    public class DataGridViewColumnDividerDoubleClickEventArgs : HandledMouseEventArgs
    {
        private int columnIndex;

        /// <include file='doc\DataGridViewColumnDividerDoubleClickEventArgs.uex' path='docs/doc[@for="DataGridViewColumnDividerDoubleClickEventArgs.DataGridViewColumnDividerDoubleClickEventArgs"]/*' />
        public DataGridViewColumnDividerDoubleClickEventArgs(int columnIndex, HandledMouseEventArgs e) : base(e.Button, e.Clicks, e.X, e.Y, e.Delta, e.Handled)
        {
            if (columnIndex < -1)
            {
                throw new ArgumentOutOfRangeException("columnIndex");
            }
            this.columnIndex = columnIndex;
        }

        /// <include file='doc\DataGridViewColumnDividerDoubleClickEventArgs.uex' path='docs/doc[@for="DataGridViewColumnDividerDoubleClickEventArgs.ColumnIndex"]/*' />
        public int ColumnIndex
        {
            get
            {
                return this.columnIndex;
            }
        }
    }
}
