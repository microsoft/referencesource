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
    using System.Runtime.Versioning;

    /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext"]/*' />
    /// <devdoc>
    ///         The BufferedGraphicsContext class can be used to perform standard double buffer
    ///         rendering techniques.
    /// </devdoc>
    public sealed class BufferedGraphicsContext : IDisposable {
        
        private Size                    maximumBuffer;
        private Size                    bufferSize;
        private Size                    virtualSize;
        private Point                   targetLoc;
        private IntPtr                  compatDC;
        private IntPtr                  dib;
        private IntPtr                  oldBitmap;
        private Graphics                compatGraphics;
        private BufferedGraphics        buffer;
        private int                     busy;
        private bool                    invalidateWhenFree;

        private const int               BUFFER_FREE = 0; //the graphics buffer is free to use
        private const int               BUFFER_BUSY_PAINTING = 1; //graphics buffer is busy being created/painting
        private const int               BUFFER_BUSY_DISPOSING = 2; //graphics buffer is busy disposing
        
        static TraceSwitch              doubleBuffering;
        
        #if DEBUG
        string stackAtBusy;
        #endif
        
        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.BufferedGraphicsContext"]/*' />
        /// <devdoc>
        ///         Basic constructor.
        /// </devdoc>
        public BufferedGraphicsContext() {
            //by defualt, the size of our maxbuffer will be 3 x standard button size
            maximumBuffer.Width = 75 * 3;
            maximumBuffer.Height = 32 * 3;

            bufferSize = Size.Empty;
        }

        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.Finalizer"]/*' />
        /// <devdoc>
        ///         Destructor.
        /// </devdoc>
        ~BufferedGraphicsContext() {
            Dispose(false);
        }

        //Internal trace switch for debugging
        //
        internal static TraceSwitch DoubleBuffering {
            get {
                if (doubleBuffering == null) {
                    doubleBuffering = new TraceSwitch("DoubleBuffering", "Output information about double buffering");
                }
                return doubleBuffering;
            }
        }

        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.MaximumBuffer"]/*' />
        /// <devdoc>
        ///         Allows you to set the maximum width and height of the buffer that will be retained in memory.
        ///         You can allocate a buffer of any size, however any request for a buffer that would have a total
        ///         memory footprint larger that the maximum size will be allocated temporarily and then discarded 
        ///         with the BufferedGraphics is released.
        /// </devdoc>
        public Size MaximumBuffer {
            get {
                return maximumBuffer;
            }
            [UIPermission(SecurityAction.Demand, Window=UIPermissionWindow.AllWindows)]
            set {
                if (value.Width <= 0 || value.Height <= 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument, "MaximumBuffer", value));
                }

                //if we've been asked to decrease the size of the maximum buffer,
                //then invalidate the older & larger buffer
                //
                if (value.Width * value.Height < maximumBuffer.Width * maximumBuffer.Height) {
                    Invalidate();
                }

                maximumBuffer = value;
            }
        }

        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.Allocate"]/*' />
        /// <devdoc>
        ///         Returns a BufferedGraphics that is matched for the specified target Graphics object.
        /// </devdoc>
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public BufferedGraphics Allocate(Graphics targetGraphics, Rectangle targetRectangle) {
            if (ShouldUseTempManager(targetRectangle)) {
                Debug.WriteLineIf(DoubleBuffering.TraceWarning, "Too big of buffer requested (" + targetRectangle.Width + " x " + targetRectangle.Height + ") ... allocating temp buffer manager");
                return AllocBufferInTempManager(targetGraphics, IntPtr.Zero, targetRectangle);
            }
            return AllocBuffer(targetGraphics, IntPtr.Zero, targetRectangle);
        }

        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.Allocate1"]/*' />
        /// <devdoc>
        ///         Returns a BufferedGraphics that is matched for the specified target HDC object.
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public BufferedGraphics Allocate(IntPtr targetDC, Rectangle targetRectangle) {
            if (ShouldUseTempManager(targetRectangle)) {
                Debug.WriteLineIf(DoubleBuffering.TraceWarning, "Too big of buffer requested (" + targetRectangle.Width + " x " + targetRectangle.Height + ") ... allocating temp buffer manager");
                return AllocBufferInTempManager(null, targetDC, targetRectangle);
            }
            return AllocBuffer(null, targetDC, targetRectangle);
        }

        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.AllocBuffer"]/*' />
        /// <devdoc>
        ///         Returns a BufferedGraphics that is matched for the specified target HDC object.
        /// </devdoc>
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        private BufferedGraphics AllocBuffer(Graphics targetGraphics, IntPtr targetDC, Rectangle targetRectangle) {

            int oldBusy = Interlocked.CompareExchange(ref busy, BUFFER_BUSY_PAINTING, BUFFER_FREE);

            // In the case were we have contention on the buffer - i.e. two threads 
            // trying to use the buffer at the same time, we just create a temp 
            // buffermanager and have the buffer dispose of it when it is done.
            //
            if (oldBusy != BUFFER_FREE) {
                Debug.WriteLineIf(DoubleBuffering.TraceWarning, "Attempt to have two buffers for a buffer manager... allocating temp buffer manager");
                return AllocBufferInTempManager(targetGraphics, targetDC, targetRectangle);
            }

            #if DEBUG
            if (DoubleBuffering.TraceVerbose) {
                stackAtBusy = new StackTrace().ToString();
            }
            #endif

            Graphics surface;
            this.targetLoc = new Point(targetRectangle.X, targetRectangle.Y);

            try {
                if (targetGraphics != null) {
                    IntPtr destDc = targetGraphics.GetHdc();
                    try {
                        surface = CreateBuffer(destDc, -targetLoc.X, -targetLoc.Y, targetRectangle.Width, targetRectangle.Height);
                    }
                    finally {
                        targetGraphics.ReleaseHdcInternal(destDc);
                    }
                }
                else {
                    surface = CreateBuffer(targetDC, -targetLoc.X, -targetLoc.Y, targetRectangle.Width, targetRectangle.Height);
                }

                this.buffer = new BufferedGraphics(surface, this, targetGraphics, targetDC, targetLoc, virtualSize);
            }
            catch {
                this.busy = BUFFER_FREE; // free the buffer so it can be disposed.
                throw;
            }
            return this.buffer;
        }

        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.AllocBufferInTempManager"]/*' />
        /// <devdoc>
        ///         Returns a BufferedGraphics that is matched for the specified target HDC object.
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:DisposeObjectsBeforeLosingScope")]
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        private BufferedGraphics AllocBufferInTempManager(Graphics targetGraphics, IntPtr targetDC, Rectangle targetRectangle) {
            BufferedGraphicsContext tempContext = null;
            BufferedGraphics tempBuffer = null;
            
            try {
                tempContext = new BufferedGraphicsContext();
                if (tempContext != null) {                
                    tempBuffer = tempContext.AllocBuffer(targetGraphics, targetDC, targetRectangle);
                    tempBuffer.DisposeContext = true;
                }
            }
            finally {
                if (tempContext != null && (tempBuffer == null || (tempBuffer != null && !tempBuffer.DisposeContext))) {
                    tempContext.Dispose();
                }
            }
                
            return tempBuffer;
        }


        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.bFillBitmapInfo"]/*' />
        /// <devdoc>
        // bFillBitmapInfo
        //
        // Fills in the fields of a BITMAPINFO so that we can create a bitmap
        // that matches the format of the display.
        //
        // This is done by creating a compatible bitmap and calling GetDIBits
        // to return the color masks.  This is done with two calls.  The first
        // call passes in biBitCount = 0 to GetDIBits which will fill in the
        // base BITMAPINFOHEADER data.  The second call to GetDIBits (passing
        // in the BITMAPINFO filled in by the first call) will return the color
        // table or bitmasks, as appropriate.
        //
        // Returns:
        //   TRUE if successful, FALSE otherwise.
        //
        // History:
        //  07-Jun-1995 -by- Gilman Wong [Microsoft]
        // Wrote it.
        //
        //  15-Nov-2000 -by- Chris Anderson [Microsoft]
        // Ported it to C#
        //
        /// </devdoc>
        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Process, ResourceScope.Process)]
        private bool bFillBitmapInfo(IntPtr hdc, IntPtr hpal, ref NativeMethods.BITMAPINFO_FLAT pbmi) {
            IntPtr hbm = IntPtr.Zero;
            bool bRet = false;
            try {

                //
                // Create a dummy bitmap from which we can query color format info
                // about the device surface.
                //
                hbm = SafeNativeMethods.CreateCompatibleBitmap(new HandleRef(null, hdc), 1, 1);

                if (hbm == IntPtr.Zero) {
                    throw new OutOfMemoryException(SR.GetString(SR.GraphicsBufferQueryFail));
                }

                pbmi.bmiHeader_biSize = Marshal.SizeOf(typeof(NativeMethods.BITMAPINFOHEADER));
                pbmi.bmiColors = new byte[NativeMethods.BITMAPINFO_MAX_COLORSIZE*4];

                //
                // Call first time to fill in BITMAPINFO header.
                //
                SafeNativeMethods.GetDIBits(new HandleRef(null, hdc), 
                                                    new HandleRef(null, hbm), 
                                                    0, 
                                                    0, 
                                                    IntPtr.Zero, 
                                                    ref pbmi, 
                                                    NativeMethods.DIB_RGB_COLORS);

                if ( pbmi.bmiHeader_biBitCount <= 8 ) {
                    bRet = bFillColorTable(hdc, hpal, ref pbmi);
                }
                else {
                    if ( pbmi.bmiHeader_biCompression == NativeMethods.BI_BITFIELDS ) {

                        //
                        // Call a second time to get the color masks.
                        // It's a GetDIBits Win32 "feature".
                        //
                        SafeNativeMethods.GetDIBits(new HandleRef(null, hdc), 
                                                new HandleRef(null, hbm), 
                                                0, 
                                                pbmi.bmiHeader_biHeight, 
                                                IntPtr.Zero, 
                                                ref pbmi,
                                                NativeMethods.DIB_RGB_COLORS);
                    }
                    bRet = true;
                }
            }
            finally {
                if (hbm != IntPtr.Zero) {
                    SafeNativeMethods.DeleteObject(new HandleRef(null, hbm));
                    hbm = IntPtr.Zero;
                }
            }
            return bRet;
        }

        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.bFillColorTable"]/*' />
        /// <devdoc>
        // bFillColorTable
        //
        // Initialize the color table of the BITMAPINFO pointed to by pbmi.  Colors
        // are set to the current system palette.
        //
        // Note: call only valid for displays of 8bpp or less.
        //
        // Returns:
        //   TRUE if successful, FALSE otherwise.
        //
        // History:
        //  23-Jan-1996 -by- Gilman Wong [Microsoft]
        // Wrote it.
        //
        //  15-Nov-2000 -by- Chris Anderson [Microsoft]
        // Ported it to C#
        //
        /// </devdoc>
        private unsafe bool bFillColorTable(IntPtr hdc, IntPtr hpal, ref NativeMethods.BITMAPINFO_FLAT pbmi) {
            bool bRet = false;
            byte[] aj = new byte[sizeof(NativeMethods.PALETTEENTRY) * 256];
            int i, cColors;

            fixed (byte* pcolors = pbmi.bmiColors) {
                fixed (byte* ppal = aj) {
                    NativeMethods.RGBQUAD* prgb = (NativeMethods.RGBQUAD*)pcolors;
                    NativeMethods.PALETTEENTRY* lppe = (NativeMethods.PALETTEENTRY*)ppal;

                    cColors = 1 << pbmi.bmiHeader_biBitCount;
                    if ( cColors <= 256 ) {
                        Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "8 bit or less...");

                        // NOTE : Didn't port "MyGetPaletteEntries" as it is only
                        //      : for 4bpp displays, which we don't work on anyway.
                        uint palRet;
                        IntPtr palHalftone = IntPtr.Zero;
                        if (hpal == IntPtr.Zero) {
                            Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "using halftone palette...");
                            palHalftone = Graphics.GetHalftonePalette();
                            palRet = SafeNativeMethods.GetPaletteEntries(new HandleRef(null, palHalftone), 0, cColors, aj);
                        }
                        else {
                            Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "using custom palette...");
                            palRet = SafeNativeMethods.GetPaletteEntries(new HandleRef(null, hpal), 0, cColors, aj);
                        }
                        if ( palRet != 0 ) {
                            for (i = 0; i < cColors; i++) {
                                prgb[i].rgbRed      = lppe[i].peRed;
                                prgb[i].rgbGreen    = lppe[i].peGreen;
                                prgb[i].rgbBlue     = lppe[i].peBlue;
                                prgb[i].rgbReserved = 0;
                            }
                            bRet = true;
                        }
                        else {
                            Debug.WriteLineIf(DoubleBuffering.TraceWarning, "bFillColorTable: MyGetSystemPaletteEntries failed\n");
                        }
                    }
                }
            }
            return bRet;
        }

        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.CreateBuffer"]/*' />
        /// <devdoc>
        ///         Returns a Graphics object representing a buffer.
        /// </devdoc>
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        private Graphics CreateBuffer(IntPtr src, int offsetX, int offsetY, int width, int height)
        {
            //create the compat DC
            busy = BUFFER_BUSY_DISPOSING;
            DisposeDC();
            busy = BUFFER_BUSY_PAINTING;
            compatDC = UnsafeNativeMethods.CreateCompatibleDC(new HandleRef(null, src));

            //recreate the bitmap if necessary
            if (width > bufferSize.Width || height > bufferSize.Height)
            {
                Debug.WriteLineIf(DoubleBuffering.TraceInfo, "allocating new bitmap: " + width + " x " + height);
                int optWidth = Math.Max(width, bufferSize.Width);
                int optHeight = Math.Max(height, bufferSize.Height);

                busy = BUFFER_BUSY_DISPOSING;
                DisposeBitmap();
                busy = BUFFER_BUSY_PAINTING;

                Debug.WriteLineIf(DoubleBuffering.TraceInfo, "    new size         : " + optWidth + " x " + optHeight);
                IntPtr pvbits = IntPtr.Zero;
                dib = CreateCompatibleDIB(src, IntPtr.Zero, optWidth, optHeight, ref pvbits);
                bufferSize = new Size(optWidth, optHeight);
            }

            //select the bitmap
            oldBitmap = SafeNativeMethods.SelectObject(new HandleRef(this, compatDC), new HandleRef(this, dib));

            //create compat graphics
            Debug.WriteLineIf(DoubleBuffering.TraceInfo, "    Create compatGraphics");
            compatGraphics = Graphics.FromHdcInternal(compatDC);
            compatGraphics.TranslateTransform(-targetLoc.X, -targetLoc.Y);
            virtualSize = new Size(width, height);

            return compatGraphics;
        }

        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.CreateCompatibleDIB"]/*' />
        /// <devdoc>
        // CreateCompatibleDIB
        //
        // Create a DIB section with an optimal format w.r.t. the specified hdc.
        //
        // If DIB <= 8bpp, then the DIB color table is initialized based on the
        // specified palette.  If the palette handle is NULL, then the system
        // palette is used.
        //
        // Note: The hdc must be a direct DC (not an info or memory DC).
        //
        // Note: On palettized displays, if the system palette changes the
        //       UpdateDIBColorTable function should be called to maintain
        //       the identity palette mapping between the DIB and the display.
        //
        // Returns:
        //   Valid bitmap handle if successful, NULL if error.
        //
        // History:
        //  23-Jan-1996 -by- Gilman Wong [Microsoft]
        // Wrote it.
        //
        //  15-Nov-2000 -by- Chris Anderson [Microsoft]
        // Ported it to C#.
        //
        /// </devdoc>        
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Interoperability", "CA1404:CallGetLastErrorImmediatelyAfterPInvoke")]
        private IntPtr CreateCompatibleDIB(IntPtr hdc, IntPtr hpal, int ulWidth, int ulHeight, ref IntPtr ppvBits) {
            if (hdc == IntPtr.Zero) {
                throw new ArgumentNullException("hdc");
            }

            IntPtr hbmRet = IntPtr.Zero;
            NativeMethods.BITMAPINFO_FLAT pbmi = new NativeMethods.BITMAPINFO_FLAT();

            //
            // Validate hdc.
            //
            int objType = UnsafeNativeMethods.GetObjectType(new HandleRef(null, hdc));

            switch(objType) {
                case NativeMethods.OBJ_DC:
                case NativeMethods.OBJ_METADC:
                case NativeMethods.OBJ_MEMDC:
                case NativeMethods.OBJ_ENHMETADC:
                    break;
                default:
                    throw new ArgumentException(SR.GetString(SR.DCTypeInvalid));
            }

            if (bFillBitmapInfo(hdc, hpal, ref pbmi)) {

                //
                // Change bitmap size to match specified dimensions.
                //

                pbmi.bmiHeader_biWidth = ulWidth;
                pbmi.bmiHeader_biHeight = ulHeight;
                if (pbmi.bmiHeader_biCompression == NativeMethods.BI_RGB) {
                    pbmi.bmiHeader_biSizeImage = 0;
                }
                else {
                    if ( pbmi.bmiHeader_biBitCount == 16 )
                        pbmi.bmiHeader_biSizeImage = ulWidth * ulHeight * 2;
                    else if ( pbmi.bmiHeader_biBitCount == 32 )
                        pbmi.bmiHeader_biSizeImage = ulWidth * ulHeight * 4;
                    else
                        pbmi.bmiHeader_biSizeImage = 0;
                }
                pbmi.bmiHeader_biClrUsed = 0;
                pbmi.bmiHeader_biClrImportant = 0;

                //
                // Create the DIB section.  Let Win32 allocate the memory and return
                // a pointer to the bitmap surface.
                //

                hbmRet = SafeNativeMethods.CreateDIBSection(new HandleRef(null, hdc), ref pbmi, NativeMethods.DIB_RGB_COLORS, ref ppvBits, IntPtr.Zero, 0);
                Win32Exception ex = null;
                if ( hbmRet == IntPtr.Zero ) {
                    ex = new Win32Exception(Marshal.GetLastWin32Error());             
#if DEBUG
                    DumpBitmapInfo(ref pbmi);
#endif
                }

#if DEBUG
                if (DoubleBuffering.TraceVerbose) {
                    DumpBitmapInfo(ref pbmi);
                }
#endif
                if(ex!=null) {
                    throw ex;
                }

            }
            return hbmRet;
        }

        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of native handles.
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <devdoc>
        ///         Disposes the DC, but leaves the bitmap alone.
        /// </devdoc>
        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Process, ResourceScope.Process)]
        private void DisposeDC()
        {
            if (oldBitmap != IntPtr.Zero && compatDC != IntPtr.Zero)
            {
                Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "restoring bitmap to DC");
                SafeNativeMethods.SelectObject(new HandleRef(this, compatDC), new HandleRef(this, oldBitmap));
                oldBitmap = IntPtr.Zero;
            }
            if (compatDC != IntPtr.Zero)
            {
                Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "delete compat DC");
                UnsafeNativeMethods.DeleteDC(new HandleRef(this, compatDC));
                compatDC = IntPtr.Zero;
            }
        }

        /// <devdoc>
        ///         Disposes the bitmap, will ASSERT if bitmap is being used (checks oldbitmap).
        ///         if ASSERTed, call DisposeDC() first.
        /// </devdoc>
        private void DisposeBitmap()
        {
            if (dib != IntPtr.Zero)
            {
                Debug.Assert(oldBitmap == IntPtr.Zero);
                Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "delete dib");

                SafeNativeMethods.DeleteObject(new HandleRef(this, dib));
                dib = IntPtr.Zero;
            }
        }

        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.Dispose1"]/*' />
        /// <devdoc>
        ///     Disposes of the Graphics buffer.
        /// </devdoc>
        private void Dispose(bool disposing) {
            Debug.WriteLineIf(DoubleBuffering.TraceInfo, "Dispose(" + disposing + ") {");
            Debug.Indent();
            int oldBusy = Interlocked.CompareExchange(ref busy, BUFFER_BUSY_DISPOSING, BUFFER_FREE);

            if(disposing){
                if (oldBusy == BUFFER_BUSY_PAINTING) {
                    #if DEBUG
                        Debug.WriteLineIf(DoubleBuffering.TraceInfo, "Stack at busy buffer: \n" + stackAtBusy);
                    #endif
                    
                    throw new InvalidOperationException(SR.GetString(SR.GraphicsBufferCurrentlyBusy));
                }
                
                if (compatGraphics != null) {
                    Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "Disposing compatGraphics");
                    compatGraphics.Dispose();
                    compatGraphics = null;
                }
            }
            else{
                Debug.Fail("Never let a graphics buffer finalize!");
            }

            DisposeDC();
            DisposeBitmap();

            if (buffer != null) {
                Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "Disposing buffer");
                buffer.Dispose();
                buffer = null;
            }
            
            bufferSize = Size.Empty;
            virtualSize = Size.Empty;
            Debug.Unindent();
            Debug.WriteLineIf(DoubleBuffering.TraceInfo, "}");

            this.busy = BUFFER_FREE;
        }

