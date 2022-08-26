// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Collections.Immutable;
using System.Diagnostics;
using System.Reflection.Internal;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;

namespace System.Reflection
{
    internal static unsafe class BlobUtilities
    {
        [SecuritySafeCritical]
        public static byte[] ReadBytes(byte* buffer, int byteCount)
        {
            if (byteCount == 0)
            {
                return new byte[0];
            }

            byte[] result = new byte[byteCount];
            Marshal.Copy((IntPtr)buffer, result, 0, byteCount);
            return result;
        }

        [SecuritySafeCritical]
        public static ImmutableArray<byte> ReadImmutableBytes(byte* buffer, int byteCount)
        {
            byte[] bytes = ReadBytes(buffer, byteCount);
            return new ImmutableArray<byte>(bytes);
        }

        public const int SizeOfGuid = 16;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void ValidateRange(int bufferLength, int start, int byteCount, string byteCountParameterName)
        {
            if (start < 0 || start > bufferLength)
            {
                Throw.ArgumentOutOfRange(nameof(start));
            }

            if (byteCount < 0 || byteCount > bufferLength - start)
            {
                Throw.ArgumentOutOfRange(byteCountParameterName);
            }
        }
    }
}
