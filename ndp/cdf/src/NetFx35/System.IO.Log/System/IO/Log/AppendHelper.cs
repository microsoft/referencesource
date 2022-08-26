//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.IO.Log
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    class AppendHelper : IDisposable
    {
        SequenceNumber prev;
        SequenceNumber next;
        FileLogRecordHeader header;
        UnmanagedBlob[] blobs;
        GCHandle[] handles;

        public AppendHelper(IList<ArraySegment<byte>> data, 
                                SequenceNumber prev, 
                                SequenceNumber next,
                                bool restartArea)
        {
            this.prev = prev;
            this.next = next;

            this.header = new FileLogRecordHeader(null);
            this.header.IsRestartArea = restartArea;
            this.header.PreviousLsn = prev;
            this.header.NextUndoLsn = next;

            this.blobs = new UnmanagedBlob[data.Count + 1];
            this.handles = new GCHandle[data.Count + 1];

            try
            {
                this.handles[0] = GCHandle.Alloc(header.Bits, GCHandleType.Pinned);
                this.blobs[0].cbSize = (uint)FileLogRecordHeader.Size;
                this.blobs[0].pBlobData = Marshal.UnsafeAddrOfPinnedArrayElement(header.Bits, 0);

                for (int i = 0; i < data.Count; i++)
                {
                    handles[i + 1] = GCHandle.Alloc(data[i].Array, GCHandleType.Pinned);
                    blobs[i + 1].cbSize = (uint)data[i].Count;
                    blobs[i + 1].pBlobData = Marshal.UnsafeAddrOfPinnedArrayElement(data[i].Array, data[i].Offset);
                }
            }
            catch
            {
                Dispose();
                throw;
            }
        }

        public UnmanagedBlob[] Blobs
        {
            get { return this.blobs; }
        }

        // Caller should always call Dispose.  Finalizer not implemented.
        public void Dispose()
        {
            try
            {
                lock (this)
                {
                    for (int i = 0; i < handles.Length; i++)
                    {
                        if (handles[i].IsAllocated)
                            handles[i].Free();
                    }
                }
            }
            catch (InvalidOperationException exception)
            {
                // This indicates something is broken in IO.Log's memory management,
                // so it's not safe to continue executing
                DiagnosticUtility.InvokeFinalHandler(exception);
            }
        }
    }
}
