// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Diagnostics;
using System.Reflection.Internal;
using System.Reflection.Metadata.Ecma335;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Security;

namespace System.Reflection.Metadata
{
    /// <summary>
    /// Reads metadata as defined byte the ECMA 335 CLI specification.
    /// </summary>
    internal sealed partial class MetadataReader
    {
        internal readonly MemoryBlock Block;
        private readonly MetadataReaderOptions _options;

        #region Constructors

        /// <summary>
        /// Creates a metadata reader from the metadata stored at the given memory location.
        /// </summary>
        /// <remarks>
        /// The memory is owned by the caller and it must be kept memory alive and unmodified throughout the lifetime of the <see cref="MetadataReader"/>.
        /// </remarks>
        /// <exception cref="ArgumentOutOfRangeException"><paramref name="length"/> is not positive.</exception>
        /// <exception cref="ArgumentNullException"><paramref name="metadata"/> is null.</exception>
        /// <exception cref="PlatformNotSupportedException">The current platform is big-endian.</exception>
        /// <exception cref="BadImageFormatException">Bad metadata header.</exception>
        [SecurityCritical]
        public unsafe MetadataReader(byte* metadata, int length, MetadataReaderOptions options)
        {
            // Do not throw here when length is 0. We'll throw BadImageFormatException later on, so that the caller doesn't need to 
            // worry about the image (stream) being empty and can handle all image errors by catching BadImageFormatException.
            if (length < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(length));
            }

            if (metadata == null)
            {
                throw new ArgumentNullException(nameof(metadata));
            }

            this.Block = new MemoryBlock(metadata, length);

            _options = options;

            var headerReader = new BlobReader(this.Block);
            this.ReadMetadataHeader(ref headerReader, out _versionString);
            _metadataKind = GetMetadataKind(_versionString);
            var streamHeaders = this.ReadStreamHeaders(ref headerReader);

            // storage header and stream headers:
            MemoryBlock metadataTableStream;
            MemoryBlock standalonePdbStream;
            this.InitializeStreamReaders(ref this.Block, streamHeaders, out _metadataStreamKind, out metadataTableStream, out standalonePdbStream);

            int[] externalTableRowCountsOpt;
            if (standalonePdbStream.Length > 0)
            {
                ReadStandalonePortablePdbStream(standalonePdbStream, out _debugMetadataHeader, out externalTableRowCountsOpt);
            }
            else
            {
                externalTableRowCountsOpt = null;
            }

            var tableReader = new BlobReader(metadataTableStream);

            HeapSizes heapSizes;
            int[] metadataTableRowCounts;
            this.ReadMetadataTableHeader(ref tableReader, out heapSizes, out metadataTableRowCounts, out _sortedTables);

            this.InitializeTableReaders(tableReader.GetMemoryBlockAt(0, tableReader.RemainingBytes), heapSizes, metadataTableRowCounts, externalTableRowCountsOpt);
        }

        #endregion

        #region Metadata Headers

        private readonly string _versionString;
        private readonly MetadataKind _metadataKind;
        private readonly MetadataStreamKind _metadataStreamKind;
        private readonly DebugMetadataHeader _debugMetadataHeader;

        internal BlobHeap BlobHeap;

        /// <summary>
        /// True if the metadata stream has minimal delta format. Used for EnC.
        /// </summary>
        /// <remarks>
        /// The metadata stream has minimal delta format if "#JTD" stream is present.
        /// Minimal delta format uses large size (4B) when encoding table/heap references.
        /// The heaps in minimal delta only contain data of the delta, 
        /// there is no padding at the beginning of the heaps that would align them 
        /// with the original full metadata heaps.
        /// </remarks>
        internal bool IsMinimalDelta;

        /// <summary>
        /// Looks like this function reads beginning of the header described in
        /// ECMA-335 24.2.1 Metadata root
        /// </summary>
        private void ReadMetadataHeader(ref BlobReader memReader, out string versionString)
        {
            if (memReader.RemainingBytes < COR20Constants.MinimumSizeofMetadataHeader)
            {
                throw new BadImageFormatException("MetadataHeaderTooSmall");
            }

            uint signature = memReader.ReadUInt32();
            if (signature != COR20Constants.COR20MetadataSignature)
            {
                throw new BadImageFormatException("MetadataSignature");
            }

            // major version
            memReader.ReadUInt16();

            // minor version
            memReader.ReadUInt16();

            // reserved:
            memReader.ReadUInt32();

            int versionStringSize = memReader.ReadInt32();
            if (memReader.RemainingBytes < versionStringSize)
            {
                throw new BadImageFormatException("NotEnoughSpaceForVersionString");
            }

            int numberOfBytesRead;
            versionString = memReader.GetMemoryBlockAt(0, versionStringSize).PeekUtf8NullTerminated(0, out numberOfBytesRead);
            memReader.Offset += versionStringSize;
        }

