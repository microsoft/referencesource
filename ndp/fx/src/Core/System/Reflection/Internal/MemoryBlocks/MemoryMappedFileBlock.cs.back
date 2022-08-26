// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;
using System.Security;

namespace System.Reflection.Internal
{
    internal unsafe sealed class MemoryMappedFileBlock : AbstractMemoryBlock
    {
        private sealed class DisposableData : CriticalDisposableObject
        {
            private IDisposable _accessor; // MemoryMappedViewAccessor

            [SecurityCritical]
            private SafeBuffer _safeBuffer;

            [SecurityCritical]
            private byte* _pointer;

            [SecuritySafeCritical]
            public DisposableData(IDisposable accessor, SafeBuffer safeBuffer, long offset)
            {
                // Make sure the current thread isn't aborted in between acquiring the pointer and assigning the fields.
#if !NETSTANDARD11
                RuntimeHelpers.PrepareConstrainedRegions();
#endif
                try
                {
                }
                finally
                {
                    byte* basePointer = null;
                    safeBuffer.AcquirePointer(ref basePointer);

                    _accessor = accessor;
                    _safeBuffer = safeBuffer;
                    _pointer = basePointer + offset;
                }
            }

            [SecuritySafeCritical]
            protected override void Release()
            {
                // Make sure the current thread isn't aborted in between zeroing the references and releasing/disposing.
                // Safe buffer only frees the underlying resource if its ref count drops to zero, so we have to make sure it does.
#if !NETSTANDARD11
                RuntimeHelpers.PrepareConstrainedRegions();
#endif
                try
                {
                }
                finally
                {
                    Interlocked.Exchange(ref _safeBuffer, null)?.ReleasePointer();
                    Interlocked.Exchange(ref _accessor, null)?.Dispose();
                }

                _pointer = null;
            }

            public byte* Pointer
            {
                [SecurityCritical]
                get
                {
                    return _pointer;
                }
            }
        }

        private readonly DisposableData _data;
        private readonly int _size;

        [SecurityCritical]
        internal unsafe MemoryMappedFileBlock(IDisposable accessor, SafeBuffer safeBuffer, long offset, int size)
        {
            _data = new DisposableData(accessor, safeBuffer, offset);
            _size = size;
        }

        public override void Dispose() => _data.Dispose();

        public override byte* Pointer
        {
            [SecurityCritical]
            get
            {
                return _data.Pointer;
            }
        }

        public override int Size => _size;
    }
}
