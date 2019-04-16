// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

using System;
using System.Runtime.CompilerServices;
using System.Runtime.ConstrainedExecution;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Microsoft.Win32.SafeHandles;
using System.Diagnostics;

using FILETIME = System.Runtime.InteropServices.ComTypes.FILETIME;

namespace System.Security.Cryptography.X509Certificates {

    internal static partial class X509Native {

        /// <summary>
        ///     Determine if a certificate has a specific property
        /// </summary>
        [SecuritySafeCritical]
        internal static bool HasCertificateProperty(SafeCertContextHandle certificateContext,
                                                    CertificateProperty property) {
            Debug.Assert(certificateContext != null, "certificateContext != null");
            Debug.Assert(!certificateContext.IsClosed && !certificateContext.IsInvalid, 
                        "!certificateContext.IsClosed && !certificateContext.IsInvalid");

            byte[] buffer = null;
            int bufferSize = 0;
            bool gotProperty = UnsafeNativeMethods.CertGetCertificateContextProperty(certificateContext,
                                                                                     property,
                                                                                     buffer,
                                                                                     ref bufferSize);
            return gotProperty ||
                   (ErrorCode)Marshal.GetLastWin32Error() == ErrorCode.MoreData;
        }

        /// <summary>
        ///     Get the NCrypt handle to the private key of a certificate 
        ///     or null if the private key cannot be acquired by NCrypt.
        /// </summary>
        [SecuritySafeCritical]
        internal static SafeNCryptKeyHandle TryAcquireCngPrivateKey(
            SafeCertContextHandle certificateContext,
            out CngKeyHandleOpenOptions openOptions)
        {
            Debug.Assert(certificateContext != null, "certificateContext != null");
            Debug.Assert(!certificateContext.IsClosed && !certificateContext.IsInvalid, 
                         "!certificateContext.IsClosed && !certificateContext.IsInvalid");

            IntPtr privateKeyPtr;

            // If the certificate has a key handle instead of a key prov info, return the
            // ephemeral key
            {
                int cbData = IntPtr.Size;

                if (UnsafeNativeMethods.CertGetCertificateContextProperty(
                    certificateContext,
                    CertificateProperty.NCryptKeyHandle,
                    out privateKeyPtr,
                    ref cbData))
                {
                    openOptions = CngKeyHandleOpenOptions.EphemeralKey;
                    return new SafeNCryptKeyHandle(privateKeyPtr, certificateContext);
                }
            }

            openOptions = CngKeyHandleOpenOptions.None;

            bool freeKey = true;
            SafeNCryptKeyHandle privateKey = null;
            RuntimeHelpers.PrepareConstrainedRegions();
            try {
                int keySpec = 0;
                if (!UnsafeNativeMethods.CryptAcquireCertificatePrivateKey(certificateContext,
                                                                           AcquireCertificateKeyOptions.AcquireOnlyNCryptKeys,
                                                                           IntPtr.Zero,
                                                                           out privateKey,
                                                                           out keySpec,
                                                                           out freeKey)) {

                    // The documentation for CryptAcquireCertificatePrivateKey says that freeKey
                    // should already be false if "key acquisition fails", and it can be presumed
                    // that privateKey was set to 0.  But, just in case:
                    freeKey = false;
                    privateKey?.SetHandleAsInvalid();
                    return null;
                }
            }
            finally {
                // It is very unlikely that Windows will tell us !freeKey other than when reporting failure,
                // because we set neither CRYPT_ACQUIRE_CACHE_FLAG nor CRYPT_ACQUIRE_USE_PROV_INFO_FLAG, which are
                // currently the only two success situations documented. However, any !freeKey response means the
                // key's lifetime is tied to that of the certificate, so re-register the handle as a child handle
                // of the certificate.
                if (!freeKey && privateKey != null && !privateKey.IsInvalid)
                {
                    var newKeyHandle = new SafeNCryptKeyHandle(privateKey.DangerousGetHandle(), certificateContext);
                    privateKey.SetHandleAsInvalid();
                    privateKey = newKeyHandle;
                    freeKey = true;
                }
            }

            return privateKey;
        }