        private MetadataKind GetMetadataKind(string versionString)
        {
            // Treat metadata as CLI raw metadata if the client doesn't want to see projections.
            if ((_options & MetadataReaderOptions.ApplyWindowsRuntimeProjections) == 0)
            {
                return MetadataKind.Ecma335;
            }

            if (!versionString.Contains("WindowsRuntime"))
            {
                return MetadataKind.Ecma335;
            }
            else if (versionString.Contains("CLR"))
            {
                return MetadataKind.ManagedWindowsMetadata;
            }
            else
            {
                return MetadataKind.WindowsMetadata;
            }
        }

        /// <summary>
        /// Reads stream headers described in ECMA-335 24.2.2 Stream header
        /// </summary>
        private StreamHeader[] ReadStreamHeaders(ref BlobReader memReader)
        {
            // storage header:
            memReader.ReadUInt16();
            int streamCount = memReader.ReadInt16();

            var streamHeaders = new StreamHeader[streamCount];
            for (int i = 0; i < streamHeaders.Length; i++)
            {
                if (memReader.RemainingBytes < COR20Constants.MinimumSizeofStreamHeader)
                {
                    throw new BadImageFormatException("StreamHeaderTooSmall");
                }

                streamHeaders[i].Offset = memReader.ReadUInt32();
                streamHeaders[i].Size = memReader.ReadInt32();
                streamHeaders[i].Name = memReader.ReadUtf8NullTerminated();
                bool aligned = memReader.TryAlign(4);

                if (!aligned || memReader.RemainingBytes == 0)
                {
                    throw new BadImageFormatException("NotEnoughSpaceForStreamHeaderName");
                }
            }

            return streamHeaders;
        }

        private void InitializeStreamReaders(
            ref MemoryBlock metadataRoot, 
            StreamHeader[] streamHeaders, 
            out MetadataStreamKind metadataStreamKind,
            out MemoryBlock metadataTableStream,
            out MemoryBlock standalonePdbStream)
        {
            metadataTableStream = default(MemoryBlock);
            standalonePdbStream = default(MemoryBlock);
            metadataStreamKind = MetadataStreamKind.Illegal;

            foreach (StreamHeader streamHeader in streamHeaders)
            {
                switch (streamHeader.Name)
                {
                    case COR20Constants.StringStreamName:
                        if (metadataRoot.Length < streamHeader.Offset + streamHeader.Size)
                        {
                            throw new BadImageFormatException("NotEnoughSpaceForStringStream");
                        }

                        break;

                    case COR20Constants.BlobStreamName:
                        if (metadataRoot.Length < streamHeader.Offset + streamHeader.Size)
                        {
                            throw new BadImageFormatException("NotEnoughSpaceForBlobStream");
                        }

                        this.BlobHeap = new BlobHeap(metadataRoot.GetMemoryBlockAt((int)streamHeader.Offset, streamHeader.Size), _metadataKind);
                        break;

                    case COR20Constants.GUIDStreamName:
                        if (metadataRoot.Length < streamHeader.Offset + streamHeader.Size)
                        {
                            throw new BadImageFormatException("NotEnoughSpaceForGUIDStream");
                        }

                        break;

                    case COR20Constants.UserStringStreamName:
                        if (metadataRoot.Length < streamHeader.Offset + streamHeader.Size)
                        {
                            throw new BadImageFormatException("NotEnoughSpaceForBlobStream");
                        }

                        break;

                    case COR20Constants.CompressedMetadataTableStreamName:
                        if (metadataRoot.Length < streamHeader.Offset + streamHeader.Size)
                        {
                            throw new BadImageFormatException("NotEnoughSpaceForMetadataStream");
                        }

                        metadataStreamKind = MetadataStreamKind.Compressed;
                        metadataTableStream = metadataRoot.GetMemoryBlockAt((int)streamHeader.Offset, streamHeader.Size);
                        break;

                    case COR20Constants.UncompressedMetadataTableStreamName:
                        if (metadataRoot.Length < streamHeader.Offset + streamHeader.Size)
                        {
                            throw new BadImageFormatException("NotEnoughSpaceForMetadataStream");
                        }

                        metadataStreamKind = MetadataStreamKind.Uncompressed;
                        metadataTableStream = metadataRoot.GetMemoryBlockAt((int)streamHeader.Offset, streamHeader.Size);
                        break;

                    case COR20Constants.MinimalDeltaMetadataTableStreamName:
                        if (metadataRoot.Length < streamHeader.Offset + streamHeader.Size)
                        {
                            throw new BadImageFormatException("NotEnoughSpaceForMetadataStream");
                        }

                        // the content of the stream is ignored
                        this.IsMinimalDelta = true;
                        break;

                    case COR20Constants.StandalonePdbStreamName:
                        if (metadataRoot.Length < streamHeader.Offset + streamHeader.Size)
                        {
                            throw new BadImageFormatException("NotEnoughSpaceForMetadataStream");
                        }

                        standalonePdbStream = metadataRoot.GetMemoryBlockAt((int)streamHeader.Offset, streamHeader.Size);
                        break;

                    default:
                        // Skip unknown streams. Some obfuscators insert invalid streams.
                        continue;
                }
            }

            if (IsMinimalDelta && metadataStreamKind != MetadataStreamKind.Uncompressed)
            {
                throw new BadImageFormatException("InvalidMetadataStreamFormat");
            }
        }

