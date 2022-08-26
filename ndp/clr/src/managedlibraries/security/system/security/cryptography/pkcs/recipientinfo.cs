// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

//
// RecipientInfo.cs
// 

namespace System.Security.Cryptography.Pkcs
{
    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Cryptography.Xml;
    using System.Globalization;

    public enum RecipientInfoType {
        Unknown      = 0,
        KeyTransport = 1,   // Must be the same as CAPI.CMSG_KEY_TRANS_RECIPIENT (1)
        KeyAgreement = 2,   // Must be the same as CAPI.CMSG_KEY_AGREE_RECIPIENT (2)
    }

    internal enum RecipientSubType {
        Unknown            = 0,
        Pkcs7KeyTransport  = 1,
        CmsKeyTransport    = 2,
        CertIdKeyAgreement = 3,
        PublicKeyAgreement = 4,
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public abstract class RecipientInfo {
        private RecipientInfoType    m_recipentInfoType;
        private RecipientSubType     m_recipientSubType;
        [SecurityCritical]
        private SafeLocalAllocHandle m_pCmsgRecipientInfo;
        private Object               m_cmsgRecipientInfo;
        private uint                 m_index;

        internal RecipientInfo () {}

        [SecurityCritical]
        internal RecipientInfo (RecipientInfoType recipientInfoType, RecipientSubType recipientSubType, SafeLocalAllocHandle pCmsgRecipientInfo, Object cmsgRecipientInfo, uint index) {
            if (recipientInfoType < RecipientInfoType.Unknown || recipientInfoType > RecipientInfoType.KeyAgreement)
                recipientInfoType = RecipientInfoType.Unknown;

            if (recipientSubType < RecipientSubType.Unknown || recipientSubType > RecipientSubType.PublicKeyAgreement)
                recipientSubType = RecipientSubType.Unknown;

            m_recipentInfoType = recipientInfoType;
            m_recipientSubType = recipientSubType;
            m_pCmsgRecipientInfo = pCmsgRecipientInfo;
            m_cmsgRecipientInfo = cmsgRecipientInfo;
            m_index = index;
        }

        public RecipientInfoType Type {
            get {
                return m_recipentInfoType;
            }
        }

        public abstract int Version { get; }

        public abstract SubjectIdentifier RecipientIdentifier { get; }

        public abstract AlgorithmIdentifier KeyEncryptionAlgorithm { get; }

        public abstract byte[] EncryptedKey { get; }

        //
        // Internal methods.
        //

        internal RecipientSubType SubType {
            get {
                return m_recipientSubType;
            }
        }

        internal SafeLocalAllocHandle pCmsgRecipientInfo {
            [SecurityCritical]
            get {
                return m_pCmsgRecipientInfo;
            }
        }

        internal Object CmsgRecipientInfo {
            get {
                return m_cmsgRecipientInfo;
            }
        }

        internal uint Index {
            get {
                return m_index;
            }
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class KeyTransRecipientInfo : RecipientInfo {
        private int                     m_version;
        private SubjectIdentifier       m_recipientIdentifier;
        private AlgorithmIdentifier     m_encryptionAlgorithm;
        private byte[]                  m_encryptedKey;

        [SecurityCritical]
        internal unsafe KeyTransRecipientInfo (SafeLocalAllocHandle pRecipientInfo, CAPI.CERT_INFO certInfo, uint index) : base(RecipientInfoType.KeyTransport, RecipientSubType.Pkcs7KeyTransport, pRecipientInfo, certInfo, index) {
            // If serial number is 0, then it is the special SKI encoding.
            int version = 2;
            byte * pb = (byte *) certInfo.SerialNumber.pbData;
            for (int i = 0; i < certInfo.SerialNumber.cbData; i++) {
                if (*pb++ != (byte) 0) {
                    version = 0;
                    break;
                }
            }

            Reset(version);
        }

        [SecurityCritical]
        internal KeyTransRecipientInfo (SafeLocalAllocHandle pRecipientInfo, CAPI.CMSG_KEY_TRANS_RECIPIENT_INFO keyTrans, uint index) : base(RecipientInfoType.KeyTransport, RecipientSubType.CmsKeyTransport, pRecipientInfo, keyTrans, index) {
            Reset((int) keyTrans.dwVersion);
        }

        public override int Version {
            get {
                return m_version;
            }
        }

        public override SubjectIdentifier RecipientIdentifier {
            [SecuritySafeCritical]
            get {
                if (m_recipientIdentifier == null) {
                    if (this.SubType == RecipientSubType.CmsKeyTransport) {
                        CAPI.CMSG_KEY_TRANS_RECIPIENT_INFO keyTrans = (CAPI.CMSG_KEY_TRANS_RECIPIENT_INFO) CmsgRecipientInfo;
                        m_recipientIdentifier = new SubjectIdentifier(keyTrans.RecipientId);
                    }
                    else {
                        CAPI.CERT_INFO certInfo = (CAPI.CERT_INFO) CmsgRecipientInfo;
                        m_recipientIdentifier = new SubjectIdentifier(certInfo);
                    }
                }

                return m_recipientIdentifier;
            }
        }

        public override AlgorithmIdentifier KeyEncryptionAlgorithm {
            [SecuritySafeCritical]
            get {
                if (m_encryptionAlgorithm == null) {
                    if (this.SubType == RecipientSubType.CmsKeyTransport) {
                        CAPI.CMSG_KEY_TRANS_RECIPIENT_INFO keyTrans = (CAPI.CMSG_KEY_TRANS_RECIPIENT_INFO) CmsgRecipientInfo;
                        m_encryptionAlgorithm = new AlgorithmIdentifier(keyTrans.KeyEncryptionAlgorithm);
                    }
                    else {
                        CAPI.CERT_INFO certInfo = (CAPI.CERT_INFO) CmsgRecipientInfo;
                        m_encryptionAlgorithm = new AlgorithmIdentifier(certInfo.SignatureAlgorithm);
                    }
                }

                return m_encryptionAlgorithm;
            }
        }

        public override byte[] EncryptedKey {
            [SecuritySafeCritical]
            get {
                if (m_encryptedKey.Length == 0) {
                    // CAPI does not provide a way to retrieve encrypted key for PKCS 7 message.
                    if (this.SubType == RecipientSubType.CmsKeyTransport) {
                        CAPI.CMSG_KEY_TRANS_RECIPIENT_INFO keyTrans = (CAPI.CMSG_KEY_TRANS_RECIPIENT_INFO) CmsgRecipientInfo;
                        if (keyTrans.EncryptedKey.cbData > 0) {
                            m_encryptedKey = new byte[keyTrans.EncryptedKey.cbData];
                            Marshal.Copy(keyTrans.EncryptedKey.pbData, m_encryptedKey, 0, m_encryptedKey.Length);
                        }
                    }
                }

                return m_encryptedKey;
            }
        }

        //
        // Private methods.
        //

        private void Reset (int version) {
            m_version = version;
            m_recipientIdentifier = null;
            m_encryptionAlgorithm = null;
            m_encryptedKey = new byte[0];
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class KeyAgreeRecipientInfo : RecipientInfo {
        private CAPI.CMSG_RECIPIENT_ENCRYPTED_KEY_INFO  m_encryptedKeyInfo;
        private uint                                    m_originatorChoice;
        private int                                     m_version;
        private SubjectIdentifierOrKey                  m_originatorIdentifier;
        private byte[]                                  m_userKeyMaterial;
        private AlgorithmIdentifier                     m_encryptionAlgorithm;
        private SubjectIdentifier                       m_recipientIdentifier;
        private byte[]                                  m_encryptedKey;
        private DateTime                                m_date;
        private CryptographicAttributeObject                  m_otherKeyAttribute;
        private uint                                    m_subIndex;

        private KeyAgreeRecipientInfo () {}

        [SecurityCritical]
        internal KeyAgreeRecipientInfo (SafeLocalAllocHandle pRecipientInfo, CAPI.CMSG_KEY_AGREE_CERT_ID_RECIPIENT_INFO certIdRecipient, uint index, uint subIndex) : base(RecipientInfoType.KeyAgreement, RecipientSubType.CertIdKeyAgreement, pRecipientInfo, certIdRecipient, index) {
            checked {
                IntPtr pEncryptedKeyInfo = Marshal.ReadIntPtr(new IntPtr((long) certIdRecipient.rgpRecipientEncryptedKeys + (long) (subIndex * Marshal.SizeOf(typeof(IntPtr)))));
                CAPI.CMSG_RECIPIENT_ENCRYPTED_KEY_INFO encryptedKeyInfo = (CAPI.CMSG_RECIPIENT_ENCRYPTED_KEY_INFO) Marshal.PtrToStructure(pEncryptedKeyInfo, typeof(CAPI.CMSG_RECIPIENT_ENCRYPTED_KEY_INFO));
                Reset(CAPI.CMSG_KEY_AGREE_ORIGINATOR_CERT, certIdRecipient.dwVersion, encryptedKeyInfo, subIndex);
            }
        }

        [SecurityCritical]
        internal KeyAgreeRecipientInfo (SafeLocalAllocHandle pRecipientInfo, CAPI.CMSG_KEY_AGREE_PUBLIC_KEY_RECIPIENT_INFO publicKeyRecipient, uint index, uint subIndex) : base(RecipientInfoType.KeyAgreement, RecipientSubType.PublicKeyAgreement, pRecipientInfo, publicKeyRecipient, index) {
            checked {
                IntPtr pEncryptedKeyInfo = Marshal.ReadIntPtr(new IntPtr((long) publicKeyRecipient.rgpRecipientEncryptedKeys + (long) (subIndex * Marshal.SizeOf(typeof(IntPtr)))));
                CAPI.CMSG_RECIPIENT_ENCRYPTED_KEY_INFO encryptedKeyInfo = (CAPI.CMSG_RECIPIENT_ENCRYPTED_KEY_INFO) Marshal.PtrToStructure(pEncryptedKeyInfo, typeof(CAPI.CMSG_RECIPIENT_ENCRYPTED_KEY_INFO));
                Reset(CAPI.CMSG_KEY_AGREE_ORIGINATOR_PUBLIC_KEY, publicKeyRecipient.dwVersion, encryptedKeyInfo, subIndex);
            }
        }

        public override int Version {
            get {
                return m_version;
            }
        }

        public SubjectIdentifierOrKey OriginatorIdentifierOrKey {
            [SecuritySafeCritical]
            get {
                if (m_originatorIdentifier == null) {
                    if (m_originatorChoice == CAPI.CMSG_KEY_AGREE_ORIGINATOR_CERT) {
                        CAPI.CMSG_KEY_AGREE_CERT_ID_RECIPIENT_INFO recipientInfo = (CAPI.CMSG_KEY_AGREE_CERT_ID_RECIPIENT_INFO) CmsgRecipientInfo;
                        m_originatorIdentifier = new SubjectIdentifierOrKey(recipientInfo.OriginatorCertId);
                    }
                    else {
                        CAPI.CMSG_KEY_AGREE_PUBLIC_KEY_RECIPIENT_INFO recipientInfo = (CAPI.CMSG_KEY_AGREE_PUBLIC_KEY_RECIPIENT_INFO) CmsgRecipientInfo;
                        m_originatorIdentifier = new SubjectIdentifierOrKey(recipientInfo.OriginatorPublicKeyInfo);
                    }
                }

                return m_originatorIdentifier;
            }
        }

        public override SubjectIdentifier RecipientIdentifier {
            [SecuritySafeCritical]
            get {
                if (m_recipientIdentifier == null)
                    m_recipientIdentifier = new SubjectIdentifier(m_encryptedKeyInfo.RecipientId);

                return m_recipientIdentifier;
            }
        }

        public DateTime Date {
            get {
                if (m_date == DateTime.MinValue) {
                    if (this.RecipientIdentifier.Type != SubjectIdentifierType.SubjectKeyIdentifier)
                        throw new InvalidOperationException(SecurityResources.GetResourceString("Cryptography_Cms_Key_Agree_Date_Not_Available"));

                    long date = (((long)(uint) m_encryptedKeyInfo.Date.dwHighDateTime) << 32) | ((long)(uint) m_encryptedKeyInfo.Date.dwLowDateTime);
                    m_date = DateTime.FromFileTimeUtc(date);
                }

                return m_date;
            }
        }

        public CryptographicAttributeObject OtherKeyAttribute {
            [SecuritySafeCritical]
            get {
                if (m_otherKeyAttribute == null) {
                    if (this.RecipientIdentifier.Type != SubjectIdentifierType.SubjectKeyIdentifier)
                        throw new InvalidOperationException(SecurityResources.GetResourceString("Cryptography_Cms_Key_Agree_Other_Key_Attribute_Not_Available"));

                    if (m_encryptedKeyInfo.pOtherAttr != IntPtr.Zero) {
                        CAPI.CRYPT_ATTRIBUTE_TYPE_VALUE otherKeyAttribute = (CAPI.CRYPT_ATTRIBUTE_TYPE_VALUE) Marshal.PtrToStructure(m_encryptedKeyInfo.pOtherAttr, typeof(CAPI.CRYPT_ATTRIBUTE_TYPE_VALUE));
                        m_otherKeyAttribute = new CryptographicAttributeObject(otherKeyAttribute);
                    }
                }

                return m_otherKeyAttribute;
            }
        }

        public override AlgorithmIdentifier KeyEncryptionAlgorithm {
            [SecuritySafeCritical]
            get {
                if (m_encryptionAlgorithm == null) {
                    if (m_originatorChoice == CAPI.CMSG_KEY_AGREE_ORIGINATOR_CERT) {
                        CAPI.CMSG_KEY_AGREE_CERT_ID_RECIPIENT_INFO recipientInfo = (CAPI.CMSG_KEY_AGREE_CERT_ID_RECIPIENT_INFO) CmsgRecipientInfo;
                        m_encryptionAlgorithm = new AlgorithmIdentifier(recipientInfo.KeyEncryptionAlgorithm);
                    }
                    else {
                        CAPI.CMSG_KEY_AGREE_PUBLIC_KEY_RECIPIENT_INFO recipientInfo = (CAPI.CMSG_KEY_AGREE_PUBLIC_KEY_RECIPIENT_INFO) CmsgRecipientInfo;
                        m_encryptionAlgorithm = new AlgorithmIdentifier(recipientInfo.KeyEncryptionAlgorithm);
                    }
                }

                return m_encryptionAlgorithm;
            }
        }

        public override byte[] EncryptedKey {
            [SecuritySafeCritical]
            get {
                if (m_encryptedKey.Length == 0) {
                    if (m_encryptedKeyInfo.EncryptedKey.cbData > 0) {
                        m_encryptedKey = new byte[m_encryptedKeyInfo.EncryptedKey.cbData];
                        Marshal.Copy(m_encryptedKeyInfo.EncryptedKey.pbData, m_encryptedKey, 0, m_encryptedKey.Length);
                    }
                }

                return m_encryptedKey;
            }
        }

        //
        // Internal methods.
        //

        internal CAPI.CERT_ID RecipientId {
            get {
                return m_encryptedKeyInfo.RecipientId;
            }
        }

        internal uint SubIndex {
            get {
                return m_subIndex;
            }
        }

        //
        // Private methods.
        //

        private void Reset (uint originatorChoice, uint version, CAPI.CMSG_RECIPIENT_ENCRYPTED_KEY_INFO encryptedKeyInfo, uint subIndex) {
            m_encryptedKeyInfo = encryptedKeyInfo;
            m_originatorChoice = originatorChoice;
            m_version = (int) version;
            m_originatorIdentifier = null;
            m_userKeyMaterial = new byte[0];
            m_encryptionAlgorithm = null;
            m_recipientIdentifier = null;
            m_encryptedKey = new byte[0];
            m_date = DateTime.MinValue;
            m_otherKeyAttribute = null;
            m_subIndex = subIndex;
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class RecipientInfoCollection : ICollection {
        [SecurityCritical]
        private SafeCryptMsgHandle  m_safeCryptMsgHandle;
        private ArrayList           m_recipientInfos;

        [SecuritySafeCritical]
        internal RecipientInfoCollection () {
            m_safeCryptMsgHandle = SafeCryptMsgHandle.InvalidHandle;
            m_recipientInfos = new ArrayList();
        }

        [SecuritySafeCritical]
        internal RecipientInfoCollection (RecipientInfo recipientInfo) {
            m_safeCryptMsgHandle = SafeCryptMsgHandle.InvalidHandle;
            m_recipientInfos = new ArrayList(1);
            m_recipientInfos.Add(recipientInfo);
        }

        [SecurityCritical]
        internal unsafe RecipientInfoCollection (SafeCryptMsgHandle safeCryptMsgHandle) {
            bool cmsSupported = PkcsUtils.CmsSupported();
            uint dwRecipients = 0;
            uint cbCount = (uint) Marshal.SizeOf(typeof(uint));

            // Use CMS if supported.
            if (cmsSupported) {
                // CMS.
                if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                    CAPI.CMSG_CMS_RECIPIENT_COUNT_PARAM,
                                                    0,
                                                    new IntPtr(&dwRecipients),
                                                    new IntPtr(&cbCount)))
                    throw new CryptographicException(Marshal.GetLastWin32Error());
            }
            else {
                // PKCS7.
                if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                    CAPI.CMSG_RECIPIENT_COUNT_PARAM,
                                                    0,
                                                    new IntPtr(&dwRecipients),
                                                    new IntPtr(&cbCount)))
                    throw new CryptographicException(Marshal.GetLastWin32Error());
            }

            m_recipientInfos = new ArrayList();
            
            for (uint index = 0; index < dwRecipients; index++) {
                if (cmsSupported) {
                    uint cbCmsRecipientInfo;
                    SafeLocalAllocHandle pbCmsRecipientInfo;

                    PkcsUtils.GetParam(safeCryptMsgHandle, CAPI.CMSG_CMS_RECIPIENT_INFO_PARAM, index, out pbCmsRecipientInfo, out cbCmsRecipientInfo);
                    CAPI.CMSG_CMS_RECIPIENT_INFO cmsRecipientInfo = (CAPI.CMSG_CMS_RECIPIENT_INFO) Marshal.PtrToStructure(pbCmsRecipientInfo.DangerousGetHandle(), typeof(CAPI.CMSG_CMS_RECIPIENT_INFO));

                    switch (cmsRecipientInfo.dwRecipientChoice) {
                    case CAPI.CMSG_KEY_TRANS_RECIPIENT:
                        CAPI.CMSG_KEY_TRANS_RECIPIENT_INFO keyTrans = (CAPI.CMSG_KEY_TRANS_RECIPIENT_INFO) Marshal.PtrToStructure(cmsRecipientInfo.pRecipientInfo, typeof(CAPI.CMSG_KEY_TRANS_RECIPIENT_INFO));
                        m_recipientInfos.Add(new KeyTransRecipientInfo(pbCmsRecipientInfo, keyTrans, index));
                        break;
                    case CAPI.CMSG_KEY_AGREE_RECIPIENT:
                        CAPI.CMSG_KEY_AGREE_RECIPIENT_INFO keyAgree = (CAPI.CMSG_KEY_AGREE_RECIPIENT_INFO) Marshal.PtrToStructure(cmsRecipientInfo.pRecipientInfo, typeof(CAPI.CMSG_KEY_AGREE_RECIPIENT_INFO));
                        switch (keyAgree.dwOriginatorChoice) {
                        case CAPI.CMSG_KEY_AGREE_ORIGINATOR_CERT:
                            CAPI.CMSG_KEY_AGREE_CERT_ID_RECIPIENT_INFO certIdRecipient = (CAPI.CMSG_KEY_AGREE_CERT_ID_RECIPIENT_INFO) Marshal.PtrToStructure(cmsRecipientInfo.pRecipientInfo, typeof(CAPI.CMSG_KEY_AGREE_CERT_ID_RECIPIENT_INFO));
                            for (uint cRecipient = 0; cRecipient < certIdRecipient.cRecipientEncryptedKeys; cRecipient++) {
                                m_recipientInfos.Add(new KeyAgreeRecipientInfo(pbCmsRecipientInfo, certIdRecipient, index, cRecipient));
                            }
                            break;
                        case CAPI.CMSG_KEY_AGREE_ORIGINATOR_PUBLIC_KEY:
                            CAPI.CMSG_KEY_AGREE_PUBLIC_KEY_RECIPIENT_INFO publicKeyRecipient = (CAPI.CMSG_KEY_AGREE_PUBLIC_KEY_RECIPIENT_INFO) Marshal.PtrToStructure(cmsRecipientInfo.pRecipientInfo, typeof(CAPI.CMSG_KEY_AGREE_PUBLIC_KEY_RECIPIENT_INFO));
                            for (uint cRecipient = 0; cRecipient < publicKeyRecipient.cRecipientEncryptedKeys; cRecipient++) {
                                m_recipientInfos.Add(new KeyAgreeRecipientInfo(pbCmsRecipientInfo, publicKeyRecipient, index, cRecipient));
                            }
                            break;
                        default:
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Invalid_Originator_Identifier_Choice"), keyAgree.dwOriginatorChoice.ToString(CultureInfo.CurrentCulture));
                        }
                        break;
                    default: 
                        throw new CryptographicException(CAPI.E_NOTIMPL);
                    }
                }
                else {
                    uint cbCertInfo;
                    SafeLocalAllocHandle pbCertInfo;

                    PkcsUtils.GetParam(safeCryptMsgHandle, CAPI.CMSG_RECIPIENT_INFO_PARAM, index, out pbCertInfo, out cbCertInfo);
                    CAPI.CERT_INFO certInfo = (CAPI.CERT_INFO) Marshal.PtrToStructure(pbCertInfo.DangerousGetHandle(), typeof(CAPI.CERT_INFO));

                    m_recipientInfos.Add(new KeyTransRecipientInfo(pbCertInfo, certInfo, index));
                }
            }

            m_safeCryptMsgHandle = safeCryptMsgHandle;
        }

        public RecipientInfo this[int index] {
            get {
                if (index < 0 || index >= m_recipientInfos.Count)
                    throw new ArgumentOutOfRangeException("index", SecurityResources.GetResourceString("ArgumentOutOfRange_Index"));
                return (RecipientInfo) m_recipientInfos[index];
            }
        }

        public int Count {
            get {
                return m_recipientInfos.Count;
            }
        }

        public RecipientInfoEnumerator GetEnumerator() {
            return new RecipientInfoEnumerator(this);
        }

        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return new RecipientInfoEnumerator(this);
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

        public void CopyTo(RecipientInfo[] array, int index) {
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
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class RecipientInfoEnumerator : IEnumerator {
        private RecipientInfoCollection m_recipientInfos;
        private int m_current;

        private RecipientInfoEnumerator() {}

        internal RecipientInfoEnumerator(RecipientInfoCollection RecipientInfos) {
            m_recipientInfos = RecipientInfos;
            m_current = -1;
        }

        public RecipientInfo Current {
            get {
                return m_recipientInfos[m_current];
            }
        }

        /// <internalonly/>
        Object IEnumerator.Current {
            get {
                return (Object) m_recipientInfos[m_current];
            }
        }

        public bool MoveNext() {
            if (m_current == ((int) m_recipientInfos.Count - 1)) {
                return false;
            }
            m_current++;
            return true;
        }

        public void Reset() {
            m_current = -1;
        }
    }
}