        /// <summary>
        ///     Get an arbitrary property of a certificate
        /// </summary>
        [SecuritySafeCritical]
        internal static byte[] GetCertificateProperty(SafeCertContextHandle certificateContext,
                                                      CertificateProperty property) {
            Debug.Assert(certificateContext != null, "certificateContext != null");
            Debug.Assert(!certificateContext.IsClosed && !certificateContext.IsInvalid, 
                         "!certificateContext.IsClosed && !certificateContext.IsInvalid");

            byte[] buffer = null;
            int bufferSize = 0;
            if (!UnsafeNativeMethods.CertGetCertificateContextProperty(certificateContext,
                                                                       property,
                                                                       buffer,
                                                                       ref bufferSize)) {
                ErrorCode errorCode = (ErrorCode)Marshal.GetLastWin32Error();
                if (errorCode != ErrorCode.MoreData) {
                    throw new CryptographicException((int)errorCode);
                }
            }

            buffer = new byte[bufferSize];
            if (!UnsafeNativeMethods.CertGetCertificateContextProperty(certificateContext,
                                                                       property,
                                                                       buffer,
                                                                       ref bufferSize)) {
                throw new CryptographicException(Marshal.GetLastWin32Error());
            }

            return buffer;
        }

        /// <summary>
        ///     Get a property of a certificate formatted as a structure
        /// </summary>
        [SecurityCritical]
        internal static T GetCertificateProperty<T>(SafeCertContextHandle certificateContext,
                                                    CertificateProperty property) where T : struct {
            Debug.Assert(certificateContext != null, "certificateContext != null");
            Debug.Assert(!certificateContext.IsClosed && !certificateContext.IsInvalid, 
                        "!certificateContext.IsClosed && !certificateContext.IsInvalid");

            byte[] rawProperty = GetCertificateProperty(certificateContext, property);
            Debug.Assert(rawProperty.Length >= Marshal.SizeOf(typeof(T)), "Property did not return expected structure");

            unsafe {
                fixed (byte* pRawProperty = &rawProperty[0]) {
                    return (T)Marshal.PtrToStructure(new IntPtr(pRawProperty), typeof(T));
                }
            }
        }

        [SecurityCritical]
        internal static bool SetCertificateKeyProvInfo(
            SafeCertContextHandle certificateContext,
            ref CRYPT_KEY_PROV_INFO provInfo)
        {
            Debug.Assert(certificateContext != null, "certificateContext != null");
            Debug.Assert(!certificateContext.IsClosed && !certificateContext.IsInvalid,
                        "!certificateContext.IsClosed && !certificateContext.IsInvalid");

            return UnsafeNativeMethods.CertSetCertificateContextProperty(
                certificateContext,
                CertificateProperty.KeyProviderInfo,
                CertSetPropertyFlags.None,
                ref provInfo);
        }

        [SecurityCritical]
        internal static bool SetCertificateNCryptKeyHandle(
           SafeCertContextHandle certificateContext,
           SafeNCryptKeyHandle keyHandle)
        {
            Debug.Assert(certificateContext != null, "certificateContext != null");
            Debug.Assert(!certificateContext.IsClosed && !certificateContext.IsInvalid,
                        "!certificateContext.IsClosed && !certificateContext.IsInvalid");

            Debug.Assert(keyHandle != null, "keyHandle != null");
            Debug.Assert(!keyHandle.IsClosed && !keyHandle.IsInvalid,
                        "!keyHandle.IsClosed && !keyHandle.IsInvalid");

            return UnsafeNativeMethods.CertSetCertificateContextProperty(
                certificateContext,
                CertificateProperty.NCryptKeyHandle,
                CertSetPropertyFlags.CERT_SET_PROPERTY_INHIBIT_PERSIST_FLAG,
                keyHandle);
        }