        #endregion

        #region Tables and Heaps

        private readonly TableMask _sortedTables;

        /// <summary>
        /// A row count for each possible table. May be indexed by <see cref="TableIndex"/>.
        /// </summary>
        internal int[] TableRowCounts;

        // debug tables
        internal DocumentTableReader DocumentTable;
        internal MethodDebugInformationTableReader MethodDebugInformationTable;

        private void ReadMetadataTableHeader(ref BlobReader reader, out HeapSizes heapSizes, out int[] metadataTableRowCounts, out TableMask sortedTables)
        {
            if (reader.RemainingBytes < MetadataStreamConstants.SizeOfMetadataTableHeader)
            {
                throw new BadImageFormatException("MetadataTableHeaderTooSmall");
            }

            // reserved (shall be ignored):
            reader.ReadUInt32();

            // major version (shall be ignored):
            reader.ReadByte();

            // minor version (shall be ignored):
            reader.ReadByte();

            // heap sizes:
            heapSizes = (HeapSizes)reader.ReadByte();

            // reserved (shall be ignored):
            reader.ReadByte();

            ulong presentTables = reader.ReadUInt64();
            sortedTables = (TableMask)reader.ReadUInt64();

            // According to ECMA-335, MajorVersion and MinorVersion have fixed values and, 
            // based on recommendation in 24.1 Fixed fields: When writing these fields it 
            // is best that they be set to the value indicated, on reading they should be ignored.
            // We will not be checking version values. We will continue checking that the set of 
            // present tables is within the set we understand.

            ulong validTables = (ulong)(TableMask.TypeSystemTables | TableMask.DebugTables);

            if ((presentTables & ~validTables) != 0)
            {
                throw new BadImageFormatException("UnknownTables");
            }

            if (_metadataStreamKind == MetadataStreamKind.Compressed)
            {
                // In general Ptr tables and EnC tables are not allowed in a compressed stream.
                // However when asked for a snapshot of the current metadata after an EnC change has been applied 
                // the CLR includes the EnCLog table into the snapshot. We need to be able to read the image,
                // so we'll allow the table here but pretend it's empty later.
                if ((presentTables & (ulong)(TableMask.PtrTables | TableMask.EnCMap)) != 0)
                {
                    throw new BadImageFormatException("IllegalTablesInCompressedMetadataStream");
                }
            }

            metadataTableRowCounts = ReadMetadataTableRowCounts(ref reader, presentTables);

            if ((heapSizes & HeapSizes.ExtraData) == HeapSizes.ExtraData)
            {
                // Skip "extra data" used by some obfuscators. Although it is not mentioned in the CLI spec,
                // it is honored by the native metadata reader.
                reader.ReadUInt32();
            }
        }

        private static int[] ReadMetadataTableRowCounts(ref BlobReader memReader, ulong presentTableMask)
        {
            ulong currentTableBit = 1;

            var rowCounts = new int[MetadataTokens.TableCount];
            for (int i = 0; i < rowCounts.Length; i++)
            {
                if ((presentTableMask & currentTableBit) != 0)
                {
                    if (memReader.RemainingBytes < sizeof(uint))
                    {
                        throw new BadImageFormatException("TableRowCountSpaceTooSmall");
                    }

                    uint rowCount = memReader.ReadUInt32();
                    if (rowCount > TokenTypeIds.RIDMask)
                    {
                        throw new BadImageFormatException("InvalidRowCount");
                    }

                    rowCounts[i] = (int)rowCount;
                }

                currentTableBit <<= 1;
            }

            return rowCounts;
        }

