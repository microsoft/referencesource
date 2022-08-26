//------------------------------------------------------------------------------
// <copyright file="Duplex.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;     
    using System;
    using System.Runtime.InteropServices;
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\Duplex.uex' path='docs/doc[@for="Duplex"]/*' />
    /// <devdoc>
    ///    <para>Specifies the printer's duplex setting.</para>
    /// </devdoc>
    [SuppressMessage("Microsoft.Design", "CA1008:EnumsShouldHaveZeroValue")]
    [Serializable]
    public enum Duplex {
        /// <include file='doc\Duplex.uex' path='docs/doc[@for="Duplex.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The printer's default duplex setting.
        ///    </para>
        /// </devdoc>
        Default = -1,

        /// <include file='doc\Duplex.uex' path='docs/doc[@for="Duplex.Simplex"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Single-sided printing.
        ///    </para>
        /// </devdoc>
        Simplex = SafeNativeMethods.DMDUP_SIMPLEX,

        /// <include file='doc\Duplex.uex' path='docs/doc[@for="Duplex.Horizontal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Double-sided, horizontal printing.
        ///       
        ///    </para>
        /// </devdoc>
        Horizontal = SafeNativeMethods.DMDUP_HORIZONTAL,

        /// <include file='doc\Duplex.uex' path='docs/doc[@for="Duplex.Vertical"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Double-sided, vertical printing.
        ///       
        ///    </para>
        /// </devdoc>
        Vertical = SafeNativeMethods.DMDUP_VERTICAL,
    }
}
