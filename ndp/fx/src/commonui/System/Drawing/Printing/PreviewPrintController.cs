//------------------------------------------------------------------------------
// <copyright file="PreviewPrintController.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using Microsoft.Win32;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Internal;
    using System.Drawing.Drawing2D;
    using System.Drawing.Imaging;
    using System.Drawing.Text;
    using System.Runtime.InteropServices;
    using System.Runtime.Versioning;

    using CodeAccessPermission = System.Security.CodeAccessPermission;

    /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController"]/*' />
    /// <devdoc>
    ///     A PrintController which "prints" to a series of images.
    /// </devdoc>
    public class PreviewPrintController : PrintController {
        private IList list = new ArrayList(); // list of PreviewPageInfo
        private System.Drawing.Graphics graphics;
        private DeviceContext dc;
        private bool antiAlias;

        private void CheckSecurity() {
            IntSecurity.SafePrinting.Demand();
        }

        /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController.IsPreview"]/*' />
        /// <devdoc>
        ///    <para>
        ///       This is new public property which notifies if this controller is used for PrintPreview.
        ///    </para>
        /// </devdoc>
        public override bool IsPreview {
            get {
                return true;
            }
        }

        /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController.OnStartPrint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements StartPrint for generating print preview information.
        ///    </para>
        /// </devdoc>
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public override void OnStartPrint(PrintDocument document, PrintEventArgs e) {
            Debug.Assert(dc == null && graphics == null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity();

            base.OnStartPrint(document, e);

            
            try {
                
                if (!document.PrinterSettings.IsValid)
                throw new InvalidPrinterException(document.PrinterSettings);

                IntSecurity.AllPrintingAndUnmanagedCode.Assert();
                
                // We need a DC as a reference; we don't actually draw on it.
                // We make sure to reuse the same one to improve performance.
                dc = document.PrinterSettings.CreateInformationContext(modeHandle);
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }
            
        }

        /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController.OnStartPage"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements StartEnd for generating print preview information.
        ///    </para>
        /// </devdoc>
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Machine | ResourceScope.Process, ResourceScope.Machine)]
        public override Graphics OnStartPage(PrintDocument document, PrintPageEventArgs e) {
            Debug.Assert(dc != null && graphics == null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity();

            base.OnStartPage(document, e);


            try {
                IntSecurity.AllPrintingAndUnmanagedCode.Assert();
                if (e.CopySettingsToDevMode) {
                    e.PageSettings.CopyToHdevmode(modeHandle);
                }

                Size size = e.PageBounds.Size;

                // Metafile framing rectangles apparently use hundredths of mm as their unit of measurement,
                // instead of the GDI+ standard hundredth of an inch.
                Size metafileSize = PrinterUnitConvert.Convert(size, PrinterUnit.Display, PrinterUnit.HundredthsOfAMillimeter);

                // Create a Metafile which accepts only GDI+ commands since we are the ones creating
                // and using this ...
                // Framework creates a dual-mode EMF for each page in the preview. 
                // When these images are displayed in preview, 
                // they are added to the dual-mode EMF. However, 
                // GDI+ breaks during this process if the image 
                // is sufficiently large and has more than 254 colors. 
                // This code path can easily be avoided by requesting
                // an EmfPlusOnly EMF..
                Metafile metafile = new Metafile(dc.Hdc, new Rectangle(0,0, metafileSize.Width, metafileSize.Height), MetafileFrameUnit.GdiCompatible, EmfType.EmfPlusOnly);
    
                PreviewPageInfo info = new PreviewPageInfo(metafile, size);
                list.Add(info);
                PrintPreviewGraphics printGraphics = new PrintPreviewGraphics(document, e);
                graphics = Graphics.FromImage(metafile);

                if (graphics != null && document.OriginAtMargins) {
                    
                    // Adjust the origin of the graphics object to be at the
                    // user-specified margin location
                    //
                    int dpiX = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(dc, dc.Hdc), SafeNativeMethods.LOGPIXELSX);
                    int dpiY = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(dc, dc.Hdc), SafeNativeMethods.LOGPIXELSY);
                    int hardMarginX_DU = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(dc, dc.Hdc), SafeNativeMethods.PHYSICALOFFSETX);
                    int hardMarginY_DU = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(dc, dc.Hdc), SafeNativeMethods.PHYSICALOFFSETY);                
                    float hardMarginX = hardMarginX_DU * 100 / dpiX;
                    float hardMarginY = hardMarginY_DU * 100 / dpiY;
                    
                    graphics.TranslateTransform(-hardMarginX, -hardMarginY);
                    graphics.TranslateTransform(document.DefaultPageSettings.Margins.Left, document.DefaultPageSettings.Margins.Top);
                }


                graphics.PrintingHelper = printGraphics;
                
    
                if (antiAlias) {
                    graphics.TextRenderingHint = TextRenderingHint.AntiAlias;
                    graphics.SmoothingMode = SmoothingMode.AntiAlias;
                }
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }
            return graphics;
        }

        /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController.OnEndPage"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements EndPage for generating print preview information.
        ///    </para>
        /// </devdoc>
        public override void OnEndPage(PrintDocument document, PrintPageEventArgs e) {
            Debug.Assert(dc != null && graphics != null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity();

            graphics.Dispose();
            graphics = null;

            base.OnEndPage(document, e);
        }

        /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController.OnEndPrint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements EndPrint for generating print preview information.
        ///    </para>
        /// </devdoc>
        public override void OnEndPrint(PrintDocument document, PrintEventArgs e) {
            Debug.Assert(dc != null && graphics == null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods are called in any particular order
            CheckSecurity();

            dc.Dispose();
            dc = null;

            base.OnEndPrint(document, e);
        }

        /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController.GetPreviewPageInfo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The "printout".
        ///    </para>
        /// </devdoc>
        public PreviewPageInfo[] GetPreviewPageInfo() {
            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity();

            PreviewPageInfo[] temp = new PreviewPageInfo[list.Count];
            list.CopyTo(temp, 0);
            return temp;
        }

        /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController.UseAntiAlias"]/*' />
        public virtual bool UseAntiAlias {
            get {
                return antiAlias;
            }
            set {
                antiAlias = value;
            }
        }
    }
}

