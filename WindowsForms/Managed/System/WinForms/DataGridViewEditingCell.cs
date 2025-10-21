//------------------------------------------------------------------------------
// <copyright file="IDataGridViewEditingCell.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    
    /// <include file='doc\DataGridViewEditingCell.uex' path='docs/doc[@for="IDataGridViewEditingCell"]/*' />
    public interface IDataGridViewEditingCell
    {
        /// <include file='doc\DataGridViewEditingCell.uex' path='docs/doc[@for="IDataGridViewEditingCell.EditingCellFormattedValue"]/*' />        
        [SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods")]
        object EditingCellFormattedValue
        {
            get;
            set;
        }

        /// <include file='doc\DataGridViewEditingCell.uex' path='docs/doc[@for="IDataGridViewEditingCell.EditingCellValueChanged"]/*' />
        bool EditingCellValueChanged
        {
            get;
            set;
        }

        /// <include file='doc\DataGridViewEditingCell.uex' path='docs/doc[@for="IDataGridViewEditingCell.GetEditingCellFormattedValue"]/*' />        
        [SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods")]
        object GetEditingCellFormattedValue(DataGridViewDataErrorContexts context);

        /// <include file='doc\DataGridViewEditingCell.uex' path='docs/doc[@for="IDataGridViewEditingCell.PrepareEditingCellForEdit"]/*' />
        void PrepareEditingCellForEdit(bool selectAll);
    }
}
