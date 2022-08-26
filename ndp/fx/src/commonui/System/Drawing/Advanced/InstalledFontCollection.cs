//------------------------------------------------------------------------------
// <copyright file="InstalledFontCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Text {

    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.Drawing.Internal;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Runtime.Versioning;

    /// <include file='doc\InstalledFontCollection.uex' path='docs/doc[@for="InstalledFontCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the fonts installed on the
    ///       system.
    ///    </para>
    /// </devdoc>
    public sealed class InstalledFontCollection : FontCollection {

        /// <include file='doc\InstalledFontCollection.uex' path='docs/doc[@for="InstalledFontCollection.InstalledFontCollection"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Text.InstalledFontCollection'/> class.
        /// </devdoc>
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public InstalledFontCollection() {

            nativeFontCollection = IntPtr.Zero;

            int status = SafeNativeMethods.Gdip.GdipNewInstalledFontCollection(out nativeFontCollection);

            if (status != SafeNativeMethods.Gdip.Ok)
                throw SafeNativeMethods.Gdip.StatusException(status);
        }
    }
}

