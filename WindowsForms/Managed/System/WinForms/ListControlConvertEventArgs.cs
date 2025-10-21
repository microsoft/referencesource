//------------------------------------------------------------------------------
// <copyright file="ListControlConvertEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System;
    using System.Windows.Forms;

    /// <include file='doc\ListControlConvertEvent.uex' path='docs/doc[@for="ListControlConvertEventArgs"]/*' />
    public class ListControlConvertEventArgs : ConvertEventArgs {
        object listItem;
        /// <include file='doc\ListControlConvertEvent.uex' path='docs/doc[@for="ListControlConvertEventArgs.ListControlConvertEventArgs"]/*' />
        public ListControlConvertEventArgs(object value, Type desiredType, object listItem) : base(value, desiredType) {
            this.listItem = listItem;
        }

        /// <include file='doc\ListControlConvertEvent.uex' path='docs/doc[@for="ListControlConvertEventArgs.ListItem"]/*' />
        public object ListItem {
            get {
                return this.listItem;
            }
        }
    }
}
