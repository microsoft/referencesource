// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

//
// Pkcs7Recipient.cs
// 

namespace System.Security.Cryptography.Pkcs {
    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography.X509Certificates;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class CmsRecipient {
        private SubjectIdentifierType   m_recipientIdentifierType;
        private X509Certificate2       m_certificate;

        //
        // Constructors.
        //

        private CmsRecipient () {}

        public CmsRecipient (X509Certificate2 certificate):this(SubjectIdentifierType.IssuerAndSerialNumber, certificate){}

        public CmsRecipient (SubjectIdentifierType recipientIdentifierType, X509Certificate2 certificate) {
            Reset(recipientIdentifierType, certificate);
        }

        //
        // Public APIs.
        //

        public SubjectIdentifierType RecipientIdentifierType {
            get {
                return m_recipientIdentifierType;
            }
        }

        public X509Certificate2 Certificate {
            get {
                return m_certificate;
            }
        }

        //
        // Private methods.
        //

        private void Reset (SubjectIdentifierType recipientIdentifierType, X509Certificate2 certificate) {
            if (certificate == null)
                throw new ArgumentNullException("certificate");

            switch (recipientIdentifierType) {
            case SubjectIdentifierType.Unknown:
                recipientIdentifierType = SubjectIdentifierType.IssuerAndSerialNumber;
                break;
            case SubjectIdentifierType.IssuerAndSerialNumber:
                break;
            case SubjectIdentifierType.SubjectKeyIdentifier:
                if (!PkcsUtils.CmsSupported())
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Not_Supported"));
                break;
            default:
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Invalid_Subject_Identifier_Type"), recipientIdentifierType.ToString());
            }

            m_recipientIdentifierType = recipientIdentifierType;
            m_certificate = certificate;
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class CmsRecipientCollection : ICollection {
        private ArrayList m_recipients;

        public CmsRecipientCollection () {
            m_recipients = new ArrayList();
        }

        public CmsRecipientCollection (CmsRecipient recipient) {
            m_recipients = new ArrayList(1);
            m_recipients.Add(recipient);
        }

        public CmsRecipientCollection (SubjectIdentifierType recipientIdentifierType, X509Certificate2Collection certificates) {
            m_recipients = new ArrayList(certificates.Count);
            for (int index = 0; index < certificates.Count; index++) {
                m_recipients.Add(new CmsRecipient(recipientIdentifierType, certificates[index]));
            }
        }

        // Perform a deep copy of a certificate collection
        private CmsRecipientCollection(CmsRecipientCollection other) {
            m_recipients = new ArrayList(other.m_recipients.Count);
            foreach (CmsRecipient recipient in other.m_recipients) {
                m_recipients.Add(new CmsRecipient(recipient.RecipientIdentifierType, new X509Certificate2(recipient.Certificate)));
            }
        }

        public CmsRecipient this[int index] {
            get {
                if (index < 0 || index >= m_recipients.Count)
                    throw new ArgumentOutOfRangeException("index", SecurityResources.GetResourceString("ArgumentOutOfRange_Index"));

                return (CmsRecipient) m_recipients[index];
            }
        }

        public int Count {
            get {
                return m_recipients.Count;
            }
        }

        public int Add (CmsRecipient recipient) {
            if (recipient == null)
                throw new ArgumentNullException("recipient");

            return m_recipients.Add(recipient);
        }

        public void Remove(CmsRecipient recipient) {
            if (recipient == null)
                throw new ArgumentNullException("recipient");

            m_recipients.Remove(recipient);
        }

        public CmsRecipientEnumerator GetEnumerator() {
            return new CmsRecipientEnumerator(this);
        }

        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return new CmsRecipientEnumerator(this);
        }

        public void CopyTo(Array array, int index) {
            if (array == null)
                throw new ArgumentNullException("array");
            if (array.Rank != 1)
                throw new ArgumentException(SecurityResources.GetResourceString("Arg_RankMultiDimNotSupported"));
            if (index < 0 || index >= array.Length)
                throw new ArgumentOutOfRangeException("index", SecurityResources.GetResourceString("ArgumentOutOfRange_Index"));
            if (index + this.Count > array.Length)
                throw new ArgumentException(SecurityResources.GetResourceString("Argument_InvalidOffLen"));

            for (int i=0; i < this.Count; i++) {
                array.SetValue(this[i], index);
                index++;
            }
        }

        public void CopyTo(CmsRecipient[] array, int index) {
            ((ICollection)this).CopyTo(array, index);
        }

        public bool IsSynchronized {
            get {
                return false;
            }
        }

        public Object SyncRoot {
            get {
                return this;
            }
        }

        internal CmsRecipientCollection DeepCopy() {
            return new CmsRecipientCollection(this);
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class CmsRecipientEnumerator : IEnumerator {
        private CmsRecipientCollection m_recipients;
        private int m_current;

        private CmsRecipientEnumerator() {}
        internal CmsRecipientEnumerator(CmsRecipientCollection recipients) {
            m_recipients = recipients;
            m_current = -1;
        }

        public CmsRecipient Current {
            get {
                return (CmsRecipient) m_recipients[m_current];
            }
        }

        /// <internalonly/>
        Object IEnumerator.Current {
            get {
                return (Object) m_recipients[m_current];
            }
        }

        public bool MoveNext() {
            if (m_current == ((int) m_recipients.Count - 1))
                return false;
            m_current++;
            return true;
        }

        public void Reset() {
            m_current = -1;
        }
    }
}
