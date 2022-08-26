//------------------------------------------------------------------------------
// <copyright file="PrinterResolutionKind.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using System.Diagnostics;
    using System;
    using System.Runtime.InteropServices;
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\PrinterResolutionKind.uex' path='docs/doc[@for="PrinterResolutionKind"]/*' />
    /// <devdoc>
    ///    <para>Specifies a printer resolution.</para>
    /// </devdoc>
    [Serializable] 
    public enum PrinterResolutionKind {
        /// <include file='doc\PrinterResolutionKind.uex' path='docs/doc[@for="PrinterResolutionKind.High"]/*' />
        /// <devdoc>
        ///    <para>
        ///       High resolution.
        ///       
        ///    </para>
        /// </devdoc>
        High = SafeNativeMethods.DMRES_HIGH,
        /// <include file='doc\PrinterResolutionKind.uex' path='docs/doc[@for="PrinterResolutionKind.Medium"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Medium resolution.
        ///       
        ///    </para>
        /// </devdoc>
        Medium = SafeNativeMethods.DMRES_MEDIUM,
        /// <include file='doc\PrinterResolutionKind.uex' path='docs/doc[@for="PrinterResolutionKind.Low"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Low resolution.
        ///       
        ///    </para>
        /// </devdoc>
        Low = SafeNativeMethods.DMRES_LOW,
        /// <include file='doc\PrinterResolutionKind.uex' path='docs/doc[@for="PrinterResolutionKind.Draft"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draft-quality resolution.
        ///       
        ///    </para>
        /// </devdoc>
        Draft = SafeNativeMethods.DMRES_DRAFT,
        /// <include file='doc\PrinterResolutionKind.uex' path='docs/doc[@for="PrinterResolutionKind.Custom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Custom resolution.
        ///       
        ///    </para>
        /// </devdoc>
        Custom = 0,
    }
}
