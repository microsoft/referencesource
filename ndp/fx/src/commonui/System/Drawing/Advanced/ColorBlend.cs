//------------------------------------------------------------------------------
// <copyright file="ColorBlend.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System;
    using System.Drawing;

    /// <include file='doc\ColorBlend.uex' path='docs/doc[@for="ColorBlend"]/*' />
    /// <devdoc>
    ///    Defines arrays of colors and positions used
    ///    for interpolating color blending in a gradient.
    /// </devdoc>
    public sealed class ColorBlend {
        Color[] colors;
        float[] positions;

        /// <include file='doc\ColorBlend.uex' path='docs/doc[@for="ColorBlend.ColorBlend"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Drawing2D.ColorBlend'/> class.
        /// </devdoc>
        public ColorBlend() {
            colors = new Color[1];
            positions = new float[1];
        }

        /// <include file='doc\ColorBlend.uex' path='docs/doc[@for="ColorBlend.ColorBlend1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Drawing2D.ColorBlend'/> class with the specified number of
        ///       colors and positions.
        ///    </para>
        /// </devdoc>
        public ColorBlend(int count) {
            colors = new Color[count];
            positions = new float[count];
        }
        
        /// <include file='doc\ColorBlend.uex' path='docs/doc[@for="ColorBlend.Colors"]/*' />
        /// <devdoc>
        ///    Represents an array of colors.
        /// </devdoc>
        public Color[] Colors {
            get {
                return colors;
            }
            set {
                colors = value;
            }
        }

        /// <include file='doc\ColorBlend.uex' path='docs/doc[@for="ColorBlend.Positions"]/*' />
        /// <devdoc>
        ///    Represents the positions along a gradient
        ///    line.
        /// </devdoc>
        public float[] Positions {
            get {
                return positions;
            }
            set {
                positions = value;
            }
        }

    }

}
