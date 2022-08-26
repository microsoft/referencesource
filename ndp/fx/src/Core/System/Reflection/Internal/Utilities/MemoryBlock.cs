// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Diagnostics;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using System.Runtime.CompilerServices;
using System.Text;
using System.Security;

namespace System.Reflection.Internal
{
    [DebuggerDisplay("{GetDebuggerDisplay(),nq}")]
    internal unsafe struct MemoryBlock
    {
        [SecurityCritical]
        internal readonly byte* Pointer;

        internal readonly int Length;

        [SecurityCritical]
        internal MemoryBlock(byte* buffer, int length)
        {
            Debug.Assert(length >= 0 && (buffer != null || length == 0));
            this.Pointer = buffer;
            this.Length = length;
        }

        [SecurityCritical]
        internal static MemoryBlock CreateChecked(byte* buffer, int length)
        {
            if (length < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(length));
            }

            if (buffer == null && length != 0)
            {
                throw new ArgumentNullException(nameof(buffer));
            }

            return new MemoryBlock(buffer, length);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private void CheckBounds(int offset, int byteCount)
        {
            if (unchecked((ulong)(uint)offset + (uint)byteCount) > (ulong)Length)
            {
                Throw.OutOfBounds();
            }
        }

        [SecuritySafeCritical]
        internal byte[] ToArray()
        {
            return Pointer == null ? null : PeekBytes(0, Length);
        }

        [SecuritySafeCritical]
        private string GetDebuggerDisplay()
        {
            if (Pointer == null)
            {
                return "<null>";
            }

            int displayedBytes;
            return GetDebuggerDisplay(out displayedBytes);
        }

        internal string GetDebuggerDisplay(out int displayedBytes)
        {
            displayedBytes = Math.Min(Length, 64);
            string result = BitConverter.ToString(PeekBytes(0, displayedBytes));
            if (displayedBytes < Length)
            {
                result += "-...";
            }

            return result;
        }

        [SecuritySafeCritical]
        internal string GetDebuggerDisplay(int offset)
        {
            if (Pointer == null)
            {
                return "<null>";
            }

            int displayedBytes;
            string display = GetDebuggerDisplay(out displayedBytes);
            if (offset < displayedBytes)
            {
                display = display.Insert(offset * 3, "*");
            }
            else if (displayedBytes == Length)
            {
                display += "*";
            }
            else
            {
                display += "*...";
            }

            return display;
        }

        [SecuritySafeCritical]
        internal MemoryBlock GetMemoryBlockAt(int offset, int length)
        {
            CheckBounds(offset, length);
            return new MemoryBlock(Pointer + offset, length);
        }

        [SecuritySafeCritical]
        internal byte PeekByte(int offset)
        {
            CheckBounds(offset, sizeof(byte));
            return Pointer[offset];
        }

        internal int PeekInt32(int offset)
        {
            uint result = PeekUInt32(offset);
            if (unchecked((int)result != result))
            {
                Throw.ValueOverflow();
            }

            return (int)result;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        [SecuritySafeCritical]
        internal uint PeekUInt32(int offset)
        {
            CheckBounds(offset, sizeof(uint));

            unchecked
            {
                byte* ptr = Pointer + offset;
                return (uint)(ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24));
            }
        }

