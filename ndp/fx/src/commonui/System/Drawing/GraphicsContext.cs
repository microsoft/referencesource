
//------------------------------------------------------------------------------
// <copyright file="Graphics.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing {
    using System;    
    using System.Drawing.Drawing2D;
    using System.Diagnostics;
    using System.Runtime.Versioning;

    /// <devdoc>
    ///     Contains information about the context of a Graphics object.
    /// </devdoc>
    internal class GraphicsContext : IDisposable {
        /// <devdoc>
        ///     The state that identifies the context.
        /// </devdoc>
        private int contextState;

        /// <devdoc>
        ///     The context's translate transform.
        /// </devdoc>
        private PointF transformOffset;

        /// <devdoc>
        ///     The context's clip region.
        /// </devdoc>
        private Region clipRegion;

        /// <devdoc>
        ///     The next context up the stack.
        /// </devdoc>
        private GraphicsContext nextContext;

        /// <devdoc>
        ///     The previous context down the stack.
        /// </devdoc>
        private GraphicsContext prevContext;

        /// <devdoc>
        ///     Flags that determines whether the context was created for a Graphics.Save() operation.
        ///     This kind of contexts are cumulative across subsequent Save() calls so the top context
        ///     info is cumulative.  This is not the same for contexts created for a Graphics.BeginContainer()
        ///     operation, in this case the new context information is reset.  See Graphics.BeginContainer()
        ///     and Graphics.Save() for more information.
        /// </devdoc>
        bool isCumulative;

        /// <devdoc>
        ///     Private constructor disallowed.
        /// </devdoc>
        private GraphicsContext() { 
        }

        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public GraphicsContext(Graphics g) {
            Matrix transform = g.Transform;
            if (!transform.IsIdentity) {
                float[] elements = transform.Elements;
                this.transformOffset.X = elements[4];
                this.transformOffset.Y = elements[5];
            }
            transform.Dispose();

            Region clip = g.Clip;
            if (clip.IsInfinite(g)) {
                clip.Dispose();
            }
            else {
                this.clipRegion = clip;
            }
        }

        /// <devdoc>
        ///     Disposes this and all contexts up the stack.
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <devdoc>
        ///     Disposes this and all contexts up the stack.
        /// </devdoc>
        public void Dispose(bool disposing) {
            if (this.nextContext != null) {
                // Dispose all contexts up the stack since they are relative to this one and its state will be invalid.
                this.nextContext.Dispose();
                this.nextContext = null;
            }

            if (this.clipRegion != null) {
                this.clipRegion.Dispose();
                this.clipRegion = null;
            }
        }

        /// <devdoc>
        ///     The state id representing the GraphicsContext.
        /// </devdoc>
        public int State {
            get {
                return this.contextState;
            }
            set {
                this.contextState = value;
            }
        }

        /// <devdoc>
        ///     The translate transform in the GraphicsContext.
        /// </devdoc>
        public PointF TransformOffset {
            get { 
                return this.transformOffset; 
            }
        }

        /// <devdoc>
        ///     The clipping region the GraphicsContext.
        /// </devdoc>
        public Region Clip {
            get { 
                return this.clipRegion; 
            }
        }

        /// <devdoc>
        ///     The next GraphicsContext object in the stack.
        /// </devdoc>
        public GraphicsContext Next {
            get {
                return this.nextContext;
            }
            set {
                this.nextContext = value;
            }
        }

        /// <devdoc>
        ///     The previous GraphicsContext object in the stack.
        /// </devdoc>
        public GraphicsContext Previous {
            get {
                return this.prevContext;
            }
            set {
                this.prevContext = value;
            }
        }

        /// <devdoc>
        ///     Determines whether this context is cumulative or not.  See filed for more info.
        /// </devdoc>
        public bool IsCumulative {
            get {
                return this.isCumulative;
            }
            set {
                this.isCumulative = value;
            }
        }
    }
}
