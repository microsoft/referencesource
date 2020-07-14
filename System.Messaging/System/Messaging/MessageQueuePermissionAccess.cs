//----------------------------------------------------
// <copyright file="MessageQueuePermissionAccess.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{

    /// <include file='doc\MessageQueuePermissionAccess.uex' path='docs/doc[@for="MessageQueuePermissionAccess"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Flags]
    [Serializable]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2217:DoNotMarkEnumsWithFlags")]
    public enum MessageQueuePermissionAccess
    {
        /// <include file='doc\MessageQueuePermissionAccess.uex' path='docs/doc[@for="MessageQueuePermissionAccess.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        None = 0,
        /// <include file='doc\MessageQueuePermissionAccess.uex' path='docs/doc[@for="MessageQueuePermissionAccess.Browse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Browse = 1 << 1,
        /// <include file='doc\MessageQueuePermissionAccess.uex' path='docs/doc[@for="MessageQueuePermissionAccess.Send"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Send = 1 << 2 | Browse,
        /// <include file='doc\MessageQueuePermissionAccess.uex' path='docs/doc[@for="MessageQueuePermissionAccess.Peek"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Peek = 1 << 3 | Browse,
        /// <include file='doc\MessageQueuePermissionAccess.uex' path='docs/doc[@for="MessageQueuePermissionAccess.Receive"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Receive = 1 << 4 | Peek,
        /// <include file='doc\MessageQueuePermissionAccess.uex' path='docs/doc[@for="MessageQueuePermissionAccess.Administer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Administer = 1 << 5 | Send | Receive | Peek,
    }
}

