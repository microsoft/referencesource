// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

//
// SignerInfo.cs
// 

namespace System.Security.Cryptography.Pkcs {
    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Runtime.Versioning;
    using System.Security.Permissions;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Cryptography.Xml;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class SignerInfo {
        private X509Certificate2                m_certificate;
        private SubjectIdentifier                m_signerIdentifier;
        private CryptographicAttributeObjectCollection m_signedAttributes;
        private CryptographicAttributeObjectCollection m_unsignedAttributes;

        private SignedCms                   m_signedCms;
        private SignerInfo                  m_parentSignerInfo;
        private byte[]                      m_encodedSignerInfo;
        [SecurityCritical]
        private SafeLocalAllocHandle        m_pbCmsgSignerInfo;
        private CAPI.CMSG_SIGNER_INFO       m_cmsgSignerInfo;

        //
        // Constructors.
        //

        private SignerInfo () {}

        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)]
        [SecurityCritical]
        internal SignerInfo (SignedCms signedCms, SafeLocalAllocHandle pbCmsgSignerInfo) {
            // Sanity check.
            Debug.Assert(signedCms != null && pbCmsgSignerInfo != null && !pbCmsgSignerInfo.IsInvalid);

            m_signedCms = signedCms;
            m_parentSignerInfo = null;
            m_encodedSignerInfo = null;
            m_pbCmsgSignerInfo = pbCmsgSignerInfo;
            m_cmsgSignerInfo = (CAPI.CMSG_SIGNER_INFO) Marshal.PtrToStructure(pbCmsgSignerInfo.DangerousGetHandle(), typeof(CAPI.CMSG_SIGNER_INFO));
        }

        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)]
        [SecuritySafeCritical]
        internal unsafe SignerInfo (SignedCms signedCms, SignerInfo parentSignerInfo, byte[] encodedSignerInfo) {
            // Sanity check.
            Debug.Assert(signedCms != null && encodedSignerInfo != null && encodedSignerInfo.Length > 0);

            uint cbCmsgSignerInfo = 0;
            SafeLocalAllocHandle pbCmsgSignerInfo = SafeLocalAllocHandle.InvalidHandle;

            fixed (byte * pEncodedSignerInfo = &encodedSignerInfo[0]) {
                if (!CAPI.DecodeObject(new IntPtr(CAPI.PKCS7_SIGNER_INFO),
                                       new IntPtr(pEncodedSignerInfo),
                                       (uint) encodedSignerInfo.Length,
                                       out pbCmsgSignerInfo,
                                       out cbCmsgSignerInfo))
                    throw new CryptographicException(Marshal.GetLastWin32Error());
            }

            m_signedCms = signedCms;
            m_parentSignerInfo = parentSignerInfo;
            m_encodedSignerInfo = (byte[]) encodedSignerInfo.Clone();
            m_pbCmsgSignerInfo = pbCmsgSignerInfo;
            m_cmsgSignerInfo = (CAPI.CMSG_SIGNER_INFO) Marshal.PtrToStructure(pbCmsgSignerInfo.DangerousGetHandle(), typeof(CAPI.CMSG_SIGNER_INFO));
        }

        //
        // Public APIs.
        //

        public int Version {
            get {
                return (int) m_cmsgSignerInfo.dwVersion;
            }
        }


        public X509Certificate2 Certificate {
            get {
                if (m_certificate == null)
                    m_certificate = PkcsUtils.FindCertificate(this.SignerIdentifier, m_signedCms.Certificates);

                return m_certificate;
            }
        }

        public SubjectIdentifier SignerIdentifier {
            [SecuritySafeCritical]
            get {
                if (m_signerIdentifier == null)
                    m_signerIdentifier = new SubjectIdentifier(m_cmsgSignerInfo);

                return m_signerIdentifier;
            }
        }

        public Oid DigestAlgorithm {
            get {
                return new Oid(m_cmsgSignerInfo.HashAlgorithm.pszObjId);
            }
        }

        public CryptographicAttributeObjectCollection SignedAttributes {
            [SecuritySafeCritical]
            get {
                if (m_signedAttributes == null)
                    m_signedAttributes = new CryptographicAttributeObjectCollection(m_cmsgSignerInfo.AuthAttrs);

                return m_signedAttributes;
            }
        }

        public CryptographicAttributeObjectCollection UnsignedAttributes {
            [SecuritySafeCritical]
            get {
                if (m_unsignedAttributes == null)
                    m_unsignedAttributes = new CryptographicAttributeObjectCollection(m_cmsgSignerInfo.UnauthAttrs);

                return m_unsignedAttributes;
            }
        }

        public SignerInfoCollection CounterSignerInfos {
            get {
                // We only support one level of counter signing.
                if (m_parentSignerInfo != null)
                    return new SignerInfoCollection();

                return new SignerInfoCollection(m_signedCms, this);
            }
        }

        public void ComputeCounterSignature () {
            ComputeCounterSignature(new CmsSigner(m_signedCms.Version == 2 ? SubjectIdentifierType.SubjectKeyIdentifier : SubjectIdentifierType.IssuerAndSerialNumber));
        }

        public void ComputeCounterSignature (CmsSigner signer) {
            // We only support one level of counter signing.
            if (m_parentSignerInfo != null)
                throw new CryptographicException(CAPI.E_NOTIMPL);
            if (signer == null)
                throw new ArgumentNullException("signer");

            if (signer.Certificate == null)
                signer.Certificate = PkcsUtils.SelectSignerCertificate();

            if (!signer.Certificate.HasPrivateKey)
                throw new CryptographicException(CAPI.NTE_NO_KEY);

            CounterSign(signer);
        }

        [SecuritySafeCritical]
        public void RemoveCounterSignature (int index) {
            // We only support one level of counter signing.
            if (m_parentSignerInfo != null)
                throw new CryptographicException(CAPI.E_NOTIMPL);

            RemoveCounterSignature(PkcsUtils.GetSignerIndex(m_signedCms.GetCryptMsgHandle(), this, 0), index);

            return;
        }

        [SecuritySafeCritical]
        public void RemoveCounterSignature (SignerInfo counterSignerInfo) {
            // We only support one level of counter signing.
            if (m_parentSignerInfo != null)
                throw new CryptographicException(CAPI.E_NOTIMPL);
            if (counterSignerInfo == null)
                throw new ArgumentNullException("counterSignerInfo");

            foreach (CryptographicAttributeObject attribute in UnsignedAttributes) {
                if (String.Compare(attribute.Oid.Value, CAPI.szOID_RSA_counterSign, StringComparison.OrdinalIgnoreCase) == 0) {
                    for (int index = 0; index < attribute.Values.Count; index++) {
                        AsnEncodedData encodedCounterSignature = (AsnEncodedData) attribute.Values[index];
                        SignerInfo counterSignerInfo2 = new SignerInfo(m_signedCms, m_parentSignerInfo, encodedCounterSignature.RawData);

                        if ((counterSignerInfo.SignerIdentifier.Type == SubjectIdentifierType.IssuerAndSerialNumber) &&
                            (counterSignerInfo2.SignerIdentifier.Type == SubjectIdentifierType.IssuerAndSerialNumber)) {
                            X509IssuerSerial issuerSerial1 = (X509IssuerSerial) counterSignerInfo.SignerIdentifier.Value;
                            X509IssuerSerial issuerSerial2 = (X509IssuerSerial) counterSignerInfo2.SignerIdentifier.Value;

                            if ((String.Compare(issuerSerial1.IssuerName, issuerSerial2.IssuerName, StringComparison.OrdinalIgnoreCase) == 0) &&
                                (String.Compare(issuerSerial1.SerialNumber, issuerSerial2.SerialNumber, StringComparison.OrdinalIgnoreCase) == 0)) {
                                RemoveCounterSignature(PkcsUtils.GetSignerIndex(m_signedCms.GetCryptMsgHandle(), this, 0), index);
                                return;
                            }
                        }
                        else if ((counterSignerInfo.SignerIdentifier.Type == SubjectIdentifierType.SubjectKeyIdentifier) &&
                                 (counterSignerInfo2.SignerIdentifier.Type == SubjectIdentifierType.SubjectKeyIdentifier)) {
                            string keyIdentifier1 = counterSignerInfo.SignerIdentifier.Value as string;
                            string keyIdentifier2 = counterSignerInfo2.SignerIdentifier.Value as string;

                            if (String.Compare(keyIdentifier1, keyIdentifier2, StringComparison.OrdinalIgnoreCase) == 0) {
                                RemoveCounterSignature(PkcsUtils.GetSignerIndex(m_signedCms.GetCryptMsgHandle(), this, 0), index);
                                return;
                            }
                        }
                    }
                }
            }

            throw new CryptographicException(CAPI.CRYPT_E_SIGNER_NOT_FOUND);
        }

        public void CheckSignature (bool verifySignatureOnly) {
            CheckSignature(new X509Certificate2Collection(), verifySignatureOnly);
        }

        public void CheckSignature (X509Certificate2Collection extraStore, bool verifySignatureOnly) {
            if (extraStore == null)
                throw new ArgumentNullException("extraStore");

            X509Certificate2 certificate = this.Certificate;
            if (certificate == null) {
                certificate = PkcsUtils.FindCertificate(SignerIdentifier, extraStore);
                if (certificate == null)
                    throw new CryptographicException(CAPI.CRYPT_E_SIGNER_NOT_FOUND);
            }

            Verify(extraStore, certificate, verifySignatureOnly);
        }

        [SecuritySafeCritical]
        public void CheckHash() {

            int cvseSize = Marshal.SizeOf(typeof(CAPI.CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA));
            CAPI.CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA cvse = new CAPI.CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA(cvseSize);
            cvse.dwSignerType  = CAPI.CMSG_VERIFY_SIGNER_NULL;
            cvse.dwSignerIndex = (uint) PkcsUtils.GetSignerIndex(m_signedCms.GetCryptMsgHandle(), this, 0);

            unsafe {
                if (!CAPI.CryptMsgControl(m_signedCms.GetCryptMsgHandle(),
                                            0,
                                            CAPI.CMSG_CTRL_VERIFY_SIGNATURE_EX,
                                            new IntPtr(&cvse)))
                    throw new CryptographicException(Marshal.GetLastWin32Error());
            }
        }

        [SecuritySafeCritical]
        public byte[] GetSignature()
        {
            byte[] ret = new byte[m_cmsgSignerInfo.EncryptedHash.cbData];
            Marshal.Copy(m_cmsgSignerInfo.EncryptedHash.pbData, ret, 0, ret.Length);
            return ret;
        }

        public Oid SignatureAlgorithm {
            get {
                return new Oid(m_cmsgSignerInfo.HashEncryptionAlgorithm.pszObjId);
            }
        }

        //
        // Internal methods.
        //

        internal CAPI.CMSG_SIGNER_INFO GetCmsgSignerInfo () {
            return m_cmsgSignerInfo;
        }

        //
        // Private methods.
        //

        [SecuritySafeCritical]
        private unsafe void CounterSign (CmsSigner signer) {
            // Sanity check.
            Debug.Assert(signer != null);

            // 


            CspParameters parameters = new CspParameters();
            if (X509Utils.GetPrivateKeyInfo(X509Utils.GetCertContext(signer.Certificate), ref parameters) == false)
                throw new CryptographicException(Marshal.GetLastWin32Error());

            KeyContainerPermission kp = new KeyContainerPermission(KeyContainerPermissionFlags.NoFlags);
            KeyContainerPermissionAccessEntry entry = new KeyContainerPermissionAccessEntry(parameters, KeyContainerPermissionFlags.Open | KeyContainerPermissionFlags.Sign);
            kp.AccessEntries.Add(entry);
            kp.Demand();

            // Get the signer's index.
            uint index = (uint) PkcsUtils.GetSignerIndex(m_signedCms.GetCryptMsgHandle(), this, 0);

            // Create CMSG_SIGNER_ENCODE_INFO structure.
            SafeLocalAllocHandle pSignerEncodeInfo = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(Marshal.SizeOf(typeof(CAPI.CMSG_SIGNER_ENCODE_INFO))));
            SafeCryptProvHandle safeCryptProvHandle;
            CAPI.CMSG_SIGNER_ENCODE_INFO signerEncodeInfo = PkcsUtils.CreateSignerEncodeInfo(signer, out safeCryptProvHandle);

            try {
                // Marshal to unmanaged memory.
                Marshal.StructureToPtr(signerEncodeInfo, pSignerEncodeInfo.DangerousGetHandle(), false);

                // Counter sign.
                if (!CAPI.CryptMsgCountersign(m_signedCms.GetCryptMsgHandle(),
                                              index,
                                              1,
                                              pSignerEncodeInfo.DangerousGetHandle()))
                    throw new CryptographicException(Marshal.GetLastWin32Error());

                // CAPI requires that the messge be re-encoded if any unauthenticated
                // attribute has been added. So, let's re-open it to decode to work
                // around this limitation.
                m_signedCms.ReopenToDecode();
            }
            finally {
                Marshal.DestroyStructure(pSignerEncodeInfo.DangerousGetHandle(), typeof(CAPI.CMSG_SIGNER_ENCODE_INFO));
                pSignerEncodeInfo.Dispose();

                // and don't forget to dispose of resources allocated for the structure.
                signerEncodeInfo.Dispose();
                safeCryptProvHandle.Dispose();
            }

            // Finally, add certs to bag of certs.
            PkcsUtils.AddCertsToMessage(m_signedCms.GetCryptMsgHandle(), m_signedCms.Certificates, PkcsUtils.CreateBagOfCertificates(signer));

            return;
        }

        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)]
        [SecuritySafeCritical]
        private unsafe void Verify (X509Certificate2Collection extraStore, X509Certificate2 certificate, bool verifySignatureOnly) {
            checked {
                // We need to find out if DSS parameters inheritance is necessary. If so, we need to 
                // first build the chain to cause CAPI to inherit and set the parameters in the 
                // CERT_PUBKEY_ALG_PARA_PROP_ID extended property. Once we have the parameters in
                // the property, we then need to retrieve a copy and point to it in the CERT_INFO
                // structure.
                SafeLocalAllocHandle pbParameters = SafeLocalAllocHandle.InvalidHandle;
                CAPI.CERT_CONTEXT pCertContext = (CAPI.CERT_CONTEXT) Marshal.PtrToStructure(X509Utils.GetCertContext(certificate).DangerousGetHandle(), typeof(CAPI.CERT_CONTEXT));

                // Point to SubjectPublicKeyInfo field inside the CERT_INFO structure.
                IntPtr pSubjectPublicKeyInfo = new IntPtr((long) pCertContext.pCertInfo + (long) Marshal.OffsetOf(typeof(CAPI.CERT_INFO), "SubjectPublicKeyInfo"));

                // Point to Algorithm field inside the SubjectPublicKeyInfo field.
                IntPtr pAlgorithm = new IntPtr((long) pSubjectPublicKeyInfo + (long) Marshal.OffsetOf(typeof(CAPI.CERT_PUBLIC_KEY_INFO), "Algorithm"));

                // Point to Parameters field inside the Algorithm field.
                IntPtr pParameters = new IntPtr((long) pAlgorithm + (long) Marshal.OffsetOf(typeof(CAPI.CRYPT_ALGORITHM_IDENTIFIER), "Parameters"));
            
                // Retrieve the pszObjId pointer.
                IntPtr pObjId = Marshal.ReadIntPtr(pAlgorithm);

                // Translate the OID to AlgId value.
                CAPI.CRYPT_OID_INFO pOIDInfo = CAPI.CryptFindOIDInfo(CAPI.CRYPT_OID_INFO_OID_KEY, pObjId, CAPI.CRYPT_PUBKEY_ALG_OID_GROUP_ID);

                // Is this DSS?
                if (pOIDInfo.Algid == CAPI.CALG_DSS_SIGN) {
                    bool inheritParameters = false;

                    // This is DSS, so inherit the parameters if necessary.
                    IntPtr pcbData = new IntPtr((long) pParameters + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "cbData"));
                    IntPtr ppbData = new IntPtr((long) pParameters + (long) Marshal.OffsetOf(typeof(CAPI.CRYPTOAPI_BLOB), "pbData"));
                    if (Marshal.ReadInt32(pcbData) == 0) {
                        inheritParameters = true;
                    }
                    else {
                        // Need to inherit if NULL pbData or *pbData is 0x05 (NULL ASN tag).
                        if (Marshal.ReadIntPtr(ppbData) == IntPtr.Zero) {
                            inheritParameters = true;
                        }
                        else {
                            IntPtr pbData = Marshal.ReadIntPtr(ppbData);
                            if ((uint) Marshal.ReadInt32(pbData) == CAPI.ASN_TAG_NULL) {
                                inheritParameters = true;
                            }
                        }
                    }

                    // Do we need to copy inherited DSS parameters?
                    if (inheritParameters) {
                        // Build the chain to force CAPI to propagate the parameters to 
                        // CERT_PUBKEY_ALG_PARA_PROP_ID extended property.
                        SafeCertChainHandle pChainContext = SafeCertChainHandle.InvalidHandle;
                        X509Utils.BuildChain(new IntPtr(CAPI.HCCE_CURRENT_USER),
                                             X509Utils.GetCertContext(certificate),
                                             null,
                                             null,
                                             null,
                                             X509RevocationMode.NoCheck,
                                             X509RevocationFlag.ExcludeRoot,
                                             DateTime.Now,
                                             new TimeSpan(0, 0, 0), // default
                                             ref pChainContext);
                        pChainContext.Dispose();

                        // The parameter is inherited in the extended property, but not copied
                        // to CERT_INFO, so we need to do it ourselves.
                        uint cbParameters = 0;

                        if (!CAPI.CAPISafe.CertGetCertificateContextProperty(X509Utils.GetCertContext(certificate),
                                                                             CAPI.CERT_PUBKEY_ALG_PARA_PROP_ID,
                                                                             pbParameters,
                                                                             ref cbParameters))
                            throw new CryptographicException(Marshal.GetLastWin32Error());

                        if (cbParameters > 0) {
                            pbParameters = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(cbParameters));
                            if (!CAPI.CAPISafe.CertGetCertificateContextProperty(X509Utils.GetCertContext(certificate),
                                                                                 CAPI.CERT_PUBKEY_ALG_PARA_PROP_ID,
                                                                                 pbParameters,
                                                                                 ref cbParameters))
                                throw new CryptographicException(Marshal.GetLastWin32Error());

                            Marshal.WriteInt32(pcbData, (int)cbParameters);
                            Marshal.WriteIntPtr(ppbData, pbParameters.DangerousGetHandle());
                        }
                    }
                }

                // Is this counter signer?
                if (m_parentSignerInfo == null) {
                    // Just plain signer.
                    if (!CAPI.CryptMsgControl(m_signedCms.GetCryptMsgHandle(),
                                              0,
                                              CAPI.CMSG_CTRL_VERIFY_SIGNATURE,
                                              pCertContext.pCertInfo)) { 
                        throw new CryptographicException(Marshal.GetLastWin32Error());
                    }
                }
                else {
                    // Counter signer, so need to first find parent signer's index.
                    int index = -1;
                    int lastWin32Error = 0;

                    // Since we allow the same signer to sign more than once,
                    // we must than try all signatures of the same signer.
                    while (true) {
                        try {
                            // Find index of parent signer.
                            index = PkcsUtils.GetSignerIndex(m_signedCms.GetCryptMsgHandle(), m_parentSignerInfo, index + 1);
                        }
                        catch (CryptographicException) {
                            // Did we ever find a signature of the same signer?
                            if (lastWin32Error == 0) {
                                // No. So we just re-throw, which is most likely CAPI.CRYPT_E_SIGNER_NOT_FOUND.
                                throw;
                            }
                            else {
                                // Yes. Throw previous error, which is most likely CAPI.NTE_BAD_SIGNATURE.
                                throw new CryptographicException(lastWin32Error);
                            }
                        }

                        // Now get the parent encoded singer info.
                        uint cbParentEncodedSignerInfo = 0;
                        SafeLocalAllocHandle pbParentEncodedSignerInfo = SafeLocalAllocHandle.InvalidHandle;

                        PkcsUtils.GetParam(m_signedCms.GetCryptMsgHandle(),
                                           CAPI.CMSG_ENCODED_SIGNER,
                                           (uint) index,
                                           out pbParentEncodedSignerInfo,
                                           out cbParentEncodedSignerInfo);

                        // Try next signer if we can't get parent of this signer.
                        if (cbParentEncodedSignerInfo == 0) {
                            lastWin32Error = CAPI.CRYPT_E_NO_SIGNER;
                            continue;
                        }

                        fixed (byte * pbEncodedSignerInfo = m_encodedSignerInfo) {
                            if (!CAPI.CAPISafe.CryptMsgVerifyCountersignatureEncoded(IntPtr.Zero,
                                                                                     CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                                                     pbParentEncodedSignerInfo.DangerousGetHandle(),
                                                                                     cbParentEncodedSignerInfo,
                                                                                     new IntPtr(pbEncodedSignerInfo),
                                                                                     (uint)m_encodedSignerInfo.Length,
                                                                                     pCertContext.pCertInfo)) {
                                // Cache the error, and try next signer.
                                lastWin32Error = Marshal.GetLastWin32Error();
                                continue;
                            }
                        }

                        // Keep alive.
                        pbParentEncodedSignerInfo.Dispose();

                        // The signature is successfully verified.
                        break;
                    }
                }

                // Verfiy the cert if requested.
                if (!verifySignatureOnly) {
                    int hr = VerifyCertificate(certificate, extraStore);
                    if (hr != CAPI.S_OK)
                        throw new CryptographicException(hr);
                }

                // Keep alive.
                pbParameters.Dispose();
            }
        }

        [SecuritySafeCritical]
        private unsafe void RemoveCounterSignature (int parentIndex, int childIndex) {
            // Just make sure this is non-negative.
            if (parentIndex < 0)
                throw new ArgumentOutOfRangeException("parentIndex");
            if (childIndex < 0) 
                throw new ArgumentOutOfRangeException("childIndex");

            uint cbCmsgCmsSignerInfo = 0;
            SafeLocalAllocHandle pbCmsgCmsSignerInfo = SafeLocalAllocHandle.InvalidHandle;

            uint cbCmsgSignerInfo = 0;
            SafeLocalAllocHandle pbCmsgSignerInfo = SafeLocalAllocHandle.InvalidHandle;

            uint index = 0;
            uint cAttr = 0;
            IntPtr pAttr = IntPtr.Zero;
            SafeCryptMsgHandle hMsg = m_signedCms.GetCryptMsgHandle();

            if (PkcsUtils.CmsSupported()) {
                PkcsUtils.GetParam(hMsg, 
                                   CAPI.CMSG_CMS_SIGNER_INFO_PARAM, 
                                   (uint) parentIndex, 
                                   out pbCmsgCmsSignerInfo, 
                                   out cbCmsgCmsSignerInfo);

                CAPI.CMSG_CMS_SIGNER_INFO cmsgCmsSignerInfo = (CAPI.CMSG_CMS_SIGNER_INFO) Marshal.PtrToStructure(pbCmsgCmsSignerInfo.DangerousGetHandle(), typeof(CAPI.CMSG_CMS_SIGNER_INFO));
                cAttr = cmsgCmsSignerInfo.UnauthAttrs.cAttr;
                pAttr = new IntPtr((long) cmsgCmsSignerInfo.UnauthAttrs.rgAttr);
            }
            else {
                PkcsUtils.GetParam(hMsg,
                                   CAPI.CMSG_SIGNER_INFO_PARAM,
                                   (uint) parentIndex, 
                                   out pbCmsgSignerInfo, 
                                   out cbCmsgSignerInfo);

                CAPI.CMSG_SIGNER_INFO cmsgSignerInfo = (CAPI.CMSG_SIGNER_INFO) Marshal.PtrToStructure(pbCmsgSignerInfo.DangerousGetHandle(), typeof(CAPI.CMSG_SIGNER_INFO));
                cAttr = cmsgSignerInfo.UnauthAttrs.cAttr;
                pAttr = new IntPtr((long) cmsgSignerInfo.UnauthAttrs.rgAttr);
            }

            // Find index for counter signature attribute.
            // Note: It is not guaranteed that CAPI will keep all counter signatures
            // in one single unauthenticated attribute. So we need to find the correct
            // unauthenticated attribute containing this counter signer which is 
            // identified by index.
            for (index = 0; index < cAttr; index++) {
                checked {
                    CAPI.CRYPT_ATTRIBUTE attr = (CAPI.CRYPT_ATTRIBUTE) Marshal.PtrToStructure(pAttr, typeof(CAPI.CRYPT_ATTRIBUTE));
                    if (String.Compare(attr.pszObjId, CAPI.szOID_RSA_counterSign, StringComparison.OrdinalIgnoreCase) == 0) {
                        if (attr.cValue > 0) {
                            // Is it in this attribute?
                            if (childIndex < (int) attr.cValue) {
                                // Found the desired counter signature attribute. So, first remove the
                                // entire attribute, then remove just the counter signature from the
                                // retrieved attribute, and finally add back the modified attribute,
                                // if necessary.
                                CAPI.CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR_PARA delPara = new CAPI.CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR_PARA(Marshal.SizeOf(typeof(CAPI.CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR_PARA)));
                                delPara.dwSignerIndex = (uint) parentIndex;
                                delPara.dwUnauthAttrIndex = index;

                                if (!CAPI.CryptMsgControl(hMsg,
                                                          0,
                                                          CAPI.CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR,
                                                          new IntPtr(&delPara)))
                                    throw new CryptographicException(Marshal.GetLastWin32Error());

                                // No need to add back if only one counter signature in this attribute.
                                if (attr.cValue > 1) {
                                    try {
                                        // There were more than one counter signatures in this attribute, so 
                                        // need to add back a new counter signature attribute which includes
                                        // the remaining counter signatures.
                                        uint cbCounterSignatureValue = (uint) ((attr.cValue - 1) * Marshal.SizeOf(typeof(CAPI.CRYPTOAPI_BLOB)));
                                        SafeLocalAllocHandle pbCounterSignatureValue = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(cbCounterSignatureValue));

                                        // Copy everything except the one being removed.
                                        CAPI.CRYPTOAPI_BLOB * pOldValue =  (CAPI.CRYPTOAPI_BLOB *) attr.rgValue;
                                        CAPI.CRYPTOAPI_BLOB * pNewValue =  (CAPI.CRYPTOAPI_BLOB *) pbCounterSignatureValue.DangerousGetHandle();

                                        for (int i = 0; i < (int) attr.cValue; i++, pOldValue++, pNewValue++) {
                                            if (i != childIndex) {
                                                *pNewValue = *pOldValue;
                                            }
                                        }

                                        // Encode the new counter signature attribute.
                                        byte[] encodedNewAttribute;
                                        CAPI.CRYPT_ATTRIBUTE newAttr = new CAPI.CRYPT_ATTRIBUTE();
                                        newAttr.pszObjId = attr.pszObjId;
                                        newAttr.cValue = attr.cValue - 1;
                                        newAttr.rgValue = pbCounterSignatureValue.DangerousGetHandle();

                                        SafeLocalAllocHandle pNewAttr = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(Marshal.SizeOf(typeof(CAPI.CRYPT_ATTRIBUTE))));
                                        Marshal.StructureToPtr(newAttr, pNewAttr.DangerousGetHandle(), false);

                                        try {
                                            if (!CAPI.EncodeObject(new IntPtr(CAPI.PKCS_ATTRIBUTE),
                                                                    pNewAttr.DangerousGetHandle(),
                                                                    out encodedNewAttribute))
                                                throw new CryptographicException(Marshal.GetLastWin32Error());
                                        }
                                        finally {
                                            Marshal.DestroyStructure(pNewAttr.DangerousGetHandle(), typeof(CAPI.CRYPT_ATTRIBUTE));
                                            pNewAttr.Dispose();
                                        }

                                        // Finally, add it back.
                                        fixed (byte * pbData = &encodedNewAttribute[0]) {
                                            CAPI.CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA addPara = new CAPI.CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA(Marshal.SizeOf(typeof(CAPI.CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA)));
                                            addPara.dwSignerIndex = (uint) parentIndex;
                                            addPara.blob.cbData = (uint) encodedNewAttribute.Length;
                                            addPara.blob.pbData = new IntPtr(pbData);

                                            if (!CAPI.CryptMsgControl(hMsg,
                                                                        0,
                                                                        CAPI.CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR,
                                                                        new IntPtr(&addPara)))
                                                throw new CryptographicException(Marshal.GetLastWin32Error());
                                        }

                                        // Keep alive.
                                        pbCounterSignatureValue.Dispose();
                                    }
                                    catch (CryptographicException) {
                                        // Roll back.
                                        byte[] encodedAttribute;
                                        if (CAPI.EncodeObject(new IntPtr(CAPI.PKCS_ATTRIBUTE),
                                                              pAttr,
                                                              out encodedAttribute)) {
                                            fixed (byte * pbData = &encodedAttribute[0]) {
                                                CAPI.CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA addPara = new CAPI.CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA(Marshal.SizeOf(typeof(CAPI.CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA)));
                                                addPara.dwSignerIndex = (uint) parentIndex;
                                                addPara.blob.cbData = (uint) encodedAttribute.Length;
                                                addPara.blob.pbData = new IntPtr(pbData);
                                                CAPI.CryptMsgControl(hMsg, 0, CAPI.CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR, new IntPtr(&addPara));
                                            }
                                        }
                                        throw;
                                    }
                                }

                                return;
                            }

                            childIndex -= (int) attr.cValue;
                        }
                    }

                    pAttr = new IntPtr((long) pAttr + (long) Marshal.SizeOf(typeof(CAPI.CRYPT_ATTRIBUTE)));
                }
            }
            
            // Keep alive.
            if (pbCmsgCmsSignerInfo != null && !pbCmsgCmsSignerInfo.IsInvalid) {
                pbCmsgCmsSignerInfo.Dispose();
            }
            if (pbCmsgSignerInfo != null && !pbCmsgSignerInfo.IsInvalid) {
                pbCmsgSignerInfo.Dispose();
            }

            throw new CryptographicException(CAPI.CRYPT_E_NO_SIGNER);
        }

        //
        // Private static.
        //

        [SecuritySafeCritical]
        private static unsafe int VerifyCertificate (X509Certificate2 certificate,
                                                     X509Certificate2Collection extraStore) {
            int dwErrorStatus;
            int hr = X509Utils.VerifyCertificate(X509Utils.GetCertContext(certificate),
                                                 null,
                                                 null,
                                                 X509RevocationMode.Online,
                                                 X509RevocationFlag.ExcludeRoot,
                                                 DateTime.Now,
                                                 new TimeSpan(0, 0, 0),
                                                 extraStore,
                                                 new IntPtr(CAPI.CERT_CHAIN_POLICY_BASE),
                                                 new IntPtr(&dwErrorStatus));
            if (hr != CAPI.S_OK)
                return dwErrorStatus;

            // Check key usages to make sure it is good for signing.
            foreach (X509Extension extension in certificate.Extensions) {
                if (String.Compare(extension.Oid.Value, CAPI.szOID_KEY_USAGE, StringComparison.OrdinalIgnoreCase) == 0) {
                    X509KeyUsageExtension keyUsage = new X509KeyUsageExtension();
                    keyUsage.CopyFrom(extension);
                    if ((keyUsage.KeyUsages & X509KeyUsageFlags.DigitalSignature) == 0 &&
                        (keyUsage.KeyUsages & X509KeyUsageFlags.NonRepudiation) == 0) {
                        hr = CAPI.CERT_E_WRONG_USAGE;
                        break;
                    }
                }
            }

            return hr;
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class SignerInfoCollection : ICollection {
        private SignerInfo[] m_signerInfos;

        internal SignerInfoCollection () {
            m_signerInfos = new SignerInfo[0];
        }

        [SecuritySafeCritical]
        internal unsafe SignerInfoCollection (SignedCms signedCms) {
            uint dwSigners = 0;
            uint cbCount = (uint) Marshal.SizeOf(typeof(uint));
            SafeCryptMsgHandle safeCryptMsgHandle = signedCms.GetCryptMsgHandle();

            if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                CAPI.CMSG_SIGNER_COUNT_PARAM,
                                                0,
                                                new IntPtr(&dwSigners),
                                                new IntPtr(&cbCount)))
                throw new CryptographicException(Marshal.GetLastWin32Error());

            SignerInfo[] signerInfos = new SignerInfo[dwSigners];
            for (int index = 0; index < dwSigners; index++) {
                uint cbCmsgSignerInfo = 0;
                if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                    CAPI.CMSG_SIGNER_INFO_PARAM,
                                                    (uint)index,
                                                    IntPtr.Zero,
                                                    new IntPtr(&cbCmsgSignerInfo)))
                    throw new CryptographicException(Marshal.GetLastWin32Error());

                SafeLocalAllocHandle pbCmsgSignerInfo = CAPI.LocalAlloc(CAPI.LMEM_FIXED, new IntPtr(cbCmsgSignerInfo));

                if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                    CAPI.CMSG_SIGNER_INFO_PARAM,
                                                    (uint)index,
                                                    pbCmsgSignerInfo,
                                                    new IntPtr(&cbCmsgSignerInfo)))
                    throw new CryptographicException(Marshal.GetLastWin32Error());

                signerInfos[index] = new SignerInfo(signedCms, pbCmsgSignerInfo);
            }

            m_signerInfos = signerInfos;
        }

        [SecuritySafeCritical]
        internal SignerInfoCollection (SignedCms signedCms, SignerInfo signerInfo) {
            SignerInfo[] signerInfos = new SignerInfo[0];

            int count = 0;
            int index = 0;

            foreach (CryptographicAttributeObject attribute in signerInfo.UnsignedAttributes) {
                if (attribute.Oid.Value == CAPI.szOID_RSA_counterSign) {
                    count += attribute.Values.Count;
                }
            }

            signerInfos = new SignerInfo[count];

            foreach (CryptographicAttributeObject attribute in signerInfo.UnsignedAttributes) {
                if (attribute.Oid.Value == CAPI.szOID_RSA_counterSign) {
                    for (int i = 0; i < attribute.Values.Count; i++) {
                        AsnEncodedData encodedSignerInfo = (AsnEncodedData) attribute.Values[i];
                        signerInfos[index++] = new SignerInfo(signedCms, signerInfo, encodedSignerInfo.RawData);
                    }
                }
            }

            m_signerInfos = signerInfos;
        }

        public SignerInfo this[int index] {
            get {
                if (index < 0 || index >= m_signerInfos.Length)
                    throw new ArgumentOutOfRangeException("index", SecurityResources.GetResourceString("ArgumentOutOfRange_Index"));

                Debug.Assert(m_signerInfos[index] != null);
                return m_signerInfos[index];
            }
        }

        public int Count {
            get {
                return m_signerInfos.Length;
            }
        }

        public SignerInfoEnumerator GetEnumerator() {
            return new SignerInfoEnumerator(this);
        }

        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return new SignerInfoEnumerator(this);
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

        public void CopyTo(SignerInfo[] array, int index) {
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
    public sealed class SignerInfoEnumerator : IEnumerator {
        private SignerInfoCollection m_signerInfos;
        private int m_current;

        private SignerInfoEnumerator() {}
        internal SignerInfoEnumerator(SignerInfoCollection signerInfos) {
            m_signerInfos = signerInfos;
            m_current = -1;
        }

        public SignerInfo Current {
            get {
                return m_signerInfos[m_current];
            }
        }

        /// <internalonly/>
        Object IEnumerator.Current {
            get {
                return (Object) m_signerInfos[m_current];
            }
        }

        public bool MoveNext() {
            if (m_current == ((int) m_signerInfos.Count - 1)) {
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
