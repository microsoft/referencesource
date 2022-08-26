//------------------------------------------------------------------------------
// <copyright file="PropertyItem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {
    using System.Runtime.InteropServices;
    using System;    
    using System.Drawing;
                               
    // sdkinc\imaging.h
    /// <include file='doc\PropertyItem.uex' path='docs/doc[@for="PropertyItem"]/*' />
    /// <devdoc>
    ///    Encapsulates a metadata property to be
    ///    included in an image file.
    /// </devdoc>
    public sealed class PropertyItem {
        int id;
        int len;
        short type;
        byte[] value;

        internal PropertyItem() {
        }

        /// <include file='doc\PropertyItem.uex' path='docs/doc[@for="PropertyItem.Id"]/*' />
        /// <devdoc>
        ///    Represents the ID of the property.
        /// </devdoc>
        public int Id {
            get { return id; }
            set { id = value; }
        }
        /// <include file='doc\PropertyItem.uex' path='docs/doc[@for="PropertyItem.Len"]/*' />
        /// <devdoc>
        ///    Represents the length of the property.
        /// </devdoc>
        public int Len {
            get { return len; }
            set { len = value; }
        }
        /// <include file='doc\PropertyItem.uex' path='docs/doc[@for="PropertyItem.Type"]/*' />
        /// <devdoc>
        ///    Represents the type of the property.
        /// </devdoc>
        public short Type {
            get { return type; }
            set { type = value; }
        }
        /// <include file='doc\PropertyItem.uex' path='docs/doc[@for="PropertyItem.Value"]/*' />
        /// <devdoc>
        ///    Contains the property value.
        /// </devdoc>
        public byte[] Value {
            get { return this.value; }
            set { this.value = value; }
        }
    }
}