#if DEBUG
        private void DumpBitmapInfo(ref NativeMethods.BITMAPINFO_FLAT pbmi) {
            //Debug.WriteLine("biSize --> " + pbmi.bmiHeader_biSize);
            Debug.WriteLine("biWidth --> " + pbmi.bmiHeader_biWidth);
            Debug.WriteLine("biHeight --> " + pbmi.bmiHeader_biHeight);
            Debug.WriteLine("biPlanes --> " + pbmi.bmiHeader_biPlanes);
            Debug.WriteLine("biBitCount --> " + pbmi.bmiHeader_biBitCount);
            //Debug.WriteLine("biCompression --> " + pbmi.bmiHeader_biCompression);
            //Debug.WriteLine("biSizeImage --> " + pbmi.bmiHeader_biSizeImage);
            //Debug.WriteLine("biXPelsPerMeter --> " + pbmi.bmiHeader_biXPelsPerMeter);
            //Debug.WriteLine("biYPelsPerMeter --> " + pbmi.bmiHeader_biYPelsPerMeter);
            //Debug.WriteLine("biClrUsed --> " + pbmi.bmiHeader_biClrUsed);
            //Debug.WriteLine("biClrImportant --> " + pbmi.bmiHeader_biClrImportant);
            //Debug.Write("bmiColors --> ");
            //for (int i=0; i<pbmi.bmiColors.Length; i++) {
            //    Debug.Write(pbmi.bmiColors[i].ToString("X"));
            //}
            Debug.WriteLine("");
        }
