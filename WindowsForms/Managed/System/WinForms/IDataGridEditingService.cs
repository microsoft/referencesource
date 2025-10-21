//------------------------------------------------------------------------------
// <copyright file="IDataGridEditingService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    
    /// <include file='doc\IDataGridEditingService.uex' path='docs/doc[@for="IDataGridEditingService"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>The DataGrid exposes hooks to request editing commands
    ///       via this interface.</para>
    /// </devdoc>
    public interface IDataGridEditingService {
        /// <include file='doc\IDataGridEditingService.uex' path='docs/doc[@for="IDataGridEditingService.BeginEdit"]/*' />
        bool BeginEdit(DataGridColumnStyle gridColumn, int rowNumber);
        
        /// <include file='doc\IDataGridEditingService.uex' path='docs/doc[@for="IDataGridEditingService.EndEdit"]/*' />
        bool EndEdit(DataGridColumnStyle gridColumn, int rowNumber, bool shouldAbort);
    }
}
