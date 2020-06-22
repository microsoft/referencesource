using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.ComponentModel;
using System.Text;
using System.Messaging.Interop;

namespace System.Messaging
{

    /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Flags]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1008:EnumsShouldHaveZeroValue")]
    public enum MessageQueueAccessRights
    {
        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.DeleteMessage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        DeleteMessage = 0x00000001,
        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.PeekMessage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        PeekMessage = 0x00000002,
        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.WriteMessage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        WriteMessage = 0x00000004,
        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.DeleteJournalMessage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        DeleteJournalMessage = 0x00000008,
        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.SetQueueProperties"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SetQueueProperties = 0x00000010,
        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.GetQueueProperties"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        GetQueueProperties = 0x00000020,
        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.DeleteQueue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        DeleteQueue = 0x00010000,
        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.GetQueuePermissions"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        GetQueuePermissions = 0x00020000,
        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.ChangeQueuePermissions"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ChangeQueuePermissions = 0x00040000,
        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.TakeQueueOwnership"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        TakeQueueOwnership = 0x00080000,

        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.ReceiveMessage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ReceiveMessage = DeleteMessage | PeekMessage,

        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.ReceiveJournalMessage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ReceiveJournalMessage = DeleteJournalMessage | PeekMessage,

        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.GenericRead"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        GenericRead = GetQueueProperties | GetQueuePermissions | ReceiveMessage | ReceiveJournalMessage,

        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.GenericWrite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        GenericWrite = GetQueueProperties | GetQueuePermissions | WriteMessage,

        /// <include file='doc\MessageQueueAccessRights.uex' path='docs/doc[@for="MessageQueueAccessRights.FullControl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        FullControl = DeleteMessage | PeekMessage | WriteMessage | DeleteJournalMessage |
                                 SetQueueProperties | GetQueueProperties | DeleteQueue | GetQueuePermissions |
                                 ChangeQueuePermissions | TakeQueueOwnership,
    }
}
