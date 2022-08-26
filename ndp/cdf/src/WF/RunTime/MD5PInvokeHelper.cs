// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// This file is used to P/Invoke the BCrypt* methods to utilizes the Windows
// implementation of MD5 for hashing.
namespace System.Workflow.Runtime
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;
    using System.Text;

    internal static class MD5PInvokeHelper
    {
        internal const int StatusSuccess = 0;
        internal const string BCryptHashLength = "HashDigestLength";

        private static object syncObject = new object();
        private static SafeBCryptAlgorithmHandle algorithmHandle = null;

        [Flags]
        internal enum BCryptOpenAlgorithmProviderFlags : int
        {
            None = 0x00000000,
            BCRYPT_ALG_HANDLE_HMAC_FLAG = 0x00000008,
        }

        [Flags]
        internal enum BCryptCreateHashFlags : int
        {
            None = 0x00000000,
            BCRYPT_HASH_REUSABLE_FLAG = 0x00000020,
        }

        internal static SafeBCryptAlgorithmHandle MD5AlgorithmProvider
        {
            get
            {
                lock (syncObject)
                {
                    if (algorithmHandle == null)
                    {
                        MD5PInvokeHelper.BCryptOpenAlgorithmProvider(out algorithmHandle, "MD5", null, MD5PInvokeHelper.BCryptOpenAlgorithmProviderFlags.None);
                    }
                }

                return algorithmHandle;
            }
        }

        // P/Invoke methods to utilize the unmanaged MD5 implementation.
        [DllImport("bcrypt.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode, SetLastError = false)]
        internal static extern int BCryptOpenAlgorithmProvider(out SafeBCryptAlgorithmHandle algorithmHandle, string algIdString, string implementationString, BCryptOpenAlgorithmProviderFlags flags);

        [DllImport("bcrypt.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode, SetLastError = false)]
        internal static extern unsafe int BCryptGetProperty(SafeBCryptHandle handleObject, string propertyString, void* outputBuffer, int outputByteLength, out int resultSize, int flags);

        [DllImport("bcrypt.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode, SetLastError = false)]
        internal static extern int BCryptCreateHash(SafeBCryptAlgorithmHandle algorithmHandle, out SafeBCryptHashHandle hashHandle, IntPtr hashObject, int hashObjectByteLength, [In, Out] byte[] secretBuffer, int secretByteLength, BCryptCreateHashFlags flags);

        [DllImport("bcrypt.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode, SetLastError = false)]
        internal static extern unsafe int BCryptHashData(SafeBCryptHashHandle hashHandle, byte* inputBuffer, int inputByteLength, int flags);

        [DllImport("bcrypt.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode, SetLastError = false)]
        internal static extern int BCryptFinishHash(SafeBCryptHashHandle hashHandle, [Out] byte[] outputBuffer, int outputByteLength, int flags);

        [DllImport("bcrypt.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode, SetLastError = false)]
        internal static extern int BCryptDestroyHash(IntPtr hashHandle);

        [DllImport("bcrypt.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode, SetLastError = false)]
        internal static extern int BCryptCloseAlgorithmProvider(IntPtr algorithmHandle, int flags);

        internal static void DisposeMD5Handles()
        {
            lock (syncObject)
            {
                if (algorithmHandle != null)
                {
                    algorithmHandle.Dispose();
                    algorithmHandle = null;
                }
            }
        }

        internal static byte[] CalculateHash(byte[] inputBuffer)
        {
            byte[] hashResult;
            int status;
            SafeBCryptHashHandle hashHandle = null;

            status = MD5PInvokeHelper.BCryptCreateHash(MD5PInvokeHelper.MD5AlgorithmProvider, out hashHandle, IntPtr.Zero, 0, null, 0, 0);
            if (status != MD5PInvokeHelper.StatusSuccess)
            {
                DisposeMD5Handles();
                throw new CryptographicException(status);
            }

            using (hashHandle)
            {
                int sizeOfHashSize;
                int hashSize;
                unsafe
                {
                    status = MD5PInvokeHelper.BCryptGetProperty(hashHandle, MD5PInvokeHelper.BCryptHashLength, &hashSize, sizeof(int), out sizeOfHashSize, 0);
                    if (status != MD5PInvokeHelper.StatusSuccess)
                    {
                        DisposeMD5Handles();
                        throw new CryptographicException(status);
                    }
                }

                unsafe
                {
                    fixed (byte* inputBytePointer = inputBuffer)
                    {
                        status = MD5PInvokeHelper.BCryptHashData(hashHandle, inputBytePointer, inputBuffer.Length, 0);
                        if (status != MD5PInvokeHelper.StatusSuccess)
                        {
                            DisposeMD5Handles();
                            throw new CryptographicException(status);
                        }
                    }
                }

                hashResult = new byte[hashSize];
                status = MD5PInvokeHelper.BCryptFinishHash(hashHandle, hashResult, hashResult.Length, 0);
                if (status != MD5PInvokeHelper.StatusSuccess)
                {
                    DisposeMD5Handles();
                    throw new CryptographicException(status);
                }
            }

            // hashHandle has been disposed by the end of the "using" block.
            hashHandle = null;

            return hashResult;
        }

        internal abstract class SafeBCryptHandle : SafeHandle, IDisposable
        {
            protected SafeBCryptHandle()
                : base(IntPtr.Zero, true)
            {
            }

            public sealed override bool IsInvalid
            {
                get
                {
                    return handle == IntPtr.Zero;
                }
            }

            protected abstract override bool ReleaseHandle();
        }

        internal sealed class SafeBCryptAlgorithmHandle : SafeBCryptHandle
        {
            private SafeBCryptAlgorithmHandle()
                : base()
            {
            }

            protected sealed override bool ReleaseHandle()
            {
                IntPtr localHandle = handle;
                handle = IntPtr.Zero;
                int status = MD5PInvokeHelper.BCryptCloseAlgorithmProvider(localHandle, 0);
                return status == MD5PInvokeHelper.StatusSuccess;
            }
        }

        internal sealed class SafeBCryptHashHandle : SafeBCryptHandle
        {
            private SafeBCryptHashHandle()
                : base()
            {
            }

            protected sealed override bool ReleaseHandle()
            {
                IntPtr localHandle = handle;
                handle = IntPtr.Zero;
                int status = MD5PInvokeHelper.BCryptDestroyHash(localHandle);
                return status == MD5PInvokeHelper.StatusSuccess;
            }
        }
    }
}
