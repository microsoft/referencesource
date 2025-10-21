//------------------------------------------------------------------------------
// <copyright file="ItemChangedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    
    using System;

    /// <include file='doc\ItemChangedEventArgs.uex' path='docs/doc[@for="ItemChangedEventArgs"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class ItemChangedEventArgs : EventArgs {

        private int index;    
    
        internal ItemChangedEventArgs(int index) {
            this.index = index;
        }

        /// <include file='doc\ItemChangedEventArgs.uex' path='docs/doc[@for="ItemChangedEventArgs.Index"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Index {
            get {
                return index;
            }
        }
    }
}
