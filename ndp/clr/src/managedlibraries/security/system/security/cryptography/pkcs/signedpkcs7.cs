// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

//
// SignedPkcs7.cs
// 

namespace System.Security.Cryptography.Pkcs {
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Cryptography.Xml;
    using System.Security.Permissions;
    using System.Text;
    using System.Threading;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class SignedCms {
        [SecurityCritical]
        private SafeCryptMsgHandle      m_safeCryptMsgHandle;
        private int                     m_version;
        private SubjectIdentifierType   m_signerIdentifierType;
        private ContentInfo             m_contentInfo;
        private bool                    m_detached;

        //
        // Constructors.
        //

        public SignedCms () :
            this(SubjectIdentifierType.IssuerAndSerialNumber,
                 new ContentInfo(Oid.FromOidValue(CAPI.szOID_RSA_data, OidGroup.ExtensionOrAttribute), new byte[0]),
                 false) {
        }

        public SignedCms (SubjectIdentifierType signerIdentifierType) :
            this(signerIdentifierType,
                 new ContentInfo(Oid.FromOidValue(CAPI.szOID_RSA_data, OidGroup.ExtensionOrAttribute), new byte[0]),
                 false) {
        }

        public SignedCms (ContentInfo contentInfo) : this(SubjectIdentifierType.IssuerAndSerialNumber, contentInfo, false) {}

        public SignedCms (SubjectIdentifierType signerIdentifierType, ContentInfo contentInfo) : this(signerIdentifierType, contentInfo, false) {}

        public SignedCms (ContentInfo contentInfo, bool detached) : this(SubjectIdentifierType.IssuerAndSerialNumber, contentInfo, detached) {}

        [SecuritySafeCritical]
        public SignedCms (SubjectIdentifierType signerIdentifierType, ContentInfo contentInfo, bool detached) {
            if (contentInfo == null)
                throw new ArgumentNullException("contentInfo");

            if (contentInfo.Content == null)
                throw new ArgumentNullException("contentInfo.Content");

            // Reset all states.
            if (signerIdentifierType != SubjectIdentifierType.SubjectKeyIdentifier && 
                signerIdentifierType != SubjectIdentifierType.IssuerAndSerialNumber &&
                signerIdentifierType != SubjectIdentifierType.NoSignature) {
                signerIdentifierType = SubjectIdentifierType.IssuerAndSerialNumber;
            }

            m_safeCryptMsgHandle = SafeCryptMsgHandle.InvalidHandle;
            m_signerIdentifierType =  signerIdentifierType;
            m_version = 0;
            m_contentInfo = contentInfo;
            m_detached = detached;
        }

        //
        // Public APIs.
        //

        public int Version {
            [SecuritySafeCritical]
            get {
                // SignedData version can change based on user's operation, so
                // return the value passed in to the constructor if no message handle is
                // available. Otherwise, query the version from the handle
                if (m_safeCryptMsgHandle == null || m_safeCryptMsgHandle.IsInvalid)
                    return m_version;

                return (int) PkcsUtils.GetVersion(m_safeCryptMsgHandle);
            }
        }

        public ContentInfo ContentInfo {
            get {
                return m_contentInfo;
            }
        }

        public bool Detached {
            get {
                return m_detached;
            }
        }

        public X509Certificate2Collection Certificates {
            [SecuritySafeCritical]
            get {
                if (m_safeCryptMsgHandle == null || m_safeCryptMsgHandle.IsInvalid) {
                    return new X509Certificate2Collection();
                }
                return PkcsUtils.GetCertificates(m_safeCryptMsgHandle);
            }
        }

        public SignerInfoCollection SignerInfos {
            [SecuritySafeCritical]
            get {
                if (m_safeCryptMsgHandle == null || m_safeCryptMsgHandle.IsInvalid) {
                    return new SignerInfoCollection();
                }

                return new SignerInfoCollection(this);
            }
        }

        [SecuritySafeCritical]
        public byte[] Encode () {
            if (m_safeCryptMsgHandle == null || m_safeCryptMsgHandle.IsInvalid)
                throw new InvalidOperationException(SecurityResources.GetResourceString("Cryptography_Cms_MessageNotSigned"));

            return PkcsUtils.GetMessage(m_safeCryptMsgHandle);
        }

        [SecuritySafeCritical]
        public void Decode (byte[] encodedMessage) {
            if (encodedMessage == null)
                throw new ArgumentNullException("encodedMessage");

            if (m_safeCryptMsgHandle != null && !m_safeCryptMsgHandle.IsInvalid) {
                m_safeCryptMsgHandle.Dispose();
            }

            m_safeCryptMsgHandle = OpenToDecode(encodedMessage, this.ContentInfo, this.Detached);
            if (!this.Detached) {
                Oid contentType = PkcsUtils.GetContentType(m_safeCryptMsgHandle);
                byte[] content = PkcsUtils.GetContent(m_safeCryptMsgHandle);
                m_contentInfo = new ContentInfo(contentType, content); 
            }
        }

        public void ComputeSignature () {
            ComputeSignature(new CmsSigner(m_signerIdentifierType), true);
        }

        public void ComputeSignature (CmsSigner signer) {
            ComputeSignature(signer, true);
        }

        [SecuritySafeCritical]
        private static int SafeGetLastWin32Error()
        {
            return Marshal.GetLastWin32Error();
        }

        [SecuritySafeCritical]
        public void ComputeSignature (CmsSigner signer, bool silent) {
            if (signer == null)
                throw new ArgumentNullException("signer");
            if (ContentInfo.Content.Length == 0)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Sign_Empty_Content"));

            if (SubjectIdentifierType.NoSignature == signer.SignerIdentifierType) {
                if (m_safeCryptMsgHandle != null && !m_safeCryptMsgHandle.IsInvalid)
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Sign_No_Signature_First_Signer"));

                // First signer.
                Sign(signer, silent);
                return;
            }

            if (signer.Certificate == null) {
                if (silent)
                    throw new InvalidOperationException(SecurityResources.GetResourceString("Cryptography_Cms_RecipientCertificateNotFound"));
                else
                    signer.Certificate = PkcsUtils.SelectSignerCertificate();
            }

            if (!signer.Certificate.HasPrivateKey)
                throw new CryptographicException(CAPI.NTE_NO_KEY);

            // 



            CspParameters parameters = new CspParameters();
            if (X509Utils.GetPrivateKeyInfo(X509Utils.GetCertContext(signer.Certificate), ref parameters))
            {

                KeyContainerPermission kp = new KeyContainerPermission(KeyContainerPermissionFlags.NoFlags);
                KeyContainerPermissionAccessEntry entry = new KeyContainerPermissionAccessEntry(parameters, KeyContainerPermissionFlags.Open | KeyContainerPermissionFlags.Sign);
                kp.AccessEntries.Add(entry);
                kp.Demand();
            }

            if (m_safeCryptMsgHandle == null || m_safeCryptMsgHandle.IsInvalid) {
                // First signer.
                Sign(signer, silent);
            }
            else {
                // Co-signing.
                CoSign(signer, silent);
            }
        }

        [SecuritySafeCritical]
        public void RemoveSignature (int index) {
            if (m_safeCryptMsgHandle == null || m_safeCryptMsgHandle.IsInvalid)
                throw new InvalidOperationException(SecurityResources.GetResourceString("Cryptography_Cms_MessageNotSigned"));

            unsafe {
                uint dwSigners = 0;
                uint cbCount = (uint) Marshal.SizeOf(typeof(uint));

                if (!CAPI.CAPISafe.CryptMsgGetParam(m_safeCryptMsgHandle,
                                                    CAPI.CMSG_SIGNER_COUNT_PARAM,
                                                    0,
                                                    new IntPtr(&dwSigners),
                                                    new IntPtr(&cbCount)))
                    throw new CryptographicException(Marshal.GetLastWin32Error());

                if (index < 0 || index >= (int) dwSigners)
                    throw new ArgumentOutOfRangeException("index", SecurityResources.GetResourceString("ArgumentOutOfRange_Index"));

                if (!CAPI.CryptMsgControl(m_safeCryptMsgHandle,
                                          0,
                                          CAPI.CMSG_CTRL_DEL_SIGNER,
                                          new IntPtr(&index))) 
                    throw new CryptographicException(Marshal.GetLastWin32Error());
            }
        }

        [SecuritySafeCritical]
        public void RemoveSignature (SignerInfo signerInfo) {
            if (signerInfo == null)
                throw new ArgumentNullException("signerInfo");

            RemoveSignature(PkcsUtils.GetSignerIndex(m_safeCryptMsgHandle, signerInfo, 0));
        }

        public void CheckSignature (bool verifySignatureOnly) {
            CheckSignature(new X509Certificate2Collection(), verifySignatureOnly);
        }

        [SecuritySafeCritical]
        public void CheckSignature (X509Certificate2Collection extraStore, bool verifySignatureOnly) {
            if (m_safeCryptMsgHandle == null || m_safeCryptMsgHandle.IsInvalid)
                throw new InvalidOperationException(SecurityResources.GetResourceString("Cryptography_Cms_MessageNotSigned"));

            if (extraStore == null)
                throw new ArgumentNullException("extraStore");

            CheckSignatures(this.SignerInfos, extraStore, verifySignatureOnly);
        }

        [SecuritySafeCritical]
        public void CheckHash () {

            if (m_safeCryptMsgHandle == null || m_safeCryptMsgHandle.IsInvalid)
                throw new InvalidOperationException(
                    SecurityResources.GetResourceString("Cryptography_Cms_MessageNotSigned"));

            CheckHashes(this.SignerInfos);
        }

        //
        // Internal methods.
        //

        [SecurityCritical]
        internal SafeCryptMsgHandle GetCryptMsgHandle() {
            return m_safeCryptMsgHandle;
        }

        [SecuritySafeCritical]
        internal void ReopenToDecode () {
            byte[] encodedMessage = PkcsUtils.GetMessage(m_safeCryptMsgHandle);
            if (m_safeCryptMsgHandle != null && !m_safeCryptMsgHandle.IsInvalid) {
                m_safeCryptMsgHandle.Dispose();
            }
            m_safeCryptMsgHandle = OpenToDecode(encodedMessage, this.ContentInfo, this.Detached);
        }

        [SecuritySafeCritical]
        private unsafe void Sign (CmsSigner signer, bool silent) {

            SafeCryptMsgHandle safeCryptMsgHandle = null;
            CAPI.CMSG_SIGNED_ENCODE_INFO signedEncodeInfo = new CAPI.CMSG_SIGNED_ENCODE_INFO(Marshal.SizeOf(typeof(CAPI.CMSG_SIGNED_ENCODE_INFO)));
            SafeCryptProvHandle safeCryptProvHandle;
            CAPI.CMSG_SIGNER_ENCODE_INFO signerEncodeInfo = PkcsUtils.CreateSignerEncodeInfo(signer, silent, out safeCryptProvHandle);

            byte[] encodedMessage = null;
            try {
                SafeLocalAllocHandle pSignerEncodeInfo = CAPI.LocalAlloc(CAPI.LMEM_FIXED, new IntPtr(Marshal.SizeOf(typeof(CAPI.CMSG_SIGNER_ENCODE_INFO))));

                try {
                    Marshal.StructureToPtr(signerEncodeInfo, pSignerEncodeInfo.DangerousGetHandle(), false);
                    X509Certificate2Collection bagOfCerts = PkcsUtils.CreateBagOfCertificates(signer);
                    SafeLocalAllocHandle pEncodedBagOfCerts = PkcsUtils.CreateEncodedCertBlob(bagOfCerts);

                    signedEncodeInfo.cSigners = 1;
                    signedEncodeInfo.rgSigners = pSignerEncodeInfo.DangerousGetHandle();
                    signedEncodeInfo.cCertEncoded = (uint) bagOfCerts.Count;
                    if (bagOfCerts.Count > 0)
                        signedEncodeInfo.rgCertEncoded = pEncodedBagOfCerts.DangerousGetHandle();

                    // Because of the way CAPI treats inner content OID, we should pass NULL
                    // for data type, otherwise detached will not work.
                    if (String.Compare(this.ContentInfo.ContentType.Value, CAPI.szOID_RSA_data, StringComparison.OrdinalIgnoreCase) == 0) {
                        safeCryptMsgHandle = CAPI.CryptMsgOpenToEncode(CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                                       Detached ? CAPI.CMSG_DETACHED_FLAG : 0,
                                                                       CAPI.CMSG_SIGNED,
                                                                       new IntPtr(&signedEncodeInfo),
                                                                       IntPtr.Zero,
                                                                       IntPtr.Zero);
                    }
                    else {
                        safeCryptMsgHandle = CAPI.CryptMsgOpenToEncode(CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                                       Detached ? CAPI.CMSG_DETACHED_FLAG : 0,
                                                                       CAPI.CMSG_SIGNED,
                                                                       new IntPtr(&signedEncodeInfo),
                                                                       this.ContentInfo.ContentType.Value,
                                                                       IntPtr.Zero);
                    }

                    if (safeCryptMsgHandle == null || safeCryptMsgHandle.IsInvalid)
                        throw new CryptographicException(Marshal.GetLastWin32Error());


                    if (this.ContentInfo.Content.Length > 0) {
                        if (!CAPI.CAPISafe.CryptMsgUpdate(safeCryptMsgHandle, this.ContentInfo.pContent, (uint) this.ContentInfo.Content.Length, true))
                            throw new CryptographicException(Marshal.GetLastWin32Error());
                    }

                    // Retrieve encoded message.
                    encodedMessage = PkcsUtils.GetContent(safeCryptMsgHandle);
                    safeCryptMsgHandle.Dispose();

                    pEncodedBagOfCerts.Dispose();
                }
                finally {
                    Marshal.DestroyStructure(pSignerEncodeInfo.DangerousGetHandle(), typeof(CAPI.CMSG_SIGNER_ENCODE_INFO));
                    pSignerEncodeInfo.Dispose();
                }
            }
            finally {
                // Don't forget to free all the resource still held inside signerEncodeInfo.
                signerEncodeInfo.Dispose();
                safeCryptProvHandle.Dispose();
            }

            // Re-open to decode.
            safeCryptMsgHandle = OpenToDecode(encodedMessage, this.ContentInfo, this.Detached);
            if (m_safeCryptMsgHandle != null && !m_safeCryptMsgHandle.IsInvalid) {
                m_safeCryptMsgHandle.Dispose();
            }
            m_safeCryptMsgHandle = safeCryptMsgHandle;
            GC.KeepAlive(signer);
        }

        [SecuritySafeCritical]
        private void CoSign (CmsSigner signer, bool silent) {
            SafeCryptProvHandle safeCryptProvHandle;
            CAPI.CMSG_SIGNER_ENCODE_INFO signerEncodeInfo = PkcsUtils.CreateSignerEncodeInfo(signer, silent, out safeCryptProvHandle);

            try {
                SafeLocalAllocHandle pSignerEncodeInfo = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(Marshal.SizeOf(typeof(CAPI.CMSG_SIGNER_ENCODE_INFO))));

                try {
                    // Marshal to unmanaged memory.
                    Marshal.StructureToPtr(signerEncodeInfo, pSignerEncodeInfo.DangerousGetHandle(), false);

                    // Add the signature.
                    if (!CAPI.CryptMsgControl(m_safeCryptMsgHandle,
                                              0,
                                              CAPI.CMSG_CTRL_ADD_SIGNER,
                                              pSignerEncodeInfo.DangerousGetHandle())) 
                        throw new CryptographicException(Marshal.GetLastWin32Error());
                }
                finally {
                    Marshal.DestroyStructure(pSignerEncodeInfo.DangerousGetHandle(), typeof(CAPI.CMSG_SIGNER_ENCODE_INFO));
                    pSignerEncodeInfo.Dispose();
                }
            }
            finally {
                // and don't forget to dispose of resources allocated for the structure.
                signerEncodeInfo.Dispose();
                safeCryptProvHandle.Dispose();
            }

            // Finally, add certs to bag of certs.
            PkcsUtils.AddCertsToMessage(m_safeCryptMsgHandle, Certificates, PkcsUtils.CreateBagOfCertificates(signer));
        }

