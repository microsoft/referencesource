//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Threading;

    using Microsoft.Win32.SafeHandles;

    public sealed class LogExtentCollection : IEnumerable<LogExtent>
    {
        LogStore store;
        int version;

        internal LogExtentCollection(LogStore store)
        {
            this.store = store;
        }

        public int Count
        {
            get
            {
                CLFS_INFORMATION info;
                this.store.GetLogFileInformation(out info);

                return (int)info.TotalContainers;
            }
        }

        public int FreeCount
        {
            get
            {
                CLFS_INFORMATION info;
                this.store.GetLogFileInformation(out info);

                return (int)info.FreeContainers;
            }
        }

        int Version
        {
            get
            {
                return this.version;
            }
        }

        LogStore Store
        {
            get
            {
                return this.store;
            }
        }

        public void Add(string path)
        {
            if (string.IsNullOrEmpty(path))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("path"));
            }
            if (this.Count == 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.InvalidOperation(SR.LogStore_SizeRequired));
            }

            UnsafeNativeMethods.AddLogContainerNoSizeSync(
                this.store.Handle,
                path);

            this.version++;
        }

        public void Add(string path, long size)
        {
            if (string.IsNullOrEmpty(path))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("path"));
            }
            if (size <= 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("size"));
            }

            CLFS_INFORMATION info;
            this.store.GetLogFileInformation(out info);
            if ((ulong)size < info.ContainerSize)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.LogStore_SizeTooSmall));
            }

            ulong ulSize = (ulong)size;
            UnsafeNativeMethods.AddLogContainerSync(
                this.store.Handle,
                ref ulSize,
                path);

            this.version++;
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }

        public IEnumerator<LogExtent> GetEnumerator()
        {
            return new LogExtentEnumerator(this);
        }

        public void Remove(string path, bool force)
        {
            if (string.IsNullOrEmpty(path))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("path"));
            }

            UnsafeNativeMethods.RemoveLogContainerSync(
                this.store.Handle,
                path,
                force);

            this.version++;
        }

        public void Remove(LogExtent extent, bool force)
        {
            if (extent == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("extent"));
            }

            UnsafeNativeMethods.RemoveLogContainerSync(
                this.store.Handle,
                extent.Path,
                force);

            this.version++;
        }

        class LogExtentEnumerator : IEnumerator<LogExtent>
        {
            LogExtentCollection collection;
            List<LogExtent>.Enumerator innerEnum;
            int version;

            public LogExtentEnumerator(LogExtentCollection collection)
            {
                this.collection = collection;
                this.version = this.collection.Version;

                SafeFileHandle logHandle = this.collection.Store.Handle;

                CLFS_SCAN_CONTEXT scanContext = new CLFS_SCAN_CONTEXT();
                try
                {
                    List<LogExtent> extents = new List<LogExtent>();

                    CLFS_INFORMATION logInfo;
                    this.collection.Store.GetLogFileInformation(out logInfo);
                    if (logInfo.TotalContainers > 0)
                    {
                        UnsafeNativeMethods.CreateLogContainerScanContextSync(
                            logHandle,
                            0,
                            logInfo.TotalContainers,
                            CLFS_SCAN_MODE.CLFS_SCAN_FORWARD,
                            ref scanContext);

                        long containerPointer = scanContext.pinfoContainer.ToInt64();

                        CLFS_CONTAINER_INFORMATION_WRAPPER info;
                        info = new CLFS_CONTAINER_INFORMATION_WRAPPER();

                        int infoSize;
                        infoSize = Marshal.SizeOf(typeof(CLFS_CONTAINER_INFORMATION_WRAPPER));

                        for (uint i = 0; i < scanContext.cContainersReturned; i++)
                        {
                            Marshal.PtrToStructure(new IntPtr(containerPointer),
                                                   info);

                            LogExtent extent = new LogExtent(
                                info.info.GetActualFileName(logHandle),
                                info.info.FileSize,
                                (LogExtentState)info.info.State);
                            extents.Add(extent);

                            containerPointer += infoSize;
                        }
                    }

                    this.innerEnum = extents.GetEnumerator();
                }
                finally
                {
                    if ((scanContext.eScanMode & CLFS_SCAN_MODE.CLFS_SCAN_INITIALIZED) != 0)
                    {
                        UnsafeNativeMethods.ScanLogContainersSyncClose(
                            ref scanContext);
                    }
                }
            }

            object IEnumerator.Current
            {
                get
                {
                    return this.Current;
                }
            }

            public LogExtent Current
            {
                get { return this.innerEnum.Current; }
            }

            public bool MoveNext()
            {
                if (this.version != this.collection.Version)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(
                                    Error.InvalidOperation(SR.InvalidOperation_EnumFailedVersion));
                }

                return this.innerEnum.MoveNext();
            }

            public void Dispose()
            {
                this.innerEnum.Dispose();
            }

            public void Reset()
            {
                ((IEnumerator)this.innerEnum).Reset();
                this.version = this.collection.Version;
            }
        }
    }
}
