//------------------------------------------------------------------------------
// <copyright file="ServiceType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;

    /// <include file='doc\ServiceType.uex' path='docs/doc[@for="ServiceType"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Flags]
    public enum ServiceType {
        /// <include file='doc\ServiceType.uex' path='docs/doc[@for="ServiceType.Adapter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Adapter = NativeMethods.SERVICE_TYPE_ADAPTER,
        /// <include file='doc\ServiceType.uex' path='docs/doc[@for="ServiceType.FileSystemDriver"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        FileSystemDriver = NativeMethods.SERVICE_TYPE_FILE_SYSTEM_DRIVER,
        /// <include file='doc\ServiceType.uex' path='docs/doc[@for="ServiceType.InteractiveProcess"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        InteractiveProcess = NativeMethods.SERVICE_TYPE_INTERACTIVE_PROCESS,
        /// <include file='doc\ServiceType.uex' path='docs/doc[@for="ServiceType.KernelDriver"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        KernelDriver = NativeMethods.SERVICE_TYPE_KERNEL_DRIVER,
        /// <include file='doc\ServiceType.uex' path='docs/doc[@for="ServiceType.RecognizerDriver"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RecognizerDriver = NativeMethods.SERVICE_TYPE_RECOGNIZER_DRIVER,
        /// <include file='doc\ServiceType.uex' path='docs/doc[@for="ServiceType.Win32OwnProcess"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Win32OwnProcess = NativeMethods.SERVICE_TYPE_WIN32_OWN_PROCESS,
        /// <include file='doc\ServiceType.uex' path='docs/doc[@for="ServiceType.Win32ShareProcess"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Win32ShareProcess = NativeMethods.SERVICE_TYPE_WIN32_SHARE_PROCESS,
    }
}
