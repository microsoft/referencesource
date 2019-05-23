//------------------------------------------------------------------------------
// <copyright file="PowerBroadcastStatus.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess {
   using System;

    /// <include file='doc\PowerBroadcastStatus.uex' path='docs/doc[@for="PowerBroadcastStatus"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum PowerBroadcastStatus {
        /// <include file='doc\PowerBroadcastStatus.uex' path='docs/doc[@for="PowerBroadcastStatus.BatteryLow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        BatteryLow = NativeMethods.PBT_APMBATTERYLOW,
        /// <include file='doc\PowerBroadcastStatus.uex' path='docs/doc[@for="PowerBroadcastStatus.OemEvent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        OemEvent = NativeMethods.PBT_APMOEMEVENT,
        /// <include file='doc\PowerBroadcastStatus.uex' path='docs/doc[@for="PowerBroadcastStatus.PowerStatusChange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        PowerStatusChange = NativeMethods.PBT_APMPOWERSTATUSCHANGE,
        /// <include file='doc\PowerBroadcastStatus.uex' path='docs/doc[@for="PowerBroadcastStatus.QuerySuspend"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        QuerySuspend = NativeMethods.PBT_APMQUERYSUSPEND,
        /// <include file='doc\PowerBroadcastStatus.uex' path='docs/doc[@for="PowerBroadcastStatus.QuerySuspendFailed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        QuerySuspendFailed = NativeMethods.PBT_APMQUERYSUSPENDFAILED,
        /// <include file='doc\PowerBroadcastStatus.uex' path='docs/doc[@for="PowerBroadcastStatus.ResumeAutomatic"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ResumeAutomatic = NativeMethods.PBT_APMRESUMEAUTOMATIC,
        /// <include file='doc\PowerBroadcastStatus.uex' path='docs/doc[@for="PowerBroadcastStatus.ResumeCritical"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ResumeCritical = NativeMethods.PBT_APMRESUMECRITICAL,
        /// <include file='doc\PowerBroadcastStatus.uex' path='docs/doc[@for="PowerBroadcastStatus.ResumeSuspend"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ResumeSuspend = NativeMethods.PBT_APMRESUMESUSPEND,
        /// <include file='doc\PowerBroadcastStatus.uex' path='docs/doc[@for="PowerBroadcastStatus.Suspend"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Suspend = NativeMethods.PBT_APMSUSPEND,
    }
}
  
