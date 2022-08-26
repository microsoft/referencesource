//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.Versioning;
    using System.Security;
    using System.Security.AccessControl;
    using System.Security.Permissions;
    using System.Threading;

    using Microsoft.Win32.SafeHandles;

    [Flags]
    enum SequenceNumberConstraint
    {
        None = 0x00,
        CanBeInvalid = 0x01,
        CanBeInactive = 0x02,

        MustBeActive = None,
        Arbitrary = CanBeInvalid | CanBeInactive
    }

    public sealed class LogStore : IDisposable
    {
        const int GENERIC_READ = unchecked((int)0x80000000);
        const int GENERIC_WRITE = 0x40000000;

        SafeFileHandle logFile;
        bool archivable;
        LogExtentCollection extents;
        LogManagementAsyncResult logManagement;
        LogPolicy logPolicy;
        FileAccess access;

        public LogStore(
            string path,
            FileMode mode)
            : this(path, mode, FileAccess.ReadWrite)
        {
        }

        public LogStore(
            string path,
            FileMode mode,
            FileAccess access)
            : this(path, mode, access, FileShare.None)
        {
        }

        public LogStore(
            string path,
            FileMode mode,
            FileAccess access,
            FileShare share)
            : this(path, mode, access, share, null)
        {
        }


        [PermissionSetAttribute(SecurityAction.Demand, Unrestricted = true)]
        [ResourceConsumption(ResourceScope.Machine)]
        [ResourceExposure(ResourceScope.Machine)]
        public LogStore(
            string path,
            FileMode mode,
            FileAccess access,
            FileShare share,
            FileSecurity fileSecurity)
        {
            Object pinningHandle = null;
            SECURITY_ATTRIBUTES secAttrs;

            try
            {

                secAttrs = GetSecAttrs(share, fileSecurity, out pinningHandle);

                this.access = access;
                // Parameter conversion and validation
                //
                // We don't need FileShare.Inheritable anymore because we took
                // care of it in the security attributes.
                //
                share &= ~FileShare.Inheritable;
                ValidateParameters(path, mode, access, share);

                // CLFS requires the path start with "log:". We'll
                // just make sure that it does.
                //
                bool addLogPrefix = true;
                if (path.Length > 4)
                {
                    if (0 == String.Compare(path,
                                            0,
                                            "log:",
                                            0,
                                            4,
                                            StringComparison.OrdinalIgnoreCase))
                    {
                        addLogPrefix = false;
                    }
                }

                if (addLogPrefix)
                {
                    path = "log:" + path;
                }

                int fAccess =
                    (access == FileAccess.Read ? GENERIC_READ :
                    access == FileAccess.Write ? GENERIC_WRITE :
                    GENERIC_READ | GENERIC_WRITE);

                int flagsAndAttributes;
                flagsAndAttributes = Const.FILE_FLAG_OVERLAPPED;

                try
                {
                    this.logFile = UnsafeNativeMethods.CreateLogFile(
                        path,
                        fAccess,
                        share,
                        secAttrs,
                        mode,
                        flagsAndAttributes);
                }
                catch (DllNotFoundException)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.PlatformNotSupported());
                }

                bool throwing = true;
                try
                {
                    bool writeOnly = (access & FileAccess.Read) == 0;
                    CommonInitialize(writeOnly);
                    throwing = false;
                }
                finally
                {
                    if (throwing)
                    {
                        this.logFile.Close();
                    }
                }
            }
            finally
            {
                if (pinningHandle != null)
                {
                    GCHandle pinHandle = (GCHandle)pinningHandle;
                    pinHandle.Free();
                }
            }
        }

        [PermissionSetAttribute(SecurityAction.Demand, Unrestricted = true)]
        public LogStore(SafeFileHandle handle)
        {
            if (handle == null)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("handle"));

            this.logFile = handle;

            CommonInitialize(false);
        }

        void CommonInitialize(bool writeOnly)
        {
            if (!ThreadPool.BindHandle(this.logFile))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(
                    new IOException(SR.GetString(SR.IO_BindFailed)));
            }

            this.logManagement = new LogManagementAsyncResult(this);

            if (!writeOnly)
            {
                CLFS_INFORMATION info;
                GetLogFileInformation(out info);
                this.archivable = ((info.Attributes &
                                    Const.FILE_ATTRIBUTE_ARCHIVE) != 0);

                this.extents = new LogExtentCollection(this);
                this.logPolicy = new LogPolicy(this);
            }
        }

        public bool Archivable
        {
            get
            {
                return this.archivable;
            }

            set
            {
                CLFS_LOG_ARCHIVE_MODE archiveMode;
                archiveMode =
                    (value ?
                     CLFS_LOG_ARCHIVE_MODE.ClfsLogArchiveEnabled :
                     CLFS_LOG_ARCHIVE_MODE.ClfsLogArchiveDisabled);
                UnsafeNativeMethods.SetLogArchiveMode(this.logFile,
                                                      archiveMode);

                this.archivable = value;
            }
        }

        public SequenceNumber BaseSequenceNumber
        {
            get
            {
                CLFS_INFORMATION info;
                GetLogFileInformation(out info);

                return new SequenceNumber(info.BaseLsn);
            }
        }

        [PermissionSetAttribute(SecurityAction.Demand, Unrestricted = true)]
        public static void Delete(
            string path)
        {
            // Parameter conversion and validation

            if (string.IsNullOrEmpty(path))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("path"));
            }

            // CLFS requires the path start with "log:". We'll
            // just make sure that it does.
            //
            bool addLogPrefix = true;
            if (path.Length > 4)
            {
                if (0 == String.Compare(path,
                                        0,
                                        "log:",
                                        0,
                                        4,
                                        StringComparison.OrdinalIgnoreCase))
                {
                    addLogPrefix = false;
                }
            }

            if (addLogPrefix)
            {
                path = "log:" + path;
            }

            try
            {
                UnsafeNativeMethods.DeleteLogFile(path, null);
            }
            catch (DllNotFoundException)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.PlatformNotSupported());
            }
        }

        public LogExtentCollection Extents
        {
            get { return this.extents; }
        }

        public long FreeBytes
        {
            get
            {
                CLFS_INFORMATION info;
                GetLogFileInformation(out info);

                // This is just a little thing to make the estimate
                // more accurate. If we say 10 bytes free, you ought
                // to be able to append a 10 byte record.
                // If we have less space free than what a header requires,
                // you can't append anything more so we return zero.
                //
                long freeBytes = checked((long)info.CurrentAvailable);
                if (freeBytes > LogLogRecordHeader.Size)
                    freeBytes -= LogLogRecordHeader.Size;
                else
                    freeBytes = 0;

                return freeBytes;
            }
        }

        public SafeFileHandle Handle
        {
            get { return this.logFile; }
        }

        public SequenceNumber LastSequenceNumber
        {
            get
            {
                CLFS_INFORMATION info;
                GetLogFileInformation(out info);

                return new SequenceNumber(info.LastLsn);
            }
        }

        public long Length
        {
            get
            {
                CLFS_INFORMATION info;
                GetLogFileInformation(out info);

                return checked((long)info.TotalAvailable);
            }
        }

        internal LogManagementAsyncResult LogManagement
        {
            get { return this.logManagement; }
        }

        public int StreamCount
        {
            get
            {
                CLFS_INFORMATION info;
                GetLogFileInformation(out info);

                return checked((int)info.TotalClients);
            }
        }

        public LogPolicy Policy
        {
            get
            {
                return this.logPolicy;
            }
        }

        internal object SyncRoot
        {
            get
            {
                return this.logManagement;
            }
        }

        internal FileAccess Access
        {
            get
            {
                return this.access;
            }
        }

        public LogArchiveSnapshot CreateLogArchiveSnapshot()
        {
            return CreateLogArchiveSnapshot(
                SequenceNumber.Invalid,
                SequenceNumber.Invalid);
        }

        public LogArchiveSnapshot CreateLogArchiveSnapshot(
            SequenceNumber first,
            SequenceNumber last)
        {

            ValidateSequenceNumber(
                ref first,
                SequenceNumberConstraint.CanBeInvalid,
                "first");
            ValidateSequenceNumber(
                ref last,
                SequenceNumberConstraint.CanBeInvalid,
                "last");
            if (!this.archivable)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotArchivable());
            }

            ulong lsnStart = first.High;
            ulong lsnEnd = last.High;

            if ((first == SequenceNumber.Invalid) ||
                (last == SequenceNumber.Invalid))
            {
                CLFS_INFORMATION logInfo;
                GetLogFileInformation(out logInfo);

                if (first == SequenceNumber.Invalid)
                {
                    lsnStart = Math.Min(logInfo.BaseLsn,
                                        logInfo.MinArchiveTailLsn);
                }

                if (last == SequenceNumber.Invalid)
                {
                    lsnEnd = logInfo.LastLsn;
                }
            }

            if (lsnStart > lsnEnd)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.Argument_SnapshotBoundsInvalid));
            }

            return new LogArchiveSnapshot(this, lsnStart, lsnEnd);
        }

        public void Dispose()
        {
            if (!this.logFile.IsInvalid)
            {
                this.logFile.Close();
            }
        }

        public void SetArchiveTail(SequenceNumber archiveTail)
        {
            ValidateSequenceNumber(
                ref archiveTail,
                SequenceNumberConstraint.MustBeActive,
                "archiveTail");
            if (!this.archivable)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotArchivable());
            }

            ulong sequence = archiveTail.High;
            UnsafeNativeMethods.SetLogArchiveTailSync(
                this.logFile,
                ref sequence);
        }

        internal void GetLogFileInformation(out CLFS_INFORMATION pinfoBuffer)
        {
            pinfoBuffer = new CLFS_INFORMATION();

            int size = Marshal.SizeOf(typeof(CLFS_INFORMATION));
            UnsafeNativeMethods.GetLogFileInformation(
                this.logFile,
                ref pinfoBuffer,
                ref size);
        }

        internal void ValidateSequenceNumber(
            ref SequenceNumber sequenceNumber,
            SequenceNumberConstraint constraint,
            string paramName)
        {
            if (sequenceNumber == SequenceNumber.Invalid)
            {
                if ((constraint & SequenceNumberConstraint.CanBeInvalid) != 0)
                {
                    return;
                }
                else
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.SequenceNumberNotActive(paramName));
                }
            }

            if (sequenceNumber.Low != 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.SequenceNumberInvalid());
            }

            if ((constraint & SequenceNumberConstraint.CanBeInactive) == 0)
            {
                CLFS_INFORMATION info;
                GetLogFileInformation(out info);

                ulong lsn = sequenceNumber.High;
                if (lsn < info.BaseLsn || lsn > info.LastLsn)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.SequenceNumberNotActive(paramName));
                }
            }
        }

        // If pinningHandle is not null, caller must free it AFTER the call to
        // CreateFile has returned.
        //
        // Microsoft: Copied this from System.IO.FileStream
        //
        private static unsafe SECURITY_ATTRIBUTES GetSecAttrs(
            FileShare share,
            FileSecurity fileSecurity,
            out object pinningHandle)
        {
            pinningHandle = null;
            SECURITY_ATTRIBUTES secAttrs = null;
            if ((share & FileShare.Inheritable) != 0 || fileSecurity != null)
            {
                secAttrs = new SECURITY_ATTRIBUTES();
                secAttrs.nLength = (int)Marshal.SizeOf(secAttrs);

                if ((share & FileShare.Inheritable) != 0)
                {
                    secAttrs.bInheritHandle = 1;
                }

                // For ACLs, get the security descriptor from the FileSecurity.
                if (fileSecurity != null)
                {
                    byte[] sd = fileSecurity.GetSecurityDescriptorBinaryForm();
                    pinningHandle = GCHandle.Alloc(sd, GCHandleType.Pinned);
                    fixed (byte* pSecDescriptor = sd)
                        secAttrs.pSecurityDescriptor = pSecDescriptor;
                }
            }
            return secAttrs;
        }

        private static void ValidateParameters(
            string path,
            FileMode mode,
            FileAccess access,
            FileShare share)
        {
            if (string.IsNullOrEmpty(path))
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("path"));

            if (!((mode == FileMode.CreateNew) ||
                  (mode == FileMode.Open) ||
                  (mode == FileMode.OpenOrCreate)))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("mode"));
            }

            if (access < FileAccess.Read || access > FileAccess.ReadWrite)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("access"));
            }

            if (share < FileShare.None || share > (FileShare.ReadWrite | FileShare.Delete))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("share"));
            }

            if ((mode == FileMode.CreateNew) && ((access & FileAccess.Write) == 0))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.Argument_CreateNewNoWrite));
            }
            if ((mode == FileMode.OpenOrCreate) && ((access & FileAccess.Write) == 0))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.Argument_OpenOrCreateNoWrite));
            }
        }
    }
}
