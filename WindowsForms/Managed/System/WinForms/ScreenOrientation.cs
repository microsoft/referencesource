//------------------------------------------------------------------------------
// <copyright file="ScreenOrientation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;


    /// <include file='doc\ScreenOrientation.uex' path='docs/doc[@for="ScreenOrientation"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the angle of screen orientation
    ///    </para>
    /// </devdoc>
    public enum ScreenOrientation {
        /// <include file='doc\ScreenOrientation.uex' path='docs/doc[@for="Day.Angle0"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The screen is oriented at 0 degrees
        ///    </para>
        /// </devdoc>
        Angle0 = 0,

        /// <include file='doc\ScreenOrientation.uex' path='docs/doc[@for="Day.Angle90"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The screen is oriented at 90 degrees
        ///    </para>
        /// </devdoc>
        Angle90 = 1,

        /// <include file='doc\ScreenOrientation.uex' path='docs/doc[@for="Day.Angle180"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The screen is oriented at 180 degrees.
        ///    </para>
        /// </devdoc>
        Angle180 = 2,

        /// <include file='doc\ScreenOrientation.uex' path='docs/doc[@for="Day.Angle270"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The screen is oriented at 270 degrees.
        ///    </para>
        /// </devdoc>
        Angle270 = 3,
    }
}

