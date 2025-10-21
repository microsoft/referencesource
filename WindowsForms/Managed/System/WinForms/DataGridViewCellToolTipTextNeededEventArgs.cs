//------------------------------------------------------------------------------
// <copyright file="DataGridViewCellToolTipTextNeededEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics;

    /// <include file='doc\DataGridViewCellToolTipTextNeededEventArgs.uex' path='docs/doc[@for="DataGridViewCellToolTipTextNeededEventArgs"]/*' />
    public class DataGridViewCellToolTipTextNeededEventArgs : DataGridViewCellEventArgs
    {
        private string toolTipText;

        internal DataGridViewCellToolTipTextNeededEventArgs(
            int columnIndex, 
            int rowIndex,
            string toolTipText) : base(columnIndex, rowIndex)
        {
            this.toolTipText = toolTipText;
        }

        /// <include file='doc\DataGridViewCellToolTipTextNeededEventArgs.uex' path='docs/doc[@for="DataGridViewCellToolTipTextNeededEventArgs.ToolTipText"]/*' />
        public string ToolTipText
        {
            get
            {
                return this.toolTipText;
            }
            set
            {
                this.toolTipText = value;
            }
        }
    }
}
