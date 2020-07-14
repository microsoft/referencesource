//------------------------------------------------------------------------------
// <copyright file="Acknowledgement.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{
    using System.Threading;

    using System.Diagnostics;

    using System;
    using System.Messaging.Interop;

    /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies what went wrong (or right) during a Message
    ///       Queuing operation. This is the type of a property of an acknowledgement
    ///       message.
    ///    </para>
    /// </devdoc>
    public enum Acknowledgment
    {
        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The default value of the <see cref='System.Messaging.Acknowledgment'/>
        ///       property. This means the message is
        ///       not an acknowledgment message.
        ///    </para>
        /// </devdoc>
        None = 0,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.AccessDenied"]/*' />
        /// <devdoc>
        ///     The sending application does not have access rights
        ///     to the destination queue.
        /// </devdoc>
        AccessDenied = NativeMethods.MESSAGE_CLASS_ACCESS_DENIED,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.BadDestinationQueue"]/*' />
        /// <devdoc>
        ///     The destination queue is not available to the sending
        ///     application.
        /// </devdoc>
        BadDestinationQueue = NativeMethods.MESSAGE_CLASS_BAD_DESTINATION_QUEUE,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.BadEncryption"]/*' />
        /// <devdoc>
        ///     The destination Queue Manager could not decrypt a private
        ///     (encrypted) message.
        /// </devdoc>
        BadEncryption = NativeMethods.MESSAGE_CLASS_BAD_ENCRYPTION,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.BadSignature"]/*' />
        /// <devdoc>
        ///     MSMQ could not authenticate the original message. The original
        ///     message's digital signature is not valid.
        /// </devdoc>
        BadSignature = NativeMethods.MESSAGE_CLASS_BAD_SIGNATURE,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.CouldNotEncrypt"]/*' />
        /// <devdoc>
        ///     The source Queue Manager could not encrypt a private message.
        /// </devdoc>
        CouldNotEncrypt = NativeMethods.MESSAGE_CLASS_COULD_NOT_ENCRYPT,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.HopCountExceeded"]/*' />
        /// <devdoc>
        ///     The original message's hop count is exceeded.
        /// </devdoc>
        HopCountExceeded = NativeMethods.MESSAGE_CLASS_HOP_COUNT_EXCEEDED,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.NotTransactionalQueue"]/*' />
        /// <devdoc>
        ///     A transaction message was sent to a non-transaction
        ///     queue.
        /// </devdoc>
        NotTransactionalQueue = NativeMethods.MESSAGE_CLASS_NOT_TRANSACTIONAL_QUEUE,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.NotTransactionalMessage"]/*' />
        /// <devdoc>
        ///     A non-transaction message was sent to a transaction
        ///     queue.
        /// </devdoc>
        NotTransactionalMessage = NativeMethods.MESSAGE_CLASS_NOT_TRANSACTIONAL_MESSAGE,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.Purged"]/*' />
        /// <devdoc>
        ///     The message was purged before reaching the destination
        ///     queue.
        /// </devdoc>
        Purged = NativeMethods.MESSAGE_CLASS_PURGED,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.QueueDeleted"]/*' />
        /// <devdoc>
        ///     The queue was deleted before the message could be read
        ///     from the queue.
        /// </devdoc>
        QueueDeleted = NativeMethods.MESSAGE_CLASS_QUEUE_DELETED,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.QueueExceedMaximumSize"]/*' />
        /// <devdoc>
        ///     The original message's destination queue is full.
        /// </devdoc>
        QueueExceedMaximumSize = NativeMethods.MESSAGE_CLASS_QUEUE_EXCEED_QUOTA,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.QueuePurged"]/*' />
        /// <devdoc>
        ///     The queue was purged and the message no longer exists.
        /// </devdoc>
        QueuePurged = NativeMethods.MESSAGE_CLASS_QUEUE_PURGED,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.ReachQueue"]/*' />
        /// <devdoc>
        ///     The original message reached its destination queue.
        /// </devdoc>
        ReachQueue = NativeMethods.MESSAGE_CLASS_REACH_QUEUE,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.ReachQueueTimeout"]/*' />
        /// <devdoc>
        ///     Either the time-to-reach-queue or time-to-be-received timer
        ///     expired before the original message could reach the destination queue.
        /// </devdoc>
        ReachQueueTimeout = NativeMethods.MESSAGE_CLASS_REACH_QUEUE_TIMEOUT,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.ReceiveTimeout"]/*' />
        /// <devdoc>
        ///     The original message was not removed from the queue before
        ///     its time-to-be-received timer expired.
        /// </devdoc>
        ReceiveTimeout = NativeMethods.MESSAGE_CLASS_RECEIVE_TIMEOUT,

        /// <include file='doc\Acknowledgement.uex' path='docs/doc[@for="Acknowledgment.Receive"]/*' />
        /// <devdoc>
        ///     The original message was retrieved by the receiving
        ///     application.
        /// </devdoc>
        Receive = NativeMethods.MESSAGE_CLASS_RECEIVE,
    }
}