#endif
        
        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.Invalidate"]/*' />
        /// <devdoc>
        ///         Invalidates the cached graphics buffer.
        /// </devdoc>
        public void Invalidate() {
            int oldBusy = Interlocked.CompareExchange(ref busy, BUFFER_BUSY_DISPOSING, BUFFER_FREE);

            //if we're not busy with our buffer, lets
            //clean it up now
            //
            if (oldBusy == BUFFER_FREE) {
                Dispose();
                this.busy = BUFFER_FREE;
            }
            else {
                //this will indicate to free the buffer 
                //as soon as it becomes non-busy
                //
                this.invalidateWhenFree = true;
            }
        }

        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.ReleaseBuffer"]/*' />
        /// <devdoc>
        ///         Returns a Graphics object representing a buffer.
        /// </devdoc>
        internal void ReleaseBuffer(BufferedGraphics buffer) {
            Debug.Assert(buffer == this.buffer, "Tried to release a bogus buffer");

            this.buffer = null;
            if (this.invalidateWhenFree) {
                this.busy = BUFFER_BUSY_DISPOSING;
                Dispose(); //clears everything (incl bitmap)
            }
            else {  //otherwise, just dispose the DC.  A new one will be created next time.
                this.busy = BUFFER_BUSY_DISPOSING;
                DisposeDC(); //only clears out the DC
            }

            this.busy = BUFFER_FREE;
        }
        
        /// <include file='doc\BufferedGraphicsContext.uex' path='docs/doc[@for="BufferedGraphicsContext.ShouldUseTempManager"]/*' />
        /// <devdoc>
        ///         This routine allows us to control the point were we start using throw away
        ///         managers for painting. Since the buffer manager stays around (by default)
        ///         for the life of the app, we don't want to consume too much memory
        ///         in the buffer. However, re-allocating the buffer for small things (like
        ///         buttons, labels, etc) will hit us on runtime performance.
        /// </devdoc>
        private bool ShouldUseTempManager(Rectangle targetBounds) {
            return (targetBounds.Width * targetBounds.Height) > (MaximumBuffer.Width * MaximumBuffer.Height);
        }

    }
}
