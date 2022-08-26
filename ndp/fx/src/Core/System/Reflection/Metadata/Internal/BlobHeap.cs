// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Collections.Immutable;
using System.Reflection.Internal;
using System.Threading;

namespace System.Reflection.Metadata.Ecma335
{
    internal struct BlobHeap
    {
        internal readonly MemoryBlock Block;

        internal BlobHeap(MemoryBlock block, MetadataKind metadataKind)
        {
            Block = block;
        }

        internal byte[] GetBytes(BlobHandle handle)
        {
            int offset = handle.GetHeapOffset();
            int bytesRead;
            int numberOfBytes = Block.PeekCompressedInteger(offset, out bytesRead);
            if (numberOfBytes == BlobReader.InvalidCompressedInteger)
            {
                return ImmutableArray<byte>.Empty.UnderlyingArray;
            }

            return Block.PeekBytes(offset + bytesRead, numberOfBytes);
        }

        internal MemoryBlock GetMemoryBlock(BlobHandle handle)
        {
            int offset, size;
            Block.PeekHeapValueOffsetAndSize(handle.GetHeapOffset(), out offset, out size);
            return Block.GetMemoryBlockAt(offset, size);
        }

        internal BlobReader GetBlobReader(BlobHandle handle)
        {
            return new BlobReader(GetMemoryBlock(handle));
        }

        public string GetDocumentName(DocumentNameBlobHandle handle)
        {
            var blobReader = GetBlobReader(handle);

            // Spec: separator is an ASCII encoded character in range [0x01, 0x7F], or byte 0 to represent an empty separator.
            int separator = blobReader.ReadByte();
            if (separator > 0x7f)
            {
                throw new BadImageFormatException("InvalidDocumentName");
            }

            var pooledBuilder = PooledStringBuilder.GetInstance();
            var builder = pooledBuilder.Builder;
            bool isFirstPart = true;
            while (blobReader.RemainingBytes > 0)
            {
                if (separator != 0 && !isFirstPart)
                {
                    builder.Append((char)separator);
                }

                var partReader = GetBlobReader(blobReader.ReadBlobHandle());

                // 
                builder.Append(partReader.ReadUTF8(partReader.Length));
                isFirstPart = false;
            }

            return pooledBuilder.ToStringAndFree();
        }
    }
}