        // internal for testing
        internal static void ReadStandalonePortablePdbStream(MemoryBlock block, out DebugMetadataHeader debugMetadataHeader, out int[] externalTableRowCounts)
        {
            var reader = new BlobReader(block);

            const int PdbIdSize = 20;
            byte[] pdbId = reader.ReadBytes(PdbIdSize);

            // ECMA-335 15.4.1.2:
            // The entry point to an application shall be static.
            // This entry point method can be a global method or it can appear inside a type. 
            // The entry point method shall either accept no arguments or a vector of strings.
            // The return type of the entry point method shall be void, int32, or unsigned int32. 
            // The entry point method cannot be defined in a generic class.
            uint entryPointToken = reader.ReadUInt32();
            int entryPointRowId = (int)(entryPointToken & TokenTypeIds.RIDMask);
            if (entryPointToken != 0 && ((entryPointToken & TokenTypeIds.TypeMask) != TokenTypeIds.MethodDef || entryPointRowId == 0))
            {
                throw new BadImageFormatException("InvalidEntryPointToken");
            }

            ulong externalTableMask = reader.ReadUInt64();

            // EnC & Ptr tables can't be referenced from standalone PDB metadata:
            const ulong validTables = (ulong)TableMask.ValidPortablePdbExternalTables;

            if ((externalTableMask & ~validTables) != 0)
            {
                throw new BadImageFormatException("UnknownTables");
            }

            externalTableRowCounts = ReadMetadataTableRowCounts(ref reader, externalTableMask);

            debugMetadataHeader = new DebugMetadataHeader(
                new ImmutableArray<byte>(pdbId),
                MethodDefinitionHandle.FromRowId(entryPointRowId));
        }

        private const int SmallIndexSize = 2;
        private const int LargeIndexSize = 4;

        private int GetReferenceSize(int[] rowCounts, TableIndex index)
        {
            return (rowCounts[(int)index] < MetadataStreamConstants.LargeTableRowCount && !IsMinimalDelta) ? SmallIndexSize : LargeIndexSize;
        }

        private void InitializeTableReaders(MemoryBlock metadataTablesMemoryBlock, HeapSizes heapSizes, int[] rowCounts, int[] externalRowCountsOpt)
        {
            // Size of reference tags in each table.
            this.TableRowCounts = rowCounts;

            // 


            // Compute HeapRef Sizes

            int guidHeapRefSize = (heapSizes & HeapSizes.GuidHeapLarge) == HeapSizes.GuidHeapLarge ? LargeIndexSize : SmallIndexSize;
            int blobHeapRefSize = (heapSizes & HeapSizes.BlobHeapLarge) == HeapSizes.BlobHeapLarge ? LargeIndexSize : SmallIndexSize;

            // Populate the Table blocks
            int totalRequiredSize = 0;

            this.DocumentTable = new DocumentTableReader(rowCounts[(int)TableIndex.Document], guidHeapRefSize, blobHeapRefSize, metadataTablesMemoryBlock, totalRequiredSize);
            totalRequiredSize += this.DocumentTable.Block.Length;

            this.MethodDebugInformationTable = new MethodDebugInformationTableReader(rowCounts[(int)TableIndex.MethodDebugInformation], GetReferenceSize(rowCounts, TableIndex.Document), blobHeapRefSize, metadataTablesMemoryBlock, totalRequiredSize);
            totalRequiredSize += this.MethodDebugInformationTable.Block.Length;

            if (totalRequiredSize > metadataTablesMemoryBlock.Length)
            {
                throw new BadImageFormatException("MetadataTablesTooSmall");
            }
        }
 
        /// <summary>

        /// <summary>
        /// Options passed to the constructor.
        /// </summary>
        public MetadataReaderOptions Options { get { return _options; } }

        /// <summary>
        /// Version string read from metadata header.
        /// </summary>
        public string MetadataVersion => _versionString;

        /// <summary>
        /// Information decoded from #Pdb stream, or null if the stream is not present.
        /// </summary>
        public DebugMetadataHeader DebugMetadataHeader => _debugMetadataHeader;
        
        public string GetString(DocumentNameBlobHandle handle)
        {
            return BlobHeap.GetDocumentName(handle);
        }

        public Document GetDocument(DocumentHandle handle)
        {
            return new Document(this, handle);
        }

        public MethodDebugInformation GetMethodDebugInformation(MethodDebugInformationHandle handle)
        {
            return new MethodDebugInformation(this, handle);
        }

        #endregion
    }
}
