//------------------------------------------------------------------------------
// <copyright file="DataGridViewSelectionMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.ComponentModel;

    /// <include file='doc\DataGridViewSelectionMode.uex' path='docs/doc[@for="DataGridViewSelectionMode.DataGridViewSelectionMode"]/*' />
    /// <devdoc>
    /// <para></para>
    /// </devdoc>
    public enum DataGridViewSelectionMode 
    {
        /// <include file='doc\DataGridViewSelectionMode.uex' path='docs/doc[@for="DataGridViewSelectionMode.CellSelect"]/*' />
        CellSelect = 0,

        /// <include file='doc\DataGridViewSelectionMode.uex' path='docs/doc[@for="DataGridViewSelectionMode.FullRowSelect"]/*' />
        FullRowSelect = 1,

        /// <include file='doc\DataGridViewSelectionMode.uex' path='docs/doc[@for="DataGridViewSelectionMode.FullColumnSelect"]/*' />
        FullColumnSelect = 2,

        /// <include file='doc\DataGridViewSelectionMode.uex' path='docs/doc[@for="DataGridViewSelectionMode.RowHeaderSelect"]/*' />
        RowHeaderSelect = 3,

        /// <include file='doc\DataGridViewSelectionMode.uex' path='docs/doc[@for="DataGridViewSelectionMode.ColumnHeaderSelect"]/*' />
        ColumnHeaderSelect = 4
    }
}
