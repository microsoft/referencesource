// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Reflection.Internal;

namespace System.Reflection.Metadata.Ecma335
{
    internal struct DocumentTableReader
    {
        internal readonly int NumberOfRows;

        private readonly bool _isGuidHeapRefSizeSmall;
        private readonly bool _isBlobHeapRefSizeSmall;

        private const int NameOffset = 0;
        private readonly int _hashAlgorithmOffset;
        private readonly int _hashOffset;
        private readonly int _languageOffset;

        internal readonly int RowSize;
        internal readonly MemoryBlock Block;

        internal DocumentTableReader(
            int numberOfRows,
            int guidHeapRefSize,
            int blobHeapRefSize,
            MemoryBlock containingBlock,
            int containingBlockOffset)
        {
            NumberOfRows = numberOfRows;
            _isGuidHeapRefSizeSmall = guidHeapRefSize == 2;
            _isBlobHeapRefSizeSmall = blobHeapRefSize == 2;

            _hashAlgorithmOffset = NameOffset + blobHeapRefSize;
            _hashOffset = _hashAlgorithmOffset + guidHeapRefSize;
            _languageOffset = _hashOffset + blobHeapRefSize;
            RowSize = _languageOffset + guidHeapRefSize;

            Block = containingBlock.GetMemoryBlockAt(containingBlockOffset, RowSize * numberOfRows);
        }

        internal DocumentNameBlobHandle GetName(DocumentHandle handle)
        {
            int rowOffset = (handle.RowId - 1) * RowSize;
            return DocumentNameBlobHandle.FromOffset(Block.PeekHeapReference(rowOffset + NameOffset, _isBlobHeapRefSizeSmall));
        }

        internal BlobHandle GetHash(DocumentHandle handle)
        {
            int rowOffset = (handle.RowId - 1) * RowSize;
            return BlobHandle.FromOffset(Block.PeekHeapReference(rowOffset + _hashOffset, _isBlobHeapRefSizeSmall));
        }
    }

    internal struct MethodDebugInformationTableReader
    {
        internal readonly int NumberOfRows;

        private readonly bool _isDocumentRefSmall;
        private readonly bool _isBlobHeapRefSizeSmall;

        private const int DocumentOffset = 0;
        private readonly int _sequencePointsOffset;

        internal readonly int RowSize;
        internal readonly MemoryBlock Block;

        internal MethodDebugInformationTableReader(
            int numberOfRows,
            int documentRefSize,
            int blobHeapRefSize,
            MemoryBlock containingBlock,
            int containingBlockOffset)
        {
            NumberOfRows = numberOfRows;
            _isDocumentRefSmall = documentRefSize == 2;
            _isBlobHeapRefSizeSmall = blobHeapRefSize == 2;

            _sequencePointsOffset = DocumentOffset + documentRefSize;
            RowSize = _sequencePointsOffset + blobHeapRefSize;

            Block = containingBlock.GetMemoryBlockAt(containingBlockOffset, RowSize * numberOfRows);
        }

        internal DocumentHandle GetDocument(MethodDebugInformationHandle handle)
        {
            int rowOffset = (handle.RowId - 1) * RowSize;
            return DocumentHandle.FromRowId(Block.PeekReference(rowOffset + DocumentOffset, _isDocumentRefSmall));
        }

        internal BlobHandle GetSequencePoints(MethodDebugInformationHandle handle)
        {
            int rowOffset = (handle.RowId - 1) * RowSize;
            return BlobHandle.FromOffset(Block.PeekHeapReference(rowOffset + _sequencePointsOffset, _isBlobHeapRefSizeSmall));
        }
    }
}
