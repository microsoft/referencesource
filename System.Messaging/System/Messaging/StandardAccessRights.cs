namespace System.Messaging
{
    /// <include file='doc\StandardAccessRights.uex' path='docs/doc[@for="StandardAccessRights"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Flags]
    public enum StandardAccessRights
    {
        /// <include file='doc\StandardAccessRights.uex' path='docs/doc[@for="StandardAccessRights.Delete"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Delete = 1 << 16,
        /// <include file='doc\StandardAccessRights.uex' path='docs/doc[@for="StandardAccessRights.ReadSecurity"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ReadSecurity = 1 << 17,
        /// <include file='doc\StandardAccessRights.uex' path='docs/doc[@for="StandardAccessRights.WriteSecurity"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        WriteSecurity = 1 << 18,
        /// <include file='doc\StandardAccessRights.uex' path='docs/doc[@for="StandardAccessRights.Synchronize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Synchronize = 1 << 20,
        /// <include file='doc\StandardAccessRights.uex' path='docs/doc[@for="StandardAccessRights.ModifyOwner"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ModifyOwner = 1 << 19,
        /// <include file='doc\StandardAccessRights.uex' path='docs/doc[@for="StandardAccessRights.Read"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Read = ReadSecurity,
        /// <include file='doc\StandardAccessRights.uex' path='docs/doc[@for="StandardAccessRights.Write"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Write = ReadSecurity,
        /// <include file='doc\StandardAccessRights.uex' path='docs/doc[@for="StandardAccessRights.Execute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Execute = ReadSecurity,
        /// <include file='doc\StandardAccessRights.uex' path='docs/doc[@for="StandardAccessRights.Required"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Required = Delete | WriteSecurity | ModifyOwner,
        /// <include file='doc\StandardAccessRights.uex' path='docs/doc[@for="StandardAccessRights.All"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        All = Delete | WriteSecurity | ModifyOwner | ReadSecurity | Synchronize,
        /// <include file='doc\StandardAccessRights.uex' path='docs/doc[@for="StandardAccessRights.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        None = 0
    }
}
