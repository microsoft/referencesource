//------------------------------------------------------------------------------
// <copyright file="DataGridViewCellErrorTextNeededEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics;

    /// <include file='doc\DataGridViewCellErrorTextNeededEventArgs.uex' path='docs/doc[@for="DataGridViewCellErrorTextNeededEventArgs"]/*' />
    public class DataGridViewCellErrorTextNeededEventArgs : DataGridViewCellEventArgs
    {
        private string errorText;

        internal DataGridViewCellErrorTextNeededEventArgs(
            int columnIndex, 
            int rowIndex,
            string errorText) : base(columnIndex, rowIndex)
        {
            this.errorText = errorText;
        }

        /// <include file='doc\DataGridViewCellErrorTextNeededEventArgs.uex' path='docs/doc[@for="DataGridViewCellErrorTextNeededEventArgs.ErrorText"]/*' />
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
    }
}
