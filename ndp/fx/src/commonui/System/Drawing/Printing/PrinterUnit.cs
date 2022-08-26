//------------------------------------------------------------------------------
// <copyright file="PrinterUnit.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;    
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\PrinterUnit.uex' path='docs/doc[@for="PrinterUnit"]/*' />
    /// <devdoc>
    ///    <para>Specifies several of
    ///       the units of measure Microsoft Win32 uses for printing.</para>
    /// </devdoc>
    public enum PrinterUnit {
        /// <include file='doc\PrinterUnit.uex' path='docs/doc[@for="PrinterUnit.Display"]/*' />
        /// <devdoc>
        ///    <para>The default unit (0.01 in.).</para>
        /// </devdoc>
        // Our default units, as well as GDI+'s
        Display = 0,

        /// <include file='doc\PrinterUnit.uex' path='docs/doc[@for="PrinterUnit.ThousandthsOfAnInch"]/*' />
        /// <devdoc>
        ///    <para>One
        ///       thousandth of an inch
        ///       (0.001 in.).</para>
        /// </devdoc>
        // Used by PAGESETUPDLG.rtMargin and rtMinMargin
        ThousandthsOfAnInch = 1,

        /// <include file='doc\PrinterUnit.uex' path='docs/doc[@for="PrinterUnit.HundredthsOfAMillimeter"]/*' />
        /// <devdoc>
        ///    <para>One hundredth of a millimeter
        ///       (0.01 mm).</para>
        /// </devdoc>
        // Used by PAGESETUPDLG.rtMargin and rtMinMargin
        HundredthsOfAMillimeter = 2,

        /// <include file='doc\PrinterUnit.uex' path='docs/doc[@for="PrinterUnit.TenthsOfAMillimeter"]/*' />
        /// <devdoc>
        ///    <para>One tenth of a millimeter
        ///       (0.1 mm).</para>
        /// </devdoc>
        // DeviceCapabilities(DC_PAPERSIZE)
        TenthsOfAMillimeter = 3,
    }
}

