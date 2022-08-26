//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;
    using System.Text;

    public sealed class LogArchiveSnapshot
    {
        SequenceNumber archiveTail;
        SequenceNumber baseSequenceNumber;
        SequenceNumber lastSequenceNumber;

        IEnumerable<FileRegion> regions;

        const int MaxFileNameLength = 260;

        internal LogArchiveSnapshot(LogStore store,
                                    ulong lsnLow,
                                    ulong lsnHigh)
        {
            StringBuilder baseLogFileName = new StringBuilder(MaxFileNameLength);
            int actualLength;
            ulong baseLogFileOffset;
            ulong baseLogFileLength;
            ulong lsnBase;
            ulong lsnLast;
            ulong lsnArchiveTail;

            SafeArchiveContext archiveContext = null;
            try
            {
                while (!UnsafeNativeMethods.PrepareLogArchive(
                        store.Handle,
                        baseLogFileName,
                        baseLogFileName.Capacity,
                        ref lsnLow,
                        ref lsnHigh,
                        out actualLength,
                        out baseLogFileOffset,
                        out baseLogFileLength,
                        out lsnBase,
                        out lsnLast,
                        out lsnArchiveTail,
                        out archiveContext))
                {
                    baseLogFileName.EnsureCapacity(actualLength + 1);
                }

                this.archiveTail = new SequenceNumber(lsnArchiveTail);
                this.baseSequenceNumber = new SequenceNumber(lsnBase);
                this.lastSequenceNumber = new SequenceNumber(lsnLast);

                List<FileRegion> regions = new List<FileRegion>();

                byte[] readBuffer = new byte[checked((uint)baseLogFileLength)];
                uint actualDataLength = 0;
                unsafe
                {
                    fixed (byte* pbReadBuffer = readBuffer)
                    {
                        UnsafeNativeMethods.ReadLogArchiveMetadata(
                            archiveContext,
                            0,
                            readBuffer.Length,
                            pbReadBuffer,
                            out actualDataLength);
                    }
                }

                byte[] baseFileData;
                if (actualDataLength == (uint)baseLogFileLength)
                {
                    baseFileData = readBuffer;
                }
                else
                {
                    baseFileData = new byte[actualDataLength];
                    Array.Copy(readBuffer, baseFileData, baseFileData.Length);
                }
                regions.Add(new FileRegion((long)baseLogFileLength,
                                           baseLogFileName.ToString(),
                                           (long)baseLogFileOffset,
                                           baseFileData));


                CLFS_ARCHIVE_DESCRIPTOR descriptor = new CLFS_ARCHIVE_DESCRIPTOR();
                while (true)
                {
                    int returnedCount;
                    if (!UnsafeNativeMethods.GetNextLogArchiveExtentSingle(
                            archiveContext,
                            ref descriptor,
                            out returnedCount))
                    {
                        break;
                    }

                    if (returnedCount < 1) break;

                    long start = checked((long)descriptor.coffLow);
                    long length = checked((long)(descriptor.coffHigh - descriptor.coffLow));
                    string fileName = descriptor.infoContainer.GetActualFileName(store.Handle);

                    FileInfo containerInfo;
                    containerInfo = new FileInfo(fileName);

                    regions.Add(new FileRegion(containerInfo.Length,
                                               fileName,
                                               start,
                                               length));
                }

                this.regions = regions.AsReadOnly();
            }
            finally
            {
                if (archiveContext != null && !archiveContext.IsInvalid)
                {
                    archiveContext.Close();
                }
            }
        }

        public IEnumerable<FileRegion> ArchiveRegions
        {
            get
            {
                return this.regions;
            }
        }

        public SequenceNumber ArchiveTail
        {
            get
            {
                return this.archiveTail;
            }
        }

        public SequenceNumber BaseSequenceNumber
        {
            get
            {
                return this.baseSequenceNumber;
            }
        }

        public SequenceNumber LastSequenceNumber
        {
            get
            {
                return this.lastSequenceNumber;
            }
        }
    }
}
