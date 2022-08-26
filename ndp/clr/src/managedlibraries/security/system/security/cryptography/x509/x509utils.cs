// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// X509Utils.cs
//

namespace System.Security.Cryptography.X509Certificates {
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;
    using System.Security.Cryptography.Xml;
    using System.Security.Permissions;
    using System.Text;

    using _FILETIME = System.Runtime.InteropServices.ComTypes.FILETIME;

    internal class X509Utils {
        private X509Utils () {}

        // this method maps X509RevocationFlag to crypto API flags.
        internal static uint MapRevocationFlags (X509RevocationMode revocationMode, X509RevocationFlag revocationFlag) {
            uint dwFlags = 0;
            if (revocationMode == X509RevocationMode.NoCheck)
                return dwFlags;

            if (revocationMode == X509RevocationMode.Offline)
                dwFlags |= CAPI.CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY;

            if (revocationFlag == X509RevocationFlag.EndCertificateOnly)
                dwFlags |= CAPI.CERT_CHAIN_REVOCATION_CHECK_END_CERT;
            else if (revocationFlag == X509RevocationFlag.EntireChain)
                dwFlags |= CAPI.CERT_CHAIN_REVOCATION_CHECK_CHAIN;
            else
                dwFlags |= CAPI.CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;

            return dwFlags;
        }

        private static readonly char[] hexValues = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
        internal static string EncodeHexString (byte[] sArray) {
            return EncodeHexString(sArray, 0, (uint) sArray.Length);
        }

        internal static string EncodeHexString (byte[] sArray, uint start, uint end) {
            String result = null;
            if (sArray != null) {
                char[] hexOrder = new char[(end - start) * 2];
                uint digit;
                for (uint i = start, j = 0; i < end; i++) {
                    digit = (uint) ((sArray[i] & 0xf0) >> 4);
                    hexOrder[j++] = hexValues[digit];
                    digit = (uint) (sArray[i] & 0x0f);
                    hexOrder[j++] = hexValues[digit];
                }
                result = new String(hexOrder);
            }
            return result;
        }

        internal static string EncodeHexStringFromInt (byte[] sArray) {
            return EncodeHexStringFromInt(sArray, 0, (uint) sArray.Length);
        }

        internal static string EncodeHexStringFromInt (byte[] sArray, uint start, uint end) {
            String result = null;
            if(sArray != null) {
                char[] hexOrder = new char[(end - start) * 2];
                uint i = end;
                uint digit, j=0;
                while (i-- > start) {
                    digit = (uint) (sArray[i] & 0xf0) >> 4;
                    hexOrder[j++] = hexValues[digit];
                    digit = (uint) (sArray[i] & 0x0f);
                    hexOrder[j++] = hexValues[digit];
                }
                result = new String(hexOrder);
            }
            return result;
        }

        internal static byte HexToByte (char val) {
            if (val <= '9' && val >= '0')
                return (byte) (val - '0');
            else if (val >= 'a' && val <= 'f')
                return (byte) ((val - 'a') + 10);
            else if (val >= 'A' && val <= 'F')
                return (byte) ((val - 'A') + 10);
            else
                return 0xFF;
        }

        internal static byte[] DecodeHexString (string s) {
            string hexString = Utils.DiscardWhiteSpaces(s);
            uint cbHex = (uint) hexString.Length / 2;
            byte[] hex = new byte[cbHex];
            int i = 0;
            for (int index = 0; index < cbHex; index++) {
                hex[index] = (byte) ((HexToByte(hexString[i]) << 4) | HexToByte(hexString[i+1]));
                i += 2;
            }
            return hex;
        }

        [SecurityCritical]
        internal static unsafe bool MemEqual (byte * pbBuf1, uint cbBuf1, byte * pbBuf2, uint cbBuf2) {
            if (cbBuf1 != cbBuf2)
                return false;

            while (cbBuf1-- > 0) {
                if (*pbBuf1++ != *pbBuf2++) {
                    return false;
                }
            }
            return true;
        }

        [SecurityCritical]
        internal static SafeLocalAllocHandle StringToAnsiPtr (string s) {
            byte[] arr = new byte[s.Length + 1];
            Encoding.ASCII.GetBytes(s, 0, s.Length, arr, 0);
            SafeLocalAllocHandle pb = CAPI.LocalAlloc(CAPI.LMEM_FIXED, new IntPtr(arr.Length));
            Marshal.Copy(arr, 0, pb.DangerousGetHandle(), arr.Length);
            return pb;
        }

