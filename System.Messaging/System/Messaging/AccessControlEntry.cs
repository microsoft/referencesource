using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.ComponentModel;
using System.Text;
using System.Messaging.Interop;

namespace System.Messaging
{
    /// <include file='doc\AccessControlEntry.uex' path='docs/doc[@for="AccessControlEntry"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class AccessControlEntry
    {
        //const int customRightsMask   = 0x0000ffff;
        const StandardAccessRights standardRightsMask = (StandardAccessRights)0x001f0000;
        const GenericAccessRights genericRightsMask = unchecked((GenericAccessRights)0xf0000000);

        internal int accessFlags = 0;
        Trustee trustee = null;
        AccessControlEntryType entryType = AccessControlEntryType.Allow;

        /// <include file='doc\AccessControlEntry.uex' path='docs/doc[@for="AccessControlEntry.AccessControlEntry"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public AccessControlEntry()
        {
        }

        /// <include file='doc\AccessControlEntry.uex' path='docs/doc[@for="AccessControlEntry.AccessControlEntry1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public AccessControlEntry(Trustee trustee)
        {
            this.Trustee = trustee;
        }

        /// <include file='doc\AccessControlEntry.uex' path='docs/doc[@for="AccessControlEntry.AccessControlEntry2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public AccessControlEntry(Trustee trustee, GenericAccessRights genericAccessRights, StandardAccessRights standardAccessRights, AccessControlEntryType entryType)
        {
            this.GenericAccessRights = genericAccessRights;
            this.StandardAccessRights = standardAccessRights;
            this.Trustee = trustee;
            this.EntryType = entryType;
        }

        /// <include file='doc\AccessControlEntry.uex' path='docs/doc[@for="AccessControlEntry.EntryType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public AccessControlEntryType EntryType
        {
            get { return entryType; }
            set
            {
                if (!ValidationUtility.ValidateAccessControlEntryType(value))
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(AccessControlEntryType));

                entryType = value;
            }
        }

        /// <include file='doc\AccessControlEntry.uex' path='docs/doc[@for="AccessControlEntry.CustomAccessRights"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected int CustomAccessRights
        {
            get
            {
                return accessFlags;
            }
            set
            {
                accessFlags = value;
            }
        }

        /// <include file='doc\AccessControlEntry.uex' path='docs/doc[@for="AccessControlEntry.GenericAccessRights"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public GenericAccessRights GenericAccessRights
        {
            get
            {
                return (GenericAccessRights)accessFlags & genericRightsMask;
            }
            set
            {
                // make sure these flags really are genericAccessRights
                if ((value & genericRightsMask) != value)
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(GenericAccessRights));

                accessFlags = (accessFlags & (int)(~genericRightsMask)) | (int)value;
            }
        }

        /// <include file='doc\AccessControlEntry.uex' path='docs/doc[@for="AccessControlEntry.StandardAccessRights"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public StandardAccessRights StandardAccessRights
        {
            get
            {
                return (StandardAccessRights)accessFlags & standardRightsMask;
            }
            set
            {
                // make sure these flags really are standardAccessRights
                if ((value & standardRightsMask) != value)
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(StandardAccessRights));

                accessFlags = (accessFlags & (int)(~standardRightsMask)) | (int)value;
            }
        }

        /// <include file='doc\AccessControlEntry.uex' path='docs/doc[@for="AccessControlEntry.Trustee"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Trustee Trustee
        {
            get
            {
                return trustee;
            }
            set
            {
                if (value == null)
                    throw new ArgumentNullException("value");

                trustee = value;
            }
        }
    }
}
