//------------------------------------------------------------------------------
// <copyright file="PrintPreviewGraphics.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.Drawing {
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using Microsoft.Win32;
    using System.Security;
    using System.Security.Permissions;
    using System.Drawing.Internal;
    using System.Drawing.Imaging;
    using System.Drawing.Text;
    using System.Drawing.Drawing2D;
    using System.Drawing.Printing;
    using System.Runtime.Versioning;

    /// <include file='doc\PrintPreviewGraphics.uex' path='docs/doc[@for="PrintPreviewGraphics"]/*' />
    /// <devdoc>
    ///    <para> Retrives the printer graphics during preview.</para>
    /// </devdoc>
    internal class PrintPreviewGraphics  {

         
         private PrintPageEventArgs printPageEventArgs;
         private PrintDocument printDocument;

         public PrintPreviewGraphics(PrintDocument document, PrintPageEventArgs e) {
             printPageEventArgs = e;
             printDocument = document;
             
         }
        
        /// <include file='doc\PrintPreviewGraphics.uex' path='docs/doc[@for="PrintPreviewGraphics.VisibleClipBounds"]/*' />
        /// <devdoc>
        ///     Gets the Visible bounds of this graphics object. Used during print preview.    
        /// </devdoc>
        public RectangleF VisibleClipBounds {
            [ResourceExposure(ResourceScope.None)]
            [ResourceConsumption(ResourceScope.Process, ResourceScope.Process)]
            get {
                IntPtr hdevMode = printPageEventArgs.PageSettings.PrinterSettings.GetHdevmodeInternal();
                
                using( DeviceContext dc = printPageEventArgs.PageSettings.PrinterSettings.CreateDeviceContext(hdevMode)) {
                    using( Graphics graphics = Graphics.FromHdcInternal(dc.Hdc) ) {                
                        if (printDocument.OriginAtMargins) {
    
                            // Adjust the origin of the graphics object to be at the user-specified margin location
                            // Note: Graphics.FromHdc internally calls SaveDC(hdc), we can still use the saved hdc to get the resolution.
                            int dpiX = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(dc, dc.Hdc), SafeNativeMethods.LOGPIXELSX);
                            int dpiY = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(dc, dc.Hdc), SafeNativeMethods.LOGPIXELSY);
                            int hardMarginX_DU = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(dc, dc.Hdc), SafeNativeMethods.PHYSICALOFFSETX);
                            int hardMarginY_DU = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(dc, dc.Hdc), SafeNativeMethods.PHYSICALOFFSETY);                
                            float hardMarginX = hardMarginX_DU * 100 / dpiX;
                            float hardMarginY = hardMarginY_DU * 100 / dpiY;

                            graphics.TranslateTransform(-hardMarginX, -hardMarginY);
                            graphics.TranslateTransform(printDocument.DefaultPageSettings.Margins.Left, printDocument.DefaultPageSettings.Margins.Top);
                        }

                        return graphics.VisibleClipBounds;
                    }
                }
            }
        }
    }
}
