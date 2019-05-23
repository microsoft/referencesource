//------------------------------------------------------------------------------
// <copyright file="ServiceControllerStatus.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;

    /// <include file='doc\ServiceControllerStatus.uex' path='docs/doc[@for="ServiceControllerStatus"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum ServiceControllerStatus {
        /// <include file='doc\ServiceControllerStatus.uex' path='docs/doc[@for="ServiceControllerStatus.ContinuePending"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ContinuePending = NativeMethods.STATE_CONTINUE_PENDING,
        /// <include file='doc\ServiceControllerStatus.uex' path='docs/doc[@for="ServiceControllerStatus.Paused"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Paused = NativeMethods.STATE_PAUSED,
        /// <include file='doc\ServiceControllerStatus.uex' path='docs/doc[@for="ServiceControllerStatus.PausePending"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        PausePending = NativeMethods.STATE_PAUSE_PENDING,
        /// <include file='doc\ServiceControllerStatus.uex' path='docs/doc[@for="ServiceControllerStatus.Running"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Running = NativeMethods.STATE_RUNNING,
        /// <include file='doc\ServiceControllerStatus.uex' path='docs/doc[@for="ServiceControllerStatus.StartPending"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        StartPending = NativeMethods.STATE_START_PENDING,
        /// <include file='doc\ServiceControllerStatus.uex' path='docs/doc[@for="ServiceControllerStatus.Stopped"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Stopped = NativeMethods.STATE_STOPPED,
        /// <include file='doc\ServiceControllerStatus.uex' path='docs/doc[@for="ServiceControllerStatus.StopPending"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        StopPending = NativeMethods.STATE_STOP_PENDING,
    }
}
