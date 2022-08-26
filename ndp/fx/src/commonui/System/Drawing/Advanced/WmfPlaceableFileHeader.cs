//------------------------------------------------------------------------------
// <copyright file="WmfPlaceableFileHeader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using System.Runtime.InteropServices;

    /// <include file='doc\WmfPlaceableFileHeader.uex' path='docs/doc[@for="WmfPlaceableFileHeader"]/*' />
    /// <devdoc>
    ///    Defines an Placeable Metafile.
    /// </devdoc>
    [StructLayout(LayoutKind.Sequential)]
    public sealed class WmfPlaceableFileHeader {
        int key = unchecked((int)0x9aC6CDD7);
        short hmf;
        short bboxLeft;
        short bboxTop;
        short bboxRight;
        short bboxBottom;
        short inch;
        int reserved;
        short checksum;
         
        /// <include file='doc\WmfPlaceableFileHeader.uex' path='docs/doc[@for="WmfPlaceableFileHeader.Key"]/*' />
        /// <devdoc>
        ///    Indicates the presence of a placeable
        ///    metafile header.
        /// </devdoc>
        public int Key {
            get { return key; }
            set { key = value; }
        }

        /// <include file='doc\WmfPlaceableFileHeader.uex' path='docs/doc[@for="WmfPlaceableFileHeader.Hmf"]/*' />
        /// <devdoc>
        ///    Stores the handle of the metafile in
        ///    memory.
        /// </devdoc>
        public short Hmf {
            get { return hmf; }
            set { hmf = value; }
        }

        /// <include file='doc\WmfPlaceableFileHeader.uex' path='docs/doc[@for="WmfPlaceableFileHeader.BboxLeft"]/*' />
        /// <devdoc>
        ///    The x-coordinate of the upper-left corner
        ///    of the bounding rectangle of the metafile image on the output device.
        /// </devdoc>
        public short BboxLeft {
            get { return bboxLeft; }
            set { bboxLeft = value; }
        }

        /// <include file='doc\WmfPlaceableFileHeader.uex' path='docs/doc[@for="WmfPlaceableFileHeader.BboxTop"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The y-coordinate of the upper-left corner of the bounding rectangle of the
        ///       metafile image on the output device.
        ///    </para>
        /// </devdoc>
        public short BboxTop {
            get { return bboxTop; }
            set { bboxTop = value; }
        }

        /// <include file='doc\WmfPlaceableFileHeader.uex' path='docs/doc[@for="WmfPlaceableFileHeader.BboxRight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The x-coordinate of the lower-right corner of the bounding rectangle of the
        ///       metafile image on the output device.
        ///    </para>
        /// </devdoc>
        public short BboxRight {
            get { return bboxRight; }
            set { bboxRight = value; }
        }
        
        /// <include file='doc\WmfPlaceableFileHeader.uex' path='docs/doc[@for="WmfPlaceableFileHeader.BboxBottom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The y-coordinate of the lower-right corner of the bounding rectangle of the
        ///       metafile image on the output device.
        ///    </para>
        /// </devdoc>
        public short BboxBottom {
            get { return bboxBottom; }
            set { bboxBottom = value; }
        }

        /// <include file='doc\WmfPlaceableFileHeader.uex' path='docs/doc[@for="WmfPlaceableFileHeader.Inch"]/*' />
        /// <devdoc>
        ///    Indicates the number of twips per inch.
        /// </devdoc>
        public short Inch {
            get { return inch; }
            set { inch = value; }
        }

        /// <include file='doc\WmfPlaceableFileHeader.uex' path='docs/doc[@for="WmfPlaceableFileHeader.Reserved"]/*' />
        /// <devdoc>
        ///    Reserved. Do not use.
        /// </devdoc>
        public int Reserved {
            get { return reserved; }
            set { reserved = value; }
        }

        /// <include file='doc\WmfPlaceableFileHeader.uex' path='docs/doc[@for="WmfPlaceableFileHeader.Checksum"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates the checksum value for the
        ///       previous ten WORDs in the header.
        ///    </para>
        /// </devdoc>
        public short Checksum {
            get { return checksum; }
            set { checksum = value; }
        }
    }
}
