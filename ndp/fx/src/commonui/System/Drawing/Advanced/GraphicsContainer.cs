//------------------------------------------------------------------------------
// <copyright file="GraphicsContainer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;
    using System.Drawing;

    /**
     * Represent the internal data of a Graphics Container object
     */
    /// <include file='doc\GraphicsContainer.uex' path='docs/doc[@for="GraphicsContainer"]/*' />
    /// <devdoc>
    ///    Represents the internal data of a graphics
    ///    container.
    /// </devdoc>
    public sealed class GraphicsContainer : MarshalByRefObject {
        /**
         * @notes How do we want to expose region data?
         *
         * @notes Need serialization methods too.  Needs to be defined.
         */

        internal GraphicsContainer(int graphicsContainer)
        {
             nativeGraphicsContainer = graphicsContainer;
        }

        internal int nativeGraphicsContainer;
    }
}
