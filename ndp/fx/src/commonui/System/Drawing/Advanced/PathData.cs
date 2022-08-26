//------------------------------------------------------------------------------
// <copyright file="PathData.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System.Drawing;
    using System;

    /**
     * Represent the internal data of a path object
     */
    /// <include file='doc\PathData.uex' path='docs/doc[@for="PathData"]/*' />
    /// <devdoc>
    ///    Contains the graphical data that makes up a
    /// <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
    /// </devdoc>
    public sealed class PathData {
        PointF[] points;
        byte[] types;
        
        /// <include file='doc\PathData.uex' path='docs/doc[@for="PathData.PathData"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Drawing2D.PathData'/> class.
        /// </devdoc>
        public PathData() {
        }

        /// <include file='doc\PathData.uex' path='docs/doc[@for="PathData.Points"]/*' />
        /// <devdoc>
        ///    Contains an array of <see cref='System.Drawing.PointF'/> objects
        ///    that represent the points through which the path is constructed.
        /// </devdoc>
        public PointF[] Points {
            get {
                return points;
            }
            set {
                points = value;
            }
        }

        /// <include file='doc\PathData.uex' path='docs/doc[@for="PathData.Types"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Contains an array of <see cref='System.Drawing.Drawing2D.PathPointType'/> objects that represent the types of
        ///       data in the corresponding elements of the <see cref='System.Drawing.Drawing2D.PathData. points'/> array.
        ///    </para>
        /// </devdoc>
        public byte[] Types {
            get {
                return types;
            }
            set {
                types = value;
            }
        }

    }

}
