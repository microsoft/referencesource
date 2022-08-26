// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Diagnostics;
using System.IO;
using System.Security;

namespace System.Reflection.Internal
{
    /// <summary>
    /// Represents raw memory owned by an external object. 
    /// </summary>
    internal unsafe sealed class ExternalMemoryBlockProvider : MemoryBlockProvider
    {
        [SecurityCritical]
        private byte* _memory;

        private int _size;

        [SecurityCritical]
        public unsafe ExternalMemoryBlockProvider(byte* memory, int size)
        {
            _memory = memory;
            _size = size;
        }

        public override int Size
        {
            get
            {
                return _size;
            }
        }

        [SecuritySafeCritical]
        protected override AbstractMemoryBlock GetMemoryBlockImpl(int start, int size)
        {
            return new ExternalMemoryBlock(this, _memory + start, size);
        }

        [SecuritySafeCritical]
        public override Stream GetStream(out StreamConstraints constraints)
        {
            constraints = new StreamConstraints(null, 0, _size);
            return new ReadOnlyUnmanagedMemoryStream(_memory, _size);
        }

        [SecuritySafeCritical]
        protected override void Dispose(bool disposing)
        {
            Debug.Assert(disposing);

            // we don't own the memory, just null out the pointer.
            _memory = null;
            _size = 0;
        }

        public byte* Pointer
        {
            [SecurityCritical]
            get
            {
                return _memory;
            }
        }
    }
}
