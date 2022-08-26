// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// DataProtection.cs
//
// 01/25/2003
//

namespace System.Security.Cryptography
{
    using Microsoft.Win32;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using System.Runtime.ConstrainedExecution;
    using System.Security.Permissions;
    using System.Globalization;

    public enum DataProtectionScope {
        CurrentUser     = 0x00,
        LocalMachine    = 0x01
    }

    public enum MemoryProtectionScope {
        SameProcess     = 0x00,
        CrossProcess    = 0x01,
        SameLogon       = 0x02
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public static class ProtectedData {
        [SecuritySafeCritical]
        public static byte[] Protect (byte[] userData,
                                      byte[] optionalEntropy,
                                      DataProtectionScope scope) {
            if (userData == null)
                throw new ArgumentNullException("userData");
            if (Environment.OSVersion.Platform == PlatformID.Win32Windows)
                throw new NotSupportedException(SecurityResources.GetResourceString("NotSupported_PlatformRequiresNT"));

            GCHandle pbDataIn = new GCHandle();
            GCHandle pOptionalEntropy = new GCHandle();
            CAPI.CRYPTOAPI_BLOB blob = new CAPI.CRYPTOAPI_BLOB();

            RuntimeHelpers.PrepareConstrainedRegions();
            try {
                pbDataIn = GCHandle.Alloc(userData, GCHandleType.Pinned);
                CAPI.CRYPTOAPI_BLOB dataIn = new CAPI.CRYPTOAPI_BLOB();
                dataIn.cbData = (uint) userData.Length;
                dataIn.pbData = pbDataIn.AddrOfPinnedObject();
                CAPI.CRYPTOAPI_BLOB entropy = new CAPI.CRYPTOAPI_BLOB();
                if (optionalEntropy != null) {
                    pOptionalEntropy = GCHandle.Alloc(optionalEntropy, GCHandleType.Pinned);
                    entropy.cbData = (uint) optionalEntropy.Length;
                    entropy.pbData = pOptionalEntropy.AddrOfPinnedObject();
                }
                uint dwFlags = CAPI.CRYPTPROTECT_UI_FORBIDDEN;
                if (scope == DataProtectionScope.LocalMachine)
                    dwFlags |= CAPI.CRYPTPROTECT_LOCAL_MACHINE;
                unsafe {
                    if (!CAPI.CryptProtectData(new IntPtr(&dataIn),
                                               String.Empty,
                                               new IntPtr(&entropy),
                                               IntPtr.Zero,
                                               IntPtr.Zero,
                                               dwFlags,
                                               new IntPtr(&blob))) {
                        int lastWin32Error = Marshal.GetLastWin32Error();

                        // One of the most common reasons that DPAPI operations fail is that the user
                        // profile is not loaded (for instance in the case of impersonation or running in a
                        // service.  In those cases, throw an exception that provides more specific details
                        // about what happened.
                        if (CAPI.ErrorMayBeCausedByUnloadedProfile(lastWin32Error)) {
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_DpApi_ProfileMayNotBeLoaded"));
                        }
                        else {
                            throw new CryptographicException(lastWin32Error);
                        }
                    }
                }

                // In some cases, the API would fail due to OOM but simply return a null pointer.
                if (blob.pbData == IntPtr.Zero)
                    throw new OutOfMemoryException();

                byte[] encryptedData = new byte[(int) blob.cbData];
                Marshal.Copy(blob.pbData, encryptedData, 0, encryptedData.Length);

                return encryptedData;
            }
            catch (EntryPointNotFoundException) {
                throw new NotSupportedException(SecurityResources.GetResourceString("NotSupported_PlatformRequiresNT"));
            }
            finally {
                if (pbDataIn.IsAllocated)
                   pbDataIn.Free();
                if (pOptionalEntropy.IsAllocated)
                   pOptionalEntropy.Free();
                if (blob.pbData != IntPtr.Zero) {
                    CAPI.CAPISafe.ZeroMemory(blob.pbData, blob.cbData);
                    CAPI.CAPISafe.LocalFree(blob.pbData);
                }
            }
        }

        [SecuritySafeCritical]
        public static byte[] Unprotect (byte[] encryptedData,
                                        byte[] optionalEntropy,
                                        DataProtectionScope scope) {
            if (encryptedData == null)
                throw new ArgumentNullException("encryptedData");
            if (Environment.OSVersion.Platform == PlatformID.Win32Windows)
                throw new NotSupportedException(SecurityResources.GetResourceString("NotSupported_PlatformRequiresNT"));

            GCHandle pbDataIn = new GCHandle();
            GCHandle pOptionalEntropy = new GCHandle();
            CAPI.CRYPTOAPI_BLOB userData = new CAPI.CRYPTOAPI_BLOB();

            RuntimeHelpers.PrepareConstrainedRegions();
            try {
                pbDataIn = GCHandle.Alloc(encryptedData, GCHandleType.Pinned);
                CAPI.CRYPTOAPI_BLOB dataIn = new CAPI.CRYPTOAPI_BLOB();
                dataIn.cbData = (uint) encryptedData.Length;
                dataIn.pbData = pbDataIn.AddrOfPinnedObject();
                CAPI.CRYPTOAPI_BLOB entropy = new CAPI.CRYPTOAPI_BLOB();
                if (optionalEntropy != null) {
                    pOptionalEntropy = GCHandle.Alloc(optionalEntropy, GCHandleType.Pinned);
                    entropy.cbData = (uint) optionalEntropy.Length;
                    entropy.pbData = pOptionalEntropy.AddrOfPinnedObject();
                }
                uint dwFlags = CAPI.CRYPTPROTECT_UI_FORBIDDEN;
                if (scope == DataProtectionScope.LocalMachine)
                    dwFlags |= CAPI.CRYPTPROTECT_LOCAL_MACHINE;
                unsafe {
                    if (!CAPI.CryptUnprotectData(new IntPtr(&dataIn),
                                                 IntPtr.Zero,
                                                 new IntPtr(&entropy),
                                                 IntPtr.Zero,
                                                 IntPtr.Zero,
                                                 dwFlags,
                                                 new IntPtr(&userData)))
                        throw new CryptographicException(Marshal.GetLastWin32Error());
                }

                // In some cases, the API would fail due to OOM but simply return a null pointer.
                if (userData.pbData == IntPtr.Zero)
                    throw new OutOfMemoryException();

                byte[] data = new byte[(int) userData.cbData];
                Marshal.Copy(userData.pbData, data, 0, data.Length);

                return data;
            }
            catch (EntryPointNotFoundException) {
                throw new NotSupportedException(SecurityResources.GetResourceString("NotSupported_PlatformRequiresNT"));
            }
            finally {
                if (pbDataIn.IsAllocated)
                   pbDataIn.Free();
                if (pOptionalEntropy.IsAllocated)
                   pOptionalEntropy.Free();
                if (userData.pbData != IntPtr.Zero) {
                    CAPI.CAPISafe.ZeroMemory(userData.pbData, userData.cbData);
                    CAPI.CAPISafe.LocalFree(userData.pbData);
                }
            }
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public static class ProtectedMemory {
        [SecuritySafeCritical]
        public static void Protect (byte[] userData, 
                                    MemoryProtectionScope scope) {
            if (userData == null)
                throw new ArgumentNullException("userData");
            if (Environment.OSVersion.Platform == PlatformID.Win32Windows)
                throw new NotSupportedException(SecurityResources.GetResourceString("NotSupported_PlatformRequiresNT"));

            VerifyScope(scope);

            // The RtlEncryptMemory and RtlDecryptMemory functions are available on WinXP and publicly published 
            // in the ntsecapi.h header file as of Windows Server 2003.
            // The Rtl functions accept data in 8 byte increments, but we don't want applications to be able to make use of this, 
            // or else they're liable to break when the user upgrades.
            if ((userData.Length == 0) || (userData.Length % CAPI.CRYPTPROTECTMEMORY_BLOCK_SIZE != 0))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_DpApi_InvalidMemoryLength"));

            uint dwFlags = (uint) scope;
            try {
                // RtlEncryptMemory return an NTSTATUS
                int status = CAPI.SystemFunction040(userData,
                                                    (uint) userData.Length,
                                                    dwFlags);
                if (status < 0) // non-negative numbers indicate success
                    throw new CryptographicException(CAPI.CAPISafe.LsaNtStatusToWinError(status));
            }
            catch (EntryPointNotFoundException) {
                throw new NotSupportedException(SecurityResources.GetResourceString("NotSupported_PlatformRequiresNT"));
            }
        }

        [SecuritySafeCritical]
        public static void Unprotect (byte[] encryptedData, 
                                      MemoryProtectionScope scope) {
            if (encryptedData == null)
                throw new ArgumentNullException("encryptedData");
            if (Environment.OSVersion.Platform == PlatformID.Win32Windows)
                throw new NotSupportedException(SecurityResources.GetResourceString("NotSupported_PlatformRequiresNT"));

            VerifyScope(scope);

            // The RtlEncryptMemory and RtlDecryptMemory functions are available on WinXP and publicly published 
            // in the ntsecapi.h header file as of Windows Server 2003.
            // The Rtl functions accept data in 8 byte increments, but we don't want applications to be able to make use of this, 
            // or else they're liable to break when the user upgrades.
            if ((encryptedData.Length == 0) || (encryptedData.Length % CAPI.CRYPTPROTECTMEMORY_BLOCK_SIZE != 0))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_DpApi_InvalidMemoryLength"));

            uint dwFlags = (uint) scope;
            try {
                // RtlDecryptMemory return an NTSTATUS
                int status = CAPI.SystemFunction041(encryptedData,
                                                    (uint) encryptedData.Length,
                                                    dwFlags);
                if (status < 0) // non-negative numbers indicate success
                    throw new CryptographicException(CAPI.CAPISafe.LsaNtStatusToWinError(status));
            }
            catch (EntryPointNotFoundException) {
                throw new NotSupportedException(SecurityResources.GetResourceString("NotSupported_PlatformRequiresNT"));
            }
        }

        private static void VerifyScope (MemoryProtectionScope scope) {
            if ((scope != MemoryProtectionScope.SameProcess) && (scope != MemoryProtectionScope.CrossProcess) &&
                (scope != MemoryProtectionScope.SameLogon))
                throw new ArgumentException(String.Format(CultureInfo.CurrentCulture, SecurityResources.GetResourceString("Arg_EnumIllegalVal"), (int) scope));
        }
    }
}
