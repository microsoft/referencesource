// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

//
// EnvelopedPkcs7.cs
// 

namespace System.Security.Cryptography.Pkcs {
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.Versioning;
    using System.Security;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Cryptography.Xml;
    using System.Security.Permissions;
    using System.Text;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class EnvelopedCms {
        [SecurityCritical]
        private SafeCryptMsgHandle               m_safeCryptMsgHandle;
        private int                              m_version;
        private SubjectIdentifierType            m_recipientIdentifierType;
        private ContentInfo                      m_contentInfo;
        private AlgorithmIdentifier              m_encryptionAlgorithm;
        private X509Certificate2Collection       m_certificates;
        private CryptographicAttributeObjectCollection m_unprotectedAttributes;

        [SecurityCritical]
        private struct CMSG_DECRYPT_PARAM {
            internal SafeCertContextHandle  safeCertContextHandle;
            internal SafeCryptProvHandle    safeCryptProvHandle;
            internal uint                   keySpec;
        }

        [SecurityCritical]
        private struct CMSG_ENCRYPT_PARAM {
            internal bool                            useCms;
            internal SafeCryptProvHandle             safeCryptProvHandle;
            internal SafeLocalAllocHandle            pvEncryptionAuxInfo;
            internal SafeLocalAllocHandle            rgpRecipients;
            internal SafeLocalAllocHandle            rgCertEncoded;
            internal SafeLocalAllocHandle            rgUnprotectedAttr;
            internal SafeLocalAllocHandle[]          rgSubjectKeyIdentifier;
            internal SafeLocalAllocHandle[]          rgszObjId;
            internal SafeLocalAllocHandle[]          rgszKeyWrapObjId;
            internal SafeLocalAllocHandle[]          rgKeyWrapAuxInfo;
            internal SafeLocalAllocHandle[]          rgEphemeralIdentifier;
            internal SafeLocalAllocHandle[]          rgszEphemeralObjId;
            internal SafeLocalAllocHandle[]          rgUserKeyingMaterial;
            internal SafeLocalAllocHandle[]          prgpEncryptedKey;
            internal SafeLocalAllocHandle[]          rgpEncryptedKey;
        }

        //
        // Constructors.
        //

        private static AlgorithmIdentifier GetDefaultEncryptionAlgorithm()
        {
            string oidValue = LocalAppContextSwitches.EnvelopedCmsUseLegacyDefaultAlgorithm ?
                CAPI.szOID_RSA_DES_EDE3_CBC :
                CAPI.szOID_NIST_AES256_CBC;

            return new AlgorithmIdentifier(Oid.FromOidValue(oidValue, OidGroup.EncryptionAlgorithm));
        }

        public EnvelopedCms () : 
            this(SubjectIdentifierType.IssuerAndSerialNumber,
                 new ContentInfo(Oid.FromOidValue(CAPI.szOID_RSA_data, OidGroup.ExtensionOrAttribute), new byte[0]),
                 GetDefaultEncryptionAlgorithm()) {
        }

        public EnvelopedCms (ContentInfo contentInfo) : 
            this(SubjectIdentifierType.IssuerAndSerialNumber, contentInfo,
                 GetDefaultEncryptionAlgorithm()) {
        }

        public EnvelopedCms (SubjectIdentifierType recipientIdentifierType, ContentInfo contentInfo) : 
            this(recipientIdentifierType,
                 contentInfo,
                 GetDefaultEncryptionAlgorithm()) {
        }

        public EnvelopedCms (ContentInfo contentInfo, AlgorithmIdentifier encryptionAlgorithm) : 
            this(SubjectIdentifierType.IssuerAndSerialNumber, contentInfo, encryptionAlgorithm) {
        }

        [SecuritySafeCritical]
        public EnvelopedCms (SubjectIdentifierType recipientIdentifierType, ContentInfo contentInfo, AlgorithmIdentifier encryptionAlgorithm) {
            if (contentInfo == null)
                throw new ArgumentNullException("contentInfo");
            if (contentInfo.Content == null)
                throw new ArgumentNullException("contentInfo.Content");
            if (encryptionAlgorithm == null)
                throw new ArgumentNullException("encryptionAlgorithm");

            m_safeCryptMsgHandle = SafeCryptMsgHandle.InvalidHandle;
            m_version = recipientIdentifierType == SubjectIdentifierType.SubjectKeyIdentifier ? 2 : 0;
            m_recipientIdentifierType = recipientIdentifierType;
            m_contentInfo = contentInfo;
            m_encryptionAlgorithm = encryptionAlgorithm;
            m_encryptionAlgorithm.Parameters = new byte[0];
            m_certificates = new X509Certificate2Collection();
            m_unprotectedAttributes = new CryptographicAttributeObjectCollection();
        }

        //
        // Public APIs.
        //

        public int Version {
            get {
                return m_version;
            }
        }

        public ContentInfo ContentInfo {
            get {
                return m_contentInfo;
            }
        }

        public AlgorithmIdentifier ContentEncryptionAlgorithm {
            get {
                return m_encryptionAlgorithm;
            }
        }

        public X509Certificate2Collection Certificates {
            get {
                return m_certificates;
            }
        }

        public CryptographicAttributeObjectCollection UnprotectedAttributes {
            get {
                return m_unprotectedAttributes;
            }
        }

        public RecipientInfoCollection RecipientInfos {
            [SecuritySafeCritical]
            get {
                if (m_safeCryptMsgHandle == null || m_safeCryptMsgHandle.IsInvalid) {
                    return new RecipientInfoCollection();
                }
                return new RecipientInfoCollection(m_safeCryptMsgHandle);
            }
        }

        [SecuritySafeCritical]
        public byte[] Encode () {
            if (m_safeCryptMsgHandle == null || m_safeCryptMsgHandle.IsInvalid)
                throw new InvalidOperationException(SecurityResources.GetResourceString("Cryptography_Cms_MessageNotEncrypted"));

            return PkcsUtils.GetContent(m_safeCryptMsgHandle);
        }

        [SecuritySafeCritical]
        public void Decode (byte[] encodedMessage) {
            if (encodedMessage == null)
                throw new ArgumentNullException("encodedMessage");

            if (m_safeCryptMsgHandle != null && !m_safeCryptMsgHandle.IsInvalid) {
                m_safeCryptMsgHandle.Dispose();
            }

            // Open to decode.
            m_safeCryptMsgHandle = OpenToDecode(encodedMessage);

            // Get version.
            m_version = (int) PkcsUtils.GetVersion(m_safeCryptMsgHandle);

            // Get contentInfo (content still encrypted).
            Oid contentType = PkcsUtils.GetContentType(m_safeCryptMsgHandle);
            byte[] content = PkcsUtils.GetContent(m_safeCryptMsgHandle);
            m_contentInfo = new ContentInfo(contentType, content); 
            
            // Get encryption algorithm.
            m_encryptionAlgorithm = PkcsUtils.GetAlgorithmIdentifier(m_safeCryptMsgHandle);

            // Get certificates.
            m_certificates = PkcsUtils.GetCertificates(m_safeCryptMsgHandle);

            // Get unprotected attributes.
            m_unprotectedAttributes = PkcsUtils.GetUnprotectedAttributes(m_safeCryptMsgHandle);
        }

        public void Encrypt () {
            Encrypt(new CmsRecipientCollection());
        }

        public void Encrypt (CmsRecipient recipient) {
            if (recipient == null)
                throw new ArgumentNullException("recipient");

            Encrypt(new CmsRecipientCollection(recipient));
        }

        public void Encrypt (CmsRecipientCollection recipients) {
            if (recipients == null)
                throw new ArgumentNullException("recipients");

            if (ContentInfo.Content.Length == 0)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Envelope_Empty_Content"));

            if (recipients.Count == 0)
                recipients = PkcsUtils.SelectRecipients(m_recipientIdentifierType);

            EncryptContent(recipients);
        }

        public void Decrypt () {
            DecryptContent(this.RecipientInfos, null);
        }

        public void Decrypt (RecipientInfo recipientInfo) {
            if (recipientInfo == null)
                throw new ArgumentNullException("recipientInfo");

            DecryptContent(new RecipientInfoCollection(recipientInfo), null);
        }

        public void Decrypt (X509Certificate2Collection extraStore) {
            if (extraStore == null)
                throw new ArgumentNullException("extraStore");

            DecryptContent(this.RecipientInfos, extraStore);
        }

        public void Decrypt (RecipientInfo recipientInfo, X509Certificate2Collection extraStore) {
            if (recipientInfo == null)
                throw new ArgumentNullException("recipientInfo");

            if (extraStore == null)
                throw new ArgumentNullException("extraStore");

            DecryptContent(new RecipientInfoCollection(recipientInfo), extraStore);
        }

        //
        // Private methods.
        //

        [SecuritySafeCritical]
        private unsafe void DecryptContent (RecipientInfoCollection recipientInfos, X509Certificate2Collection extraStore) {
            int hr = CAPI.CRYPT_E_RECIPIENT_NOT_FOUND;

            if (m_safeCryptMsgHandle == null || m_safeCryptMsgHandle.IsInvalid)
                throw new InvalidOperationException(SecurityResources.GetResourceString("Cryptography_Cms_NoEncryptedMessageToEncode"));

            for (int index = 0; index < recipientInfos.Count; index++) {
                RecipientInfo recipientInfo = recipientInfos[index];
                CMSG_DECRYPT_PARAM cmsgDecryptParam = new CMSG_DECRYPT_PARAM();

                // Get CSP parameters
                int hr2 = GetCspParams(recipientInfo, extraStore, ref cmsgDecryptParam);

                if (hr2 == CAPI.S_OK) {
                    // 


                    CspParameters parameters = new CspParameters();
                    if (X509Utils.GetPrivateKeyInfo(cmsgDecryptParam.safeCertContextHandle, ref parameters))
                    {
                        KeyContainerPermission kp = new KeyContainerPermission(KeyContainerPermissionFlags.NoFlags);
                        KeyContainerPermissionAccessEntry entry = new KeyContainerPermissionAccessEntry(parameters, KeyContainerPermissionFlags.Open | KeyContainerPermissionFlags.Decrypt);
                        kp.AccessEntries.Add(entry);
                        kp.Demand();
                    }

                    // Decrypt the content.
                    switch (recipientInfo.Type) {
                    case RecipientInfoType.KeyTransport:
                        CAPI.CMSG_CTRL_DECRYPT_PARA ctrlDecryptPara = new CAPI.CMSG_CTRL_DECRYPT_PARA(Marshal.SizeOf(typeof(CAPI.CMSG_CTRL_DECRYPT_PARA)));
                        ctrlDecryptPara.hCryptProv = cmsgDecryptParam.safeCryptProvHandle.DangerousGetHandle();
                        ctrlDecryptPara.dwKeySpec = cmsgDecryptParam.keySpec;
                        ctrlDecryptPara.dwRecipientIndex = (uint) recipientInfo.Index;

                        if (!CAPI.CryptMsgControl(m_safeCryptMsgHandle,
                                                  0,
                                                  CAPI.CMSG_CTRL_DECRYPT,
                                                  new IntPtr(&ctrlDecryptPara)))
                            hr2 = Marshal.GetHRForLastWin32Error();

                        GC.KeepAlive(ctrlDecryptPara);
                        break;

                    case RecipientInfoType.KeyAgreement:
                        SafeCertContextHandle pOriginatorCert = SafeCertContextHandle.InvalidHandle;
                        KeyAgreeRecipientInfo keyAgree = (KeyAgreeRecipientInfo) recipientInfo;
                        CAPI.CMSG_CMS_RECIPIENT_INFO cmsRecipientInfo = (CAPI.CMSG_CMS_RECIPIENT_INFO) Marshal.PtrToStructure(keyAgree.pCmsgRecipientInfo.DangerousGetHandle(), typeof(CAPI.CMSG_CMS_RECIPIENT_INFO));

                        CAPI.CMSG_CTRL_KEY_AGREE_DECRYPT_PARA keyAgreeDecryptPara = new CAPI.CMSG_CTRL_KEY_AGREE_DECRYPT_PARA(Marshal.SizeOf(typeof(CAPI.CMSG_CTRL_KEY_AGREE_DECRYPT_PARA)));
                        keyAgreeDecryptPara.hCryptProv = cmsgDecryptParam.safeCryptProvHandle.DangerousGetHandle();
                        keyAgreeDecryptPara.dwKeySpec = cmsgDecryptParam.keySpec;
                        keyAgreeDecryptPara.pKeyAgree = cmsRecipientInfo.pRecipientInfo;
                        keyAgreeDecryptPara.dwRecipientIndex = keyAgree.Index;
                        keyAgreeDecryptPara.dwRecipientEncryptedKeyIndex = keyAgree.SubIndex;

                        if (keyAgree.SubType == RecipientSubType.CertIdKeyAgreement) {
                            CAPI.CMSG_KEY_AGREE_CERT_ID_RECIPIENT_INFO certIdKeyAgree = (CAPI.CMSG_KEY_AGREE_CERT_ID_RECIPIENT_INFO) keyAgree.CmsgRecipientInfo;
                            SafeCertStoreHandle hCertStore = BuildOriginatorStore(this.Certificates, extraStore);

                            pOriginatorCert = CAPI.CertFindCertificateInStore(hCertStore, 
                                                                              CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                                              0, 
                                                                              CAPI.CERT_FIND_CERT_ID,
                                                                              new IntPtr(&certIdKeyAgree.OriginatorCertId),
                                                                              SafeCertContextHandle.InvalidHandle);
                            if (pOriginatorCert == null || pOriginatorCert.IsInvalid) {
                                hr2 = CAPI.CRYPT_E_NOT_FOUND;
                                break;
                            }

                            CAPI.CERT_CONTEXT pCertContext = (CAPI.CERT_CONTEXT) Marshal.PtrToStructure(pOriginatorCert.DangerousGetHandle(), typeof(CAPI.CERT_CONTEXT));
                            CAPI.CERT_INFO certInfo = (CAPI.CERT_INFO) Marshal.PtrToStructure(pCertContext.pCertInfo, typeof(CAPI.CERT_INFO));
                            keyAgreeDecryptPara.OriginatorPublicKey = certInfo.SubjectPublicKeyInfo.PublicKey;
                        }
                        else {
                            CAPI.CMSG_KEY_AGREE_PUBLIC_KEY_RECIPIENT_INFO publicKeyAgree = (CAPI.CMSG_KEY_AGREE_PUBLIC_KEY_RECIPIENT_INFO) keyAgree.CmsgRecipientInfo;
                            keyAgreeDecryptPara.OriginatorPublicKey = publicKeyAgree.OriginatorPublicKeyInfo.PublicKey;
                        }
                        
                        if (!CAPI.CryptMsgControl(m_safeCryptMsgHandle,
                                                  0,
                                                  CAPI.CMSG_CTRL_KEY_AGREE_DECRYPT,
                                                  new IntPtr(&keyAgreeDecryptPara)))
                            hr2 = Marshal.GetHRForLastWin32Error();

                        GC.KeepAlive(keyAgreeDecryptPara);
                        GC.KeepAlive(pOriginatorCert);
                        break;

                    default:
                        throw new CryptographicException(CAPI.E_NOTIMPL);
                    }

                    GC.KeepAlive(cmsgDecryptParam);
                }

                // Content decrypted?
                if (hr2 == CAPI.S_OK) {
                    // Yes, so retrieve it.
                    uint cbContent = 0;
                    SafeLocalAllocHandle pbContent = SafeLocalAllocHandle.InvalidHandle;

                    PkcsUtils.GetParam(m_safeCryptMsgHandle, CAPI.CMSG_CONTENT_PARAM, 0, out pbContent, out cbContent);

                    if (cbContent > 0) {
                        Oid contentType = PkcsUtils.GetContentType(m_safeCryptMsgHandle);
                        byte[] content = new byte[cbContent];
                        Marshal.Copy(pbContent.DangerousGetHandle(), content, 0, (int) cbContent);

                        m_contentInfo = new ContentInfo(contentType, content);
                    }

                    pbContent.Dispose();

                    hr = CAPI.S_OK;
                    break;
                }
                else {
                    // Try next recipient.
                    hr = hr2;
                }
            }

            if (hr != CAPI.S_OK)
                throw new CryptographicException(hr);

            return;
        }

        [SecuritySafeCritical]
        private unsafe void EncryptContent (CmsRecipientCollection recipients) {
            CMSG_ENCRYPT_PARAM encryptParam = new CMSG_ENCRYPT_PARAM();

            if (recipients.Count < 1)
                throw new CryptographicException(CAPI.CRYPT_E_RECIPIENT_NOT_FOUND);

            foreach (CmsRecipient recipient in recipients) {
                if (recipient.Certificate == null)
                    throw new ArgumentNullException(SecurityResources.GetResourceString("Cryptography_Cms_RecipientCertificateNotFound"));

                if ((PkcsUtils.GetRecipientInfoType(recipient.Certificate) == RecipientInfoType.KeyAgreement) ||
                    (recipient.RecipientIdentifierType == SubjectIdentifierType.SubjectKeyIdentifier))
                    encryptParam.useCms = true;
            }

            if (!encryptParam.useCms) {
                if (this.Certificates.Count > 0 || this.UnprotectedAttributes.Count > 0) {
                    encryptParam.useCms = true;
                }
            }

            if (encryptParam.useCms && !PkcsUtils.CmsSupported()) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Not_Supported"));
            }

            CAPI.CMSG_ENVELOPED_ENCODE_INFO encodeInfo = new CAPI.CMSG_ENVELOPED_ENCODE_INFO(Marshal.SizeOf(typeof(CAPI.CMSG_ENVELOPED_ENCODE_INFO)));
            SafeLocalAllocHandle ceei = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(Marshal.SizeOf(typeof(CAPI.CMSG_ENVELOPED_ENCODE_INFO))));

