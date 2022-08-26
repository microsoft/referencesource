// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Diagnostics;
using System.Reflection.Metadata.Ecma335;

namespace System.Reflection.Metadata
{
    internal struct MethodDefinitionHandle : IEquatable<MethodDefinitionHandle>
    {
        private const uint tokenType = TokenTypeIds.MethodDef;
        private const byte tokenTypeSmall = (byte)HandleType.MethodDef;
        private readonly int _rowId;

        private MethodDefinitionHandle(int rowId)
        {
            Debug.Assert(TokenTypeIds.IsValidRowId(rowId));
            _rowId = rowId;
        }

        internal static MethodDefinitionHandle FromRowId(int rowId)
        {
            return new MethodDefinitionHandle(rowId);
        }

        public static implicit operator Handle(MethodDefinitionHandle handle)
        {
            return new Handle(tokenTypeSmall, handle._rowId);
        }

        public static explicit operator MethodDefinitionHandle(Handle handle)
        {
            if (handle.VType != tokenTypeSmall)
            {
                Throw.InvalidCast();
            }

            return new MethodDefinitionHandle(handle.RowId);
        }

        public bool IsNil
        {
            get
            {
                return RowId == 0;
            }
        }

        internal int RowId { get { return _rowId; } }

        public static bool operator ==(MethodDefinitionHandle left, MethodDefinitionHandle right)
        {
            return left._rowId == right._rowId;
        }

        public override bool Equals(object obj)
        {
            return obj is MethodDefinitionHandle && ((MethodDefinitionHandle)obj)._rowId == _rowId;
        }

        public bool Equals(MethodDefinitionHandle other)
        {
            return _rowId == other._rowId;
        }

        public override int GetHashCode()
        {
            return _rowId.GetHashCode();
        }

        public static bool operator !=(MethodDefinitionHandle left, MethodDefinitionHandle right)
        {
            return left._rowId != right._rowId;
        }

        /// <summary>
        /// Returns a handle to <see cref="MethodDebugInformation"/> corresponding to this handle.
        /// </summary>
        /// <remarks>
        /// The resulting handle is only valid within the context of a <see cref="MetadataReader"/> open on the Portable PDB blob,
        /// which in case of standalone PDB file is a different reader than the one containing this method definition.
        /// </remarks>
        public MethodDebugInformationHandle ToDebugInformationHandle()
        {
            return MethodDebugInformationHandle.FromRowId(_rowId);
        }
    }

    // #Blob heap handle
    internal struct BlobHandle : IEquatable<BlobHandle>
    {
        // bits:
        //     31: IsVirtual
        // 29..30: 0
        //  0..28: Heap offset or Virtual Value (16 bits) + Virtual Index (8 bits)
        private readonly uint _value;

        internal enum VirtualIndex : byte
        {
            Nil,

            // B0 3F 5F 7F 11 D5 0A 3A
            ContractPublicKeyToken,

            // 00, 24, 00, 00, 04, ...
            ContractPublicKey,

            // Template for projected AttributeUsage attribute blob
            AttributeUsage_AllowSingle,

            // Template for projected AttributeUsage attribute blob with AllowMultiple=true
            AttributeUsage_AllowMultiple,

            Count
        }

        private BlobHandle(uint value)
        {
            _value = value;
        }

        internal static BlobHandle FromOffset(int heapOffset)
        {
            return new BlobHandle((uint)heapOffset);
        }

        internal const int TemplateParameterOffset_AttributeUsageTarget = 2;

        internal unsafe void SubstituteTemplateParameters(byte[] blob)
        {
            Debug.Assert(blob.Length >= TemplateParameterOffset_AttributeUsageTarget + 4);

            fixed (byte* ptr = &blob[TemplateParameterOffset_AttributeUsageTarget])
            {
                *((uint*)ptr) = VirtualValue;
            }
        }

        public bool IsNil
        {
            get { return _value == 0; }
        }

        internal int GetHeapOffset()
        {
            Debug.Assert(!IsVirtual, "is virtual");
            return (int)_value;
        }

        internal VirtualIndex GetVirtualIndex()
        {
            Debug.Assert(IsVirtual, "is not virtual");
            return (VirtualIndex)(_value & 0xff);
        }

        internal bool IsVirtual
        {
            get { return (_value & TokenTypeIds.VirtualBit) != 0; }
        }

        private ushort VirtualValue
        {
            get { return unchecked((ushort)(_value >> 8)); }
        }

        public override bool Equals(object obj)
        {
            return obj is BlobHandle && Equals((BlobHandle)obj);
        }

        public bool Equals(BlobHandle other)
        {
            return _value == other._value;
        }

        public override int GetHashCode()
        {
            return unchecked((int)_value);
        }
    }
}
