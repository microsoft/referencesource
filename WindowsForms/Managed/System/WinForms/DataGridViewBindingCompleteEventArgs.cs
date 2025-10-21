//------------------------------------------------------------------------------
// <copyright file="DataGridViewBindingCompleteEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System.ComponentModel;

    /// <include file='doc\DataGridViewBindingCompleteEventArgs.uex' path='docs/doc[@for="DataGridViewBindingCompleteEventArgs"]/*' />
    public class DataGridViewBindingCompleteEventArgs : EventArgs
    {
        private ListChangedType listChangedType;

        /// <include file='doc\DataGridViewBindingCompleteEventArgs.uex' path='docs/doc[@for="DataGridViewBindingCompleteEventArgs.DataGridViewBindingCompleteEventArgs"]/*' />
        public DataGridViewBindingCompleteEventArgs(ListChangedType listChangedType)
        {
            this.listChangedType = listChangedType;
        }

        /// <include file='doc\DataGridViewBindingCompleteEventArgs.uex' path='docs/doc[@for="DataGridViewBindingCompleteEventArgs.ListchangedType"]/*' />
        public ListChangedType ListChangedType
        {
            get
            {
                return this.listChangedType;
            }
        }
    }
}