            SetCspParams(this.ContentEncryptionAlgorithm, ref encryptParam);
            encodeInfo.ContentEncryptionAlgorithm.pszObjId = this.ContentEncryptionAlgorithm.Oid.Value;
            //encodeInfo.hCryptProv = encryptParam.safeCryptProvHandle.DangerousGetHandle(); 
            if (encryptParam.pvEncryptionAuxInfo != null && !encryptParam.pvEncryptionAuxInfo.IsInvalid) {
                encodeInfo.pvEncryptionAuxInfo = encryptParam.pvEncryptionAuxInfo.DangerousGetHandle();
            }

            encodeInfo.cRecipients = (uint) recipients.Count;

            List<SafeCertContextHandle> certContexts = null;
            if (encryptParam.useCms) {
                SetCmsRecipientParams(recipients, this.Certificates, this.UnprotectedAttributes, this.ContentEncryptionAlgorithm, ref encryptParam);
                encodeInfo.rgCmsRecipients = encryptParam.rgpRecipients.DangerousGetHandle();
                if (encryptParam.rgCertEncoded != null && !encryptParam.rgCertEncoded.IsInvalid) {
                    encodeInfo.cCertEncoded = (uint) this.Certificates.Count;
                    encodeInfo.rgCertEncoded = encryptParam.rgCertEncoded.DangerousGetHandle();
                }
                if (encryptParam.rgUnprotectedAttr != null && !encryptParam.rgUnprotectedAttr.IsInvalid) {
                    encodeInfo.cUnprotectedAttr = (uint) this.UnprotectedAttributes.Count;
                    encodeInfo.rgUnprotectedAttr = encryptParam.rgUnprotectedAttr.DangerousGetHandle();
                }
            }
            else {
                SetPkcs7RecipientParams(recipients, ref encryptParam, out certContexts);
                encodeInfo.rgpRecipients = encryptParam.rgpRecipients.DangerousGetHandle();
            }

