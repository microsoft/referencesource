//------------------------------------------------------------------------------
// <copyright file="MessageType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{

    using System.Diagnostics;
    using System;

    /// <include file='doc\MessageType.uex' path='docs/doc[@for="MessageType"]/*' />
    /// <devdoc>
    ///    A message can be a normal MSMQ message, a positive or negative 
    ///    (arrival and read) acknowledgment message, or a report message.
    /// </devdoc>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1008:EnumsShouldHaveZeroValue")]
    public enum MessageType
    {
        /// <include file='doc\MessageType.uex' path='docs/doc[@for="MessageType.Acknowledgment"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Acknowledgment = 1,
        /// <include file='doc\MessageType.uex' path='docs/doc[@for="MessageType.Normal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Normal = 2,
        /// <include file='doc\MessageType.uex' path='docs/doc[@for="MessageType.Report"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Report = 3,
    }
}
