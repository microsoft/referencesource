//------------------------------------------------------------------------------
// <copyright file="BootMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using System.Diagnostics;

    /// <include file='doc\BootMode.uex' path='docs/doc[@for="BootMode"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the mode to start the computer
    ///       in.
    ///    </para>
    /// </devdoc>
    public enum BootMode {
        /// <include file='doc\BootMode.uex' path='docs/doc[@for="BootMode.Normal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts the computer in standard mode.
        ///    </para>
        /// </devdoc>
        Normal = 0,
        /// <include file='doc\BootMode.uex' path='docs/doc[@for="BootMode.FailSafe"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts the computer by using only the basic
        ///       files and
        ///       drivers.
        ///    </para>
        /// </devdoc>
        FailSafe = 1,
        /// <include file='doc\BootMode.uex' path='docs/doc[@for="BootMode.FailSafeWithNetwork"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts the computer by using the basic files, drivers and
        ///       the services and drivers
        ///       necessary to start networking.
        ///    </para>
        /// </devdoc>
        FailSafeWithNetwork = 2,
    }
}

