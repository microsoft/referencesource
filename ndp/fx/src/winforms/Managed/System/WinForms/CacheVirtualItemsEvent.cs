//------------------------------------------------------------------------------
// <copyright file="CacheVirtualItemsEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
/*
 */
using System;
namespace System.Windows.Forms {
    /// <include file='doc\CacheVirtualItemsEventArgs.uex' path='docs/doc[@for="CacheVirtualItemsEventArgs"]/*' />
    public class CacheVirtualItemsEventArgs : EventArgs {
        private int startIndex;
        private int endIndex;
        /// <include file='doc\CacheVirtualItemsEventArgs.uex' path='docs/doc[@for="CacheVirtualItemsEventArgs.CacheVirtualItemsEventArgs"]/*' />
        public CacheVirtualItemsEventArgs(int startIndex, int endIndex) {
            this.startIndex = startIndex;
            this.endIndex = endIndex;
        }
        /// <include file='doc\CacheVirtualItemsEventArgs.uex' path='docs/doc[@for="CacheVirtualItemsEventArgs.StartIndex"]/*' />
        public int StartIndex {
            get {
                return startIndex;
            }
        }
        /// <include file='doc\CacheVirtualItemsEventArgs.uex' path='docs/doc[@for="CacheVirtualItemsEventArgs.EndIndex"]/*' />
        public int EndIndex {
            get {
                return endIndex;
            }
        }
    }
}
