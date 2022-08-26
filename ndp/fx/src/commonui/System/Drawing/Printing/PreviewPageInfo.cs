//------------------------------------------------------------------------------
// <copyright file="PreviewPageInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using System.Diagnostics;
    using System;
    using System.Drawing;

    /// <include file='doc\PreviewPageInfo.uex' path='docs/doc[@for="PreviewPageInfo"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies print preview information for
    ///       a single page. This class cannot be inherited.
    ///    </para>
    /// </devdoc>
    public sealed class PreviewPageInfo {
        private Image image;

        // Physical measures in hundredths of an inch
        private Size physicalSize = Size.Empty;

        /// <include file='doc\PreviewPageInfo.uex' path='docs/doc[@for="PreviewPageInfo.PreviewPageInfo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Printing.PreviewPageInfo'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public PreviewPageInfo(Image image, Size physicalSize) {
            this.image = image;
            this.physicalSize = physicalSize;
        }

        /// <include file='doc\PreviewPageInfo.uex' path='docs/doc[@for="PreviewPageInfo.Image"]/*' />
        /// <devdoc>
        ///    <para>Gets the image of the printed page.</para>
        /// </devdoc>
        public Image Image {
            get { return image;}
        }

        // Physical measures in hundredths of an inch
        /// <include file='doc\PreviewPageInfo.uex' path='docs/doc[@for="PreviewPageInfo.PhysicalSize"]/*' />
        /// <devdoc>
        ///    <para> Gets the size of the printed page, in hundredths of an inch.</para>
        /// </devdoc>
        public Size PhysicalSize {
            get { return physicalSize;}
        }
    }
}