        [SecurityCritical]
        internal static SafeCertContextHandle GetCertContext (X509Certificate2 certificate) {
            SafeCertContextHandle safeCertContext = CAPI.CertDuplicateCertificateContext(certificate.Handle);
            GC.KeepAlive(certificate);
            return safeCertContext;
        }

        [SecurityCritical]
        internal static bool GetPrivateKeyInfo (SafeCertContextHandle safeCertContext, ref CspParameters parameters) {
            SafeLocalAllocHandle ptr = SafeLocalAllocHandle.InvalidHandle;
            uint cbData = 0;
            if (!CAPI.CAPISafe.CertGetCertificateContextProperty(safeCertContext,
                                                                 CAPI.CERT_KEY_PROV_INFO_PROP_ID,
                                                                 ptr,
                                                                 ref cbData)) {
                int dwErrorCode = Marshal.GetLastWin32Error();
                if (dwErrorCode == CAPI.CRYPT_E_NOT_FOUND)
                    return false;
                else
                    throw new CryptographicException(Marshal.GetLastWin32Error());
            }

            ptr = CAPI.LocalAlloc(CAPI.LMEM_FIXED, new IntPtr(cbData));
            if (!CAPI.CAPISafe.CertGetCertificateContextProperty(safeCertContext,
                                                                 CAPI.CERT_KEY_PROV_INFO_PROP_ID,
                                                                 ptr,
                                                                 ref cbData)) {
                int dwErrorCode = Marshal.GetLastWin32Error();
                if (dwErrorCode == CAPI.CRYPT_E_NOT_FOUND)
                    return false;
                else
                    throw new CryptographicException(Marshal.GetLastWin32Error());
            }

            CAPI.CRYPT_KEY_PROV_INFO pKeyProvInfo = (CAPI.CRYPT_KEY_PROV_INFO) Marshal.PtrToStructure(ptr.DangerousGetHandle(), typeof(CAPI.CRYPT_KEY_PROV_INFO));
            parameters.ProviderName = pKeyProvInfo.pwszProvName;
            parameters.KeyContainerName = pKeyProvInfo.pwszContainerName;
            parameters.ProviderType = (int) pKeyProvInfo.dwProvType;
            parameters.KeyNumber = (int) pKeyProvInfo.dwKeySpec;
            parameters.Flags = (CspProviderFlags) ((pKeyProvInfo.dwFlags & CAPI.CRYPT_MACHINE_KEYSET) == CAPI.CRYPT_MACHINE_KEYSET ? CspProviderFlags.UseMachineKeyStore : 0);

            ptr.Dispose();
            return true;
        }

        // this method create a memory store from a certificate collection
        [SecurityCritical]
        internal static SafeCertStoreHandle ExportToMemoryStore (X509Certificate2Collection collection,
                                                                 X509Certificate2Collection collection2 = null) {
            //
            // We need to Assert all StorePermission flags since this is a memory store and we want 
            // semi-trusted code to be able to export certificates to a memory store.
            //

            StorePermission sp = new StorePermission(StorePermissionFlags.AllFlags);
            sp.Assert();

            SafeCertStoreHandle safeCertStoreHandle;

            // we always want to use CERT_STORE_ENUM_ARCHIVED_FLAG since we want to preserve the collection in this operation.
            // By default, Archived certificates will not be included.

            safeCertStoreHandle = CAPI.CertOpenStore(new IntPtr(CAPI.CERT_STORE_PROV_MEMORY), 
                                                     CAPI.X509_ASN_ENCODING | CAPI.PKCS_7_ASN_ENCODING,
                                                     IntPtr.Zero,
                                                     CAPI.CERT_STORE_ENUM_ARCHIVED_FLAG | CAPI.CERT_STORE_CREATE_NEW_FLAG, 
                                                     null);

            if (safeCertStoreHandle == null || safeCertStoreHandle.IsInvalid)
                throw new CryptographicException(Marshal.GetLastWin32Error());

            AddToStore(safeCertStoreHandle, collection);

            if (collection2 != null) {
                AddToStore(safeCertStoreHandle, collection2);
            }

            return safeCertStoreHandle;
        }

