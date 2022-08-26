// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

//
// PkcsUtils.cs
// 

namespace System.Security.Cryptography.Pkcs {
    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Cryptography.Xml;
    using System.Text;
    using System.Runtime.Versioning;

    internal static class PkcsUtils {
        private static volatile int m_cmsSupported = -1;

        private struct I_CRYPT_ATTRIBUTE {
            internal IntPtr pszObjId;
            internal uint   cValue;
            internal IntPtr rgValue;    // PCRYPT_ATTR_BLOB
        }

        internal static uint AlignedLength (uint length) {
            return ((length + (uint) 7) & ((uint) 0xfffffff8));
        }

        [SecuritySafeCritical]
        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)]
        internal static bool CmsSupported () {
            if (m_cmsSupported == -1) {
                using(SafeLibraryHandle hModule = CAPI.CAPISafe.LoadLibrary("Crypt32.dll")) {
                    if (!hModule.IsInvalid) {
                    IntPtr pFunc = CAPI.CAPISafe.GetProcAddress(hModule, "CryptMsgVerifyCountersignatureEncodedEx");
                    m_cmsSupported = pFunc == IntPtr.Zero ? 0 : 1;
                    }
                }
            }

            return m_cmsSupported == 0 ? false : true;
        }

        [SecuritySafeCritical]
        internal static RecipientInfoType GetRecipientInfoType (X509Certificate2 certificate) {
            RecipientInfoType recipientInfoType = RecipientInfoType.Unknown;

            if (certificate != null) {
                CAPI.CERT_CONTEXT pCertContext = (CAPI.CERT_CONTEXT) Marshal.PtrToStructure(X509Utils.GetCertContext(certificate).DangerousGetHandle(), typeof(CAPI.CERT_CONTEXT));
                CAPI.CERT_INFO certInfo = (CAPI.CERT_INFO) Marshal.PtrToStructure(pCertContext.pCertInfo, typeof(CAPI.CERT_INFO));

                uint algId = X509Utils.OidToAlgId(certInfo.SubjectPublicKeyInfo.Algorithm.pszObjId);
                if (algId == CAPI.CALG_RSA_KEYX)
                    recipientInfoType = RecipientInfoType.KeyTransport;
                else if (algId == CAPI.CALG_DH_SF || algId == CAPI.CALG_DH_EPHEM)
                    recipientInfoType = RecipientInfoType.KeyAgreement;
                else
                    recipientInfoType = RecipientInfoType.Unknown;
            }

            return recipientInfoType;
        }

        [SecurityCritical]
        internal static unsafe int GetMaxKeyLength (SafeCryptProvHandle safeCryptProvHandle, uint algId) {
            uint enumFlag = CAPI.CRYPT_FIRST;
            uint cbPeex = (uint) Marshal.SizeOf(typeof(CAPI.PROV_ENUMALGS_EX));
            SafeLocalAllocHandle pPeex = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(Marshal.SizeOf(typeof(CAPI.PROV_ENUMALGS_EX))));

            using (pPeex) {
                while (CAPI.CAPISafe.CryptGetProvParam(safeCryptProvHandle, CAPI.PP_ENUMALGS_EX, pPeex.DangerousGetHandle(), new IntPtr(&cbPeex), enumFlag)) {
                    CAPI.PROV_ENUMALGS_EX peex = (CAPI.PROV_ENUMALGS_EX) Marshal.PtrToStructure(pPeex.DangerousGetHandle(), typeof(CAPI.PROV_ENUMALGS_EX));

                    if (peex.aiAlgid == algId)
                        return (int) peex.dwMaxLen;

                    enumFlag = 0;
                }
            }

            throw new CryptographicException(CAPI.CRYPT_E_UNKNOWN_ALGO);
        }

        [SecurityCritical]
        internal static unsafe uint GetVersion (SafeCryptMsgHandle safeCryptMsgHandle) {
            uint dwVersion = 0;
            uint cbCount = (uint) Marshal.SizeOf(typeof(uint));
            if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                CAPI.CMSG_VERSION_PARAM,
                                                0,
                                                new IntPtr(&dwVersion),
                                                new IntPtr(&cbCount)))
                checkErr(Marshal.GetLastWin32Error());

            return dwVersion;
        }

        [SecurityCritical]
        internal static unsafe uint GetMessageType (SafeCryptMsgHandle safeCryptMsgHandle) {
            uint dwMsgType = 0;
            uint cbMsgType = (uint) Marshal.SizeOf(typeof(uint));
            if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                CAPI.CMSG_TYPE_PARAM,
                                                0,
                                                new IntPtr(&dwMsgType),
                                                new IntPtr(&cbMsgType)))
                checkErr(Marshal.GetLastWin32Error());

            return dwMsgType;
        }

        [SecurityCritical]
        internal static unsafe AlgorithmIdentifier GetAlgorithmIdentifier (SafeCryptMsgHandle safeCryptMsgHandle) {
            AlgorithmIdentifier algorithmIdentifier = new AlgorithmIdentifier();

            uint cbAlgorithm = 0;
            if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                CAPI.CMSG_ENVELOPE_ALGORITHM_PARAM,
                                                0,
                                                IntPtr.Zero,
                                                new IntPtr(&cbAlgorithm)))
                checkErr(Marshal.GetLastWin32Error());

            if (cbAlgorithm > 0) {
                SafeLocalAllocHandle pbAlgorithm = CAPI.LocalAlloc(CAPI.LMEM_FIXED, new IntPtr(cbAlgorithm));
                if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                    CAPI.CMSG_ENVELOPE_ALGORITHM_PARAM,
                                                    0,
                                                    pbAlgorithm,
                                                    new IntPtr(&cbAlgorithm)))
                    checkErr(Marshal.GetLastWin32Error());

                CAPI.CRYPT_ALGORITHM_IDENTIFIER cryptAlgorithmIdentifier = (CAPI.CRYPT_ALGORITHM_IDENTIFIER) Marshal.PtrToStructure(pbAlgorithm.DangerousGetHandle(), typeof(CAPI.CRYPT_ALGORITHM_IDENTIFIER));
                algorithmIdentifier = new AlgorithmIdentifier(cryptAlgorithmIdentifier);
                pbAlgorithm.Dispose();
            }

            return algorithmIdentifier;
        }

        [SecurityCritical]
        internal static unsafe void GetParam (SafeCryptMsgHandle safeCryptMsgHandle,
                                              uint paramType,
                                              uint index,
                                              out SafeLocalAllocHandle pvData,
                                              out uint cbData) {
            cbData = 0;
            pvData = SafeLocalAllocHandle.InvalidHandle;

            fixed (uint * pcbData = &cbData) {
                if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                    paramType,
                                                    index,
                                                    pvData,
                                                    new IntPtr(pcbData)))
                    checkErr(Marshal.GetLastWin32Error());

                if (cbData > 0) {
                    pvData = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(cbData));

                    if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                        paramType,
                                                        index,
                                                        pvData,
                                                        new IntPtr(pcbData)))
                        checkErr(Marshal.GetLastWin32Error());
                }
            }
        }

        [SecurityCritical]
        internal static unsafe void GetParam (SafeCryptMsgHandle safeCryptMsgHandle,
                                              uint paramType,
                                              uint index,
                                              out byte[] pvData,
                                              out uint cbData) {
            cbData = 0;
            pvData = new byte[0];

            fixed (uint * pcbData = &cbData) {
                if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                    paramType,
                                                    index,
                                                    IntPtr.Zero,
                                                    new IntPtr(pcbData)))
                    checkErr(Marshal.GetLastWin32Error());

                if (cbData > 0) {
                    pvData = new byte[cbData];

                    fixed (byte * ppvData = &pvData[0]) {
                        if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                            paramType,
                                                            index,
                                                            new IntPtr(ppvData),
                                                            new IntPtr(pcbData)))
                            checkErr(Marshal.GetLastWin32Error());
                    }
                }
            }
        }

        [SecurityCritical]
        internal static unsafe X509Certificate2Collection GetCertificates (SafeCryptMsgHandle safeCryptMsgHandle) {
            uint dwCount = 0;
            uint cbCount = (uint) Marshal.SizeOf(typeof(uint));
            X509Certificate2Collection certificates = new X509Certificate2Collection();

            if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                CAPI.CMSG_CERT_COUNT_PARAM,
                                                0,
                                                new IntPtr(&dwCount),
                                                new IntPtr(&cbCount)))
                checkErr(Marshal.GetLastWin32Error());

            for (uint index = 0; index < dwCount; index++) {
                uint cbEncoded = 0;
                SafeLocalAllocHandle pbEncoded = SafeLocalAllocHandle.InvalidHandle;

                GetParam(safeCryptMsgHandle, CAPI.CMSG_CERT_PARAM, index, out pbEncoded, out cbEncoded);
                if (cbEncoded > 0) {
                    SafeCertContextHandle safeCertContextHandle = CAPI.CAPISafe.CertCreateCertificateContext(CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                                                                             pbEncoded,
                                                                                                             cbEncoded);
                    if (safeCertContextHandle == null || safeCertContextHandle.IsInvalid)
                        throw new CryptographicException(Marshal.GetLastWin32Error());

                    certificates.Add(new X509Certificate2(safeCertContextHandle.DangerousGetHandle()));
                    safeCertContextHandle.Dispose();
                }
            }

            return certificates;
        }

        [SecurityCritical]
        internal static byte[] GetContent (SafeCryptMsgHandle safeCryptMsgHandle) {
            uint cbContent = 0;
            byte[] content = new byte[0];

            GetParam(safeCryptMsgHandle, CAPI.CMSG_CONTENT_PARAM, 0, out content, out cbContent);

            return content;
        }

        [SecurityCritical]
        internal static Oid GetContentType (SafeCryptMsgHandle safeCryptMsgHandle) {
            uint cbContentType = 0;
            byte[] contentType = new byte[0];

            GetParam(safeCryptMsgHandle, CAPI.CMSG_INNER_CONTENT_TYPE_PARAM, 0, out contentType, out cbContentType);
            if (contentType.Length > 0 && contentType[contentType.Length - 1] == 0) {
                byte[] temp = new byte[contentType.Length - 1];
                Array.Copy(contentType, 0, temp, 0, temp.Length);
                contentType = temp;
            }
            return new Oid(Encoding.ASCII.GetString(contentType));
        }

        [SecurityCritical]
        internal static byte[] GetMessage (SafeCryptMsgHandle safeCryptMsgHandle) {
            uint cbMessage = 0;
            byte[] message = new byte[0];

            GetParam(safeCryptMsgHandle, CAPI.CMSG_ENCODED_MESSAGE, 0, out message, out cbMessage);
            return message;
        }

        [SecurityCritical]
        internal static unsafe int GetSignerIndex (SafeCryptMsgHandle safeCrytpMsgHandle, SignerInfo signerInfo, int startIndex) {
            uint dwSigners = 0;
            uint cbCount = (uint) Marshal.SizeOf(typeof(uint));

            if (!CAPI.CAPISafe.CryptMsgGetParam(safeCrytpMsgHandle,
                                                CAPI.CMSG_SIGNER_COUNT_PARAM,
                                                0,
                                                new IntPtr(&dwSigners),
                                                new IntPtr(&cbCount)))
                checkErr(Marshal.GetLastWin32Error());

            for (int index = startIndex; index < (int) dwSigners; index++) {
                uint cbCmsgSignerInfo = 0;

                if (!CAPI.CAPISafe.CryptMsgGetParam(safeCrytpMsgHandle,
                                                    CAPI.CMSG_SIGNER_INFO_PARAM,
                                                    (uint)index,
                                                    IntPtr.Zero,
                                                    new IntPtr(&cbCmsgSignerInfo)))
                    checkErr(Marshal.GetLastWin32Error());

                if (cbCmsgSignerInfo > 0) {
                    SafeLocalAllocHandle pbCmsgSignerInfo = CAPI.LocalAlloc(CAPI.LMEM_FIXED, new IntPtr(cbCmsgSignerInfo));

                    if (!CAPI.CAPISafe.CryptMsgGetParam(safeCrytpMsgHandle,
                                                        CAPI.CMSG_SIGNER_INFO_PARAM,
                                                        (uint)index,
                                                        pbCmsgSignerInfo,
                                                        new IntPtr(&cbCmsgSignerInfo)))
                        checkErr(Marshal.GetLastWin32Error());

                    CAPI.CMSG_SIGNER_INFO cmsgSignerInfo1 = signerInfo.GetCmsgSignerInfo();
                    CAPI.CMSG_SIGNER_INFO cmsgSignerInfo2 = (CAPI.CMSG_SIGNER_INFO) Marshal.PtrToStructure(pbCmsgSignerInfo.DangerousGetHandle(), typeof(CAPI.CMSG_SIGNER_INFO));

                    if (X509Utils.MemEqual((byte *) cmsgSignerInfo1.Issuer.pbData,
                                  cmsgSignerInfo1.Issuer.cbData,
                                  (byte *) cmsgSignerInfo2.Issuer.pbData,
                                  cmsgSignerInfo2.Issuer.cbData) &&
                        X509Utils.MemEqual((byte *) cmsgSignerInfo1.SerialNumber.pbData,
                                  cmsgSignerInfo1.SerialNumber.cbData,
                                  (byte *) cmsgSignerInfo2.SerialNumber.pbData,
                                  cmsgSignerInfo2.SerialNumber.cbData)) {
                        return index; // Signer's index is found.
                    }

                    // Keep alive.
                    pbCmsgSignerInfo.Dispose();
                }
            }

            throw new CryptographicException(CAPI.CRYPT_E_SIGNER_NOT_FOUND);
        }

        [SecurityCritical]
        internal static unsafe CryptographicAttributeObjectCollection GetUnprotectedAttributes (SafeCryptMsgHandle safeCryptMsgHandle) {
            uint cbUnprotectedAttr = 0;
            CryptographicAttributeObjectCollection attributes = new CryptographicAttributeObjectCollection();
            SafeLocalAllocHandle pbUnprotectedAttr = SafeLocalAllocHandle.InvalidHandle;
            if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                CAPI.CMSG_UNPROTECTED_ATTR_PARAM,
                                                0,
                                                pbUnprotectedAttr,
                                                new IntPtr(&cbUnprotectedAttr))) {
                int lastWin32Error = Marshal.GetLastWin32Error();
                if (lastWin32Error != CAPI.CRYPT_E_ATTRIBUTES_MISSING)
                    checkErr(Marshal.GetLastWin32Error());
            }

            if (cbUnprotectedAttr > 0) {
                using (pbUnprotectedAttr = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(cbUnprotectedAttr))) {
                    if (!CAPI.CAPISafe.CryptMsgGetParam(safeCryptMsgHandle,
                                                        CAPI.CMSG_UNPROTECTED_ATTR_PARAM,
                                                        0,
                                                        pbUnprotectedAttr,
                                                        new IntPtr(&cbUnprotectedAttr)))
                        checkErr(Marshal.GetLastWin32Error());

                    attributes = new CryptographicAttributeObjectCollection(pbUnprotectedAttr);
                }
            }
            return attributes;
        }

        [SecurityCritical]
        internal unsafe static X509IssuerSerial DecodeIssuerSerial (CAPI.CERT_ISSUER_SERIAL_NUMBER pIssuerAndSerial) {
            SafeLocalAllocHandle ptr = SafeLocalAllocHandle.InvalidHandle;
            uint cbSize = CAPI.CAPISafe.CertNameToStrW(CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                       new IntPtr(&pIssuerAndSerial.Issuer),
                                                       CAPI.CERT_X500_NAME_STR | CAPI.CERT_NAME_STR_REVERSE_FLAG,
                                                       ptr,
                                                       0);
            if (cbSize <= 1) // The API actually return 1 when It fails; which is not what the documentation says.
                throw new CryptographicException(Marshal.GetLastWin32Error());

            ptr = CAPI.LocalAlloc(CAPI.LMEM_FIXED, new IntPtr(checked(2 * cbSize)));
            cbSize = CAPI.CAPISafe.CertNameToStrW(CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                  new IntPtr(&pIssuerAndSerial.Issuer),
                                                  CAPI.CERT_X500_NAME_STR | CAPI.CERT_NAME_STR_REVERSE_FLAG,
                                                  ptr,
                                                  cbSize);
            if (cbSize <= 1)
                throw new CryptographicException(Marshal.GetLastWin32Error());

            X509IssuerSerial issuerSerial = new X509IssuerSerial();
            issuerSerial.IssuerName = Marshal.PtrToStringUni(ptr.DangerousGetHandle());
            byte[] serial = new byte[pIssuerAndSerial.SerialNumber.cbData];
            Marshal.Copy(pIssuerAndSerial.SerialNumber.pbData, serial, 0, serial.Length);
            issuerSerial.SerialNumber = X509Utils.EncodeHexStringFromInt(serial);

            ptr.Dispose();
            return issuerSerial;
        }

        [SecuritySafeCritical]
        internal static string DecodeOctetString (byte[] encodedOctetString) {
            uint cbDecoded = 0;
            SafeLocalAllocHandle pbDecoded = null;

            if (!CAPI.DecodeObject(new IntPtr(CAPI.X509_OCTET_STRING),
                                   encodedOctetString,
                                   out pbDecoded,
                                   out cbDecoded))
                throw new CryptographicException(Marshal.GetLastWin32Error());

            if (cbDecoded == 0)
                return String.Empty;

            CAPI.CRYPTOAPI_BLOB decodedBlob = (CAPI.CRYPTOAPI_BLOB) Marshal.PtrToStructure(pbDecoded.DangerousGetHandle(), typeof(CAPI.CRYPTOAPI_BLOB));
            if (decodedBlob.cbData == 0)
                return String.Empty;
            string octetString = Marshal.PtrToStringUni(decodedBlob.pbData);
            pbDecoded.Dispose();

            return octetString;
        }

        [SecuritySafeCritical]
        internal static byte[] DecodeOctetBytes (byte[] encodedOctetString) {
            uint cbDecoded = 0;

            SafeLocalAllocHandle pbDecoded = null;
            if (!CAPI.DecodeObject(new IntPtr(CAPI.X509_OCTET_STRING),
                                   encodedOctetString,
                                   out pbDecoded,
                                   out cbDecoded))
                throw new CryptographicException(Marshal.GetLastWin32Error());

            if (cbDecoded == 0)
                return new byte[0];

            using (pbDecoded) {
                return CAPI.BlobToByteArray(pbDecoded.DangerousGetHandle());
            }
        }

        internal static byte[] EncodeOctetString (string octetString) {
            // Marshal data to be encoded to unmanaged memory.
            byte[] octets = new byte[2 * (octetString.Length + 1)];
            Encoding.Unicode.GetBytes(octetString, 0, octetString.Length, octets, 0);
            return EncodeOctetString(octets);
        }

        [SecuritySafeCritical]
        internal static unsafe byte[] EncodeOctetString (byte[] octets) {
            fixed (byte * pbOctets = octets) {
                CAPI.CRYPTOAPI_BLOB octetsBlob = new CAPI.CRYPTOAPI_BLOB();
                octetsBlob.cbData = (uint) octets.Length;
                octetsBlob.pbData = new IntPtr(pbOctets);

                // Encode data.
                byte[] encodedOctets = new byte[0];
                if (!CAPI.EncodeObject(new IntPtr((long) CAPI.X509_OCTET_STRING),
                                       new IntPtr((long) &octetsBlob),
                                       out encodedOctets)) {
                    throw new CryptographicException(Marshal.GetLastWin32Error());
                }
                return encodedOctets;
            }
        }

        internal static string DecodeObjectIdentifier (byte[] encodedObjId, int offset) {
            StringBuilder objId = new StringBuilder("");
            if (0 < (encodedObjId.Length - offset)) {
                byte b = encodedObjId[offset];
                byte c = (byte) ((uint) b / 40);
                objId.Append(c.ToString(null, null));
                objId.Append(".");
                c = (byte) ((uint) b % 40);
                objId.Append(c.ToString(null, null));

                ulong s = 0;
                for (int index = offset + 1; index < encodedObjId.Length; index++) {
                    c = encodedObjId[index];
                    s = (s << 7) + (ulong) (c & 0x7f);
                    if (0 == (c & 0x80)) {
                        objId.Append(".");
                        objId.Append(s.ToString(null, null));
                        s = 0;
                    }
                }

                // s should be 0 at this point, otherwise we have a bad ASN.
                if (0 != s) {
                    throw new CryptographicException(CAPI.CRYPT_E_BAD_ENCODE);
                }
            }

            return objId.ToString();
        }

        internal static CmsRecipientCollection SelectRecipients (SubjectIdentifierType recipientIdentifierType) {
            X509Store store = new X509Store("AddressBook");
            store.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly);

            X509Certificate2Collection certificates = new X509Certificate2Collection(store.Certificates);

            foreach (X509Certificate2 certificate in store.Certificates) {
                if (certificate.NotBefore <= DateTime.Now && certificate.NotAfter >= DateTime.Now) {
                    bool validUsages = true;
                    foreach (X509Extension extension in certificate.Extensions) {
                        if (String.Compare(extension.Oid.Value, CAPI.szOID_KEY_USAGE, StringComparison.OrdinalIgnoreCase) == 0) {
                            X509KeyUsageExtension keyUsage = new X509KeyUsageExtension();
                            keyUsage.CopyFrom(extension);
                            if ((keyUsage.KeyUsages & X509KeyUsageFlags.KeyEncipherment) == 0 &&
                                (keyUsage.KeyUsages & X509KeyUsageFlags.KeyAgreement) == 0) {
                                validUsages = false;
                            }
                            break;
                        }
                    }

                    if (validUsages) {
                        certificates.Add(certificate);
                    }
                }
            }

            if (certificates.Count < 1)
                throw new CryptographicException(CAPI.CRYPT_E_RECIPIENT_NOT_FOUND);

            X509Certificate2Collection recipients = X509Certificate2UI.SelectFromCollection(certificates, null, null, X509SelectionFlag.MultiSelection);
            if (recipients.Count < 1) 
                throw new CryptographicException(CAPI.ERROR_CANCELLED);

            return new CmsRecipientCollection(recipientIdentifierType, recipients);
        }

        internal static X509Certificate2 SelectSignerCertificate () {
            X509Store store = new X509Store();
            store.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly | OpenFlags.IncludeArchived);

            X509Certificate2Collection certificates = new X509Certificate2Collection();

            foreach (X509Certificate2 certificate in store.Certificates) {
                if (certificate.HasPrivateKey && certificate.NotBefore <= DateTime.Now && certificate.NotAfter >= DateTime.Now) {
                    bool validUsages = true;
                    foreach (X509Extension extension in certificate.Extensions) {
                        if (String.Compare(extension.Oid.Value, CAPI.szOID_KEY_USAGE, StringComparison.OrdinalIgnoreCase) == 0) {
                            X509KeyUsageExtension keyUsage = new X509KeyUsageExtension();
                            keyUsage.CopyFrom(extension);
                            if ((keyUsage.KeyUsages & X509KeyUsageFlags.DigitalSignature) == 0 &&
                                (keyUsage.KeyUsages & X509KeyUsageFlags.NonRepudiation) == 0) {
                                validUsages = false;
                            }
                            break;
                        }
                    }

                    if (validUsages) {
                        certificates.Add(certificate);
                    }
                }
            }

            if (certificates.Count < 1)
                throw new CryptographicException(CAPI.CRYPT_E_SIGNER_NOT_FOUND);

            certificates = X509Certificate2UI.SelectFromCollection(certificates, null, null, X509SelectionFlag.SingleSelection);
            if (certificates.Count < 1) 
                throw new CryptographicException(CAPI.ERROR_CANCELLED);

            Debug.Assert(certificates.Count == 1);

            return certificates[0];
        }

        [SecuritySafeCritical]
        internal static AsnEncodedDataCollection GetAsnEncodedDataCollection (CAPI.CRYPT_ATTRIBUTE cryptAttribute) {
            AsnEncodedDataCollection list = new AsnEncodedDataCollection();
            Oid oid = new Oid(cryptAttribute.pszObjId);
            string szOid = oid.Value;

            for (uint index = 0; index < cryptAttribute.cValue; index++) {
                checked {
                    IntPtr pAttributeBlob = new IntPtr((long)cryptAttribute.rgValue + (index * Marshal.SizeOf(typeof(CAPI.CRYPTOAPI_BLOB))));
                    Pkcs9AttributeObject attribute = new Pkcs9AttributeObject(oid, CAPI.BlobToByteArray(pAttributeBlob));
                    Pkcs9AttributeObject customAttribute = CryptoConfig.CreateFromName(szOid) as Pkcs9AttributeObject;
                    if (customAttribute != null) {
                        customAttribute.CopyFrom(attribute);
                        attribute = customAttribute;
                    }
                    list.Add(attribute);
                }
            }
            return list;
        }

        [SecurityCritical]
        internal static AsnEncodedDataCollection GetAsnEncodedDataCollection (CAPI.CRYPT_ATTRIBUTE_TYPE_VALUE cryptAttribute) {
            AsnEncodedDataCollection list = new AsnEncodedDataCollection();
            list.Add(new Pkcs9AttributeObject(new Oid(cryptAttribute.pszObjId), CAPI.BlobToByteArray(cryptAttribute.Value)));
            return list;
        }

        [SecurityCritical]
        internal static unsafe IntPtr CreateCryptAttributes (CryptographicAttributeObjectCollection attributes) {
            attributes = attributes.DeepCopy();

            // NULL if no attribute.
            if (attributes.Count == 0)
                return IntPtr.Zero;

            //
            // The goal here is to compute the size needed for the attributes we are passing to CMSG_SIGNER_ENCODE_INFO
            // The unmanaged memory structure we are creating here has the following layout:
            //
            // Let cAttr = number of attributes.
            //
            // This to create the array of CRYPT_ATTRIBUTE
            // for i = 0 to cAttr {
            //     CRYPT_ATTRRIBUTE[i]                           // pszObjId | cValue | rgValue
            // }
            //
            // This is to fill in the data for each entry of CRYPT_ATTRIBUTE array above.
            // for i = 0 to cAttr {
            //     objId[i]                                      // Value of the Oid, i.e "1.2.3.4"
            //     for j = 0 to CRYPT_ATTRIBUTE[i].cValue - 1 {  // Array of CRYPTOAPI_BLOB
            //        CRYPT_ATTRIBUTE[i].rgValue[j].cbData       // Data size
            //        CRYPT_ATTRIBUTE[i].rgValue[j].pbData       // Pointer to data
            //     }
            //     for j = 0 to CRYPT_ATTRIBUTE[i].cValue - 1 {  // Data for each entry of the CRYPTOAPI_BLOB array above.
            //        *CRYPT_ATTRIBUTE[i].rgValue[j].pbData      // The actual data
            //    }
            // }

            checked {
                uint totalLength = 0;
                uint cryptAttrSize = AlignedLength((uint) Marshal.SizeOf(typeof(I_CRYPT_ATTRIBUTE)));
                uint cryptBlobSize = AlignedLength((uint) Marshal.SizeOf(typeof(CAPI.CRYPTOAPI_BLOB)));

                // First compute the total serialized unmanaged memory size needed.
                // For each attribute, we add the CRYPT_ATTRIBUTE size, the size
                // needed for the ObjId, and the size needed for all the values
                // inside each attribute which is computed in inner loop.
                foreach (CryptographicAttributeObject attribute in attributes) {
                    totalLength += cryptAttrSize;  // sizeof(CRYPT_ATTRIBUTE)
                    totalLength += AlignedLength((uint) (attribute.Oid.Value.Length + 1));  // strlen(pszObjId) + 1

                    // For each value within the attribute, we add the CRYPT_ATTR_BLOB size and 
                    // the actual size needed for the data.
                    foreach (AsnEncodedData attributeValue in attribute.Values) {
                        totalLength += cryptBlobSize;   // Add CRYPT_ATTR_BLOB size
                        totalLength += AlignedLength((uint) attributeValue.RawData.Length); // Data size
                    }
                }

                // Allocate the unmanaged memory blob to hold the entire serialized CRYPT_ATTRIBUTE array.
                SafeLocalAllocHandle pCryptAttributes = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(totalLength));

                // Now fill up unmanaged memory with data from the managed side.
                I_CRYPT_ATTRIBUTE * pCryptAttribute = (I_CRYPT_ATTRIBUTE *) pCryptAttributes.DangerousGetHandle();
                IntPtr pAttrData = new IntPtr((long) pCryptAttributes.DangerousGetHandle() + (cryptAttrSize * attributes.Count));

                foreach (CryptographicAttributeObject attribute in attributes) {
                    byte * pszObjId = (byte *) pAttrData;
                    byte[] objId = new byte[attribute.Oid.Value.Length + 1];
                    CAPI.CRYPTOAPI_BLOB * pDataBlob = (CAPI.CRYPTOAPI_BLOB *) (pszObjId + AlignedLength((uint) objId.Length));

                    // CRYPT_ATTRIBUTE.pszObjId
                    pCryptAttribute->pszObjId = (IntPtr) pszObjId;

                    // CRYPT_ATTRIBUTE.cValue
                    pCryptAttribute->cValue = (uint) attribute.Values.Count;

                    // CRYPT_ATTRIBUTE.rgValue
                    pCryptAttribute->rgValue = (IntPtr) pDataBlob;

                    // ObjId - The actual dotted value of the OID.
                    Encoding.ASCII.GetBytes(attribute.Oid.Value, 0, attribute.Oid.Value.Length, objId, 0);
                    Marshal.Copy(objId, 0, pCryptAttribute->pszObjId, objId.Length);

                    // cValue of CRYPT_ATTR_BLOBs followed by cValue of actual data.
                    IntPtr pbEncodedData = new IntPtr((long) pDataBlob + (attribute.Values.Count * cryptBlobSize));
                    foreach (AsnEncodedData value in attribute.Values) {
                        // Retrieve encoded data.
                        byte[] encodedData = value.RawData;

                        // Write data
                        if (encodedData.Length > 0) {
                            // CRYPT_ATTR_BLOB.cbData
                            pDataBlob->cbData = (uint) encodedData.Length;

                            // CRYPT_ATTR_BLOB.pbData
                            pDataBlob->pbData = pbEncodedData;

                            Marshal.Copy(encodedData, 0, pbEncodedData, encodedData.Length);
                            pbEncodedData = new IntPtr((long) pbEncodedData + AlignedLength((uint) encodedData.Length));
                        }

                        // Advance pointer.
                        pDataBlob++;
                    }

                    // Advance pointers.
                    pCryptAttribute++;
                    pAttrData = pbEncodedData;
                }

                // Since we are returning IntPtr, we MUST supress finalizer, otherwise
                // the GC can collect the memory underneath us!!!
                GC.SuppressFinalize(pCryptAttributes);

                return pCryptAttributes.DangerousGetHandle();
            }
        }

        [SecuritySafeCritical]
        internal static CAPI.CMSG_SIGNER_ENCODE_INFO CreateSignerEncodeInfo (CmsSigner signer, out SafeCryptProvHandle hProv) {
            return CreateSignerEncodeInfo(signer, false, out hProv);
        }

        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)]
        [SecuritySafeCritical]
        internal static unsafe CAPI.CMSG_SIGNER_ENCODE_INFO CreateSignerEncodeInfo (CmsSigner signer, bool silent, out SafeCryptProvHandle hProv) {
            CAPI.CMSG_SIGNER_ENCODE_INFO cmsSignerEncodeInfo = new CAPI.CMSG_SIGNER_ENCODE_INFO(Marshal.SizeOf(typeof(CAPI.CMSG_SIGNER_ENCODE_INFO)));

            SafeCryptProvHandle safeCryptProvHandle = SafeCryptProvHandle.InvalidHandle;
            uint keySpec = 0;

            cmsSignerEncodeInfo.HashAlgorithm.pszObjId = signer.DigestAlgorithm.Value;

            if (0 == String.Compare(
                signer.Certificate.PublicKey.Oid.Value,
                CAPI.szOID_X957_DSA,
                StringComparison.Ordinal))
            {
                cmsSignerEncodeInfo.HashEncryptionAlgorithm.pszObjId = CAPI.szOID_X957_sha1DSA;
            }

            cmsSignerEncodeInfo.cAuthAttr = (uint) signer.SignedAttributes.Count;
            cmsSignerEncodeInfo.rgAuthAttr = CreateCryptAttributes(signer.SignedAttributes);

            cmsSignerEncodeInfo.cUnauthAttr = (uint) signer.UnsignedAttributes.Count;
            cmsSignerEncodeInfo.rgUnauthAttr = CreateCryptAttributes(signer.UnsignedAttributes);

            if (signer.SignerIdentifierType == SubjectIdentifierType.NoSignature) {
                cmsSignerEncodeInfo.HashEncryptionAlgorithm.pszObjId = CAPI.szOID_PKIX_NO_SIGNATURE;
                cmsSignerEncodeInfo.pCertInfo  = IntPtr.Zero;
                cmsSignerEncodeInfo.dwKeySpec  = keySpec;

                //  If the HashEncryptionAlgorithm is set to szOID_PKIX_NO_SIGNATURE, then,
                //  the signature value only contains the hash octets. hCryptProv must still
                //  be specified. However, since a private key isn't used the hCryptProv can be
                //  acquired using CRYPT_VERIFYCONTEXT.
                if (!CAPI.CryptAcquireContext(ref safeCryptProvHandle,
                                              null,
                                              null,
                                              CAPI.PROV_RSA_FULL,
                                                CAPI.CRYPT_VERIFYCONTEXT)) {
                        throw new CryptographicException(Marshal.GetLastWin32Error());
                }

                cmsSignerEncodeInfo.hCryptProv = safeCryptProvHandle.DangerousGetHandle();
                hProv = safeCryptProvHandle;

                // Fake up the SignerId so our server can recognize it
                // dwIdChoice
                cmsSignerEncodeInfo.SignerId.dwIdChoice = CAPI.CERT_ID_ISSUER_SERIAL_NUMBER;

                // Issuer
                X500DistinguishedName dummyName = new X500DistinguishedName(CAPI.DummySignerCommonName);
                dummyName.Oid = Oid.FromOidValue(CAPI.szOID_RDN_DUMMY_SIGNER, OidGroup.ExtensionOrAttribute);
                cmsSignerEncodeInfo.SignerId.Value.IssuerSerialNumber.Issuer.cbData = (uint)dummyName.RawData.Length;
                SafeLocalAllocHandle pbDataIssuer = 
                    CAPI.LocalAlloc(CAPI.LPTR, 
                                    new IntPtr(cmsSignerEncodeInfo.SignerId.Value.IssuerSerialNumber.Issuer.cbData));
                Marshal.Copy(dummyName.RawData, 0, pbDataIssuer.DangerousGetHandle(), dummyName.RawData.Length);
                cmsSignerEncodeInfo.SignerId.Value.IssuerSerialNumber.Issuer.pbData = pbDataIssuer.DangerousGetHandle();
                GC.SuppressFinalize(pbDataIssuer);

                // SerialNumber
                cmsSignerEncodeInfo.SignerId.Value.IssuerSerialNumber.SerialNumber.cbData = (uint)1;
                SafeLocalAllocHandle pbDataSerialNumber = 
                        CAPI.LocalAlloc(CAPI.LPTR, 
                                        new IntPtr(cmsSignerEncodeInfo.SignerId.Value.IssuerSerialNumber.SerialNumber.cbData));
                byte * pSerialNumber = (byte *)pbDataSerialNumber.DangerousGetHandle();
                *pSerialNumber = 0x00;
                cmsSignerEncodeInfo.SignerId.Value.IssuerSerialNumber.SerialNumber.pbData = 
                    pbDataSerialNumber.DangerousGetHandle();
                GC.SuppressFinalize(pbDataSerialNumber);

                return cmsSignerEncodeInfo;
            }

            SafeCertContextHandle safeCertContextHandle = X509Utils.GetCertContext(signer.Certificate);
            int hr = GetCertPrivateKey(safeCertContextHandle, out safeCryptProvHandle, out keySpec);

            if (hr != CAPI.S_OK)
                throw new CryptographicException(hr);

            cmsSignerEncodeInfo.dwKeySpec = keySpec;
            cmsSignerEncodeInfo.hCryptProv = safeCryptProvHandle.DangerousGetHandle();
            hProv = safeCryptProvHandle;

            CAPI.CERT_CONTEXT pCertContext = *((CAPI.CERT_CONTEXT*) safeCertContextHandle.DangerousGetHandle());
            cmsSignerEncodeInfo.pCertInfo  = pCertContext.pCertInfo;

            // If CMS, then fill in the Subject Key Identifier (SKI) or SignerId.
            if (signer.SignerIdentifierType == SubjectIdentifierType.SubjectKeyIdentifier) {
                uint cbData = 0;
                SafeLocalAllocHandle pbData = SafeLocalAllocHandle.InvalidHandle;
                if (!CAPI.CAPISafe.CertGetCertificateContextProperty(safeCertContextHandle,
                                                                     CAPI.CERT_KEY_IDENTIFIER_PROP_ID,
                                                                     pbData,
                                                                     ref cbData))
                    throw new CryptographicException(Marshal.GetLastWin32Error());

                if (cbData > 0) {
                    pbData = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(cbData));

                    if (!CAPI.CAPISafe.CertGetCertificateContextProperty(safeCertContextHandle,
                                                                         CAPI.CERT_KEY_IDENTIFIER_PROP_ID,
                                                                         pbData,
                                                                         ref cbData))
                        throw new CryptographicException(Marshal.GetLastWin32Error());

                    cmsSignerEncodeInfo.SignerId.dwIdChoice = CAPI.CERT_ID_KEY_IDENTIFIER;
                    cmsSignerEncodeInfo.SignerId.Value.KeyId.cbData = cbData;
                    cmsSignerEncodeInfo.SignerId.Value.KeyId.pbData = pbData.DangerousGetHandle();
                    
                    // Since we are storing only IntPtr in CMSG_SIGNER_ENCODE_INFO.SignerId.KeyId.pbData,
                    // we MUST supress finalizer, otherwise the GC can collect the resource underneath us!!!
                    GC.SuppressFinalize(pbData);
                }
            }

            return cmsSignerEncodeInfo;
        }

        [SecurityCritical]
        internal static int GetCertPrivateKey(SafeCertContextHandle safeCertContextHandle, out SafeCryptProvHandle safeCryptProvHandle, out uint keySpec) {
            bool freeCsp = false;

            IntPtr hKey;
            uint cbSize = (uint)IntPtr.Size;
            safeCryptProvHandle = null;

            if (CAPI.CAPISafe.CertGetCertificateContextProperty(
                safeCertContextHandle,
                CAPI.CERT_NCRYPT_KEY_HANDLE_PROP_ID,
                out hKey,
                ref cbSize))
            {
                keySpec = 0;
                safeCryptProvHandle = new SafeCryptProvHandle(hKey, safeCertContextHandle);
                return CAPI.S_OK;
            }

            // Check to see if KEY_PROV_INFO contains "MS Base ..."
            // If so, acquire "MS Enhanced..." or "MS Strong".
            // if failed, then use CryptAcquireCertificatePrivateKey
            CspParameters parameters = new CspParameters();
            if (X509Utils.GetPrivateKeyInfo(safeCertContextHandle, ref parameters) == false)
                throw new CryptographicException(Marshal.GetLastWin32Error());

            if (String.Compare(parameters.ProviderName, CAPI.MS_DEF_PROV, StringComparison.OrdinalIgnoreCase) == 0)
            {
                SafeCryptProvHandle provHandle = SafeCryptProvHandle.InvalidHandle;

                if (CAPI.CryptAcquireContext(ref provHandle, parameters.KeyContainerName, CAPI.MS_ENHANCED_PROV, CAPI.PROV_RSA_FULL, 0) ||
                    CAPI.CryptAcquireContext(ref provHandle, parameters.KeyContainerName, CAPI.MS_STRONG_PROV, CAPI.PROV_RSA_FULL, 0))
                {
                    safeCryptProvHandle = provHandle;
                }
            }

            keySpec = (uint)parameters.KeyNumber;
            int hr = CAPI.S_OK;

            uint flags = CAPI.CRYPT_ACQUIRE_COMPARE_KEY_FLAG | CAPI.CRYPT_ACQUIRE_USE_PROV_INFO_FLAG;
            if (parameters.ProviderType == 0)
            {
                //
                // The ProviderType being 0 indicates that this cert is using a CNG key. Set the flag to tell CryptAcquireCertificatePrivateKey that it's okay to give
                // us a CNG key.
                //
                // (This should be equivalent to passing CRYPT_ACQUIRE_ALLOW_NCRYPT_KEY_FLAG. But fixing it this way restricts the code path changes
                // within Crypt32 to the cases that were already non-functional in 4.6.1. Thus, it is a "safer" way to fix it for a point release.)
                //
                flags |= CAPI.CRYPT_ACQUIRE_PREFER_NCRYPT_KEY_FLAG;
            }

            if ((safeCryptProvHandle == null) || (safeCryptProvHandle.IsInvalid))
            {
                hKey = IntPtr.Zero;

                if (CAPI.CAPISafe.CryptAcquireCertificatePrivateKey(safeCertContextHandle,
                                                                    flags,
                                                                    IntPtr.Zero,
                                                                    ref hKey,
                                                                    ref keySpec,
                                                                    ref freeCsp))
                {
                    safeCryptProvHandle = new SafeCryptProvHandle(hKey, freeCsp);
                }
                else
                {
                    hr = Marshal.GetHRForLastWin32Error();
                }
            }

            return hr;
        }

        [SecuritySafeCritical]
        internal static X509Certificate2Collection CreateBagOfCertificates (CmsSigner signer) {
            X509Certificate2Collection certificates = new X509Certificate2Collection();

            //
            // First add extra bag of certs.
            //

            certificates.AddRange(signer.Certificates);

            //
            // Then include chain option.
            //

            if (signer.IncludeOption != X509IncludeOption.None) {
                if (signer.IncludeOption == X509IncludeOption.EndCertOnly) {
                    certificates.Add(signer.Certificate);
                }
                else {
                    int cCerts = 1;
                    X509Chain chain = new X509Chain();
                    chain.Build(signer.Certificate);

                    // Can't honor the option if we only have a partial chain.
                    if ((chain.ChainStatus.Length > 0) && 
                        ((chain.ChainStatus[0].Status & X509ChainStatusFlags.PartialChain) == X509ChainStatusFlags.PartialChain))
                        throw new CryptographicException(CAPI.CERT_E_CHAINING);

                    if (signer.IncludeOption == X509IncludeOption.WholeChain) {
                        cCerts = chain.ChainElements.Count;
                    }
                    else {
                        // Default to ExcludeRoot.
                        if (chain.ChainElements.Count > 1) {
                            cCerts = chain.ChainElements.Count - 1;
                        }
                    }

                    for (int i = 0; i < cCerts; i++) {
                        certificates.Add(chain.ChainElements[i].Certificate);
                    }
                }
            }

            return certificates;
        }

        [SecurityCritical]
        internal static unsafe SafeLocalAllocHandle CreateEncodedCertBlob (X509Certificate2Collection certificates) {
            SafeLocalAllocHandle certBlob = SafeLocalAllocHandle.InvalidHandle;

            certificates = new X509Certificate2Collection(certificates);

            if (certificates.Count > 0) {
                checked {
                    certBlob = CAPI.LocalAlloc(CAPI.LMEM_FIXED, new IntPtr(certificates.Count * Marshal.SizeOf(typeof(CAPI.CRYPTOAPI_BLOB))));
                    CAPI.CRYPTOAPI_BLOB * pCertBlob = (CAPI.CRYPTOAPI_BLOB * ) certBlob.DangerousGetHandle();

                    foreach (X509Certificate2 certificate in certificates) {
                        SafeCertContextHandle safeCertContextHandle = X509Utils.GetCertContext(certificate);
                        CAPI.CERT_CONTEXT pCertContext = *((CAPI.CERT_CONTEXT*) safeCertContextHandle.DangerousGetHandle());

                        pCertBlob->cbData = pCertContext.cbCertEncoded;
                        pCertBlob->pbData = pCertContext.pbCertEncoded;
                        pCertBlob++;
                    }
                }
            }

            return certBlob;
        }

        [SecuritySafeCritical]
        internal static unsafe uint AddCertsToMessage (SafeCryptMsgHandle safeCryptMsgHandle, X509Certificate2Collection bagOfCerts, X509Certificate2Collection chainOfCerts) {
            uint certsAdded = 0;

            foreach (X509Certificate2 certificate in chainOfCerts) {
                // Skip it if already in the bag of certs.
                X509Certificate2Collection foundCerts = bagOfCerts.Find(X509FindType.FindByThumbprint, certificate.Thumbprint, false);
                if (foundCerts.Count == 0) {
                    SafeCertContextHandle safeCertContextHandle = X509Utils.GetCertContext(certificate);
                    CAPI.CERT_CONTEXT pCertContext = *((CAPI.CERT_CONTEXT*) safeCertContextHandle.DangerousGetHandle());

                    CAPI.CRYPTOAPI_BLOB certBlob = new CAPI.CRYPTOAPI_BLOB();
                    certBlob.cbData = pCertContext.cbCertEncoded;
                    certBlob.pbData = pCertContext.pbCertEncoded;

                    if (!CAPI.CryptMsgControl(safeCryptMsgHandle,
                                              0,
                                              CAPI.CMSG_CTRL_ADD_CERT,
                                              new IntPtr((long) &certBlob)))
                        throw new CryptographicException(Marshal.GetLastWin32Error());
                    certsAdded++;
                }
            }

            return certsAdded;
        }

        internal static X509Certificate2 FindCertificate (SubjectIdentifier identifier, X509Certificate2Collection certificates) {
            X509Certificate2 certificate = null;

            if (certificates != null && certificates.Count > 0) {
                X509Certificate2Collection filters;
                switch (identifier.Type) {
                case SubjectIdentifierType.IssuerAndSerialNumber:
                    filters = certificates.Find(X509FindType.FindByIssuerDistinguishedName, ((X509IssuerSerial) identifier.Value).IssuerName, false);
                    if (filters.Count > 0) {
                        filters = filters.Find(X509FindType.FindBySerialNumber, ((X509IssuerSerial) identifier.Value).SerialNumber, false);
                        if (filters.Count > 0)
                            certificate = filters[0];
                    }
                    break;
                case SubjectIdentifierType.SubjectKeyIdentifier:
                    filters = certificates.Find(X509FindType.FindBySubjectKeyIdentifier, identifier.Value, false);
                    if (filters.Count > 0)
                        certificate = filters[0];

                    break;
                }
            }

            return certificate;
        }

        private static void checkErr (int err) {
            if (CAPI.CRYPT_E_INVALID_MSG_TYPE != err)
                throw new CryptographicException(err);
        }

        // for Key ID signing only
        [SecuritySafeCritical]
        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)]
        internal static unsafe X509Certificate2 CreateDummyCertificate (CspParameters parameters) {
            SafeCertContextHandle handle = SafeCertContextHandle.InvalidHandle;

            // hProv
            SafeCryptProvHandle hProv = SafeCryptProvHandle.InvalidHandle;
            UInt32 dwFlags = 0;
            if (0 != (parameters.Flags & CspProviderFlags.UseMachineKeyStore))
            {
                dwFlags |= CAPI.CRYPT_MACHINE_KEYSET;
            }
            if (0 != (parameters.Flags & CspProviderFlags.UseDefaultKeyContainer))
            {
                dwFlags |= CAPI.CRYPT_VERIFYCONTEXT;
            }
            if (0 != (parameters.Flags & CspProviderFlags.NoPrompt))
            {
                dwFlags |= CAPI.CRYPT_SILENT;
            }
            bool rc = CAPI.CryptAcquireContext(ref hProv,
                                               parameters.KeyContainerName,
                                               parameters.ProviderName,
                                               (uint)parameters.ProviderType,
                                               dwFlags);
            if (!rc)
                throw new CryptographicException(Marshal.GetLastWin32Error());

            // pKeyProvInfo
            CAPI.CRYPT_KEY_PROV_INFO KeyProvInfo = new CAPI.CRYPT_KEY_PROV_INFO();
            KeyProvInfo.pwszProvName       = parameters.ProviderName;
            KeyProvInfo.pwszContainerName  = parameters.KeyContainerName;
            KeyProvInfo.dwProvType         = (uint)parameters.ProviderType;
            KeyProvInfo.dwKeySpec          = (uint)parameters.KeyNumber ;
            KeyProvInfo.dwFlags            = (uint)((parameters.Flags & CspProviderFlags.UseMachineKeyStore) == CspProviderFlags.UseMachineKeyStore ? CAPI.CRYPT_MACHINE_KEYSET : 0);

            SafeLocalAllocHandle pKeyProvInfo = CAPI.LocalAlloc(CAPI.LPTR, 
                                                                new IntPtr(Marshal.SizeOf(typeof(CAPI.CRYPT_KEY_PROV_INFO))));
            Marshal.StructureToPtr(KeyProvInfo, pKeyProvInfo.DangerousGetHandle(), false);

            // Signature
            CAPI.CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm = new CAPI.CRYPT_ALGORITHM_IDENTIFIER();
            SignatureAlgorithm.pszObjId = CAPI.szOID_OIWSEC_sha1RSASign;

            SafeLocalAllocHandle pSignatureAlgorithm = CAPI.LocalAlloc(CAPI.LPTR, 
                                                                new IntPtr( Marshal.SizeOf(typeof(CAPI.CRYPT_ALGORITHM_IDENTIFIER))));
            Marshal.StructureToPtr(SignatureAlgorithm, pSignatureAlgorithm.DangerousGetHandle(), false);

            // pSubjectIssuerBlob
            X500DistinguishedName subjectName = new X500DistinguishedName("cn=CMS Signer Dummy Certificate");
            fixed (byte * pbOctets = subjectName.RawData) {
                CAPI.CRYPTOAPI_BLOB SubjectIssuerBlob = new CAPI.CRYPTOAPI_BLOB();
                SubjectIssuerBlob.cbData = (uint)subjectName.RawData.Length;
                SubjectIssuerBlob.pbData = new IntPtr(pbOctets);

                handle = CAPI.CAPIUnsafe.CertCreateSelfSignCertificate(hProv,
                                                                       new IntPtr(&SubjectIssuerBlob),
                                                                       1,
                                                                       pKeyProvInfo.DangerousGetHandle(),
                                                                       pSignatureAlgorithm.DangerousGetHandle(),
                                                                       IntPtr.Zero,  //StartTime
                                                                       IntPtr.Zero,  //EndTime
                                                                       IntPtr.Zero); //Extensions
            }

            Marshal.DestroyStructure(pKeyProvInfo.DangerousGetHandle(), typeof(CAPI.CRYPT_KEY_PROV_INFO));
            pKeyProvInfo.Dispose();
            Marshal.DestroyStructure(pSignatureAlgorithm.DangerousGetHandle(), typeof(CAPI.CRYPT_ALGORITHM_IDENTIFIER));
            pSignatureAlgorithm.Dispose();

            if (handle == null || handle.IsInvalid)
                throw new CryptographicException(Marshal.GetLastWin32Error());

            X509Certificate2 certificate = new X509Certificate2(handle.DangerousGetHandle());
            handle.Dispose();
            return certificate;
        }
    }
}
