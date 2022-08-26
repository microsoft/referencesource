// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System.Collections.Immutable;
using System.Diagnostics;
using System.Reflection.Internal;
using System.Reflection.Metadata;
using System.Runtime.InteropServices;
using System.Security;

namespace System.Reflection.PortableExecutable
{
    internal struct PEMemoryBlock
    {
        private readonly AbstractMemoryBlock _block;
        private readonly int _offset;

        internal PEMemoryBlock(AbstractMemoryBlock block, int offset = 0)
        {
            Debug.Assert(block != null);
            Debug.Assert(offset >= 0 && offset <= block.Size);

            _block = block;
            _offset = offset;
        }

        /// <summary>
        /// Pointer to the first byte of the block.
        /// </summary>
        public unsafe byte* Pointer
        {
            [SecurityCritical]
            get { return (_block != null) ? _block.Pointer + _offset : null; }
        }

        /// <summary>
        /// Length of the block.
        /// </summary>
        public int Length => _block?.Size - _offset ?? 0;

        /// <summary>
        /// Creates <see cref="BlobReader"/> for a blob spanning the entire block.
        /// </summary>
        [SecuritySafeCritical]
        public unsafe BlobReader GetReader()
        {
            return new BlobReader(Pointer, Length);
        }

        /// <summary>
        /// Creates <see cref="BlobReader"/> for a blob spanning a part of the block.
        /// </summary>
        /// <exception cref="ArgumentOutOfRangeException">Specified range is not contained within the block.</exception>
        [SecuritySafeCritical]
        public unsafe BlobReader GetReader(int start, int length)
        {
            BlobUtilities.ValidateRange(Length, start, length, nameof(length));
            return new BlobReader(Pointer + start, length);
        }
    }
}