        [SecurityCritical]
        private static void AddToStore(SafeCertStoreHandle safeCertStoreHandle, X509Certificate2Collection collection)
        {
            //
            // We use CertAddCertificateLinkToStore to keep a link to the original store, so any property changes get
            // applied to the original store. This has a limit of 99 links per cert context however.
            //
            // X509Store.Add(Range) uses CertAddCertificateContextToStore, which would lose information like ephemeral
            // private key associations.
            foreach (X509Certificate2 x509 in collection) {
                using (SafeCertContextHandle ctx = X509Utils.GetCertContext(x509)) {
                    if (!CAPI.CertAddCertificateLinkToStore(safeCertStoreHandle,
                                                            ctx,
                                                            CAPI.CERT_STORE_ADD_ALWAYS,
                                                            SafeCertContextHandle.InvalidHandle))
                        throw new CryptographicException(Marshal.GetLastWin32Error());
                }
            }
        }

        [SecuritySafeCritical]
        internal static uint OidToAlgId (string value) {
            SafeLocalAllocHandle pszOid = StringToAnsiPtr(value);
            CAPI.CRYPT_OID_INFO pOIDInfo = CAPI.CryptFindOIDInfo(CAPI.CRYPT_OID_INFO_OID_KEY, pszOid, 0);
            return pOIDInfo.Algid;
        }

        internal static bool IsSelfSigned (X509Chain chain) {
            X509ChainElementCollection elements = chain.ChainElements;
            if (elements.Count != 1)
                return false;
            X509Certificate2 certificate = elements[0].Certificate;
            if (String.Compare(certificate.SubjectName.Name, certificate.IssuerName.Name, StringComparison.OrdinalIgnoreCase) == 0)
                return true;
            return false;
        }


        [SecurityCritical]
        internal static SafeLocalAllocHandle CopyOidsToUnmanagedMemory (OidCollection oids) {
            SafeLocalAllocHandle safeLocalAllocHandle = SafeLocalAllocHandle.InvalidHandle;
            if (oids == null || oids.Count == 0)
                return safeLocalAllocHandle;

            int ptrSize = oids.Count * Marshal.SizeOf(typeof(IntPtr));
            int oidSize = 0;
            foreach (Oid oid in oids) {
                oidSize += (oid.Value.Length + 1);
            }
            safeLocalAllocHandle = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr((uint) ptrSize + (uint) oidSize));
            IntPtr pOid = new IntPtr((long)safeLocalAllocHandle.DangerousGetHandle() + ptrSize);
            for (int index=0; index < oids.Count; index++) {
                Marshal.WriteIntPtr(new IntPtr((long) safeLocalAllocHandle.DangerousGetHandle() + index * Marshal.SizeOf(typeof(IntPtr))), pOid); 
                byte[] ansiOid = Encoding.ASCII.GetBytes(oids[index].Value);
                Marshal.Copy(ansiOid, 0, pOid, ansiOid.Length);
                pOid = new IntPtr((long) pOid + oids[index].Value.Length + 1);
            }
            return safeLocalAllocHandle;
        }

        //
        // Builds a certificate chain.
        //

        [SecurityCritical]
        internal static X509Certificate2Collection GetCertificates(SafeCertStoreHandle safeCertStoreHandle) {
            X509Certificate2Collection collection = new X509Certificate2Collection();
            IntPtr pEnumContext = CAPI.CertEnumCertificatesInStore(safeCertStoreHandle, IntPtr.Zero);
            while (pEnumContext != IntPtr.Zero) {
                X509Certificate2 certificate = new X509Certificate2(pEnumContext);
                collection.Add(certificate);
                pEnumContext = CAPI.CertEnumCertificatesInStore(safeCertStoreHandle, pEnumContext);
            }
            return collection;
        }

