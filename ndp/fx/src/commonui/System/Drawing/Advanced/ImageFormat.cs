//------------------------------------------------------------------------------
// <copyright file="ImageFormat.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {

    using System;
    using System.Diagnostics;
    using System.Drawing;
    using System.ComponentModel;

    /**
     * Image format constants
     */
    /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat"]/*' />
    /// <devdoc>
    ///    Specifies the format of the image.
    /// </devdoc>
    [TypeConverterAttribute(typeof(ImageFormatConverter))]
    public sealed class ImageFormat {
        // Format IDs
        // private static ImageFormat undefined = new ImageFormat(new Guid("{b96b3ca9-0728-11d3-9d7b-0000f81ef32e}"));
        private static ImageFormat memoryBMP = new ImageFormat(new Guid("{b96b3caa-0728-11d3-9d7b-0000f81ef32e}"));
        private static ImageFormat bmp       = new ImageFormat(new Guid("{b96b3cab-0728-11d3-9d7b-0000f81ef32e}"));
        private static ImageFormat emf       = new ImageFormat(new Guid("{b96b3cac-0728-11d3-9d7b-0000f81ef32e}"));
        private static ImageFormat wmf       = new ImageFormat(new Guid("{b96b3cad-0728-11d3-9d7b-0000f81ef32e}"));
        private static ImageFormat jpeg      = new ImageFormat(new Guid("{b96b3cae-0728-11d3-9d7b-0000f81ef32e}"));
        private static ImageFormat png       = new ImageFormat(new Guid("{b96b3caf-0728-11d3-9d7b-0000f81ef32e}"));
        private static ImageFormat gif       = new ImageFormat(new Guid("{b96b3cb0-0728-11d3-9d7b-0000f81ef32e}"));
        private static ImageFormat tiff      = new ImageFormat(new Guid("{b96b3cb1-0728-11d3-9d7b-0000f81ef32e}"));
        private static ImageFormat exif      = new ImageFormat(new Guid("{b96b3cb2-0728-11d3-9d7b-0000f81ef32e}"));
        private static ImageFormat photoCD   = new ImageFormat(new Guid("{b96b3cb3-0728-11d3-9d7b-0000f81ef32e}"));
        private static ImageFormat flashPIX  = new ImageFormat(new Guid("{b96b3cb4-0728-11d3-9d7b-0000f81ef32e}"));
        private static ImageFormat icon      = new ImageFormat(new Guid("{b96b3cb5-0728-11d3-9d7b-0000f81ef32e}"));


        private Guid guid;

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.ImageFormat"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Imaging.ImageFormat'/> class with the specified GUID.
        /// </devdoc>
        public ImageFormat(Guid guid) {
            this.guid = guid;
        }

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.Guid"]/*' />
        /// <devdoc>
        ///    Specifies a global unique identifier (GUID)
        ///    that represents this <see cref='System.Drawing.Imaging.ImageFormat'/>.
        /// </devdoc>
        public Guid Guid {
            get { return guid;}
        }

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.MemoryBmp"]/*' />
        /// <devdoc>
        ///    Specifies a memory bitmap image format.
        /// </devdoc>
        public static ImageFormat MemoryBmp {
            get { return memoryBMP;}
        }

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.Bmp"]/*' />
        /// <devdoc>
        ///    Specifies the bitmap image format.
        /// </devdoc>
        public static ImageFormat Bmp {
            get { return bmp;}
        }

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.Emf"]/*' />
        /// <devdoc>
        ///    Specifies the enhanced Windows metafile
        ///    image format.
        /// </devdoc>
        public static ImageFormat Emf {
            get { return emf;}
        }

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.Wmf"]/*' />
        /// <devdoc>
        ///    Specifies the Windows metafile image
        ///    format.
        /// </devdoc>
        public static ImageFormat Wmf {
            get { return wmf;}
        }

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.Gif"]/*' />
        /// <devdoc>
        ///    Specifies the GIF image format.
        /// </devdoc>
        public static ImageFormat Gif {
            get { return gif;}
        }

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.Jpeg"]/*' />
        /// <devdoc>
        ///    Specifies the JPEG image format.
        /// </devdoc>
        public static ImageFormat Jpeg {
            get { return jpeg;}
        }

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.Png"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the W3C PNG image format.
        ///    </para>
        /// </devdoc>
        public static ImageFormat Png {
            get { return png;}
        }

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.Tiff"]/*' />
        /// <devdoc>
        ///    Specifies the Tag Image File
        ///    Format (TIFF) image format.
        /// </devdoc>
        public static ImageFormat Tiff {
            get { return tiff;}
        }

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.Exif"]/*' />
        /// <devdoc>
        ///    Specifies the Exchangable Image Format
        ///    (EXIF).
        /// </devdoc>
        public static ImageFormat Exif {
            get { return exif;}
        }

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.Icon"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the Windows icon image format.
        ///    </para>
        /// </devdoc>
        public static ImageFormat Icon {
            get { return icon;}
        }

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.Equals"]/*' />
        /// <devdoc>
        ///    Returns a value indicating whether the
        ///    specified object is an <see cref='System.Drawing.Imaging.ImageFormat'/> equivalent to this <see cref='System.Drawing.Imaging.ImageFormat'/>.
        /// </devdoc>
        public override bool Equals(object o) {
            ImageFormat format = o as ImageFormat;
            if (format == null)
                return false;
            return this.guid == format.guid;
        }

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a hash code.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return this.guid.GetHashCode();
        }

#if !FEATURE_PAL        
        // Find any random encoder which supports this format
        internal ImageCodecInfo FindEncoder() {
            ImageCodecInfo[] codecs = ImageCodecInfo.GetImageEncoders();
            foreach (ImageCodecInfo codec in codecs) {
                if (codec.FormatID.Equals(this.guid))
                    return codec;
            }
            return null;
        }
#endif

        /// <include file='doc\ImageFormat.uex' path='docs/doc[@for="ImageFormat.ToString"]/*' />
        /// <devdoc>
        ///    Converts this <see cref='System.Drawing.Imaging.ImageFormat'/> to a human-readable string.
        /// </devdoc>
        public override string ToString() {
            if (this == memoryBMP) return "MemoryBMP";
            if (this == bmp) return "Bmp";
            if (this == emf) return "Emf";
            if (this == wmf) return "Wmf";
            if (this == gif) return "Gif";
            if (this == jpeg) return "Jpeg";
            if (this == png) return "Png";
            if (this == tiff) return "Tiff";
            if (this == exif) return "Exif";
            if (this == icon) return "Icon";
            return "[ImageFormat: " + guid + "]";
        }
    }
}
