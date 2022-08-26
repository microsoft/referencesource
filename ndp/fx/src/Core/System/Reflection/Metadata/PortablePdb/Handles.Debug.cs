// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Diagnostics;
using System.Reflection.Metadata.Ecma335;

namespace System.Reflection.Metadata
{
    internal struct DocumentHandle : IEquatable<DocumentHandle>
    {
        private readonly int _rowId;

        private DocumentHandle(int rowId)
        {
            Debug.Assert(TokenTypeIds.IsValidRowId(rowId));
            _rowId = rowId;
        }

        internal static DocumentHandle FromRowId(int rowId)
        {
            return new DocumentHandle(rowId);
        }

        public bool IsNil
        {
            get
            {
                return RowId == 0;
            }
        }

        internal int RowId { get { return _rowId; } }

        public static bool operator ==(DocumentHandle left, DocumentHandle right)
        {
            return left._rowId == right._rowId;
        }

        public override bool Equals(object obj)
        {
            return obj is DocumentHandle && ((DocumentHandle)obj)._rowId == _rowId;
        }

        public bool Equals(DocumentHandle other)
        {
            return _rowId == other._rowId;
        }

        public override int GetHashCode()
        {
            return _rowId.GetHashCode();
        }

        public static bool operator !=(DocumentHandle left, DocumentHandle right)
        {
            return left._rowId != right._rowId;
        }
    }

    internal struct MethodDebugInformationHandle : IEquatable<MethodDebugInformationHandle>
    {
        private const uint tokenType = TokenTypeIds.MethodDebugInformation;
        private const byte tokenTypeSmall = (byte)HandleType.MethodDebugInformation;
        private readonly int _rowId;

        private MethodDebugInformationHandle(int rowId)
        {
            Debug.Assert(TokenTypeIds.IsValidRowId(rowId));
            _rowId = rowId;
        }

        internal static MethodDebugInformationHandle FromRowId(int rowId)
        {
            return new MethodDebugInformationHandle(rowId);
        }

        public static implicit operator Handle(MethodDebugInformationHandle handle)
        {
            return new Handle(tokenTypeSmall, handle._rowId);
        }

        public static explicit operator MethodDebugInformationHandle(Handle handle)
        {
            if (handle.VType != tokenTypeSmall)
            {
                Throw.InvalidCast();
            }

            return new MethodDebugInformationHandle(handle.RowId);
        }

        public bool IsNil
        {
            get
            {
                return RowId == 0;
            }
        }

        internal int RowId { get { return _rowId; } }

        public static bool operator ==(MethodDebugInformationHandle left, MethodDebugInformationHandle right)
        {
            return left._rowId == right._rowId;
        }

        public override bool Equals(object obj)
        {
            return obj is MethodDebugInformationHandle && ((MethodDebugInformationHandle)obj)._rowId == _rowId;
        }

        public bool Equals(MethodDebugInformationHandle other)
        {
            return _rowId == other._rowId;
        }

        public override int GetHashCode()
        {
            return _rowId.GetHashCode();
        }

        public static bool operator !=(MethodDebugInformationHandle left, MethodDebugInformationHandle right)
        {
            return left._rowId != right._rowId;
        }
    }
}
