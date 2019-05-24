//------------------------------------------------------------------------------
// <copyright file="ServiceStartMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ServiceProcess {

    using System.Diagnostics;

    using System;

    /// <include file='doc\ServiceStartMode.uex' path='docs/doc[@for="ServiceStartMode"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum ServiceStartMode {

        /// <include file='doc\ServiceStartMode.uex' path='docs/doc[@for="ServiceStartMode.Manual"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Manual = NativeMethods.START_TYPE_DEMAND,
        /// <include file='doc\ServiceStartMode.uex' path='docs/doc[@for="ServiceStartMode.Automatic"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Automatic = NativeMethods.START_TYPE_AUTO,
        /// <include file='doc\ServiceStartMode.uex' path='docs/doc[@for="ServiceStartMode.Disabled"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Disabled = NativeMethods.START_TYPE_DISABLED,
        /// <include file='doc\ServiceStartMode.uex' path='docs/doc[@for="ServiceStartMode.Boot"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Boot = NativeMethods.START_TYPE_BOOT,
        /// <include file='doc\ServiceStartMode.uex' path='docs/doc[@for="ServiceStartMode.System"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        System = NativeMethods.START_TYPE_SYSTEM,

    }

}
