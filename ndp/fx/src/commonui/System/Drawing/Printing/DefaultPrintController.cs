//------------------------------------------------------------------------------
// <copyright file="DefaultPrintController.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using Microsoft.Win32;
    using System.ComponentModel;
    using System;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Internal;
    using System.Runtime.InteropServices;
    using System.Runtime.Versioning;

    using CodeAccessPermission = System.Security.CodeAccessPermission;

    /// <include file='doc\DefaultPrintController.uex' path='docs/doc[@for="StandardPrintController"]/*' />
    /// <devdoc>
    ///    <para>Specifies a print controller that sends information to a printer.
    ///       </para>
    /// </devdoc>
    public class StandardPrintController : PrintController {
        private DeviceContext dc;
        private Graphics graphics;

        private void CheckSecurity(PrintDocument document) {
            if (document.PrinterSettings.PrintDialogDisplayed) {
                IntSecurity.SafePrinting.Demand();
            }
            else if (document.PrinterSettings.IsDefaultPrinter) {
                IntSecurity.DefaultPrinting.Demand();
            }
            else {
                IntSecurity.AllPrinting.Demand();
            }
        }

        /// <include file='doc\DefaultPrintController.uex' path='docs/doc[@for="StandardPrintController.OnStartPrint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements StartPrint for printing to a physical printer.
        ///    </para>
        /// </devdoc>
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public override void OnStartPrint(PrintDocument document, PrintEventArgs e) {
            Debug.Assert(dc == null && graphics == null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity(document);

            base.OnStartPrint(document, e);
            // the win32 methods below SuppressUnmanagedCodeAttributes so assertin on UnmanagedCodePermission is redundant
            if (!document.PrinterSettings.IsValid)
                throw new InvalidPrinterException(document.PrinterSettings);

            dc = document.PrinterSettings.CreateDeviceContext(modeHandle);
            SafeNativeMethods.DOCINFO info = new SafeNativeMethods.DOCINFO();
            info.lpszDocName = document.DocumentName;
            if (document.PrinterSettings.PrintToFile)
                info.lpszOutput = document.PrinterSettings.OutputPort; //This will be "FILE:"
            else
                info.lpszOutput = null;
            info.lpszDatatype = null;
            info.fwType = 0;

            int result = SafeNativeMethods.StartDoc(new HandleRef(this.dc, dc.Hdc), info);
            if (result <= 0) {
                int error = Marshal.GetLastWin32Error();
                if (error == SafeNativeMethods.ERROR_CANCELLED) {
                    e.Cancel = true;
                }
                else {
                    throw new Win32Exception(error);
                }
            }
        }

        /// <include file='doc\DefaultPrintController.uex' path='docs/doc[@for="StandardPrintController.OnStartPage"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements StartPage for printing to a physical printer.
        ///    </para>
        /// </devdoc>
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public override Graphics OnStartPage(PrintDocument document, PrintPageEventArgs e) {
            Debug.Assert(dc != null && graphics == null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity(document);

            base.OnStartPage(document, e);
            try {
            
                IntSecurity.AllPrintingAndUnmanagedCode.Assert();
    
                e.PageSettings.CopyToHdevmode(modeHandle);
                IntPtr modePointer = SafeNativeMethods.GlobalLock(new HandleRef(this, modeHandle));
                try {
                    IntPtr result = SafeNativeMethods.ResetDC(new HandleRef(this.dc, dc.Hdc), new HandleRef(null, modePointer));
                    Debug.Assert(result == dc.Hdc, "ResetDC didn't return the same handle I gave it");
                }
                finally {
                    SafeNativeMethods.GlobalUnlock(new HandleRef(this, modeHandle));
                }
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }

            // int horizontalResolution = Windows.GetDeviceCaps(dc.Hdc, SafeNativeMethods.HORZRES);
            // int verticalResolution = Windows.GetDeviceCaps(dc.Hdc, SafeNativeMethods.VERTRES);

            graphics = Graphics.FromHdcInternal( dc.Hdc );

            if (graphics != null && document.OriginAtMargins) {

                // Adjust the origin of the graphics object to be at the
                // user-specified margin location
                //
                int dpiX = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(this.dc, dc.Hdc), SafeNativeMethods.LOGPIXELSX);
                int dpiY = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(this.dc, dc.Hdc), SafeNativeMethods.LOGPIXELSY);
                int hardMarginX_DU = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(this.dc, dc.Hdc), SafeNativeMethods.PHYSICALOFFSETX);
                int hardMarginY_DU = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(this.dc, dc.Hdc), SafeNativeMethods.PHYSICALOFFSETY);                
                float hardMarginX = hardMarginX_DU * 100 / dpiX;
                float hardMarginY = hardMarginY_DU * 100 / dpiY;
                
                graphics.TranslateTransform(-hardMarginX, -hardMarginY);
                graphics.TranslateTransform(document.DefaultPageSettings.Margins.Left, document.DefaultPageSettings.Margins.Top);
            }
            

            int result2 = SafeNativeMethods.StartPage(new HandleRef(this.dc, dc.Hdc));
            if (result2 <= 0)
                throw new Win32Exception();
            return graphics;
        }

        /// <include file='doc\DefaultPrintController.uex' path='docs/doc[@for="StandardPrintController.OnEndPage"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements EndPage for printing to a physical printer.
        ///    </para>
        /// </devdoc>
        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Process, ResourceScope.Process)]
        public override void OnEndPage(PrintDocument document, PrintPageEventArgs e) {
            Debug.Assert(dc != null && graphics != null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity(document);
            IntSecurity.UnmanagedCode.Assert();

            try 
            {
                int result = SafeNativeMethods.EndPage(new HandleRef(this.dc, dc.Hdc));
                if (result <= 0)
                    throw new Win32Exception();
            }
            finally {
                CodeAccessPermission.RevertAssert();
                graphics.Dispose(); // Dispose of GDI+ Graphics; keep the DC
                graphics = null;
            }
            base.OnEndPage(document, e);
        }

        /// <include file='doc\DefaultPrintController.uex' path='docs/doc[@for="StandardPrintController.OnEndPrint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements EndPrint for printing to a physical printer.
        ///    </para>
        /// </devdoc>
        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Process, ResourceScope.Process)]
        public override void OnEndPrint(PrintDocument document, PrintEventArgs e) {
            Debug.Assert(dc != null && graphics == null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity(document);
            IntSecurity.UnmanagedCode.Assert();

            try 
            {
                if (dc != null) {
                    try {
                        int result = (e.Cancel) ? SafeNativeMethods.AbortDoc(new HandleRef(dc, dc.Hdc)) : SafeNativeMethods.EndDoc(new HandleRef(dc, dc.Hdc));
                        if (result <= 0)
                            throw new Win32Exception();
                    }
                    finally {
                        dc.Dispose();
                        dc = null;
                    }
                }
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }
            base.OnEndPrint(document, e);
        }
    }
}

