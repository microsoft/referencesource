// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using System.Runtime.CompilerServices;

namespace System.Reflection
{
    // This file defines an internal class used to throw exceptions. The main purpose is to reduce code size.
    // Also it improves the likelihood that callers will be inlined.
    internal static class Throw
    {
        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void InvalidCast()
        {
            throw new InvalidCastException();
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void LitteEndianArchitectureRequired()
        {
            throw new PlatformNotSupportedException("LitteEndianArchitectureRequired");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void InvalidArgument(string message, string parameterName)
        {
            throw new ArgumentException(message, parameterName);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void InvalidOperation(string message)
        {
            throw new InvalidOperationException(message);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void HeapHandleRequired()
        {
            throw new ArgumentException("NotMetadataHeapHandle", "handle");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void EntityOrUserStringHandleRequired()
        {
            throw new ArgumentException("NotMetadataTableOrUserStringHandle", "handle");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void InvalidToken()
        {
            throw new ArgumentException("InvalidToken", "token");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void ArgumentNull(string parameterName)
        {
            throw new ArgumentNullException(parameterName);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void ValueArgumentNull()
        {
            throw new ArgumentNullException("value");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void BuilderArgumentNull()
        {
            throw new ArgumentNullException("builder");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void ArgumentOutOfRange(string parameterName)
        {
            throw new ArgumentOutOfRangeException(parameterName);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void ArgumentOutOfRange(string parameterName, string message)
        {
            throw new ArgumentOutOfRangeException(parameterName, message);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void IndexOutOfRange()
        {
            throw new ArgumentOutOfRangeException("index");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void TableIndexOutOfRange()
        {
            throw new ArgumentOutOfRangeException("tableIndex");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void ValueArgumentOutOfRange()
        {
            throw new ArgumentOutOfRangeException("value");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void OutOfBounds()
        {
            throw new BadImageFormatException("OutOfBoundsRead");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void WriteOutOfBounds()
        {
            throw new InvalidOperationException("OutOfBoundsWrite");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void InvalidCodedIndex()
        {
            throw new BadImageFormatException("InvalidCodedIndex");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void InvalidHandle()
        {
            throw new BadImageFormatException("InvalidHandle");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void InvalidCompressedInteger()
        {
            throw new BadImageFormatException("InvalidCompressedInteger");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void ImageTooSmall()
        {
            throw new BadImageFormatException("ImageTooSmall");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void ImageTooSmallOrContainsInvalidOffsetOrCount()
        {
            throw new BadImageFormatException("ImageTooSmallOrContainsInvalidOffsetOrCount");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void ReferenceOverflow()
        {
            throw new BadImageFormatException("RowIdOrHeapOffsetTooLarge");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void TableNotSorted(TableIndex tableIndex)
        {
            throw new BadImageFormatException("MetadataTableNotSorted");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void InvalidOperation_PEImageNotAvailable()
        {
            throw new InvalidOperationException("PEImageNotAvailable");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void ValueOverflow()
        {
            throw new BadImageFormatException("ValueTooLarge");
        }
        internal static void SequencePointValueOutOfRange()
        {
            throw new BadImageFormatException("SequencePointValueOutOfRange");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void PEReaderDisposed()
        {
            throw new ObjectDisposedException(nameof(PortableExecutable.PEReader));
        }
    }
}