        /// <summary>
        ///     Duplicate the certificate context into a safe handle
        /// </summary>
        [SecuritySafeCritical]
        internal static SafeCertContextHandle DuplicateCertContext(IntPtr context) {
            Debug.Assert(context != IntPtr.Zero);

            return UnsafeNativeMethods.CertDuplicateCertificateContext(context);
        }

        // Gets a SafeHandle for the X509 certificate. The caller owns the returned handle and should dispose of it. It
        // can be used independently of the lifetime of the original X509Certificate.
        [SecuritySafeCritical]
        internal static SafeCertContextHandle GetCertificateContext(X509Certificate certificate) {
            SafeCertContextHandle certificateContext = DuplicateCertContext(certificate.Handle);
            // Make sure to keep the X509Certificate object alive until after its certificate context is
            // duplicated, otherwise it could end up being closed out from underneath us before we get a
            // chance to duplicate the handle.
            GC.KeepAlive(certificate);
            return certificateContext;
        }
    }
    /// <summary>
    ///     Native interop layer for X509 certificate and Authenticode functions. Native definitions can be
    ///     found in wincrypt.h or msaxlapi.h
    /// </summary>
    internal static partial class X509Native {
        /// <summary>
        ///     Flags for CertVerifyAuthenticodeLicense
        /// </summary>
        [Flags]
        public enum AxlVerificationFlags {
            None                        = 0x00000000,
            NoRevocationCheck           = 0x00000001,   // AXL_REVOCATION_NO_CHECK
            RevocationCheckEndCertOnly  = 0x00000002,   // AXL_REVOCATION_CHECK_END_CERT_ONLY
            RevocationCheckEntireChain  = 0x00000004,   // AXL_REVOCATION_CHECK_ENTIRE_CHAIN
            UrlOnlyCacheRetrieval       = 0x00000008,   // AXL_URL_ONLY_CACHE_RETRIEVAL
            LifetimeSigning             = 0x00000010,   // AXL_LIFETIME_SIGNING
            TrustMicrosoftRootOnly      = 0x00000020    // AXL_TRUST_MICROSOFT_ROOT_ONLY
        }

