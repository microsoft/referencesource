//------------------------------------------------------------------------------
// <copyright file="FrameDimension.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {

    using System;
    using System.Diagnostics;
    using System.Drawing;
    using System.ComponentModel;

    /**
     * frame dimension constants (used with Bitmap.FrameDimensionsList)
     */
    /// <include file='doc\FrameDimension.uex' path='docs/doc[@for="FrameDimension"]/*' />
    /// <devdoc>
    ///    
    /// </devdoc>
    // [TypeConverterAttribute(typeof(FrameDimensionConverter))]
    public sealed class FrameDimension {
        // Frame dimension GUIDs, from sdkinc\imgguids.h
        private static FrameDimension time = new FrameDimension(new Guid("{6aedbd6d-3fb5-418a-83a6-7f45229dc872}"));
        private static FrameDimension resolution = new FrameDimension(new Guid("{84236f7b-3bd3-428f-8dab-4ea1439ca315}"));
        private static FrameDimension page = new FrameDimension(new Guid("{7462dc86-6180-4c7e-8e3f-ee7333a7a483}"));

        private Guid guid;

        /// <include file='doc\FrameDimension.uex' path='docs/doc[@for="FrameDimension.FrameDimension"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Imaging.FrameDimension'/> class with the specified GUID.
        /// </devdoc>
        public FrameDimension(Guid guid) {
            this.guid = guid;
        }

        /// <include file='doc\FrameDimension.uex' path='docs/doc[@for="FrameDimension.Guid"]/*' />
        /// <devdoc>
        ///    Specifies a global unique identifier (GUID)
        ///    that represents this <see cref='System.Drawing.Imaging.FrameDimension'/>.
        /// </devdoc>
        public Guid Guid {
            get { return guid;}
        }

        /// <include file='doc\FrameDimension.uex' path='docs/doc[@for="FrameDimension.Time"]/*' />
        /// <devdoc>
        ///    The time dimension.
        /// </devdoc>
        public static FrameDimension Time {
            get { return time;}
        }

        /// <include file='doc\FrameDimension.uex' path='docs/doc[@for="FrameDimension.Resolution"]/*' />
        /// <devdoc>
        ///    The resolution dimension.
        /// </devdoc>
        public static FrameDimension Resolution {
            get { return resolution;}
        }

        /// <include file='doc\FrameDimension.uex' path='docs/doc[@for="FrameDimension.Page"]/*' />
        /// <devdoc>
        ///    The page dimension.
        /// </devdoc>
        public static FrameDimension Page {
            get { return page;}
        }
        /// <include file='doc\FrameDimension.uex' path='docs/doc[@for="FrameDimension.Equals"]/*' />
        /// <devdoc>
        ///    Returns a value indicating whether the
        ///    specified object is an <see cref='System.Drawing.Imaging.FrameDimension'/> equivalent to this <see cref='System.Drawing.Imaging.FrameDimension'/>.
        /// </devdoc>
        public override bool Equals(object o) {
            FrameDimension format = o as FrameDimension;
            if (format == null)
                return false;
            return this.guid == format.guid;
        }

        /// <include file='doc\FrameDimension.uex' path='docs/doc[@for="FrameDimension.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return guid.GetHashCode();
        }
        
        /// <include file='doc\FrameDimension.uex' path='docs/doc[@for="FrameDimension.ToString"]/*' />
        /// <devdoc>
        ///    Converts this <see cref='System.Drawing.Imaging.FrameDimension'/> to a human-readable string.
        /// </devdoc>
        public override string ToString() {
            if (this == time) return "Time";
            if (this == resolution) return "Resolution";
            if (this == page) return "Page";
            return "[FrameDimension: " + guid + "]";
        }
    }
}
