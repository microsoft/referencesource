//------------------------------------------------------------------------------
// <copyright file="FileIntegrity.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler
{
    using Microsoft.Win32;
    using Microsoft.Win32.SafeHandles;
    using System;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Helps checking file integrity.
    /// </summary>
    internal static class FileIntegrity
    {
        private static readonly Lazy<bool> s_lazyIsEnabled = new Lazy<bool>(() =>
        {
            // The flag used below to load the library requires Windows 8 or above. 
            // OSVersion value depends on application manifest but in any case anything
            // prior to 6.2 (Windows 8) is not supported.
            var osVersion = Environment.OSVersion.Version;
            if (osVersion.Major < 6 || (osVersion.Major == 6 && osVersion.Minor < 2))
                return false;

            // Attempt to load module using proper flag
            const int LOAD_LIBRARY_SEARCH_SYSTEM32 = 0x00000800;
            using (SafeLibraryHandle safeLibraryHandle = SafeLibraryHandle.LoadLibraryEx(ExternDll.Wldp, IntPtr.Zero, LOAD_LIBRARY_SEARCH_SYSTEM32))
            {
                if (safeLibraryHandle.IsInvalid)
                    return false;

                // Check for needed functions, and avoid EntryPointNotFoundException
                IntPtr hModule = UnsafeNativeMethods.GetModuleHandle(ExternDll.Wldp);
                bool hasAllRequiredFunctions =
                    hModule != IntPtr.Zero &&
                    IntPtr.Zero != UnsafeNativeMethods.GetProcAddress(hModule, "WldpIsDynamicCodePolicyEnabled") &&
                    IntPtr.Zero != UnsafeNativeMethods.GetProcAddress(hModule, "WldpSetDynamicCodeTrust") &&
                    IntPtr.Zero != UnsafeNativeMethods.GetProcAddress(hModule, "WldpQueryDynamicCodeTrust");

                if (!hasAllRequiredFunctions)
                    return false;

                int isEnabled = 0;
                int hResult = UnsafeNativeMethods.WldpIsDynamicCodePolicyEnabled(out isEnabled);
                Marshal.ThrowExceptionForHR(hResult, new IntPtr(-1));

                return (isEnabled != 0);
            }
        });

        /// <summary>
        /// Gets a value indicating wheter file integrity check is enabled.
        /// </summary>
        public static bool IsEnabled => s_lazyIsEnabled.Value;

        /// <summary>
        /// Mark the given file as trusted, any changes happening after that will erase the trusted flag.
        /// </summary>
        /// <param name="safeFileHandle">File to be marked as trusted.</param>
        /// <exception cref="Exception">
        /// Exception throw by <code>Marshal.ThrowExceptionForHR</code> if the method fails to set the trust flag on the file.
        /// </exception>
        public static void MarkAsTrusted(SafeFileHandle safeFileHandle)
        {
            int hResult = UnsafeNativeMethods.WldpSetDynamicCodeTrust(safeFileHandle);
            Marshal.ThrowExceptionForHR(hResult, new IntPtr(-1));
        }

        /// <summary>
        /// Checks if the given file is marked as trusted.
        /// </summary>
        /// <param name="safeFileHandle">File to be marked as trusted.</param>
        /// <exception cref="Exception">
        /// Exception throw by <code>Marshal.ThrowExceptionForHR</code> if the method fails to check the trust flag on the file.
        /// </exception>
        /// <returns>True if the integrity flag was present on the file, false otherwise.</returns>
        public static bool IsTrusted(SafeFileHandle safeFileHandle)
        {
            // if the call was successful but the flag was not present the API returns HRESULT_FROM_NT(STATUS_NOT_FOUND) .
            const int HRESULT_STATUS_NOT_FOUND = unchecked((int)0xD0000225);
            int hResult = UnsafeNativeMethods.WldpQueryDynamicCodeTrust(safeFileHandle, IntPtr.Zero, 0);
            if (hResult == HRESULT_STATUS_NOT_FOUND)
                return false;

            Marshal.ThrowExceptionForHR(hResult, new IntPtr(-1));

            return true;
        }
    }
}