        /// <summary>
        /// Decodes a compressed integer value starting at offset. 
        /// See Metadata Specification section II.23.2: Blobs and signatures.
        /// </summary>
        /// <param name="offset">Offset to the start of the compressed data.</param>
        /// <param name="numberOfBytesRead">Bytes actually read.</param>
        /// <returns>
        /// Value between 0 and 0x1fffffff, or <see cref="BlobReader.InvalidCompressedInteger"/> if the value encoding is invalid.
        /// </returns>
        [SecuritySafeCritical]
        internal int PeekCompressedInteger(int offset, out int numberOfBytesRead)
        {
            CheckBounds(offset, 0);

            byte* ptr = Pointer + offset;
            long limit = Length - offset;

            if (limit == 0)
            {
                numberOfBytesRead = 0;
                return BlobReader.InvalidCompressedInteger;
            }

            byte headerByte = ptr[0];
            if ((headerByte & 0x80) == 0)
            {
                numberOfBytesRead = 1;
                return headerByte;
            }
            else if ((headerByte & 0x40) == 0)
            {
                if (limit >= 2)
                {
                    numberOfBytesRead = 2;
                    return ((headerByte & 0x3f) << 8) | ptr[1];
                }
            }
            else if ((headerByte & 0x20) == 0)
            {
                if (limit >= 4)
                {
                    numberOfBytesRead = 4;
                    return ((headerByte & 0x1f) << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
                }
            }

            numberOfBytesRead = 0;
            return BlobReader.InvalidCompressedInteger;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        [SecuritySafeCritical]
        internal ushort PeekUInt16(int offset)
        {
            CheckBounds(offset, sizeof(ushort));

            unchecked
            {
                byte* ptr = Pointer + offset;
                return (ushort)(ptr[0] | (ptr[1] << 8));
            }
        }

        // When reference has tag bits.
        internal uint PeekTaggedReference(int offset, bool smallRefSize)
        {
            return PeekReferenceUnchecked(offset, smallRefSize);
        }

        // Use when searching for a tagged or non-tagged reference.
        // The result may be an invalid reference and shall only be used to compare with a valid reference.
        internal uint PeekReferenceUnchecked(int offset, bool smallRefSize)
        {
            return smallRefSize ? PeekUInt16(offset) : PeekUInt32(offset);
        }

        // When reference has at most 24 bits.
        internal int PeekReference(int offset, bool smallRefSize)
        {
            if (smallRefSize)
            {
                return PeekUInt16(offset);
            }

            uint value = PeekUInt32(offset);

            if (!TokenTypeIds.IsValidRowId(value))
            {
                Throw.ReferenceOverflow();
            }

            return (int)value;
        }

        // #String, #Blob heaps
        internal int PeekHeapReference(int offset, bool smallRefSize)
        {
            if (smallRefSize)
            {
                return PeekUInt16(offset);
            }

            uint value = PeekUInt32(offset);

            if (!HeapHandleType.IsValidHeapOffset(value))
            {
                Throw.ReferenceOverflow();
            }

            return (int)value;
        }

        [SecuritySafeCritical]
        internal Guid PeekGuid(int offset)
        {
            CheckBounds(offset, sizeof(Guid));

            byte* ptr = Pointer + offset;
            if (BitConverter.IsLittleEndian)
            {
                return *(Guid*)ptr;
            }
            else
            {
                unchecked
                {
                    return new Guid(
                        (int)(ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24)),
                        (short)(ptr[4] | (ptr[5] << 8)),
                        (short)(ptr[6] | (ptr[7] << 8)),
                        ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15]);
                }
            }
        }

        [SecuritySafeCritical]
        internal string PeekUtf16(int offset, int byteCount)
        {
            CheckBounds(offset, byteCount);

            byte* ptr = Pointer + offset;
            if (BitConverter.IsLittleEndian)
            {
                // doesn't allocate a new string if byteCount == 0
                return new string((char*)ptr, 0, byteCount / sizeof(char));
            }
            else
            {
                return Encoding.Unicode.GetString(ptr, byteCount);
            }
        }

        [SecuritySafeCritical]
        internal string PeekUtf8(int offset, int byteCount)
        {
            CheckBounds(offset, byteCount);
            return Encoding.UTF8.GetString(Pointer + offset, byteCount);
        }

        /// <summary>
        /// Read UTF8 at the given offset up to the given terminator, null terminator, or end-of-block.
        /// </summary>
        /// <param name="offset">Offset in to the block where the UTF8 bytes start.</param>
        /// <param name="prefix">UTF8 encoded prefix to prepend to the bytes at the offset before decoding.</param>
        /// <param name="utf8Decoder">The UTF8 decoder to use that allows user to adjust fallback and/or reuse existing strings without allocating a new one.</param>
        /// <param name="numberOfBytesRead">The number of bytes read, which includes the terminator if we did not hit the end of the block.</param>
        /// <param name="terminator">A character in the ASCII range that marks the end of the string. 
        /// If a value other than '\0' is passed we still stop at the null terminator if encountered first.</param>
        /// <returns>The decoded string.</returns>
        [SecuritySafeCritical]
        internal string PeekUtf8NullTerminated(int offset, out int numberOfBytesRead, char terminator = '\0')
        {
            Debug.Assert(terminator <= 0x7F);
            CheckBounds(offset, 0);
            int length = GetUtf8NullTerminatedLength(offset, out numberOfBytesRead, terminator);
            return new string((sbyte*)Pointer, offset, length, Encoding.UTF8);
        }

        /// <summary>
        /// Get number of bytes from offset to given terminator, null terminator, or end-of-block (whichever comes first).
        /// Returned length does not include the terminator, but numberOfBytesRead out parameter does.
        /// </summary>
        /// <param name="offset">Offset in to the block where the UTF8 bytes start.</param>
        /// <param name="terminator">A character in the ASCII range that marks the end of the string. 
        /// If a value other than '\0' is passed we still stop at the null terminator if encountered first.</param>
        /// <param name="numberOfBytesRead">The number of bytes read, which includes the terminator if we did not hit the end of the block.</param>
        /// <returns>Length (byte count) not including terminator.</returns>
        [SecuritySafeCritical]
        internal int GetUtf8NullTerminatedLength(int offset, out int numberOfBytesRead, char terminator = '\0')
        {
            CheckBounds(offset, 0);

            Debug.Assert(terminator <= 0x7f);

            byte* start = Pointer + offset;
            byte* end = Pointer + Length;
            byte* current = start;

            while (current < end)
            {
                byte b = *current;
                if (b == 0 || b == terminator)
                {
                    break;
                }

                current++;
            }

            int length = (int)(current - start);
            numberOfBytesRead = length;
            if (current < end)
            {
                // we also read the terminator
                numberOfBytesRead++;
            }

            return length;
        }