        internal const uint X509_ASN_ENCODING = 0x00000001;
        internal const string szOID_ECC_PUBLIC_KEY = "1.2.840.10045.2.1";   //Copied from Windows header file
        internal const int CRYPT_MACHINE_KEYSET = 0x00000020;

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        internal struct CERT_CONTEXT {
            internal uint dwCertEncodingType;
            internal IntPtr pbCertEncoded;
            internal uint cbCertEncoded;
            internal IntPtr pCertInfo;
            internal IntPtr hCertStore;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        internal struct CERT_PUBLIC_KEY_INFO {
            internal CRYPT_ALGORITHM_IDENTIFIER Algorithm;
            internal CRYPT_BIT_BLOB PublicKey;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        internal struct CERT_INFO {
            internal uint dwVersion;
            internal CRYPTOAPI_BLOB SerialNumber;
            internal CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm;
            internal CRYPTOAPI_BLOB Issuer;
            internal FILETIME NotBefore;
            internal FILETIME NotAfter;
            internal CRYPTOAPI_BLOB Subject;
            internal CERT_PUBLIC_KEY_INFO SubjectPublicKeyInfo;
            internal CRYPT_BIT_BLOB IssuerUniqueId;
            internal CRYPT_BIT_BLOB SubjectUniqueId;
            internal uint cExtension;
            internal IntPtr rgExtension; // PCERT_EXTENSION
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        internal struct CRYPT_ALGORITHM_IDENTIFIER {
            [MarshalAs(UnmanagedType.LPStr)]
            internal string pszObjId;
            internal CRYPTOAPI_BLOB Parameters;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        internal struct CRYPT_BIT_BLOB {
            internal uint cbData;
            internal IntPtr pbData;
            internal uint cUnusedBits;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        internal struct CRYPTOAPI_BLOB {
            internal uint cbData;
            internal IntPtr pbData;
        }

        /// <summary>
        ///     Flags for the CryptAcquireCertificatePrivateKey API
        /// </summary>
        internal enum AcquireCertificateKeyOptions {
            None = 0x00000000,
            AcquireOnlyNCryptKeys = 0x00040000,   // CRYPT_ACQUIRE_ONLY_NCRYPT_KEY_FLAG
        }

        /// <summary>
        ///     Well known certificate property IDs
        /// </summary>
        internal enum CertificateProperty {
            KeyProviderInfo = 2,    // CERT_KEY_PROV_INFO_PROP_ID 
            KeyContext = 5,    // CERT_KEY_CONTEXT_PROP_ID
            NCryptKeyHandle = 78, // CERT_NCRYPT_KEY_HANDLE_PROP_ID
        }

        [Flags]
        internal enum CertSetPropertyFlags : int {
            CERT_SET_PROPERTY_INHIBIT_PERSIST_FLAG = 0x40000000,
            None = 0x00000000,
        }

        /// <summary>
        ///     Error codes returned from X509 APIs
        /// </summary>
        internal enum ErrorCode {
            Success = 0x00000000,       // ERROR_SUCCESS
            MoreData = 0x000000ea,       // ERROR_MORE_DATA
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct CRYPT_KEY_PROV_INFO {
            [MarshalAs(UnmanagedType.LPWStr)]
            internal string pwszContainerName;

            [MarshalAs(UnmanagedType.LPWStr)]
            internal string pwszProvName;

            internal int dwProvType;

            internal int dwFlags;

            internal int cProvParam;

            internal IntPtr rgProvParam;        // PCRYPT_KEY_PROV_PARAM

            internal int dwKeySpec;
        }

        [StructLayout(LayoutKind.Sequential)]
        [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
        public struct AXL_AUTHENTICODE_SIGNER_INFO {
            public int cbSize;
            public int dwError;
            public CapiNative.AlgorithmId algHash;

            // Each of the next fields are Unicode strings, however we need to manually marshal them since
            // they are allocated and freed by the native AXL code and should not have their memory handled
            // by the marshaller.
            public IntPtr pwszHash;
            public IntPtr pwszDescription;
            public IntPtr pwszDescriptionUrl;

            public IntPtr pChainContext;        // PCERT_CHAIN_CONTEXT
        }

        [StructLayout(LayoutKind.Sequential)]
        [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
        public struct AXL_AUTHENTICODE_TIMESTAMPER_INFO {
            public int cbsize;
            public int dwError;
            public CapiNative.AlgorithmId algHash;
            public FILETIME ftTimestamp;
            public IntPtr pChainContext;        // PCERT_CHAIN_CONTEXT
        }

        [SuppressUnmanagedCodeSecurity]
        [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
#pragma warning disable 618 // System.Core.dll still uses SecurityRuleSet.Level1
        [SecurityCritical(SecurityCriticalScope.Everything)]
#pragma warning restore 618
        public static class UnsafeNativeMethods {
            /// <summary>
            ///     Get the hash value of a key blob
            /// </summary>
            [DllImport("clr")]
            public static extern int _AxlGetIssuerPublicKeyHash(IntPtr pCertContext,
                                                                [Out]out SafeAxlBufferHandle ppwszPublicKeyHash);

            /// <summary>
            ///     Release any resources used to create an authenticode signer info structure
            /// </summary>
            [DllImport("clr")]
            [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
            public static extern int CertFreeAuthenticodeSignerInfo(ref AXL_AUTHENTICODE_SIGNER_INFO pSignerInfo);

            /// <summary>
            ///     Release any resources used to create an authenticode timestamper info structure
            /// </summary>
            [DllImport("clr")]
            [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
            public static extern int CertFreeAuthenticodeTimestamperInfo(ref AXL_AUTHENTICODE_TIMESTAMPER_INFO pTimestamperInfo);

            /// <summary>
            ///     Verify the authenticode signature on a manifest
            /// </summary>
            /// <remarks>
            ///     Code must have permission to open and enumerate certificate stores to use this API
            /// </remarks>
            [DllImport("clr")]
            public static extern int CertVerifyAuthenticodeLicense(ref CapiNative.CRYPTOAPI_BLOB pLicenseBlob,
                                                                   AxlVerificationFlags dwFlags,
                                                                   [In, Out] ref AXL_AUTHENTICODE_SIGNER_INFO pSignerInfo,
                                                                   [In, Out] ref AXL_AUTHENTICODE_TIMESTAMPER_INFO pTimestamperInfo);

            [DllImport("crypt32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool CertGetCertificateContextProperty(SafeCertContextHandle pCertContext,
                                                                          CertificateProperty dwPropId,
                                                                          [Out, MarshalAs(UnmanagedType.LPArray)] byte[] pvData,
                                                                          [In, Out] ref int pcbData);

            [DllImport("crypt32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool CertGetCertificateContextProperty(SafeCertContextHandle pCertContext,
                                                                          CertificateProperty dwPropId,
                                                                          [Out] out IntPtr pvData,
                                                                          [In, Out] ref int pcbData);

            [DllImport("crypt32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool CertSetCertificateContextProperty(SafeCertContextHandle pCertContext,
                                                                          CertificateProperty dwPropId,
                                                                          CertSetPropertyFlags dwFlags,
                                                                          [In] ref CRYPT_KEY_PROV_INFO pvData);

            [DllImport("crypt32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool CertSetCertificateContextProperty(SafeCertContextHandle pCertContext,
                                                                          CertificateProperty dwPropId,
                                                                          CertSetPropertyFlags dwFlags,
                                                                          [In] SafeNCryptKeyHandle pvData);

            [DllImport("crypt32.dll")]
            internal static extern SafeCertContextHandle CertDuplicateCertificateContext(IntPtr certContext);       // CERT_CONTEXT *

            [DllImport("crypt32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool CryptAcquireCertificatePrivateKey(SafeCertContextHandle pCert,
                                                                          AcquireCertificateKeyOptions dwFlags,
                                                                          IntPtr pvReserved,        // void *
                                                                          [Out] out SafeNCryptKeyHandle phCryptProvOrNCryptKey,
                                                                          [Out] out int dwKeySpec,
                                                                          [Out, MarshalAs(UnmanagedType.Bool)] out bool pfCallerFreeProvOrNCryptKey);
        }
    }

    [SecurityCritical]
    internal struct PinAndClear : IDisposable
    {
        private byte[] _data;
        private System.Runtime.InteropServices.GCHandle _gcHandle;

        [SecurityCritical]
        internal static PinAndClear Track(byte[] data)
        {
            return new PinAndClear
            {
                _gcHandle = System.Runtime.InteropServices.GCHandle.Alloc(
                    data,
                    System.Runtime.InteropServices.GCHandleType.Pinned),
                _data = data,
            };
        }

        [SecurityCritical]
        public void Dispose()
        {
            Array.Clear(_data, 0, _data.Length);
            _gcHandle.Free();
        }
    }

    internal sealed class SafeCertContextHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        [SecuritySafeCritical]
        private SafeCertContextHandle() : base(true) { }

        // 0 is an Invalid Handle
        [SecuritySafeCritical]
        internal SafeCertContextHandle(IntPtr handle)
            : base(true)
        {
            SetHandle(handle);
        }

        internal static SafeCertContextHandle InvalidHandle
        {
            [SecuritySafeCritical]
            get {
                SafeCertContextHandle invalidHandle = new SafeCertContextHandle(IntPtr.Zero);
                // This is valid since we don't expose any way to replace the handle value
                GC.SuppressFinalize(invalidHandle);
                return invalidHandle;
            }
        }

        [DllImport("Crypt32.dll", SetLastError = true),
         ResourceExposure(ResourceScope.None)]
        //#if !FEATURE_CORESYSTEM
        //        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
        //#endif
        private static extern bool CertFreeCertificateContext(IntPtr pCertContext);

#if FEATURE_CORESYSTEM
        [SecurityCritical]
#endif
        [SecuritySafeCritical]
        override protected bool ReleaseHandle()
        {
            return CertFreeCertificateContext(handle);
        }
    }
}
