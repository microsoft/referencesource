//------------------------------------------------------------------------------
// <copyright file="BufferedGraphics.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing {
    using System;
    using System.ComponentModel;
    using System.Collections;
    using System.Drawing;
    using System.Drawing.Drawing2D;
    using System.Drawing.Text;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Security;
    using System.Security.Permissions;
    using System.Diagnostics.CodeAnalysis;
    using System.Runtime.Versioning;
    
    /// <include file='doc\BufferedGraphics.uex' path='docs/doc[@for="BufferedGraphics"]/*' />
    /// <devdoc>
    ///         The BufferedGraphics class can be thought of as a "Token" or "Reference" to the
    ///         buffer that a BufferedGraphicsContext creates. While a BufferedGraphics is 
    ///         outstanding, the memory associated with the buffer is locked. The general design
    ///         is such that under normal conditions a single BufferedGraphics will be in use at
    ///         one time for a given BufferedGraphicsContext.
    /// </devdoc>
    [SuppressMessage("Microsoft.Usage", "CA2216:DisposableTypesShouldDeclareFinalizer")]
    public sealed class BufferedGraphics : IDisposable {
	    
        private Graphics                bufferedGraphicsSurface;
        private Graphics                targetGraphics;
        private BufferedGraphicsContext context;
        private IntPtr                  targetDC;
        private Point                   targetLoc;
        private Size                    virtualSize;
        private bool                    disposeContext;    
        private static int              rop = 0xcc0020; // RasterOp.SOURCE.GetRop();

        /// <include file='doc\BufferedGraphics.uex' path='docs/doc[@for="BufferedGraphics.BufferedGraphics"]/*' />
        /// <devdoc>
        ///         Internal constructor, this class is created by the BufferedGraphicsContext.
        /// </devdoc>
        internal BufferedGraphics(Graphics bufferedGraphicsSurface, BufferedGraphicsContext context, Graphics targetGraphics,
                                  IntPtr targetDC, Point targetLoc, Size virtualSize) {
            this.context = context;
            this.bufferedGraphicsSurface = bufferedGraphicsSurface;
            this.targetDC = targetDC;
            this.targetGraphics = targetGraphics;
            this.targetLoc = targetLoc;
            this.virtualSize = virtualSize;
        }

        ~BufferedGraphics() {
            Dispose(false);
        }

        /// <include file='doc\BufferedGraphics.uex' path='docs/doc[@for="BufferedGraphics.Dispose"]/*' />
        /// <devdoc>
        ///         Disposes the object and releases the lock on the memory.
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
        }

        private void Dispose(bool disposing) {
            if (disposing) {
                if (context != null) {
                    context.ReleaseBuffer(this);

                    if (DisposeContext) {
                        context.Dispose();
                        context = null;
                    }
                }
                if (bufferedGraphicsSurface != null) {
                    bufferedGraphicsSurface.Dispose();
                    bufferedGraphicsSurface = null;
                }
            }
        }
        
        /// <include file='doc\BufferedGraphics.uex' path='docs/doc[@for="BufferedGraphics.DisposeContext"]/*' />
        /// <devdoc>
        ///         Internal property - determines if we need to dispose of the Context when this is disposed
        /// </devdoc>
        internal bool DisposeContext {
            get {
                return disposeContext;
            }
            set {
                disposeContext = value;
            }
        }

        /// <include file='doc\BufferedGraphics.uex' path='docs/doc[@for="BufferedGraphics.Graphics"]/*' />
        /// <devdoc>
        ///         Allows access to the Graphics wrapper for the buffer.
        /// </devdoc>
        public Graphics Graphics {
            get {
                Debug.Assert(bufferedGraphicsSurface != null, "The BufferedGraphicsSurface is null!");
                return bufferedGraphicsSurface;
            }
        }

        /// <include file='doc\BufferedGraphics.uex' path='docs/doc[@for="BufferedGraphics.Render"]/*' />
        /// <devdoc>
        ///         Renders the buffer to the original graphics used to allocate the buffer.
        /// </devdoc>
        public void Render() {
            if (targetGraphics != null) {
                Render(targetGraphics);
            }
            else {
                RenderInternal(new HandleRef(Graphics, targetDC),  this);
            }
        }

        /// <include file='doc\BufferedGraphics.uex' path='docs/doc[@for="BufferedGraphics.Render1"]/*' />
        /// <devdoc>
        ///         Renders the buffer to the specified target graphics.
        /// </devdoc>
        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Process, ResourceScope.Process)]
        public void Render(Graphics target) {
            if (target != null) {
                IntPtr targetDC = target.GetHdc();

                try {
                    RenderInternal(new HandleRef(target, targetDC), this);
                }
                finally {
                    target.ReleaseHdcInternal(targetDC);
                }
            }
        }

        /// <include file='doc\BufferedGraphics.uex' path='docs/doc[@for="BufferedGraphics.Render2"]/*' />
        /// <devdoc>
        ///         Renders the buffer to the specified target HDC.
        /// </devdoc>
        public void Render(IntPtr targetDC) {
            IntSecurity.UnmanagedCode.Demand();

            RenderInternal(new HandleRef(null, targetDC), this);
        }

        /// <include file='doc\BufferedGraphics.uex' path='docs/doc[@for="BufferedGraphics.RenderInternal"]/*' />
        /// <devdoc>
        ///         Internal method that renders the specified buffer into the target.
        /// </devdoc>
        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Process, ResourceScope.Process)]
        private void RenderInternal(HandleRef refTargetDC, BufferedGraphics buffer) {
            IntPtr sourceDC = buffer.Graphics.GetHdc();

            try {
                SafeNativeMethods.BitBlt(refTargetDC, targetLoc.X, targetLoc.Y, virtualSize.Width, virtualSize.Height, 
                                         new HandleRef(buffer.Graphics, sourceDC), 0, 0, rop); 
            }
            finally {
                buffer.Graphics.ReleaseHdcInternal(sourceDC);
            }
        }

    }
}