        //
        // Private static methods.
        //
        [SecuritySafeCritical]
        private static SafeCryptMsgHandle OpenToDecode (byte[] encodedMessage,
                                                               ContentInfo contentInfo,
                                                               bool detached) {
            // Open the message for decode.
            SafeCryptMsgHandle safeCryptMsgHandle = CAPI.CAPISafe.CryptMsgOpenToDecode(
                                                            CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                            detached ? CAPI.CMSG_DETACHED_FLAG : 0,
                                                            0,
                                                            IntPtr.Zero,
                                                            IntPtr.Zero,
                                                            IntPtr.Zero);
            if (safeCryptMsgHandle == null || safeCryptMsgHandle.IsInvalid)
                throw new CryptographicException(Marshal.GetLastWin32Error());

            // ---- the message.
            if (!CAPI.CAPISafe.CryptMsgUpdate(safeCryptMsgHandle, encodedMessage, (uint) encodedMessage.Length, true))
                throw new CryptographicException(Marshal.GetLastWin32Error());

            // Make sure this is PKCS7 SignedData type.
            if (CAPI.CMSG_SIGNED != PkcsUtils.GetMessageType(safeCryptMsgHandle))
                throw new CryptographicException(CAPI.CRYPT_E_INVALID_MSG_TYPE);

            // If detached, then update message with content if available.
            if (detached) {
                byte[] content  = contentInfo.Content;

                if (content != null && content.Length > 0) {
                    if (!CAPI.CAPISafe.CryptMsgUpdate(safeCryptMsgHandle, content, (uint) content.Length, true))
                        throw new CryptographicException(Marshal.GetLastWin32Error());
                }
            }

            return safeCryptMsgHandle;
        }

        private static void CheckSignatures (SignerInfoCollection signers, 
                                             X509Certificate2Collection extraStore, 
                                             bool verifySignatureOnly) {
            if (signers == null || signers.Count < 1)
                throw new CryptographicException(CAPI.CRYPT_E_NO_SIGNER);

            foreach (SignerInfo signer in signers) {
                signer.CheckSignature(extraStore, verifySignatureOnly);
                if (signer.CounterSignerInfos.Count > 0)
                    CheckSignatures(signer.CounterSignerInfos, extraStore, verifySignatureOnly);
            }
        }

        private static void CheckHashes (SignerInfoCollection signers) {
            if (signers == null || signers.Count < 1)
                throw new CryptographicException(CAPI.CRYPT_E_NO_SIGNER);

            foreach (SignerInfo signer in signers) {
                if (signer.SignerIdentifier.Type == SubjectIdentifierType.NoSignature)
                    signer.CheckHash();
            }
        }
    }
}
