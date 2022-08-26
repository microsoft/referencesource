//------------------------------------------------------------------------------
// <copyright file="ColorMap.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {

    using System.Diagnostics;

    using System;
    using System.Drawing;

    /// <include file='doc\ColorMap.uex' path='docs/doc[@for="ColorMap"]/*' />
    /// <devdoc>
    ///    Defines a map for converting colors.
    /// </devdoc>
    public sealed class ColorMap {
        Color oldColor;
        Color newColor;
        
        /// <include file='doc\ColorMap.uex' path='docs/doc[@for="ColorMap.ColorMap"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Imaging.ColorMap'/> class.
        ///    </para>
        /// </devdoc>
        public ColorMap() {
            oldColor = new Color();
            newColor = new Color();
        }
        
        /// <include file='doc\ColorMap.uex' path='docs/doc[@for="ColorMap.OldColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the existing <see cref='System.Drawing.Color'/> to be
        ///       converted.
        ///    </para>
        /// </devdoc>
        public Color OldColor {
            get { return oldColor; }
            set { oldColor = value; }
        }
        /// <include file='doc\ColorMap.uex' path='docs/doc[@for="ColorMap.NewColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifes the new <see cref='System.Drawing.Color'/> to which to convert.
        ///    </para>
        /// </devdoc>
        public Color NewColor {
            get { return newColor; }
            set { newColor = value; }
        }
    }

}