            Marshal.StructureToPtr(encodeInfo, ceei.DangerousGetHandle(), false);

            try {
                SafeCryptMsgHandle safeCryptMsgHandle = CAPI.CryptMsgOpenToEncode(CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                                                  0,
                                                                                  CAPI.CMSG_ENVELOPED,
                                                                                  ceei.DangerousGetHandle(),
                                                                                  this.ContentInfo.ContentType.Value,
                                                                                  IntPtr.Zero);
                if (safeCryptMsgHandle == null || safeCryptMsgHandle.IsInvalid)
                    throw new CryptographicException(Marshal.GetLastWin32Error());

                if (m_safeCryptMsgHandle != null && !m_safeCryptMsgHandle.IsInvalid) {
                    m_safeCryptMsgHandle.Dispose();
                }

                m_safeCryptMsgHandle = safeCryptMsgHandle;
            }
            finally {
                Marshal.DestroyStructure(ceei.DangerousGetHandle(), typeof(CAPI.CMSG_ENVELOPED_ENCODE_INFO));
                ceei.Dispose();
            }

            byte[] encodedContent = new byte[0];
            if (String.Compare(this.ContentInfo.ContentType.Value, CAPI.szOID_RSA_data, StringComparison.OrdinalIgnoreCase) == 0) {
                byte[] content = this.ContentInfo.Content;
                fixed (byte * pbContent = content) {
                    CAPI.CRYPTOAPI_BLOB dataBlob = new CAPI.CRYPTOAPI_BLOB();
                    dataBlob.cbData = (uint) content.Length;
                    dataBlob.pbData = new IntPtr(pbContent);
                    if (!CAPI.EncodeObject(new IntPtr(CAPI.X509_OCTET_STRING), new IntPtr(&dataBlob), out encodedContent))
                        throw new CryptographicException(Marshal.GetLastWin32Error());
                }
            }
            else {
                encodedContent = this.ContentInfo.Content;
            }
            if (encodedContent.Length > 0) {
                if (!CAPI.CAPISafe.CryptMsgUpdate(m_safeCryptMsgHandle, encodedContent, (uint) encodedContent.Length, true))
                    throw new CryptographicException(Marshal.GetLastWin32Error());
            }

