//------------------------------------------------------------------------------
// <copyright file="DataGridViewRowErrorTextNeededEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics;

    /// <include file='doc\DataGridViewRowErrorTextNeededEventArgs.uex' path='docs/doc[@for="DataGridViewRowErrorTextNeededEventArgs"]/*' />
    public class DataGridViewRowErrorTextNeededEventArgs : EventArgs
    {
        private int rowIndex;
        private string errorText;

        internal DataGridViewRowErrorTextNeededEventArgs(int rowIndex, string errorText)
        {
            Debug.Assert(rowIndex >= -1);
            this.rowIndex = rowIndex;
            this.errorText = errorText;
        }

        /// <include file='doc\DataGridViewRowErrorTextNeededEventArgs.uex' path='docs/doc[@for="DataGridViewRowErrorTextNeededEventArgs.ErrorText"]/*' />
        public string ErrorText
        {
            get
            {
                return this.errorText;
            }
            set
            {
                this.errorText = value;
            }
        }

        /// <include file='doc\DataGridViewRowErrorTextNeededEventArgs.uex' path='docs/doc[@for="DataGridViewRowErrorTextNeededEventArgs.RowIndex"]/*' />
        public int RowIndex
        {
            get
            {
                return this.rowIndex;
            }
        }
    }
}
