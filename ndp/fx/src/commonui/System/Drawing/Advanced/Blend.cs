//------------------------------------------------------------------------------
// <copyright file="Blend.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System;
    using System.Drawing;

    /// <include file='doc\Blend.uex' path='docs/doc[@for="Blend"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines a blend pattern for a <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/>
    ///       .
    ///    </para>
    /// </devdoc>
    public sealed class Blend {
        float[] factors;
        float[] positions;

        /// <include file='doc\Blend.uex' path='docs/doc[@for="Blend.Blend"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Drawing2D.Blend'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public Blend() {
            factors = new float[1];
            positions = new float[1];
        }

        /// <include file='doc\Blend.uex' path='docs/doc[@for="Blend.Blend1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Drawing2D.Blend'/>
        ///       class with the specified number of factors and positions.
        ///    </para>
        /// </devdoc>
        public Blend(int count) {
            factors = new float[count];
            positions = new float[count];
        }
        /// <include file='doc\Blend.uex' path='docs/doc[@for="Blend.Factors"]/*' />
        /// <devdoc>
        ///    Specifies an array of blend factors for the
        ///    gradient.
        /// </devdoc>
        public float[] Factors { 
            get {
                return factors;
            }
            set {
                factors = value;
            }
        }

        /// <include file='doc\Blend.uex' path='docs/doc[@for="Blend.Positions"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies an array of blend positions for the gradient.
        ///    </para>
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
