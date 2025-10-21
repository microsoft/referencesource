//------------------------------------------------------------------------------
// <copyright file="ColumnWidthChangedEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;


    /// <include file='doc\ColumnWidthChangedEvent.uex' path='docs/doc[@for="ColumnWidthChangedEventArgs"]/*' />
    /// <devdoc>
    /// </devdoc>
    public class ColumnWidthChangedEventArgs : EventArgs {
        readonly int columnIndex;

        /// <include file='doc\ColumnWidthChangedEvent.uex' path='docs/doc[@for="ColumnWidthEventArgs.ColumnWidthChangedEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ColumnWidthChangedEventArgs(int columnIndex) {
            this.columnIndex = columnIndex;
        }

        /// <include file='doc\ColumnWidthChangedEvent.uex' path='docs/doc[@for="ColumnWidthChangedEventArgs.ColumnIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int ColumnIndex {
            get {
                return columnIndex;
            }

        }
    }
}