        [SecurityCritical]
        internal static unsafe int BuildChain (IntPtr hChainEngine,
                                               SafeCertContextHandle pCertContext,
                                               X509Certificate2Collection extraStore,
                                               OidCollection applicationPolicy,
                                               OidCollection certificatePolicy,
                                               X509RevocationMode revocationMode,
                                               X509RevocationFlag revocationFlag,
                                               DateTime verificationTime,
                                               TimeSpan timeout,
                                               ref SafeCertChainHandle ppChainContext) {
            if (pCertContext == null || pCertContext.IsInvalid)
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_InvalidContextHandle"), "pCertContext");

            SafeCertStoreHandle hCertStore = SafeCertStoreHandle.InvalidHandle;
            if (extraStore != null && extraStore.Count > 0)
                hCertStore = X509Utils.ExportToMemoryStore(extraStore);

            CAPI.CERT_CHAIN_PARA ChainPara = new CAPI.CERT_CHAIN_PARA();

            // Initialize the structure size.
            ChainPara.cbSize = (uint) Marshal.SizeOf(ChainPara);

            // Application policy
            SafeLocalAllocHandle applicationPolicyHandle = SafeLocalAllocHandle.InvalidHandle;
            if (applicationPolicy != null && applicationPolicy.Count > 0) {
                ChainPara.RequestedUsage.dwType = CAPI.USAGE_MATCH_TYPE_AND;
                ChainPara.RequestedUsage.Usage.cUsageIdentifier = (uint) applicationPolicy.Count;
                applicationPolicyHandle = X509Utils.CopyOidsToUnmanagedMemory(applicationPolicy);
                ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = applicationPolicyHandle.DangerousGetHandle();
            }

            // Certificate policy
            SafeLocalAllocHandle certificatePolicyHandle = SafeLocalAllocHandle.InvalidHandle;
            if (certificatePolicy != null && certificatePolicy.Count > 0) {
                ChainPara.RequestedIssuancePolicy.dwType = CAPI.USAGE_MATCH_TYPE_AND;
                ChainPara.RequestedIssuancePolicy.Usage.cUsageIdentifier = (uint) certificatePolicy.Count;
                certificatePolicyHandle = X509Utils.CopyOidsToUnmanagedMemory(certificatePolicy);
                ChainPara.RequestedIssuancePolicy.Usage.rgpszUsageIdentifier = certificatePolicyHandle.DangerousGetHandle();
            }

            ChainPara.dwUrlRetrievalTimeout = (uint) timeout.Milliseconds;

            _FILETIME ft = new _FILETIME();
            *((long*) &ft) = verificationTime.ToFileTime();

            uint flags = X509Utils.MapRevocationFlags(revocationMode, revocationFlag);

            // Build the chain.
            if (!CAPI.CAPISafe.CertGetCertificateChain(hChainEngine,
                                                       pCertContext,
                                                       ref ft,
                                                       hCertStore,
                                                       ref ChainPara,
                                                       flags,
                                                       IntPtr.Zero,
                                                       ref ppChainContext))
                return Marshal.GetHRForLastWin32Error();

            applicationPolicyHandle.Dispose();
            certificatePolicyHandle.Dispose();

            return CAPI.S_OK;
        }

        //
        // Verifies whether a certificate is valid for the specified policy.
        // S_OK means the certificate is valid for the specified policy.
        // S_FALSE means the certificate is invalid for the specified policy.
        // Anything else is an error.
        //

        [SecurityCritical]
        internal static unsafe int VerifyCertificate (SafeCertContextHandle pCertContext,
                                                      OidCollection applicationPolicy,
                                                      OidCollection certificatePolicy,
                                                      X509RevocationMode revocationMode,
                                                      X509RevocationFlag revocationFlag,
                                                      DateTime verificationTime,
                                                      TimeSpan timeout,
                                                      X509Certificate2Collection extraStore,
                                                      IntPtr pszPolicy,
                                                      IntPtr pdwErrorStatus) {
            if (pCertContext == null || pCertContext.IsInvalid)
                throw new ArgumentException("pCertContext");

            CAPI.CERT_CHAIN_POLICY_PARA PolicyPara = new CAPI.CERT_CHAIN_POLICY_PARA(Marshal.SizeOf(typeof(CAPI.CERT_CHAIN_POLICY_PARA)));
            CAPI.CERT_CHAIN_POLICY_STATUS PolicyStatus = new CAPI.CERT_CHAIN_POLICY_STATUS(Marshal.SizeOf(typeof(CAPI.CERT_CHAIN_POLICY_STATUS)));

            // Build the chain.
            SafeCertChainHandle pChainContext = SafeCertChainHandle.InvalidHandle;
            int hr = X509Utils.BuildChain(new IntPtr(CAPI.HCCE_CURRENT_USER),
                                          pCertContext, 
                                          extraStore,
                                          applicationPolicy, 
                                          certificatePolicy,
                                          revocationMode,
                                          revocationFlag,
                                          verificationTime,
                                          timeout,
                                          ref pChainContext);
            if (hr != CAPI.S_OK)
                return hr;

            // Verify the chain using the specified policy.
            if (CAPI.CAPISafe.CertVerifyCertificateChainPolicy(pszPolicy, pChainContext, ref PolicyPara, ref PolicyStatus)) {
                if (pdwErrorStatus != IntPtr.Zero)
                    *(uint*) pdwErrorStatus = PolicyStatus.dwError;

                if (PolicyStatus.dwError != 0)
                    return CAPI.S_FALSE;
            } else {
                // The API failed.
                return Marshal.GetHRForLastWin32Error();
            }

            return CAPI.S_OK;
        }
    }
}