        [SecuritySafeCritical]
        internal byte[] PeekBytes(int offset, int byteCount)
        {
            CheckBounds(offset, byteCount);
            return BlobUtilities.ReadBytes(Pointer + offset, byteCount);
        }

        internal int IndexOf(byte b, int start)
        {
            CheckBounds(start, 0);
            return IndexOfUnchecked(b, start);
        }

        [SecuritySafeCritical]
        internal int IndexOfUnchecked(byte b, int start)
        {
            byte* p = Pointer + start;
            byte* end = Pointer + Length;
            while (p < end)
            {
                if (*p == b)
                {
                    return (int)(p - Pointer);
                }

                p++;
            }

            return -1;
        }

        /// <summary>
        /// In a table ordered by a column containing entity references searches for a row with the specified reference.
        /// </summary>
        /// <returns>Returns row number [0..RowCount) or -1 if not found.</returns>
        internal int BinarySearchReference(
            int rowCount,
            int rowSize,
            int referenceOffset,
            uint referenceValue,
            bool isReferenceSmall)
        {
            int startRowNumber = 0;
            int endRowNumber = rowCount - 1;
            while (startRowNumber <= endRowNumber)
            {
                int midRowNumber = (startRowNumber + endRowNumber) / 2;
                uint midReferenceValue = PeekReferenceUnchecked(midRowNumber * rowSize + referenceOffset, isReferenceSmall);
                if (referenceValue > midReferenceValue)
                {
                    startRowNumber = midRowNumber + 1;
                }
                else if (referenceValue < midReferenceValue)
                {
                    endRowNumber = midRowNumber - 1;
                }
                else
                {
                    return midRowNumber;
                }
            }

            return -1;
        }

        // Row number [0, ptrTable.Length) or -1 if not found.
        internal int BinarySearchReference(
            int[] ptrTable,
            int rowSize,
            int referenceOffset,
            uint referenceValue,
            bool isReferenceSmall)
        {
            int startRowNumber = 0;
            int endRowNumber = ptrTable.Length - 1;
            while (startRowNumber <= endRowNumber)
            {
                int midRowNumber = (startRowNumber + endRowNumber) / 2;
                uint midReferenceValue = PeekReferenceUnchecked((ptrTable[midRowNumber] - 1) * rowSize + referenceOffset, isReferenceSmall);
                if (referenceValue > midReferenceValue)
                {
                    startRowNumber = midRowNumber + 1;
                }
                else if (referenceValue < midReferenceValue)
                {
                    endRowNumber = midRowNumber - 1;
                }
                else
                {
                    return midRowNumber;
                }
            }

            return -1;
        }

        internal int[] BuildPtrTable(
            int numberOfRows,
            int rowSize,
            int referenceOffset,
            bool isReferenceSmall)
        {
            int[] ptrTable = new int[numberOfRows];
            uint[] unsortedReferences = new uint[numberOfRows];

            for (int i = 0; i < ptrTable.Length; i++)
            {
                ptrTable[i] = i + 1;
            }

            ReadColumn(unsortedReferences, rowSize, referenceOffset, isReferenceSmall);
            Array.Sort(ptrTable, (int a, int b) => { return unsortedReferences[a - 1].CompareTo(unsortedReferences[b - 1]); });
            return ptrTable;
        }

        private void ReadColumn(
            uint[] result,
            int rowSize,
            int referenceOffset,
            bool isReferenceSmall)
        {
            int offset = referenceOffset;
            int totalSize = this.Length;

            int i = 0;
            while (offset < totalSize)
            {
                result[i] = PeekReferenceUnchecked(offset, isReferenceSmall);
                offset += rowSize;
                i++;
            }

            Debug.Assert(i == result.Length);
        }

        internal bool PeekHeapValueOffsetAndSize(int index, out int offset, out int size)
        {
            int bytesRead;
            int numberOfBytes = PeekCompressedInteger(index, out bytesRead);
            if (numberOfBytes == BlobReader.InvalidCompressedInteger)
            {
                offset = 0;
                size = 0;
                return false;
            }

            offset = index + bytesRead;
            size = numberOfBytes;
            return true;
        }
    }
}
