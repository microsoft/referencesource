// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Collections.Immutable;
using System.Reflection.Metadata;
using System.Security;

namespace System.Reflection.Internal
{
    /// <summary>
    /// Represents a disposable blob of memory accessed via unsafe pointer.
    /// </summary>
    internal abstract class AbstractMemoryBlock : IDisposable
    {
        /// <summary>abstractmemoryblock.cs
        /// Pointer to the underlying data (not valid after disposal).
        /// </summary>
        public unsafe abstract byte* Pointer
        {
            [SecuritySafeCritical]
            get;
        }

        /// <summary>
        /// Size of the block.
        /// </summary>
        public abstract int Size { get; }

        [SecuritySafeCritical]
        public unsafe BlobReader GetReader() => new BlobReader(Pointer, Size);

        /// <summary>
        /// Disposes the block. 
        /// </summary>
        /// <remarks>
        /// The operation is idempotent, but must not be called concurrently with any other operations on the block.
        /// 
        /// Using the block after dispose is an error in our code and therefore no effort is made to throw a tidy 
        /// ObjectDisposedException and null ref or AV is possible.
        /// </remarks>
        public abstract void Dispose();
    }
}
