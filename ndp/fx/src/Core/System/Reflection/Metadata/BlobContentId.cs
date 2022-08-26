// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Collections.Generic;
using System.Collections.Immutable;
using System.Reflection.Internal;
using System.Security;

namespace System.Reflection.Metadata
{
    internal struct BlobContentId : IEquatable<BlobContentId>
    {
        private const int Size = BlobUtilities.SizeOfGuid + sizeof(uint);

        public Guid Guid { get; }
        public uint Stamp { get; }

        public BlobContentId(Guid guid, uint stamp)
        {
            Guid = guid;
            Stamp = stamp;
        }

        public BlobContentId(ImmutableArray<byte> id) 
            : this(id.UnderlyingArray)
        {
        }

        [SecuritySafeCritical]
        public unsafe BlobContentId(byte[] id)
        {
            if (id == null)
            {
                throw new ArgumentNullException(nameof(id));
            }

            if (id.Length != Size)
            {
                throw new ArgumentException("UnexpectedArrayLength", nameof(id));
            }

            fixed (byte* ptr = &id[0])
            {
                var reader = new BlobReader(ptr, id.Length);
                Guid = reader.ReadGuid();
                Stamp = reader.ReadUInt32();
            }
        }

        public bool IsDefault => Guid == default(Guid) && Stamp == 0;

        public bool Equals(BlobContentId other) => Guid == other.Guid && Stamp == other.Stamp;
        public override bool Equals(object obj) => obj is BlobContentId && Equals((BlobContentId)obj);
        public override int GetHashCode() => Hash.Combine(Stamp, Guid.GetHashCode());
        public static bool operator ==(BlobContentId left, BlobContentId right) => left.Equals(right);
        public static bool operator !=(BlobContentId left, BlobContentId right) => !left.Equals(right);
    }
}
