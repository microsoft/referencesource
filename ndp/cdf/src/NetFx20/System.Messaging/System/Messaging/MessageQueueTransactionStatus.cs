//------------------------------------------------------------------------------
// <copyright file="MessageQueueTransactionStatus.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{
    using System.Diagnostics;
    using System.Messaging.Interop;

    /// <include file='doc\MessageQueueTransactionStatus.uex' path='docs/doc[@for="MessageQueueTransactionStatus"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum MessageQueueTransactionStatus
    {
        /// <include file='doc\MessageQueueTransactionStatus.uex' path='docs/doc[@for="MessageQueueTransactionStatus.Aborted"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Aborted = 0,
        /// <include file='doc\MessageQueueTransactionStatus.uex' path='docs/doc[@for="MessageQueueTransactionStatus.Commited"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Committed = 1,
        /// <include file='doc\MessageQueueTransactionStatus.uex' path='docs/doc[@for="MessageQueueTransactionStatus.Initialized"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Initialized = 2,
        /// <include file='doc\MessageQueueTransactionStatus.uex' path='docs/doc[@for="MessageQueueTransactionStatus.Pending"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Pending = 3,
    }

}
