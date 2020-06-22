//------------------------------------------------------------------------------
// <copyright file="AcknowledgeTypes.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{

    using System.Diagnostics;

    using System;
    using System.Messaging.Interop;

    /// <include file='doc\AcknowledgeTypes.uex' path='docs/doc[@for="AcknowledgeTypes"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies what kind of acknowledgment to get after sending a message.
    ///    </para>
    /// </devdoc>
    [Flags]
    public enum AcknowledgeTypes
    {

        /// <include file='doc\AcknowledgeTypes.uex' path='docs/doc[@for="AcknowledgeTypes.PositiveArrival"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Use this value to request a positive acknowledgment when the message
        ///       reaches the queue. 
        ///    </para>
        /// </devdoc>
        PositiveArrival = NativeMethods.ACKNOWLEDGE_POSITIVE_ARRIVAL,

        /// <include file='doc\AcknowledgeTypes.uex' path='docs/doc[@for="AcknowledgeTypes.PositiveReceive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Use this value to request a positive acknowledgment when the message 
        ///       is successfully retrieved from the queue.
        ///    </para>
        /// </devdoc>
        PositiveReceive = NativeMethods.ACKNOWLEDGE_POSITIVE_RECEIVE,

        /// <include file='doc\AcknowledgeTypes.uex' path='docs/doc[@for="AcknowledgeTypes.NegativeReceive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Use this value to request a negative acknowledgment when the message fails
        ///       to be retrieved from the queue. 
        ///    </para>
        /// </devdoc>                    
        NegativeReceive = NativeMethods.ACKNOWLEDGE_NEGATIVE_RECEIVE,

        /// <include file='doc\AcknowledgeTypes.uex' path='docs/doc[@for="AcknowledgeTypes.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Use this value to request no acknowledgment messages (positive or negative) to be posted.
        ///    </para>
        /// </devdoc>
        None = NativeMethods.ACKNOWLEDGE_NONE,

        /// <include file='doc\AcknowledgeTypes.uex' path='docs/doc[@for="AcknowledgeTypes.NotAcknowledgeReachQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Use this value to request a negative acknowledgment when the message cannot
        ///       reach the queue. This can happen when the time-to-reach-queue
        ///       timer expires, or a message cannot be authenticated.
        ///    </para>
        /// </devdoc>
        NotAcknowledgeReachQueue = NativeMethods.ACKNOWLEDGE_NEGATIVE_ARRIVAL,

        /// <include file='doc\AcknowledgeTypes.uex' path='docs/doc[@for="AcknowledgeTypes.NotAcknowledgeReceive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Use this value to request a negative acknowledgment when an error occurs and
        ///       the message cannot be retrieved from the queue before its
        ///       time-to-be-received timer expires.
        ///    </para>
        /// </devdoc>
        NotAcknowledgeReceive = NegativeReceive |
                                                       NativeMethods.ACKNOWLEDGE_NEGATIVE_ARRIVAL,

        /// <include file='doc\AcknowledgeTypes.uex' path='docs/doc[@for="AcknowledgeTypes.FullReachQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Use this value
        ///       to request full acknowledgment (positive or negative) depending
        ///       on whether or not the message reaches the queue.
        ///       This can happen when the time-to-reach-queue timer expires,
        ///       or a message cannot be authenticated.
        ///    </para>
        /// </devdoc>
        FullReachQueue = NotAcknowledgeReachQueue |
                                        PositiveArrival,

        /// <include file='doc\AcknowledgeTypes.uex' path='docs/doc[@for="AcknowledgeTypes.FullReceive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Use this value to request full acknowledgment (positive or negative) depending
        ///       on whether or not the message is retrieved from the queue
        ///       before its time-to-be-received timer expires.
        ///    </para>
        /// </devdoc>
        FullReceive = NotAcknowledgeReceive |
                                PositiveReceive,

    }
}
