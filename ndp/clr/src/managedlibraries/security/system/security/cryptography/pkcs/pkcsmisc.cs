// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

//
// PkcsMisc.cs
//
// 02/09/2003
// 

namespace System.Security.Cryptography.Pkcs {
    using System.Collections;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Cryptography.Xml;

    public enum KeyAgreeKeyChoice {
        Unknown      = 0,
        EphemeralKey = 1,
        StaticKey    = 2,
    }

    public enum SubjectIdentifierType {
        Unknown                = 0,  // Use any of the following as appropriate
        IssuerAndSerialNumber  = 1,  // X509IssuerSerial
        SubjectKeyIdentifier   = 2,  // SKI hex string
        NoSignature            = 3  // NoSignature
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class SubjectIdentifier {
        private SubjectIdentifierType m_type;
        private Object                m_value;

        private SubjectIdentifier () {}
        [SecurityCritical]
        internal SubjectIdentifier (CAPI.CERT_INFO certInfo) : this(certInfo.Issuer, certInfo.SerialNumber) {}
        [SecurityCritical]
        internal SubjectIdentifier (CAPI.CMSG_SIGNER_INFO signerInfo) : this(signerInfo.Issuer, signerInfo.SerialNumber) {}

        internal SubjectIdentifier (SubjectIdentifierType type, Object value) {
            Reset(type, value);
        }

        [SecurityCritical]
        internal unsafe SubjectIdentifier (CAPI.CRYPTOAPI_BLOB issuer, CAPI.CRYPTOAPI_BLOB serialNumber) {
            // If serial number is 0, then it is the special SKI encoding or NoSignature
            bool isSKIorHashOnly = true;
            byte * pb = (byte *) serialNumber.pbData;
            for (uint i = 0; i < serialNumber.cbData; i++) {
                if (*pb++ != (byte) 0) {
                    isSKIorHashOnly = false;
                    break;
                }
            }

            if (isSKIorHashOnly) {
                byte[] issuerBytes = new byte[issuer.cbData];
                Marshal.Copy(issuer.pbData, issuerBytes, 0, issuerBytes.Length);
                X500DistinguishedName dummyName = new X500DistinguishedName(issuerBytes);
                if (String.Compare(CAPI.DummySignerCommonName, dummyName.Name, StringComparison.OrdinalIgnoreCase) == 0) {
                    Reset(SubjectIdentifierType.NoSignature, null);
                    return;
                }
            }

            if (isSKIorHashOnly) {
                // Decode disguised SKI in issuer field (See WinCrypt.h for more info).  Note that some certificates may contain
                // an all-zero serial number but not be encoded with an szOID_KEYID_RDN.  In order to allow use of signatures created
                // using these certificates, we will first try to find the szOID_KEYID_RDN, but if it does not exist, fall back to just
                // decoding the incoming issuer and serial number.
                m_type = SubjectIdentifierType.SubjectKeyIdentifier;
                m_value = String.Empty;

                uint cbCertNameInfo = 0;
                SafeLocalAllocHandle pbCertNameInfo = SafeLocalAllocHandle.InvalidHandle;

                if (CAPI.DecodeObject(new IntPtr(CAPI.X509_NAME),
                                      issuer.pbData,
                                      issuer.cbData,
                                      out pbCertNameInfo,
                                      out cbCertNameInfo)) {
                    using (pbCertNameInfo) {
                        checked {
                            CAPI.CERT_NAME_INFO certNameInfo = (CAPI.CERT_NAME_INFO) Marshal.PtrToStructure(pbCertNameInfo.DangerousGetHandle(), typeof(CAPI.CERT_NAME_INFO));
                            for (uint i = 0; i < certNameInfo.cRDN; i++) {
                                CAPI.CERT_RDN certRdn = (CAPI.CERT_RDN) Marshal.PtrToStructure(new IntPtr((long) certNameInfo.rgRDN + (long) (i * Marshal.SizeOf(typeof(CAPI.CERT_RDN)))), typeof(CAPI.CERT_RDN));

                                for (uint j = 0; j < certRdn.cRDNAttr; j++)
                                {
                                    CAPI.CERT_RDN_ATTR certRdnAttr = (CAPI.CERT_RDN_ATTR)Marshal.PtrToStructure(new IntPtr((long)certRdn.rgRDNAttr + (long)(j * Marshal.SizeOf(typeof(CAPI.CERT_RDN_ATTR)))), typeof(CAPI.CERT_RDN_ATTR));

                                    if (String.Compare(CAPI.szOID_KEYID_RDN, certRdnAttr.pszObjId, StringComparison.OrdinalIgnoreCase) == 0)
                                    {
                                        if (certRdnAttr.dwValueType == CAPI.CERT_RDN_OCTET_STRING)
                                        {
                                            byte[] ski = new byte[certRdnAttr.Value.cbData];
                                            Marshal.Copy(certRdnAttr.Value.pbData, ski, 0, ski.Length);
                                            Reset(SubjectIdentifierType.SubjectKeyIdentifier, X509Utils.EncodeHexString(ski));
                                            return;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            CAPI.CERT_ISSUER_SERIAL_NUMBER IssuerAndSerial;
            IssuerAndSerial.Issuer = issuer;
            IssuerAndSerial.SerialNumber = serialNumber;
            X509IssuerSerial issuerSerial = PkcsUtils.DecodeIssuerSerial(IssuerAndSerial);
            Reset(SubjectIdentifierType.IssuerAndSerialNumber, issuerSerial);
        }

        [SecurityCritical]
        internal SubjectIdentifier (CAPI.CERT_ID certId) {
            switch (certId.dwIdChoice) {
            case CAPI.CERT_ID_ISSUER_SERIAL_NUMBER:
                X509IssuerSerial issuerSerial = PkcsUtils.DecodeIssuerSerial(certId.Value.IssuerSerialNumber);
                Reset(SubjectIdentifierType.IssuerAndSerialNumber, issuerSerial);
                break;
            case CAPI.CERT_ID_KEY_IDENTIFIER:
                byte[] ski = new byte[certId.Value.KeyId.cbData];
                Marshal.Copy(certId.Value.KeyId.pbData, ski, 0, ski.Length);
                Reset(SubjectIdentifierType.SubjectKeyIdentifier, X509Utils.EncodeHexString(ski));
                break;
            default:
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Invalid_Subject_Identifier_Type"), certId.dwIdChoice.ToString(CultureInfo.InvariantCulture));
            }
        }

        public SubjectIdentifierType Type {
            get {
                return m_type;
            }
        }

        public Object Value {
            get {
                return m_value;
            }
        }

        //
        // Internal methods.
        //

        internal void Reset (SubjectIdentifierType type, Object value) {
            switch (type) {
            case SubjectIdentifierType.NoSignature:
            case SubjectIdentifierType.Unknown:
                break;
            case SubjectIdentifierType.IssuerAndSerialNumber:
                if (value.GetType() != typeof(X509IssuerSerial)) {
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Invalid_Subject_Identifier_Type_Value_Mismatch"), value.GetType().ToString());
                }
                break;
            case SubjectIdentifierType.SubjectKeyIdentifier:
                if (!PkcsUtils.CmsSupported()) {
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Not_Supported"));
                }
                if (value.GetType() != typeof(string)) {
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Invalid_Subject_Identifier_Type_Value_Mismatch"), value.GetType().ToString());
                }
                break;
            default:
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Invalid_Subject_Identifier_Type"), type.ToString());
            }

            m_type = type;
            m_value = value;
        }
    }

    public enum SubjectIdentifierOrKeyType {
        Unknown                = 0,  // Use any of the following as appropriate
        IssuerAndSerialNumber  = 1,  // X509IssuerSerial
        SubjectKeyIdentifier   = 2,  // SKI hex string
        PublicKeyInfo          = 3,  // PublicKeyInfo
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class PublicKeyInfo {
        private AlgorithmIdentifier m_algorithm;
        private byte[]              m_keyValue;

        private PublicKeyInfo () {}

        [SecurityCritical]
        internal PublicKeyInfo (CAPI.CERT_PUBLIC_KEY_INFO keyInfo) {
            m_algorithm = new AlgorithmIdentifier(keyInfo);
            m_keyValue = new byte[keyInfo.PublicKey.cbData];
            if (m_keyValue.Length > 0) {
                Marshal.Copy(keyInfo.PublicKey.pbData, m_keyValue, 0, m_keyValue.Length);
            }
        }

        public AlgorithmIdentifier Algorithm {
            get {
                return m_algorithm;
            }
        }

        public byte[] KeyValue {
            get {
                return m_keyValue;
            }
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class SubjectIdentifierOrKey {
        private SubjectIdentifierOrKeyType m_type;
        private Object                     m_value;

        private SubjectIdentifierOrKey () {}

        internal SubjectIdentifierOrKey (SubjectIdentifierOrKeyType type, Object value) {
            Reset(type, value);
        }

        [SecurityCritical]
        internal SubjectIdentifierOrKey (CAPI.CERT_ID certId) {
            switch (certId.dwIdChoice) {
            case CAPI.CERT_ID_ISSUER_SERIAL_NUMBER:
                X509IssuerSerial issuerSerial = PkcsUtils.DecodeIssuerSerial(certId.Value.IssuerSerialNumber);
                Reset(SubjectIdentifierOrKeyType.IssuerAndSerialNumber, issuerSerial);
                break;
            case CAPI.CERT_ID_KEY_IDENTIFIER:
                byte[] ski = new byte[certId.Value.KeyId.cbData];
                Marshal.Copy(certId.Value.KeyId.pbData, ski, 0, ski.Length);
                Reset(SubjectIdentifierOrKeyType.SubjectKeyIdentifier, X509Utils.EncodeHexString(ski));
                break;
            default:
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Invalid_Subject_Identifier_Type"), certId.dwIdChoice.ToString(CultureInfo.InvariantCulture));
            }
        }

        [SecurityCritical]
        internal SubjectIdentifierOrKey (CAPI.CERT_PUBLIC_KEY_INFO publicKeyInfo) {
            Reset(SubjectIdentifierOrKeyType.PublicKeyInfo, new PublicKeyInfo(publicKeyInfo));
        }
        
        public SubjectIdentifierOrKeyType Type {
            get {
                return m_type;
            }
        }

        public Object Value {
            get {
                return m_value;
            }
        }

        //
        // Internal methods.
        //

        internal void Reset (SubjectIdentifierOrKeyType type, Object value) {
            switch (type) {
            case SubjectIdentifierOrKeyType.Unknown:
                break;
            case SubjectIdentifierOrKeyType.IssuerAndSerialNumber:
                if (value.GetType() != typeof(X509IssuerSerial)) {
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Invalid_Subject_Identifier_Type_Value_Mismatch"), value.GetType().ToString());
                }
                break;
            case SubjectIdentifierOrKeyType.SubjectKeyIdentifier:
                if (!PkcsUtils.CmsSupported()) {
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Not_Supported"));
                }
                if (value.GetType() != typeof(string)) {
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Invalid_Subject_Identifier_Type_Value_Mismatch"), value.GetType().ToString());
                }
                break;
            case SubjectIdentifierOrKeyType.PublicKeyInfo:
                if (!PkcsUtils.CmsSupported()) {
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Not_Supported"));
                }
                if (value.GetType() != typeof(PublicKeyInfo)) {
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Invalid_Subject_Identifier_Type_Value_Mismatch"), value.GetType().ToString());
                }
                break;
            default:
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Cms_Invalid_Subject_Identifier_Type"), type.ToString());
            }

            m_type = type;
            m_value = value;
        }
    }


    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class AlgorithmIdentifier {
        private Oid     m_oid;
        private int     m_keyLength;
        private byte[]  m_parameters;

        public AlgorithmIdentifier () {
            Reset(Oid.FromOidValue(CAPI.szOID_RSA_DES_EDE3_CBC, OidGroup.EncryptionAlgorithm), 0, new byte[0]);
        }

        public AlgorithmIdentifier (Oid oid) {
            Reset(oid, 0, new byte[0]);
        }

        public AlgorithmIdentifier (Oid oid, int keyLength) {
            Reset(oid, keyLength, new byte[0]);
        }

        [SecurityCritical]
        internal AlgorithmIdentifier (CAPI.CERT_PUBLIC_KEY_INFO keyInfo) {
            SafeLocalAllocHandle pKeyInfo = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(Marshal.SizeOf(typeof(CAPI.CERT_PUBLIC_KEY_INFO))));
            Marshal.StructureToPtr(keyInfo, pKeyInfo.DangerousGetHandle(), false);
            int keyLength = (int) CAPI.CAPISafe.CertGetPublicKeyLength(CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING, pKeyInfo.DangerousGetHandle());
            byte[] parameters = new byte[keyInfo.Algorithm.Parameters.cbData];
            if (parameters.Length > 0) {
                Marshal.Copy(keyInfo.Algorithm.Parameters.pbData, parameters, 0, parameters.Length);
            }
            Marshal.DestroyStructure(pKeyInfo.DangerousGetHandle(), typeof(CAPI.CERT_PUBLIC_KEY_INFO));
            pKeyInfo.Dispose();
            Reset(Oid.FromOidValue(keyInfo.Algorithm.pszObjId, OidGroup.PublicKeyAlgorithm), keyLength, parameters);
        }

        [SecurityCritical]
        internal AlgorithmIdentifier (CAPI.CRYPT_ALGORITHM_IDENTIFIER algorithmIdentifier) {
            int keyLength = 0;
            uint cbParameters = 0;
            SafeLocalAllocHandle pbParameters = SafeLocalAllocHandle.InvalidHandle;
            byte[] parameters = new byte[0];

            uint algId = X509Utils.OidToAlgId(algorithmIdentifier.pszObjId);

            if (algId == CAPI.CALG_RC2) {
                if (algorithmIdentifier.Parameters.cbData > 0) {
                    if (!CAPI.DecodeObject(new IntPtr(CAPI.PKCS_RC2_CBC_PARAMETERS),
                                           algorithmIdentifier.Parameters.pbData,
                                           algorithmIdentifier.Parameters.cbData,
                                           out pbParameters,
                                           out cbParameters))
                        throw new CryptographicException(Marshal.GetLastWin32Error());

                    CAPI.CRYPT_RC2_CBC_PARAMETERS rc2Parameters = (CAPI.CRYPT_RC2_CBC_PARAMETERS) Marshal.PtrToStructure(pbParameters.DangerousGetHandle(), typeof(CAPI.CRYPT_RC2_CBC_PARAMETERS));
                    switch (rc2Parameters.dwVersion) {
                    case CAPI.CRYPT_RC2_40BIT_VERSION:
                        keyLength = 40;
                        break;
                    case CAPI.CRYPT_RC2_56BIT_VERSION:
                        keyLength = 56;
                        break;
                    case CAPI.CRYPT_RC2_128BIT_VERSION:
                        keyLength = 128;
                        break;
                    }
                    // Retrieve IV if available.
                    if (rc2Parameters.fIV) {
                        parameters = (byte[]) rc2Parameters.rgbIV.Clone();
                    }
                }
            }
            else if (algId == CAPI.CALG_RC4 || algId == CAPI.CALG_DES || algId == CAPI.CALG_3DES) {
                // Retrieve the IV if available. For non RC2, the parameter contains the IV 
                // (for RC4 the IV is really the salt). There are (128 - KeyLength) / 8
                // bytes of RC4 salt.
                if (algorithmIdentifier.Parameters.cbData > 0) {
                    if (!CAPI.DecodeObject(new IntPtr(CAPI.X509_OCTET_STRING),
                                           algorithmIdentifier.Parameters.pbData,
                                           algorithmIdentifier.Parameters.cbData,
                                           out pbParameters,
                                           out cbParameters))
                        throw new CryptographicException(Marshal.GetLastWin32Error());

                    if (cbParameters > Marshal.SizeOf(typeof(CAPI.CRYPTOAPI_BLOB))) {
                        CAPI.CRYPTOAPI_BLOB blob = (CAPI.CRYPTOAPI_BLOB)Marshal.PtrToStructure(pbParameters.DangerousGetHandle(), typeof(CAPI.CRYPTOAPI_BLOB));

                        if (algId == CAPI.CALG_RC4) {
                            if (blob.cbData > 0) {
                                parameters = new byte[blob.cbData];
                                Marshal.Copy(blob.pbData, parameters, 0, parameters.Length);
                            }
                        }
                        else {
                            // This should be the same as the RC4 code, but for compatibility
                            // * Allocate an array as big as the CRYPTOAPI_BLOB
                            // * Copy in the cbData value
                            // * Copy in the (pbData) value
                            //
                            // But don't copy in the pbData pointer value (or, rather, clear it out),
                            // among other things it makes decoding the same contents into two
                            // different EnvelopedCms objects say the parameters were different.
                            parameters = new byte[cbParameters];
                            Marshal.Copy(pbParameters.DangerousGetHandle(), parameters, 0, parameters.Length);
                            Array.Clear(parameters, sizeof(uint), (int)(parameters.Length - blob.cbData - sizeof(uint)));
                        }
                    }
                }

                // Determine key length.
                if (algId == CAPI.CALG_RC4) {
                    // For RC4, keyLength = 128 - (salt length * 8).
                    keyLength = 128 - ((int) parameters.Length * 8);
                }
                else if (algId == CAPI.CALG_DES) {
                    // DES key length is fixed at 64 (or 56 without the parity bits).
                    keyLength = 64;
                }
                else {
                    // 3DES key length is fixed at 192 (or 168 without the parity bits).
                    keyLength = 192;
                }
            }
            else {
                // Everything else, don't decode it as CAPI may not expose or know how.
                if (algorithmIdentifier.Parameters.cbData > 0) {
                    parameters = new byte[algorithmIdentifier.Parameters.cbData];
                    Marshal.Copy(algorithmIdentifier.Parameters.pbData, parameters, 0, parameters.Length);
                }
            }

            Reset(Oid.FromOidValue(algorithmIdentifier.pszObjId, OidGroup.All), keyLength, parameters);
            pbParameters.Dispose();
        }

        public Oid Oid {
            get {
                return m_oid;
            }
            set {
                m_oid = value;
            }
        }

        public int KeyLength {
            get {
                return m_keyLength;
            }
            set {
                m_keyLength = value;
            }
        }

        public byte[] Parameters {
            get {
                return m_parameters;
            }
            set {
                m_parameters = value;
            }
        }

        //
        // Private methods.
        //

        private void Reset (Oid oid, int keyLength, byte[] parameters) {
            m_oid = oid;
            m_keyLength = keyLength;
            m_parameters = parameters;
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class ContentInfo {
        private Oid      m_contentType;
        private byte[]   m_content;
        private IntPtr   m_pContent = IntPtr.Zero;
        private GCHandle m_gcHandle;
        //
        // Constructors
        //

        private ContentInfo () :
            this(Oid.FromOidValue(CAPI.szOID_RSA_data, OidGroup.ExtensionOrAttribute), new byte[0]) {
        }

        public ContentInfo (byte[] content) :
            this(Oid.FromOidValue(CAPI.szOID_RSA_data, OidGroup.ExtensionOrAttribute), content) {
        }

        public ContentInfo (Oid contentType, byte[] content) {
            if (contentType == null)
                throw new ArgumentNullException("contentType");
            if (content == null)
                throw new ArgumentNullException("content");

            m_contentType = contentType;
            m_content = content;
        }

        public Oid ContentType {
            get {
                return m_contentType;
            }
        }

        public byte[] Content {
            get {
                return m_content;
            }
        }

        [SecuritySafeCritical]
        ~ContentInfo()
        {
            if (m_gcHandle.IsAllocated) {
                m_gcHandle.Free();
            }
        }

        internal IntPtr pContent {
            [SecurityCritical]
            get {
                if (IntPtr.Zero == m_pContent) {
                    if (m_content != null && m_content.Length != 0) {
                        m_gcHandle = GCHandle.Alloc(m_content, GCHandleType.Pinned);
                        //m_pContent = handle.AddrOfPinnedObject();
                        m_pContent = Marshal.UnsafeAddrOfPinnedArrayElement(m_content, 0);
                    }
                }
                return m_pContent;
            }
        }

        [SecuritySafeCritical]
        public static Oid GetContentType (byte[] encodedMessage) {
            if (encodedMessage == null)
                throw new ArgumentNullException("encodedMessage");

            SafeCryptMsgHandle safeCryptMsgHandle = CAPI.CAPISafe.CryptMsgOpenToDecode(
                                                            CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                            0,
                                                            0,
                                                            IntPtr.Zero,
                                                            IntPtr.Zero,
                                                            IntPtr.Zero);
            if (safeCryptMsgHandle == null || safeCryptMsgHandle.IsInvalid)
                throw new CryptographicException(Marshal.GetLastWin32Error());

            if (!CAPI.CAPISafe.CryptMsgUpdate(safeCryptMsgHandle, encodedMessage, (uint) encodedMessage.Length, true))
                throw new CryptographicException(Marshal.GetLastWin32Error());

            Oid contentType;
            switch (PkcsUtils.GetMessageType(safeCryptMsgHandle)) {
            case CAPI.CMSG_DATA:
                contentType = Oid.FromOidValue(CAPI.szOID_RSA_data, OidGroup.ExtensionOrAttribute);
                break;
            case CAPI.CMSG_SIGNED:
                contentType = Oid.FromOidValue(CAPI.szOID_RSA_signedData, OidGroup.ExtensionOrAttribute);
                break;
            case CAPI.CMSG_ENVELOPED:
                contentType = Oid.FromOidValue(CAPI.szOID_RSA_envelopedData, OidGroup.ExtensionOrAttribute);
                break;
            case CAPI.CMSG_SIGNED_AND_ENVELOPED:
                contentType = Oid.FromOidValue(CAPI.szOID_RSA_signEnvData, OidGroup.ExtensionOrAttribute);
                break;
            case CAPI.CMSG_HASHED:
                contentType = Oid.FromOidValue(CAPI.szOID_RSA_hashedData, OidGroup.ExtensionOrAttribute);
                break;
            case CAPI.CMSG_ENCRYPTED:
                contentType = Oid.FromOidValue(CAPI.szOID_RSA_encryptedData, OidGroup.ExtensionOrAttribute);
                break;
            default:
                throw new CryptographicException(CAPI.CRYPT_E_INVALID_MSG_TYPE);
            }

            safeCryptMsgHandle.Dispose();

            return contentType;
        }
    }
}
