//------------------------------------------------------------------------------
// <copyright file="DataGridViewAutoSizeColumnsModeEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;

    /// <include file='doc\DataGridViewAutoSizeColumnsModeEventArgs.uex' path='docs/doc[@for="DataGridViewAutoSizeColumnsModeEventArgs"]/*' />
    public class DataGridViewAutoSizeColumnsModeEventArgs : EventArgs
    {
        private DataGridViewAutoSizeColumnMode[] previousModes;

        /// <include file='doc\DataGridViewAutoSizeColumnsModeEventArgs.uex' path='docs/doc[@for="DataGridViewAutoSizeColumnsModeEventArgs.DataGridViewAutoSizeColumnsModeEventArgs"]/*' />
        public DataGridViewAutoSizeColumnsModeEventArgs(DataGridViewAutoSizeColumnMode[] previousModes)
        {
            this.previousModes = previousModes;
        }

        /// <include file='doc\DataGridViewAutoSizeColumnsModeEventArgs.uex' path='docs/doc[@for="DataGridViewAutoSizeColumnsModeEventArgs.PreviousModes"]/*' />
        [
            SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays") // Returning a collection would be overkill.
        ]
        public DataGridViewAutoSizeColumnMode[] PreviousModes
        {
            get
            {
                return this.previousModes;
            }
        }
    }
}
