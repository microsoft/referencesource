using System;
using System.Messaging.Interop;

namespace System.Messaging {
    /// <include file='doc\AccessControlEntryType.uex' path='docs/doc[@for="AccessControlEntryType"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>    
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1008:EnumsShouldHaveZeroValue")]
    public enum AccessControlEntryType {
        /// <include file='doc\AccessControlEntryType.uex' path='docs/doc[@for="AccessControlEntryType.Allow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Allow  = NativeMethods.GRANT_ACCESS,
        /// <include file='doc\AccessControlEntryType.uex' path='docs/doc[@for="AccessControlEntryType.Set"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Set    = NativeMethods.SET_ACCESS,
        /// <include file='doc\AccessControlEntryType.uex' path='docs/doc[@for="AccessControlEntryType.Deny"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Deny   = NativeMethods.DENY_ACCESS,
        /// <include file='doc\AccessControlEntryType.uex' path='docs/doc[@for="AccessControlEntryType.Revoke"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Revoke = NativeMethods.REVOKE_ACCESS
    }
}
