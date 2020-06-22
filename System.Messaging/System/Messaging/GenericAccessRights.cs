namespace System.Messaging
{
    /// <include file='doc\GenericAccessRights.uex' path='docs/doc[@for="GenericAccessRights"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Flags]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2217:DoNotMarkEnumsWithFlags")]
    public enum GenericAccessRights
    {
        /// <include file='doc\GenericAccessRights.uex' path='docs/doc[@for="GenericAccessRights.All"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        All = 1 << 28,
        /// <include file='doc\GenericAccessRights.uex' path='docs/doc[@for="GenericAccessRights.Execute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Execute = 1 << 29,
        /// <include file='doc\GenericAccessRights.uex' path='docs/doc[@for="GenericAccessRights.Write"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Write = 1 << 30,
        /// <include file='doc\GenericAccessRights.uex' path='docs/doc[@for="GenericAccessRights.Read"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Read = 1 << 31,
        /// <include file='doc\GenericAccessRights.uex' path='docs/doc[@for="GenericAccessRights.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        None = 0
    }
}
