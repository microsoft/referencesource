//------------------------------------------------------------------------------
// <copyright file="DataGridViewRowsRemovedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics;
    using System.Globalization;

    /// <include file='doc\DataGridViewRowsRemovedEventArgs.uex' path='docs/doc[@for="DataGridViewRowsRemovedEventArgs"]/*' />
    public class DataGridViewRowsRemovedEventArgs : EventArgs
    {
        private int rowIndex, rowCount;

        /// <include file='doc\DataGridViewRowsRemovedEventArgs.uex' path='docs/doc[@for="DataGridViewRowsRemovedEventArgs.DataGridViewRowsRemovedEventArgs"]/*' />
        public DataGridViewRowsRemovedEventArgs(int rowIndex, int rowCount)
        {
            if (rowIndex < 0)
            {
                throw new ArgumentOutOfRangeException("rowIndex", SR.GetString(SR.InvalidLowBoundArgumentEx, "rowIndex", rowIndex.ToString(CultureInfo.CurrentCulture), (0).ToString(CultureInfo.CurrentCulture)));
            }
            if (rowCount < 1)
            {
                throw new ArgumentOutOfRangeException("rowCount", SR.GetString(SR.InvalidLowBoundArgumentEx, "rowCount", rowCount.ToString(CultureInfo.CurrentCulture), (1).ToString(CultureInfo.CurrentCulture)));
            }
            this.rowIndex = rowIndex;
            this.rowCount = rowCount;
        }

        /// <include file='doc\DataGridViewRowsRemovedEventArgs.uex' path='docs/doc[@for="DataGridViewRowsRemovedEventArgs.RowIndex"]/*' />
        public int RowIndex
        {
            get
            {
                return this.rowIndex;
            }
        }

        /// <include file='doc\DataGridViewRowsRemovedEventArgs.uex' path='docs/doc[@for="DataGridViewRowsRemovedEventArgs.RowCount"]/*' />
        public int RowCount
        {
            get
            {
                return this.rowCount;
            }
        }
    }
}
