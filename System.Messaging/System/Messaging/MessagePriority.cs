//------------------------------------------------------------------------------
// <copyright file="MessagePriority.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{

    using System.Diagnostics;

    using System;

    /// <include file='doc\MessagePriority.uex' path='docs/doc[@for="MessagePriority"]/*' />
    /// <devdoc>
    ///    Message priority effects how MSMQ handles the message while it is in route, 
    ///    as well as where the message is placed in the queue. Higher priority messages 
    ///    are given preference during routing, and inserted toward the front of the queue. 
    ///    Messages with the same priority are placed in the queue according to their arrival 
    ///    time.
    /// </devdoc>
    public enum MessagePriority
    {
        /// <include file='doc\MessagePriority.uex' path='docs/doc[@for="MessagePriority.Lowest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Lowest = 0,
        /// <include file='doc\MessagePriority.uex' path='docs/doc[@for="MessagePriority.VeryLow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        VeryLow = 1,
        /// <include file='doc\MessagePriority.uex' path='docs/doc[@for="MessagePriority.Low"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Low = 2,
        /// <include file='doc\MessagePriority.uex' path='docs/doc[@for="MessagePriority.Normal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Normal = 3,
        /// <include file='doc\MessagePriority.uex' path='docs/doc[@for="MessagePriority.AboveNormal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        AboveNormal = 4,
        /// <include file='doc\MessagePriority.uex' path='docs/doc[@for="MessagePriority.High"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        High = 5,
        /// <include file='doc\MessagePriority.uex' path='docs/doc[@for="MessagePriority.VeryHigh"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        VeryHigh = 6,
        /// <include file='doc\MessagePriority.uex' path='docs/doc[@for="MessagePriority.Highest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Highest = 7,
    }
}