            // Keep alive
            GC.KeepAlive(encryptParam);
            GC.KeepAlive(recipients);
            GC.KeepAlive(certContexts);
        }

        //
        // Private static methods.
        //

        [SecuritySafeCritical]
        private static SafeCryptMsgHandle OpenToDecode (byte[] encodedMessage) {
            SafeCryptMsgHandle safeCryptMsgHandle = null;

            // Open the message for decode.
            safeCryptMsgHandle = CAPI.CAPISafe.CryptMsgOpenToDecode(CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                                    0,
                                                                    0,
                                                                    IntPtr.Zero,
                                                                    IntPtr.Zero,
                                                                    IntPtr.Zero);
            if (safeCryptMsgHandle == null || safeCryptMsgHandle.IsInvalid)
                throw new CryptographicException(Marshal.GetLastWin32Error());

            // ---- the message.
            if (!CAPI.CAPISafe.CryptMsgUpdate(safeCryptMsgHandle, encodedMessage, (uint) encodedMessage.Length, true))
                throw new CryptographicException(Marshal.GetLastWin32Error());

            // Make sure this is EnvelopedData type.
            if (CAPI.CMSG_ENVELOPED != PkcsUtils.GetMessageType(safeCryptMsgHandle))
                throw new CryptographicException(CAPI.CRYPT_E_INVALID_MSG_TYPE);

            return safeCryptMsgHandle;
        }

        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)]
        [SecurityCritical]
        private unsafe static int /* HRESULT */ GetCspParams (RecipientInfo recipientInfo,
                                                              X509Certificate2Collection extraStore,
                                                              ref CMSG_DECRYPT_PARAM cmsgDecryptParam) {
            int hr = CAPI.CRYPT_E_RECIPIENT_NOT_FOUND;
            SafeCertContextHandle safeCertContextHandle = SafeCertContextHandle.InvalidHandle;
            SafeCertStoreHandle safeCertStoreHandle = BuildDecryptorStore(extraStore);

            switch (recipientInfo.Type) {
            case RecipientInfoType.KeyTransport:
                if (recipientInfo.SubType == RecipientSubType.Pkcs7KeyTransport) {
                    safeCertContextHandle = CAPI.CertFindCertificateInStore(safeCertStoreHandle, 
                                                                            CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                                            0, 
                                                                            CAPI.CERT_FIND_SUBJECT_CERT,
                                                                            recipientInfo.pCmsgRecipientInfo.DangerousGetHandle(), 
                                                                            SafeCertContextHandle.InvalidHandle);
                }
                else {
                    CAPI.CMSG_KEY_TRANS_RECIPIENT_INFO keyTrans = (CAPI.CMSG_KEY_TRANS_RECIPIENT_INFO) recipientInfo.CmsgRecipientInfo;
                    safeCertContextHandle = CAPI.CertFindCertificateInStore(safeCertStoreHandle, 
                                                                            CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                                            0, 
                                                                            CAPI.CERT_FIND_CERT_ID,
                                                                            new IntPtr((byte *) &keyTrans.RecipientId), 
                                                                            SafeCertContextHandle.InvalidHandle);
                }
                break;

            case RecipientInfoType.KeyAgreement:
                KeyAgreeRecipientInfo keyAgree = (KeyAgreeRecipientInfo) recipientInfo;
                CAPI.CERT_ID recipientId = keyAgree.RecipientId;
                safeCertContextHandle = CAPI.CertFindCertificateInStore(safeCertStoreHandle, 
                                                                        CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                                        0, 
                                                                        CAPI.CERT_FIND_CERT_ID,
                                                                        new IntPtr(&recipientId),
                                                                        SafeCertContextHandle.InvalidHandle);
                break;

            default: // Others not supported.
                hr = CAPI.E_NOTIMPL;
                break;
            }

            // The store handle isn't needed now that CertFindCertificateInStore is done.
            safeCertStoreHandle.Dispose();

            // Acquire CSP if the recipient's cert is found.
            if (safeCertContextHandle != null && !safeCertContextHandle.IsInvalid) {
                SafeCryptProvHandle safeCryptProvHandle;
                uint keySpec;

                hr = PkcsUtils.GetCertPrivateKey(safeCertContextHandle, out safeCryptProvHandle, out keySpec);

                if (safeCryptProvHandle != null && !safeCryptProvHandle.IsInvalid) {
                    cmsgDecryptParam.safeCryptProvHandle = safeCryptProvHandle;
                }
                else {
                    cmsgDecryptParam.safeCryptProvHandle = null;
                }

                cmsgDecryptParam.safeCertContextHandle = safeCertContextHandle;
                cmsgDecryptParam.keySpec = keySpec;
            }

            return hr;
        }

        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)]
        [SecurityCritical]
        private static void SetCspParams (AlgorithmIdentifier contentEncryptionAlgorithm, ref CMSG_ENCRYPT_PARAM encryptParam) {

            encryptParam.safeCryptProvHandle = SafeCryptProvHandle.InvalidHandle;
            encryptParam.pvEncryptionAuxInfo = SafeLocalAllocHandle.InvalidHandle;

            // Try with CRYPT_VERIFYCONTEXT
            SafeCryptProvHandle hCryptProv = SafeCryptProvHandle.InvalidHandle;
            if (!CAPI.CryptAcquireContext(ref hCryptProv, IntPtr.Zero, IntPtr.Zero, CAPI.PROV_RSA_FULL, CAPI.CRYPT_VERIFYCONTEXT)) {
                throw new CryptographicException(Marshal.GetLastWin32Error());
            }

            uint algId = X509Utils.OidToAlgId(contentEncryptionAlgorithm.Oid.Value);
            if (algId == CAPI.CALG_RC2 || algId == CAPI.CALG_RC4) {
                CAPI.CMSG_RC2_AUX_INFO auxInfo = new CAPI.CMSG_RC2_AUX_INFO(Marshal.SizeOf(typeof(CAPI.CMSG_RC2_AUX_INFO)));
                uint keyLength = (uint) contentEncryptionAlgorithm.KeyLength;
                if (keyLength == 0) {
                    keyLength = (uint) PkcsUtils.GetMaxKeyLength(hCryptProv, algId);
                }
                auxInfo.dwBitLen = keyLength;
                SafeLocalAllocHandle pvAuxInfo = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(Marshal.SizeOf(typeof(CAPI.CMSG_RC2_AUX_INFO))));
                Marshal.StructureToPtr(auxInfo, pvAuxInfo.DangerousGetHandle(), false);
                encryptParam.pvEncryptionAuxInfo = pvAuxInfo;
            }

            encryptParam.safeCryptProvHandle = hCryptProv;
        }

        [SecurityCritical]
        private static unsafe void SetCmsRecipientParams(CmsRecipientCollection           recipients, 
                                                         X509Certificate2Collection      certificates, 
                                                         CryptographicAttributeObjectCollection unprotectedAttributes,
                                                         AlgorithmIdentifier              contentEncryptionAlgorithm,
                                                         ref CMSG_ENCRYPT_PARAM           encryptParam) {
            checked {
                recipients = recipients.DeepCopy();
                certificates = new X509Certificate2Collection(certificates);

                int index = 0;
                uint[] recipientInfoTypes = new uint[recipients.Count];
                int cKeyAgree = 0;
                int reiSize = recipients.Count * Marshal.SizeOf(typeof(CAPI.CMSG_RECIPIENT_ENCODE_INFO));
                int totalSize = reiSize;

                for (index = 0; index < recipients.Count; index++) {
                    recipientInfoTypes[index] = (uint) PkcsUtils.GetRecipientInfoType(recipients[index].Certificate);

                    if (recipientInfoTypes[index] == CAPI.CMSG_KEY_TRANS_RECIPIENT) {
                        totalSize += Marshal.SizeOf(typeof(CAPI.CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO));
                    }
                    else if (recipientInfoTypes[index] == CAPI.CMSG_KEY_AGREE_RECIPIENT) {
                        cKeyAgree++;
                        totalSize += Marshal.SizeOf(typeof(CAPI.CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO));
                    }
                    else {
                        throw new CryptographicException(CAPI.CRYPT_E_UNKNOWN_ALGO);
                    }
                }

                encryptParam.rgpRecipients = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(totalSize));
                encryptParam.rgCertEncoded = SafeLocalAllocHandle.InvalidHandle;
                encryptParam.rgUnprotectedAttr = SafeLocalAllocHandle.InvalidHandle;
                encryptParam.rgSubjectKeyIdentifier = new SafeLocalAllocHandle[recipients.Count];
                encryptParam.rgszObjId = new SafeLocalAllocHandle[recipients.Count];

                if (cKeyAgree > 0) {
                    encryptParam.rgszKeyWrapObjId = new SafeLocalAllocHandle[cKeyAgree];
                    encryptParam.rgKeyWrapAuxInfo = new SafeLocalAllocHandle[cKeyAgree];
                    encryptParam.rgEphemeralIdentifier = new SafeLocalAllocHandle[cKeyAgree];
                    encryptParam.rgszEphemeralObjId = new SafeLocalAllocHandle[cKeyAgree];
                    encryptParam.rgUserKeyingMaterial = new SafeLocalAllocHandle[cKeyAgree];
                    encryptParam.prgpEncryptedKey = new SafeLocalAllocHandle[cKeyAgree];
                    encryptParam.rgpEncryptedKey = new SafeLocalAllocHandle[cKeyAgree];
                }

                // Create encode certs array.
                if (certificates.Count > 0) {
                    encryptParam.rgCertEncoded = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(certificates.Count * Marshal.SizeOf(typeof(CAPI.CRYPTOAPI_BLOB))));
                    for (index = 0; index < certificates.Count; index++) {
                        CAPI.CERT_CONTEXT pCertContext = (CAPI.CERT_CONTEXT) Marshal.PtrToStructure(X509Utils.GetCertContext(certificates[index]).DangerousGetHandle(), typeof(CAPI.CERT_CONTEXT));
                        CAPI.CRYPTOAPI_BLOB * pBlob = (CAPI.CRYPTOAPI_BLOB *) new IntPtr((long) encryptParam.rgCertEncoded.DangerousGetHandle() + 
                                                                                            (index * Marshal.SizeOf(typeof(CAPI.CRYPTOAPI_BLOB))));
                        pBlob->cbData = pCertContext.cbCertEncoded;
                        pBlob->pbData = pCertContext.pbCertEncoded;
                    }
                }

                // Create unprotected attributes array.
                if (unprotectedAttributes.Count > 0) {
                    encryptParam.rgUnprotectedAttr = new SafeLocalAllocHandle(PkcsUtils.CreateCryptAttributes(unprotectedAttributes));
                }

                // pKeyInfo = CMSG_ENVELOPED_ENCODE_INFO.rgCmsRecipients
                cKeyAgree = 0;
                IntPtr pKeyInfo = new IntPtr((long) encryptParam.rgpRecipients.DangerousGetHandle() + reiSize);
                for (index = 0; index < recipients.Count; index++) {
                    CmsRecipient recipient = recipients[index];
                    X509Certificate2 certificate = recipient.Certificate;
                    CAPI.CERT_CONTEXT pCertContext = (CAPI.CERT_CONTEXT) Marshal.PtrToStructure(X509Utils.GetCertContext(certificate).DangerousGetHandle(), typeof(CAPI.CERT_CONTEXT));
                    CAPI.CERT_INFO certInfo = (CAPI.CERT_INFO) Marshal.PtrToStructure(pCertContext.pCertInfo, typeof(CAPI.CERT_INFO));

                    CAPI.CMSG_RECIPIENT_ENCODE_INFO * pEncodeInfo = (CAPI.CMSG_RECIPIENT_ENCODE_INFO *) new IntPtr((long) encryptParam.rgpRecipients.DangerousGetHandle() + 
                                                                                                                    (index * Marshal.SizeOf(typeof(CAPI.CMSG_RECIPIENT_ENCODE_INFO))));
                    // CMSG_RECIPIENT_ENCODE_INFO.dwRecipientChoice
                    pEncodeInfo->dwRecipientChoice = (uint) recipientInfoTypes[index];

                    // CMSG_RECIPIENT_ENCODE_INFO.pRecipientInfo (pKeyTrans or pKeyAgree)
                    pEncodeInfo->pRecipientInfo = pKeyInfo;

                    if (recipientInfoTypes[index] == CAPI.CMSG_KEY_TRANS_RECIPIENT) {
                        // Fill in CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO.

                        // cbSize
                        IntPtr pcbSize = new IntPtr((long) pKeyInfo + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO), "cbSize"));
                        Marshal.WriteInt32(pcbSize, Marshal.SizeOf(typeof(CAPI.CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO)));

                        // KeyEncryptionAlgorithm
                        IntPtr pKeyEncryptionAlgorithm = new IntPtr((long) pKeyInfo + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO), "KeyEncryptionAlgorithm"));

                        byte[] objId = Encoding.ASCII.GetBytes(certInfo.SubjectPublicKeyInfo.Algorithm.pszObjId);
                        encryptParam.rgszObjId[index] = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(objId.Length + 1));
                        Marshal.Copy(objId, 0, encryptParam.rgszObjId[index].DangerousGetHandle(), objId.Length);

                        // KeyEncryptionAlgorithm.pszObjId
                        IntPtr pszObjId = new IntPtr((long) pKeyEncryptionAlgorithm + (long) Marshal.OffsetOf(typeof(CAPI.CRYPT_ALGORITHM_IDENTIFIER), "pszObjId"));
                        Marshal.WriteIntPtr(pszObjId, encryptParam.rgszObjId[index].DangerousGetHandle());

                        // KeyEncryptionAlgorithm.Parameters
                        IntPtr pParameters = new IntPtr((long) pKeyEncryptionAlgorithm + (long) Marshal.OffsetOf(typeof(CAPI.CRYPT_ALGORITHM_IDENTIFIER), "Parameters"));

                        // KeyEncryptionAlgorithm.Parameters.cbData
                        IntPtr pcbData = new IntPtr((long) pParameters + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "cbData"));
                        Marshal.WriteInt32(pcbData, (int) certInfo.SubjectPublicKeyInfo.Algorithm.Parameters.cbData);

                        // KeyEncryptionAlgorithm.Parameters.pbData
                        IntPtr ppbData = new IntPtr((long) pParameters + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "pbData"));
                        Marshal.WriteIntPtr(ppbData, certInfo.SubjectPublicKeyInfo.Algorithm.Parameters.pbData);

                        // Skip pvKeyEncryptionAuxInfo
                        // Skip hCryptProv

                        // RecipientPublicKey
                        IntPtr pRecipientPublicKey = new IntPtr((long) pKeyInfo + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO), "RecipientPublicKey"));

                        // RecipientPublicKey.cbData
                        pcbData = new IntPtr((long) pRecipientPublicKey + (long) Marshal.OffsetOf(typeof(CAPI.CRYPT_BIT_BLOB), "cbData"));
                        Marshal.WriteInt32(pcbData, (int) certInfo.SubjectPublicKeyInfo.PublicKey.cbData);

                        // RecipientPublicKey.pbData
                        ppbData = new IntPtr((long) pRecipientPublicKey + (long) Marshal.OffsetOf(typeof(CAPI.CRYPT_BIT_BLOB), "pbData"));
                        Marshal.WriteIntPtr(ppbData, certInfo.SubjectPublicKeyInfo.PublicKey.pbData);

                        // RecipientPublicKey.cUnusedBits
                        IntPtr pcUnusedBIts = new IntPtr((long) pRecipientPublicKey + (long) Marshal.OffsetOf(typeof(CAPI.CRYPT_BIT_BLOB), "cUnusedBits"));
                        Marshal.WriteInt32(pcUnusedBIts, (int) certInfo.SubjectPublicKeyInfo.PublicKey.cUnusedBits);

                        // RecipientId
                        IntPtr pRecipientId = new IntPtr((long) pKeyInfo + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO), "RecipientId"));
                        if (recipient.RecipientIdentifierType == SubjectIdentifierType.SubjectKeyIdentifier) {
                            uint cbData = 0;
                            SafeLocalAllocHandle pbData = SafeLocalAllocHandle.InvalidHandle;
                            if (!CAPI.CAPISafe.CertGetCertificateContextProperty(X509Utils.GetCertContext(certificate),
                                                                                    CAPI.CERT_KEY_IDENTIFIER_PROP_ID,
                                                                                    pbData,
                                                                                    ref cbData))
                                throw new CryptographicException(Marshal.GetLastWin32Error());

                            pbData = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(cbData));

                            if (!CAPI.CAPISafe.CertGetCertificateContextProperty(X509Utils.GetCertContext(certificate),
                                                                                    CAPI.CERT_KEY_IDENTIFIER_PROP_ID,
                                                                                    pbData,
                                                                                    ref cbData))
                                throw new CryptographicException(Marshal.GetLastWin32Error());

                            encryptParam.rgSubjectKeyIdentifier[index] = pbData;

                            // RecipientId.dwIdChoice
                            IntPtr pdwIdChoice = new IntPtr((long) pRecipientId + (long) Marshal.OffsetOf(typeof(CAPI.CERT_ID), "dwIdChoice"));
                            Marshal.WriteInt32(pdwIdChoice, (int) CAPI.CERT_ID_KEY_IDENTIFIER);

                            // RecipientId.KeyId
                            IntPtr pKeyId = new IntPtr((long) pRecipientId + (long) Marshal.OffsetOf(typeof(CAPI.CERT_ID), "Value"));

                            // RecipientId.KeyId.cbData
                            pcbData = new IntPtr((long) pKeyId + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "cbData"));
                            Marshal.WriteInt32(pcbData, (int) cbData);

                            // RecipientId.KeyId.pbData
                            ppbData = new IntPtr((long) pKeyId + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "pbData"));
                            Marshal.WriteIntPtr(ppbData, pbData.DangerousGetHandle());
                        }
                        else {
                            // RecipientId.dwIdChoice
                            IntPtr pdwIdChoice = new IntPtr((long) pRecipientId + (long) Marshal.OffsetOf(typeof(CAPI.CERT_ID), "dwIdChoice"));
                            Marshal.WriteInt32(pdwIdChoice, (int) CAPI.CERT_ID_ISSUER_SERIAL_NUMBER);

                            // RecipientId.IssuerSerialNumber
                            IntPtr pIssuerSerialNumber = new IntPtr((long) pRecipientId + (long) Marshal.OffsetOf(typeof(CAPI.CERT_ID), "Value"));

                            // RecipientId.IssuerSerialNumber.Issuer
                            IntPtr pIssuer = new IntPtr((long) pIssuerSerialNumber + (long) Marshal.OffsetOf(typeof(CAPI.CERT_ISSUER_SERIAL_NUMBER), "Issuer"));

                            // RecipientId.IssuerSerialNumber.Issuer.cbData
                            pcbData = new IntPtr((long) pIssuer + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "cbData"));
                            Marshal.WriteInt32(pcbData, (int) certInfo.Issuer.cbData);

                            // RecipientId.IssuerSerialNumber.Issuer.pbData
                            ppbData = new IntPtr((long) pIssuer + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "pbData"));
                            Marshal.WriteIntPtr(ppbData, certInfo.Issuer.pbData);

                            // RecipientId.IssuerSerialNumber.SerialNumber
                            IntPtr pSerialNumber = new IntPtr((long) pIssuerSerialNumber + (long) Marshal.OffsetOf(typeof(CAPI.CERT_ISSUER_SERIAL_NUMBER), "SerialNumber"));

                            // RecipientId.IssuerSerialNumber.SerialNumber.cbData
                            pcbData = new IntPtr((long) pSerialNumber + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "cbData"));
                            Marshal.WriteInt32(pcbData, (int) certInfo.SerialNumber.cbData);

                            // RecipientId.IssuerSerialNumber.SerialNumber.pbData
                            ppbData = new IntPtr((long) pSerialNumber + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "pbData"));
                            Marshal.WriteIntPtr(ppbData, certInfo.SerialNumber.pbData);
                        }
                        pKeyInfo = new IntPtr((long) pKeyInfo + Marshal.SizeOf(typeof(CAPI.CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO)));
                    }
                    else if (recipientInfoTypes[index] == CAPI.CMSG_KEY_AGREE_RECIPIENT) {
                        // Fill in CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO.

                        // cbSize
                        IntPtr pcbSize = new IntPtr((long) pKeyInfo + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO), "cbSize"));
                        Marshal.WriteInt32(pcbSize, Marshal.SizeOf(typeof(CAPI.CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO)));

                        // KeyEncryptionAlgorithm
                        IntPtr pKeyEncryptionAlgorithm = new IntPtr((long) pKeyInfo + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO), "KeyEncryptionAlgorithm"));

                        byte[] objId = Encoding.ASCII.GetBytes(CAPI.szOID_RSA_SMIMEalgESDH);
                        encryptParam.rgszObjId[index] = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(objId.Length + 1));
                        Marshal.Copy(objId, 0, encryptParam.rgszObjId[index].DangerousGetHandle(), objId.Length);

                        // KeyEncryptionAlgorithm.pszObjId
                        IntPtr pszObjId = new IntPtr((long) pKeyEncryptionAlgorithm + (long) Marshal.OffsetOf(typeof(CAPI.CRYPT_ALGORITHM_IDENTIFIER), "pszObjId"));
                        Marshal.WriteIntPtr(pszObjId, encryptParam.rgszObjId[index].DangerousGetHandle());

                        // Skip KeyEncryptionAlgorithm.Parameters
                        // Skip pvKeyEncryptionAuxInfo

                        // KeyWrapAlgorithm
                        IntPtr pKeyWrapAlgorithm = new IntPtr((long) pKeyInfo + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO), "KeyWrapAlgorithm"));

                        uint algId = X509Utils.OidToAlgId(contentEncryptionAlgorithm.Oid.Value);
                        if (algId == CAPI.CALG_RC2) {
                            objId = Encoding.ASCII.GetBytes(CAPI.szOID_RSA_SMIMEalgCMSRC2wrap);
                        }
                        else {
                            objId = Encoding.ASCII.GetBytes(CAPI.szOID_RSA_SMIMEalgCMS3DESwrap);
                        }
                        encryptParam.rgszKeyWrapObjId[cKeyAgree] = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(objId.Length + 1));
                        Marshal.Copy(objId, 0, encryptParam.rgszKeyWrapObjId[cKeyAgree].DangerousGetHandle(), objId.Length);

                        // KeyWrapAlgorithm.pszObjId
                        pszObjId = new IntPtr((long) pKeyWrapAlgorithm + (long) Marshal.OffsetOf(typeof(CAPI.CRYPT_ALGORITHM_IDENTIFIER), "pszObjId"));
                        Marshal.WriteIntPtr(pszObjId, encryptParam.rgszKeyWrapObjId[cKeyAgree].DangerousGetHandle());

                        // Skip KeyWrapAlgorithm.Parameters

                        // Fill in pvKeyWrapAuxInfo for RC2.
                        if (algId == CAPI.CALG_RC2) {
                            IntPtr pKeyWrapAuxInfo = new IntPtr((long) pKeyInfo + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO), "pvKeyWrapAuxInfo"));
                            Marshal.WriteIntPtr(pKeyWrapAuxInfo, encryptParam.pvEncryptionAuxInfo.DangerousGetHandle());
                        }

                        // Skip hCryptProv
                        // Skip dwKeySpec

                        // dwKeyChoice
                        IntPtr pdwKeyChoice = new IntPtr((long) pKeyInfo + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO), "dwKeyChoice"));
                        Marshal.WriteInt32(pdwKeyChoice, (int) CAPI.CMSG_KEY_AGREE_EPHEMERAL_KEY_CHOICE);

                        // pEphemeralAlgorithm
                        IntPtr pEphemeralAlgorithm = new IntPtr((long) pKeyInfo + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO), "pEphemeralAlgorithmOrSenderId"));
                        encryptParam.rgEphemeralIdentifier[cKeyAgree] = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(Marshal.SizeOf(typeof(CAPI.CRYPT_ALGORITHM_IDENTIFIER))));
                        Marshal.WriteIntPtr(pEphemeralAlgorithm, encryptParam.rgEphemeralIdentifier[cKeyAgree].DangerousGetHandle());

                        // pEphemeralAlgorithm.pszObjId
                        objId = Encoding.ASCII.GetBytes(certInfo.SubjectPublicKeyInfo.Algorithm.pszObjId);
                        encryptParam.rgszEphemeralObjId[cKeyAgree] = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(objId.Length + 1));
                        Marshal.Copy(objId, 0, encryptParam.rgszEphemeralObjId[cKeyAgree].DangerousGetHandle(), objId.Length);

                        pszObjId = new IntPtr((long) encryptParam.rgEphemeralIdentifier[cKeyAgree].DangerousGetHandle() + (long) Marshal.OffsetOf(typeof(CAPI.CRYPT_ALGORITHM_IDENTIFIER), "pszObjId"));
                        Marshal.WriteIntPtr(pszObjId, encryptParam.rgszEphemeralObjId[cKeyAgree].DangerousGetHandle());

                        // pEphemeralAlgorithm.Parameters
                        IntPtr pParameters = new IntPtr((long) encryptParam.rgEphemeralIdentifier[cKeyAgree].DangerousGetHandle() + (long) Marshal.OffsetOf(typeof(CAPI.CRYPT_ALGORITHM_IDENTIFIER), "Parameters"));

                        // pEphemeralAlgorithm.Parameters.cbData
                        IntPtr pcbData = new IntPtr((long) pParameters + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "cbData"));
                        Marshal.WriteInt32(pcbData, (int) certInfo.SubjectPublicKeyInfo.Algorithm.Parameters.cbData);

                        // pEphemeralAlgorithm.Parameters.pbData
                        IntPtr ppbData = new IntPtr((long) pParameters + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "pbData"));
                        Marshal.WriteIntPtr(ppbData, certInfo.SubjectPublicKeyInfo.Algorithm.Parameters.pbData);

                        // Skip UserKeyingMaterial

                        // cRecipientEncryptedKeys
                        IntPtr pcRecipientEncryptedKeys = new IntPtr((long) pKeyInfo + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO), "cRecipientEncryptedKeys"));
                        Marshal.WriteInt32(pcRecipientEncryptedKeys, 1);

                        // rgpRecipientEncryptedKeys
                        encryptParam.prgpEncryptedKey[cKeyAgree] = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(Marshal.SizeOf(typeof(IntPtr))));
                        IntPtr prgpRecipientEncryptedKeys = new IntPtr((long) pKeyInfo + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO), "rgpRecipientEncryptedKeys"));
                        Marshal.WriteIntPtr(prgpRecipientEncryptedKeys, encryptParam.prgpEncryptedKey[cKeyAgree].DangerousGetHandle());
                        encryptParam.rgpEncryptedKey[cKeyAgree] = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(Marshal.SizeOf(typeof(CAPI.CMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO))));
                        Marshal.WriteIntPtr(encryptParam.prgpEncryptedKey[cKeyAgree].DangerousGetHandle(), encryptParam.rgpEncryptedKey[cKeyAgree].DangerousGetHandle());

                        // rgpRecipientEncryptedKeys.cbSize
                        pcbSize = new IntPtr((long) encryptParam.rgpEncryptedKey[cKeyAgree].DangerousGetHandle() + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO), "cbSize"));
                        Marshal.WriteInt32(pcbSize, Marshal.SizeOf(typeof(CAPI.CMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO)));

                        // rgpRecipientEncryptedKeys.RecipientPublicKey
                        IntPtr pRecipientPublicKey = new IntPtr((long) encryptParam.rgpEncryptedKey[cKeyAgree].DangerousGetHandle() + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO), "RecipientPublicKey"));

                        // rgpRecipientEncryptedKeys.RecipientPublicKey.cbData
                        pcbData = new IntPtr((long) pRecipientPublicKey + (long) Marshal.OffsetOf(typeof(CAPI.CRYPT_BIT_BLOB), "cbData"));
                        Marshal.WriteInt32(pcbData, (int) certInfo.SubjectPublicKeyInfo.PublicKey.cbData);

                        // rgpRecipientEncryptedKeys.RecipientPublicKey.pbData
                        ppbData = new IntPtr((long) pRecipientPublicKey + (long) Marshal.OffsetOf(typeof(CAPI.CRYPT_BIT_BLOB), "pbData"));
                        Marshal.WriteIntPtr(ppbData, certInfo.SubjectPublicKeyInfo.PublicKey.pbData);

                        // rgpRecipientEncryptedKeys.RecipientPublicKey.cUnusedBits
                        IntPtr pcUnusedBits = new IntPtr((long) pRecipientPublicKey + (long) Marshal.OffsetOf(typeof(CAPI.CRYPT_BIT_BLOB), "cUnusedBits"));
                        Marshal.WriteInt32(pcUnusedBits, (int) certInfo.SubjectPublicKeyInfo.PublicKey.cUnusedBits);                    

                        // rgpRecipientEncryptedKeys.RecipientId
                        IntPtr pRecipientId = new IntPtr((long) encryptParam.rgpEncryptedKey[cKeyAgree].DangerousGetHandle() + (long) Marshal.OffsetOf(typeof(CAPI.CMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO), "RecipientId"));

                        // rgpRecipientEncryptedKeys.RecipientId.dwIdChoice
                        IntPtr pdwIdChoice = new IntPtr((long) pRecipientId + (long) Marshal.OffsetOf(typeof(CAPI.CERT_ID), "dwIdChoice"));

                        if (recipient.RecipientIdentifierType == SubjectIdentifierType.SubjectKeyIdentifier) {
                            Marshal.WriteInt32(pdwIdChoice, (int) CAPI.CERT_ID_KEY_IDENTIFIER);

                            // rgpRecipientEncryptedKeys.RecipientId.KeyId
                            IntPtr pKeyId = new IntPtr((long) pRecipientId + (long) Marshal.OffsetOf(typeof(CAPI.CERT_ID), "Value"));

                            uint cbKeyId = 0;
                            SafeLocalAllocHandle pbKeyId = SafeLocalAllocHandle.InvalidHandle;
                            if (!CAPI.CAPISafe.CertGetCertificateContextProperty(X509Utils.GetCertContext(certificate),
                                                                                    CAPI.CERT_KEY_IDENTIFIER_PROP_ID,
                                                                                    pbKeyId,
                                                                                    ref cbKeyId))
                                throw new CryptographicException(Marshal.GetLastWin32Error());

                            pbKeyId = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(cbKeyId));
                            if (!CAPI.CAPISafe.CertGetCertificateContextProperty(X509Utils.GetCertContext(certificate),
                                                                                    CAPI.CERT_KEY_IDENTIFIER_PROP_ID,
                                                                                    pbKeyId,
                                                                                    ref cbKeyId))
                                throw new CryptographicException(Marshal.GetLastWin32Error());

                            encryptParam.rgSubjectKeyIdentifier[cKeyAgree] = pbKeyId;

                            // rgpRecipientEncryptedKeys.RecipientId.KeyId.cbData
                            pcbData = new IntPtr((long) pKeyId + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "cbData"));
                            Marshal.WriteInt32(pcbData, (int) cbKeyId);

                            // rgpRecipientEncryptedKeys.RecipientId.KeyId.pbData
                            ppbData = new IntPtr((long) pKeyId + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "pbData"));
                            Marshal.WriteIntPtr(ppbData, pbKeyId.DangerousGetHandle());
                        }
                        else {
                            Marshal.WriteInt32(pdwIdChoice, (int) CAPI.CERT_ID_ISSUER_SERIAL_NUMBER);

                            // rgpRecipientEncryptedKeys.RecipientId.IssuerSerialNumber
                            IntPtr pIssuerSerial = new IntPtr((long) pRecipientId + (long) Marshal.OffsetOf(typeof(CAPI.CERT_ID), "Value"));

                            // rgpRecipientEncryptedKeys.RecipientId.IssuerSerialNumber.Issuer
                            IntPtr pIssuer = new IntPtr((long) pIssuerSerial + (long) Marshal.OffsetOf(typeof(CAPI.CERT_ISSUER_SERIAL_NUMBER), "Issuer"));

                            // rgpRecipientEncryptedKeys.RecipientId.IssuerSerialNumber.Issuer.cbData
                            pcbData = new IntPtr((long) pIssuer + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "cbData"));
                            Marshal.WriteInt32(pcbData, (int) certInfo.Issuer.cbData);

                            // rgpRecipientEncryptedKeys.RecipientId.IssuerSerialNumber.Issuer.pbData
                            ppbData = new IntPtr((long) pIssuer + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "pbData"));
                            Marshal.WriteIntPtr(ppbData, certInfo.Issuer.pbData);

                            // rgpRecipientEncryptedKeys.RecipientId.IssuerSerialNumber.SerialNumber
                            IntPtr pSerialNumber = new IntPtr((long) pIssuerSerial + (long) Marshal.OffsetOf(typeof(CAPI.CERT_ISSUER_SERIAL_NUMBER), "SerialNumber"));

                            // rgpRecipientEncryptedKeys.RecipientId.IssuerSerialNumber.SerialNumber.cbData
                            pcbData = new IntPtr((long) pSerialNumber + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "cbData"));
                            Marshal.WriteInt32(pcbData, (int) certInfo.SerialNumber.cbData);

                            // rgpRecipientEncryptedKeys.RecipientId.IssuerSerialNumber.SerialNumber.pbData
                            ppbData = new IntPtr((long) pSerialNumber + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "pbData"));
                            Marshal.WriteIntPtr(ppbData, certInfo.SerialNumber.pbData);
                        }

                        // Bump key agree count.
                        cKeyAgree++;
                        pKeyInfo = new IntPtr((long) pKeyInfo + Marshal.SizeOf(typeof(CAPI.CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO)));
                    }
                    else {
                        // Should never get here!
                        Debug.Assert(false);
                    }
                }
            }
        }

        /// <summary>
        /// Creates native buffer of CERT_INFOs for recipients
        /// </summary>
        /// <param name="recipients">Collection of recipients for the message</param>
        /// <param name="encryptParam">Encrypt param that will hold the native buffer</param>
        /// <param name="certContexts">List of SafeCertContextHandles that control the lifetime of the CERT_INFOs.
        /// This must be kept alive until encryptParam is no longer needed</param>
        [SecurityCritical]
        private static unsafe void SetPkcs7RecipientParams (CmsRecipientCollection recipients,
                                                            ref CMSG_ENCRYPT_PARAM encryptParam,
                                                            out List<SafeCertContextHandle> certContexts) {
            int index = 0;

            // Must read recipients.Count only once to avoid a security issue where another thread
            // could add objects to recipients after we allocate the native buffer, but while we're
            // iterating. That could lead to a buffer overflow
            int numRecipients = recipients.Count;

            certContexts = new List<SafeCertContextHandle>();

            checked {
                uint totalSize = (uint)numRecipients * (uint)Marshal.SizeOf(typeof(IntPtr));
                encryptParam.rgpRecipients = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(totalSize));

                IntPtr pCertInfo = encryptParam.rgpRecipients.DangerousGetHandle();

                for (index = 0; index < numRecipients; index++) {
                    SafeCertContextHandle certContext = X509Utils.GetCertContext(recipients[index].Certificate);
                    certContexts.Add(certContext);
                    IntPtr dangerousCertContextHandle = certContext.DangerousGetHandle();
                    CAPI.CERT_CONTEXT pCertContext = (CAPI.CERT_CONTEXT) Marshal.PtrToStructure(dangerousCertContextHandle, typeof(CAPI.CERT_CONTEXT));

                    Marshal.WriteIntPtr(pCertInfo, pCertContext.pCertInfo);
                    pCertInfo = new IntPtr((long) pCertInfo + Marshal.SizeOf(typeof(IntPtr)));
                }
                Debug.Assert(index == totalSize / (uint)Marshal.SizeOf(typeof(IntPtr)));
            }
        }

        [SecurityCritical]
        private static SafeCertStoreHandle BuildDecryptorStore (X509Certificate2Collection extraStore) {
            // Build store where to find recipient's certificate.
            X509Certificate2Collection recipientStore = new X509Certificate2Collection();

            // Include CU and LM MY stores.
            try {
                X509Store cuMy = new X509Store("MY", StoreLocation.CurrentUser);
                cuMy.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly | OpenFlags.IncludeArchived);
                recipientStore.AddRange(cuMy.Certificates);
                cuMy.Close();
            }
            catch (SecurityException) {
                // X509Store.Open() may not have permission. Ignore.
            }
            try {
                X509Store lmMy = new X509Store("MY", StoreLocation.LocalMachine);
                lmMy.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly | OpenFlags.IncludeArchived);
                recipientStore.AddRange(lmMy.Certificates);
                lmMy.Close();
            }
            catch (SecurityException) {
                // Ignore. May be in extra store.
            }

            if (recipientStore.Count == 0 && extraStore.Count == 0)
                throw new CryptographicException(CAPI.CRYPT_E_RECIPIENT_NOT_FOUND);

            // Return memory store handle, including extraStore.
            try {
                return X509Utils.ExportToMemoryStore(recipientStore, extraStore);
            } finally {
                foreach (X509Certificate2 cert in recipientStore) {
                    cert.Reset();
                }
            }
        }

        [SecurityCritical]
        private static SafeCertStoreHandle BuildOriginatorStore (X509Certificate2Collection bagOfCerts, X509Certificate2Collection extraStore) {
            // Build store where to find originator's certificate.
            X509Certificate2Collection originatorStore = new X509Certificate2Collection();

            // Include CU and LM MY stores.
            try {
                X509Store cuMy = new X509Store("AddressBook", StoreLocation.CurrentUser);
                cuMy.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly | OpenFlags.IncludeArchived);
                originatorStore.AddRange(cuMy.Certificates);
                cuMy.Close();
            }
            catch (SecurityException) {
                // X509Store.Open() may not have permission. Ignore.
            }
            try {
                X509Store lmMy = new X509Store("AddressBook", StoreLocation.LocalMachine);
                lmMy.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly | OpenFlags.IncludeArchived);
                originatorStore.AddRange(lmMy.Certificates);
                lmMy.Close();
            }
            catch (SecurityException) {
                // Ignore. May be in bag of certs or extra store.
            }

            X509Certificate2Collection includedButNotDisposed;

            // Finally, include bag of certs and extra store, if specified.
            if (bagOfCerts != null && extraStore != null) {
                includedButNotDisposed = new X509Certificate2Collection();
                includedButNotDisposed.AddRange(bagOfCerts);
                includedButNotDisposed.AddRange(extraStore);
            } else if (bagOfCerts != null) {
                includedButNotDisposed = bagOfCerts;
            } else if (extraStore != null) {
                includedButNotDisposed = extraStore;
            } else {
                includedButNotDisposed = null;
            }

            if (originatorStore.Count == 0 && includedButNotDisposed.Count == 0)
                throw new CryptographicException(CAPI.CRYPT_E_NOT_FOUND);

            // Return memory store handle.
            try {
                return X509Utils.ExportToMemoryStore(originatorStore, includedButNotDisposed);
            } finally {
                foreach (X509Certificate2 cert in originatorStore) {
                    cert.Reset();
                }
            }
        }
    }
}
