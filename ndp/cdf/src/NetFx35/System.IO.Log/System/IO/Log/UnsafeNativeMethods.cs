//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.ServiceModel;
    using System.ServiceModel.Diagnostics;
    using System.IO;
    using System.Runtime;
    using System.Runtime.InteropServices;
    using System.Runtime.Versioning;
    using System.Security;
    using System.Text;
    using System.Threading;
    using Microsoft.Win32.SafeHandles;

    
    [StructLayout(LayoutKind.Sequential)]
    internal class SECURITY_ATTRIBUTES
    {
        internal int nLength = 0;
        internal unsafe byte* pSecurityDescriptor = null;
        internal int bInheritHandle = 0;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct CLFS_INFORMATION
    {
        public ulong TotalAvailable;       // Total log data space available.
        public ulong CurrentAvailable;     // Useable space in the log file.
        public ulong TotalUndoCommitment;  // Aggregate space reserved for UNDO's.
        public ulong BaseFileSize;         // Size of the base log file.
        public ulong ContainerSize;        // Size of a container.
        public uint TotalContainers;       // Total number of containers.
        public uint FreeContainers;        // Number of containers not in active log.
        public uint TotalClients;          // Total number of clients.
        public uint Attributes;            // Physical log file attributes.
        public uint FlushThreshold;        // Physical log file flush threshold.
        public uint SectorSize;            // Underlying container sector size.
        public ulong MinArchiveTailLsn;    // Marks the global archive tail.
        public ulong BaseLsn;              // Start of the active log region.
        public ulong LastFlushedLsn;       // Last flushed LSN in active log.
        public ulong LastLsn;              // End of active log region.
        public ulong RestartLsn;           // Location of restart
        // record.
        public Guid Identity;              // Unique log identifier.
    }

    internal enum CLFS_CONTEXT_MODE
    {
        ClfsContextNone = 0,
        ClfsContextUndoNext = 1,
        ClfsContextPrevious = 2,
        ClfsContextForward = 3
    }

    [Flags]
    internal enum CLFS_SCAN_MODE : byte
    {
        CLFS_SCAN_INIT = 0x01,
        CLFS_SCAN_FORWARD = 0x02,
        CLFS_SCAN_BACKWARD = 0x04,
        CLFS_SCAN_CLOSE = 0x08,
        CLFS_SCAN_INITIALIZED = 0x10,
        CLFS_SCAN_BUFFERED = 0x20
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct CLFS_WRITE_ENTRY
    {
        public IntPtr Buffer;
        public int ByteLength;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    internal struct CLFS_CONTAINER_INFORMATION
    {
        public uint FileAttributes;       // File system attribute flag.
        public ulong CreationTime;        // File creation time.
        public ulong LastAccessTime;      // Last time container was read/written.
        public ulong LastWriteTime;       // Last time container was written.
        public long FileSize;             // Size of container in bytes.
        public uint FileNameActualLength; // Length of the actual file name.
        public uint FileNameLength;       // Length of file name in buffer
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Const.CLFS_MAX_CONTAINER_INFO)]
        private string FileName;          // File system name for container.
        public uint State;                // Current state of the container.
        public uint PhysicalContainerId;  // Physical container identifier.
        public uint LogicalContainerId;   // Logical container
        // identifier.

        public string GetActualFileName(SafeFileHandle hLog)
        {
            if (this.FileNameActualLength == this.FileNameLength)
            {
                return this.FileName;
            }
            else
            {
                int actualLength = checked((int)(this.FileNameActualLength + 1));
                StringBuilder builder = new StringBuilder(actualLength);

                UnsafeNativeMethods.GetLogContainerName(
                    hLog,
                    (int)(this.LogicalContainerId),
                    builder,
                    builder.Capacity,
                    out actualLength);

                this.FileName = builder.ToString();
                this.FileNameLength = this.FileNameActualLength;

                return this.FileName;
            }
        }

        void SilenceCompiler()
        {
            this.FileAttributes = 0;
            this.CreationTime = 0;
            this.LastAccessTime = 0;
            this.LastWriteTime = 0;
            this.FileSize = 0;
            this.FileNameActualLength = 0;
            this.FileNameLength = 0;
            this.FileName = null;
            this.State = 0;
            this.PhysicalContainerId = 0;
            this.LogicalContainerId = 0;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal class CLFS_CONTAINER_INFORMATION_WRAPPER
    {
        public CLFS_CONTAINER_INFORMATION info = new CLFS_CONTAINER_INFORMATION();
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct CLFS_NODE_ID
    {
        public uint cType;
        public uint cbNode;
    }

    [StructLayout(LayoutKind.Explicit)]
    internal struct CLFS_SCAN_CONTEXT
    {
        [FieldOffset(0)]
        public CLFS_NODE_ID cidNode;
        [FieldOffset(8)]
        public IntPtr hLog;
        [FieldOffset(16)]
        public uint cIndex;
        [FieldOffset(24)]
        public uint cContainers;
        [FieldOffset(32)]
        public uint cContainersReturned;
        [FieldOffset(40)]
        public CLFS_SCAN_MODE eScanMode;
        [FieldOffset(48)]
        public IntPtr pinfoContainer;

        void SilenceCompiler()
        {
            this.cidNode.cbNode = 0;
            this.cidNode.cType = 0;
            this.hLog = IntPtr.Zero;
            this.cIndex = 0;
            this.cContainers = 0;
            this.cContainersReturned = 0;
            this.eScanMode = CLFS_SCAN_MODE.CLFS_SCAN_BACKWARD;
            this.pinfoContainer = IntPtr.Zero;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct CLFS_ARCHIVE_DESCRIPTOR
    {
        public ulong coffLow;
        public ulong coffHigh;

        public CLFS_CONTAINER_INFORMATION infoContainer;
    }

    internal enum CLFS_LOG_ARCHIVE_MODE
    {
        ClfsLogArchiveEnabled = 0x01,
        ClfsLogArchiveDisabled = 0x02
    }


    internal enum CLFS_MGMT_POLICY_TYPE
    {
        ClfsMgmtPolicyMaximumSize = 0x0,
        ClfsMgmtPolicyMinimumSize,
        ClfsMgmtPolicyNewContainerSize,
        ClfsMgmtPolicyGrowthRate,
        ClfsMgmtPolicyLogTail,
        ClfsMgmtPolicyAutoShrink,
        ClfsMgmtPolicyAutoGrow,
        ClfsMgmtPolicyNewContainerPrefix,
        ClfsMgmtPolicyNewContainerSuffix,

        ClfsMgmtPolicyInvalid
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct CLFS_MGMT_POLICY_COMMON
    {
        const uint CLFS_MGMT_POLICY_VERSION = 0x01;

        public uint Version;
        public uint LengthInBytes;
        public uint PolicyFlags;
        public CLFS_MGMT_POLICY_TYPE PolicyType;

        public CLFS_MGMT_POLICY_COMMON(
            uint size,
            uint flags,
            CLFS_MGMT_POLICY_TYPE type)
        {
            this.Version = CLFS_MGMT_POLICY_VERSION;
            this.LengthInBytes = size;
            this.PolicyFlags = flags;
            this.PolicyType = type;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct CLFS_MGMT_POLICY_MAXIMUMSIZE
    {
        public CLFS_MGMT_POLICY_COMMON Common;
        public uint Containers;
        uint Padding;

        public CLFS_MGMT_POLICY_MAXIMUMSIZE(uint flags)
        {
            this.Common = new CLFS_MGMT_POLICY_COMMON(
                (uint)Marshal.SizeOf(typeof(CLFS_MGMT_POLICY_MAXIMUMSIZE)),
                flags,
                CLFS_MGMT_POLICY_TYPE.ClfsMgmtPolicyMaximumSize);
            this.Containers = 0;
            this.Padding = 0;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct CLFS_MGMT_POLICY_MINIMUMSIZE
    {
        public CLFS_MGMT_POLICY_COMMON Common;
        public uint Containers;
        uint Padding;

        public CLFS_MGMT_POLICY_MINIMUMSIZE(uint flags)
        {
            this.Common = new CLFS_MGMT_POLICY_COMMON(
                (uint)Marshal.SizeOf(typeof(CLFS_MGMT_POLICY_MINIMUMSIZE)),
                flags,
                CLFS_MGMT_POLICY_TYPE.ClfsMgmtPolicyMinimumSize);
            this.Containers = 2; // NOTE: Default from spec
            this.Padding = 0;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct CLFS_MGMT_POLICY_GROWTHRATE
    {
        public CLFS_MGMT_POLICY_COMMON Common;
        public uint AbsoluteGrowthInContainers;
        public uint RelativeGrowthPercentage;

        public CLFS_MGMT_POLICY_GROWTHRATE(uint flags)
        {
            this.Common = new CLFS_MGMT_POLICY_COMMON(
                (uint)Marshal.SizeOf(typeof(CLFS_MGMT_POLICY_GROWTHRATE)),
                flags,
                CLFS_MGMT_POLICY_TYPE.ClfsMgmtPolicyGrowthRate);
            this.AbsoluteGrowthInContainers = 1; // NOTE: Default from spec
            this.RelativeGrowthPercentage = 0;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct CLFS_MGMT_POLICY_LOGTAIL
    {
        public CLFS_MGMT_POLICY_COMMON Common;
        public uint MinimumAvailablePercentage;
        public uint MinimumAvailableContainers;

        public CLFS_MGMT_POLICY_LOGTAIL(uint flags)
        {
            this.Common = new CLFS_MGMT_POLICY_COMMON(
                (uint)Marshal.SizeOf(typeof(CLFS_MGMT_POLICY_LOGTAIL)),
                flags,
                CLFS_MGMT_POLICY_TYPE.ClfsMgmtPolicyLogTail);
            this.MinimumAvailablePercentage = 35; // NOTE: Default from spec
            this.MinimumAvailableContainers = 0;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct CLFS_MGMT_POLICY_AUTOSHRINK
    {
        public CLFS_MGMT_POLICY_COMMON Common;
        public uint Percentage;
        uint Padding;

        public CLFS_MGMT_POLICY_AUTOSHRINK(uint flags)
        {
            this.Common = new CLFS_MGMT_POLICY_COMMON(
                (uint)Marshal.SizeOf(typeof(CLFS_MGMT_POLICY_AUTOSHRINK)),
                flags,
                CLFS_MGMT_POLICY_TYPE.ClfsMgmtPolicyAutoShrink);
            this.Percentage = 0;
            this.Padding = 0;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct CLFS_MGMT_POLICY_AUTOGROW
    {
        public CLFS_MGMT_POLICY_COMMON Common;
        public uint Enabled;
        uint Padding;

        public CLFS_MGMT_POLICY_AUTOGROW(uint flags)
        {
            this.Common = new CLFS_MGMT_POLICY_COMMON(
                (uint)Marshal.SizeOf(typeof(CLFS_MGMT_POLICY_AUTOGROW)),
                flags,
                CLFS_MGMT_POLICY_TYPE.ClfsMgmtPolicyAutoGrow);
            this.Enabled = 0; // NOTE: Default from spec
            this.Padding = 0;
        }
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    internal struct CLFS_MGMT_POLICY_NEWCONTAINERPREFIX
    {
        public CLFS_MGMT_POLICY_COMMON Common;
        public ushort PrefixLengthInBytes;
        // We add six characters to the buffer size to work around Windows OS 1635324.
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Const.MAX_PATH + 6)]
        public string PrefixString;

        public CLFS_MGMT_POLICY_NEWCONTAINERPREFIX(uint flags)
        {
            this.Common = new CLFS_MGMT_POLICY_COMMON(
                (uint)Marshal.SizeOf(typeof(CLFS_MGMT_POLICY_NEWCONTAINERPREFIX)),
                flags,
                CLFS_MGMT_POLICY_TYPE.ClfsMgmtPolicyNewContainerPrefix);
            this.PrefixString = "Container";  // NOTE: Default from spec
            this.PrefixLengthInBytes = (ushort)(this.PrefixString.Length * 2);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct CLFS_MGMT_POLICY_NEXTCONTAINERSUFFIX
    {
        public CLFS_MGMT_POLICY_COMMON Common;
        public ulong NextContainerSuffix;

        public CLFS_MGMT_POLICY_NEXTCONTAINERSUFFIX(uint flags)
        {
            this.Common = new CLFS_MGMT_POLICY_COMMON(
                (uint)Marshal.SizeOf(typeof(CLFS_MGMT_POLICY_NEXTCONTAINERSUFFIX)),
                flags,
                CLFS_MGMT_POLICY_TYPE.ClfsMgmtPolicyNewContainerSuffix);
            this.NextContainerSuffix = 0;
        }
    }

    internal enum CLFS_MGMT_NOTIFICATION_TYPE
    {
        ClfsMgmtAdvanceTailNotification = 0,
        ClfsMgmtLogFullHandlerNotification,
        ClfsMgmtLogUnpinnedNotification
    }

    [StructLayout(LayoutKind.Sequential)]
    internal class CLFS_MGMT_NOTIFICATION
    {
        // NOTE: The translation of this structure isn't as good as it
        //       should be, because it must be blittable.
        //
        // Nature of the notification.
        //
        public int Notification;

        //
        // Target LSN for base LSN advancement if the
        // notification type is ClfsMgmtAdvanceTailNotification.
        //
        public ulong Lsn;

        //
        // TRUE if the log is pinned, FALSE otherwise.
        // Especially meaningful when receiving an error
        // status for ClfsMgmtLogFullHandlerNotification.
        //
        public int LogIsPinned;

        public CLFS_MGMT_NOTIFICATION()
        {
            this.Notification = (int)CLFS_MGMT_NOTIFICATION_TYPE.ClfsMgmtAdvanceTailNotification;
            this.Lsn = 0;
            this.LogIsPinned = 0;
        }
    }

    /*
    [StructLayout(LayoutKind.Sequential)]
    internal class LOG_MANAGEMENT_CALLBACKS
    {
        public IntPtr CallbackContext = IntPtr.Zero;

        public IntPtr AdvanceTailCallback = IntPtr.Zero;
        public IntPtr LogFullHandlerCallback = IntPtr.Zero;
        public IntPtr LogUnpinnedCallback = IntPtr.Zero;
    }
    */

    internal static class Const
    {
        internal const int MAX_PATH = 260;
        internal const int CLFS_MAX_CONTAINER_INFO = 256;

        internal const byte ClfsNullRecord = 0x00; // Null record type.        
        internal const byte ClfsDataRecord = 0x01; // Client data record.
        internal const byte ClfsRestartRecord = 0x02; // Restart record.
        internal const byte ClfsStartRecord = 0x04; // Start of continuation record.
        internal const byte ClfsEndRecord = 0x08; // End of continuation record.
        internal const byte ClfsContinuationRecord = 0x10; // Continuation record.
        internal const byte ClfsLastRecord = 0x20; // Last record in log block.
        internal const byte ClfsAnyRecord = 0x3F; // Any record type.
        internal const byte ClfsRecordMask = 0x3F; // Mask testing valid record types.

        // Valid client records are restart and data records.
        internal const byte ClsClientRecord = 0x03;
        internal const byte ClfsClientRecord = 0x03;

        internal const int CLFS_FLAG_FORCE_APPEND = 0x00000001; // Flag to force an append to log queue
        internal const int CLFS_FLAG_FORCE_FLUSH = 0x00000002; // Flag to force a log flush
        internal const int CLFS_FLAG_USE_RESERVATION = 0x00000004; // Flag to charge a data append to reservation

        // CLFS currently doesn't support policy to be persisted. 
        // LOG_POLICY_PERSIST value is changed to 0 as a work around. 

        internal const uint LOG_POLICY_OVERWRITE = 0x01;
        internal const uint LOG_POLICY_PERSIST = 0x00; //0x02;

        internal const int FILE_ATTRIBUTE_ARCHIVE = 0x00000020;
        internal const int FILE_FLAG_OVERLAPPED = 0x40000000;

        // Map this value to CLFS_LSN_INVALID value
        internal const ulong CLFS_LSN_INVALID = 0xFFFFFFFF00000000;
    }

    [SuppressUnmanagedCodeSecurity]
    unsafe internal static class UnsafeNativeMethods
    {
        internal const string CLFSW32 = "clfsw32.dll";
        internal const string KERNEL32 = "kernel32.dll";

        [ResourceExposure(ResourceScope.Machine)]
        [ResourceConsumption(ResourceScope.Machine)]
        public static SafeFileHandle CreateLogFile(
            string pszLogFileName,
            int fDesiredAccess,
            System.IO.FileShare dwShareMode,
            SECURITY_ATTRIBUTES psaLogFile,
            System.IO.FileMode fCreateDisposition,
            int fFlagsAndAttributes)
        {
            SafeFileHandle ret = _CreateLogFile(
                pszLogFileName,
                fDesiredAccess,
                dwShareMode,
                psaLogFile,
                fCreateDisposition,
                fFlagsAndAttributes);

            if (ret.IsInvalid)
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                switch (errorCode)
                {
                    case Error.ERROR_SHARING_VIOLATION:
                    case Error.ERROR_ALREADY_EXISTS:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_FILE_NOT_FOUND:
                    case Error.ERROR_PATH_NOT_FOUND:
                    case Error.ERROR_NO_SYSTEM_RESOURCES:
                    case Error.ERROR_OUTOFMEMORY:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_INVALID_NAME:
                    case Error.ERROR_BAD_PATHNAME:
                    case Error.ERROR_BAD_NETPATH:
                    case Error.ERROR_CANT_RESOLVE_FILENAME:
                    case Error.ERROR_LOG_BLOCK_VERSION:
                    case Error.ERROR_LOG_BLOCK_INVALID:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));
                    case Error.ERROR_NOT_SUPPORTED:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperArgument(SR.GetString(SR.Argument_FileNameInvalid));
                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }

            return ret;
        }

        public static void DeleteLogFile(
            string pszLogFileName,
            SECURITY_ATTRIBUTES psaLogFile)
        {
            if (!_DeleteLogFile(pszLogFileName, psaLogFile))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                switch (errorCode)
                {
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_FILE_NOT_FOUND:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_SHARING_VIOLATION:
                    case Error.ERROR_INVALID_NAME:
                    case Error.ERROR_BAD_PATHNAME:
                    case Error.ERROR_BAD_NETPATH:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));
                    case Error.ERROR_NOT_SUPPORTED:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperArgument(SR.GetString(SR.Argument_FileNameInvalid));
                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }

        public static void GetLogFileInformation(
            SafeFileHandle hLog,
            ref CLFS_INFORMATION pinfoBuffer,
            ref int cbBuffer)
        {
            if (!_GetLogFileInformation(hLog, ref pinfoBuffer, ref cbBuffer))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                switch (errorCode)
                {
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }

        public static void FlushLogToLsnSync(
            SafeMarshalContext pvMarshalContext,
            ref ulong plsnFlush,
            out ulong plsnLastFlushed)
        {
            if (!_FlushLogToLsnSync(pvMarshalContext,
                                    ref plsnFlush,
                                    out plsnLastFlushed,
                                    null))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                    // This means that CLFS has a bug, so it is not safe to continue processing.
                    DiagnosticUtility.FailFast("Unexpected IO in progress (FlushToLsnSync)");
                }
                switch (errorCode)
                {
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }

        public static uint FlushLogToLsnAsync(
            SafeMarshalContext pvMarshalContext,
            ref ulong plsnFlush,
            IntPtr plsnLastFlushed,
            NativeOverlapped* overlapped)
        {
            uint errorCode = Error.ERROR_SUCCESS;
            if (!_FlushLogToLsnAsync(pvMarshalContext,
                                     ref plsnFlush,
                                     plsnLastFlushed,
                                     overlapped))
            {
                errorCode = (uint)Marshal.GetLastWin32Error();
            }

            return errorCode;
        }

        public static Exception FlushLogToLsnFilter(uint errorCode)
        {
            if (errorCode == Error.ERROR_IO_PENDING)
            {
                // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                // This means that CLFS has a bug, so it is not safe to continue processing.
                DiagnosticUtility.FailFast("Async must be handled elsewhere");
            }
            switch (errorCode)
            {
                case Error.ERROR_INVALID_HANDLE:
                case Error.ERROR_ACCESS_DENIED:
                case Error.ERROR_INVALID_PARAMETER:
                    return DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                default:
                    return DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
            }
        }

        public static SafeMarshalContext CreateLogMarshallingArea(
            SafeFileHandle hLog,
            IntPtr pfnAllocBuffer,
            IntPtr pfnFreeBuffer,
            IntPtr pvBlockAllocContext,
            int cbMarshallingBlock,
            int cMaxWriteBlocks,
            int cMaxReadBlocks)
        {
            SafeMarshalContext ret;
            if (!_CreateLogMarshallingArea(hLog,
                                           pfnAllocBuffer,
                                           pfnFreeBuffer,
                                           pvBlockAllocContext,
                                           cbMarshallingBlock,
                                           cMaxWriteBlocks,
                                           cMaxReadBlocks,
                                           out ret))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                Utility.CloseInvalidOutSafeHandle(ret);
                switch (errorCode)
                {
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_OUTOFMEMORY:
                    case Error.ERROR_NO_SYSTEM_RESOURCES:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_INVALID_STATE:
                    case Error.ERROR_LOG_NOT_ENOUGH_CONTAINERS:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }

            return ret;
        }


        public static void AlignReservedLogSingle(SafeMarshalContext pvMarshal,
                                                  long reservation,
                                                  out long pcbAlignReservation)
        {
            if (!_AlignReservedLogSingle(pvMarshal,
                                         1,
                                         ref reservation,
                                         out pcbAlignReservation))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                switch (errorCode)
                {
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_IO_DEVICE:
                    case Error.ERROR_LOG_RESERVATION_INVALID:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }


        public static void AllocReservedLog(SafeMarshalContext pvMarshal,
                                            int cReservedRecords,
                                            ref long pcbAdjustment)
        {
            if (!_AllocReservedLog(pvMarshal,
                                   cReservedRecords,
                                   ref pcbAdjustment))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                switch (errorCode)
                {
                    case Error.ERROR_NO_SYSTEM_RESOURCES:
                    case Error.ERROR_LOG_FULL:
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_IO_DEVICE:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }


        public static void FreeReservedLog(SafeMarshalContext pvMarshal,
                                           uint cReservedRecords,
                                           ref long pcbAdjustment)
        {
            if (!_FreeReservedLog(pvMarshal,
                                  cReservedRecords,
                                  ref pcbAdjustment))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                switch (errorCode)
                {
                    case Error.ERROR_NO_SYSTEM_RESOURCES:
                    case Error.ERROR_LOG_FULL:
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_IO_DEVICE:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }


        public static bool ReadLogRestartAreaSync(
            SafeMarshalContext pvMarshal,
            out byte* ppvRestartBuffer,
            out int pcbRestartBuffer,
            out ulong plsn,
            out SafeReadContext ppvContext)
        {
            if (!_ReadLogRestartArea(pvMarshal,
                                     out ppvRestartBuffer,
                                     out pcbRestartBuffer,
                                     out plsn,
                                     out ppvContext,
                                     null))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                Utility.CloseInvalidOutSafeHandle(ppvContext);
                ppvContext = null;
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                    // This means that CLFS has a bug, so it is not safe to continue processing.
                    DiagnosticUtility.FailFast("Unexpected IO in progress (ReadLogRA)");
                }
                switch (errorCode)
                {
                    case Error.ERROR_LOG_NO_RESTART:
                        return false;

                    case Error.ERROR_LOG_BLOCK_INCOMPLETE:
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_IO_DEVICE:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }

            return true;
        }

        public static bool ReadPreviousLogRestartAreaSync(
            SafeReadContext pvReadContext,
            out byte* ppvRestartBuffer,
            out int pcbRestartBuffer,
            out ulong plsnRestart)
        {
            if (!_ReadPreviousLogRestartArea(pvReadContext,
                                             out ppvRestartBuffer,
                                             out pcbRestartBuffer,
                                             out plsnRestart,
                                             null))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                    // This means that CLFS has a bug, so it is not safe to continue processing.
                    DiagnosticUtility.FailFast("Unexpected IO in progress (ReadPrevRA)");
                }
                switch (errorCode)
                {
                    case Error.ERROR_LOG_START_OF_LOG:
                        return false;

                    case Error.ERROR_LOG_BLOCK_INCOMPLETE:
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_IO_DEVICE:
                    case Error.ERROR_LOG_READ_CONTEXT_INVALID:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }

            return true;
        }

        public static bool ReadLogRecordSync(SafeMarshalContext pvMarshal,
                                             [In] ref ulong plsnFirst,
                                             CLFS_CONTEXT_MODE ecxMode,
                                             out byte* ppvReadBuffer,
                                             out int pcbReadBuffer,
                                             out byte peRecordType,
                                             out ulong plsnUndoNext,
                                             out ulong plsnPrevious,
                                             out SafeReadContext ppvReadContext)
        {
            if (!_ReadLogRecord(pvMarshal,
                                ref plsnFirst,
                                ecxMode,
                                out ppvReadBuffer,
                                out pcbReadBuffer,
                                out peRecordType,
                                out plsnUndoNext,
                                out plsnPrevious,
                                out ppvReadContext,
                                null))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                Utility.CloseInvalidOutSafeHandle(ppvReadContext);
                ppvReadContext = null;
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                    // This means that CLFS has a bug, so it is not safe to continue processing.
                    DiagnosticUtility.FailFast("No async in ReadLogRecordSync");
                }
                switch (errorCode)
                {
                    case Error.ERROR_HANDLE_EOF:
                        return false;

                    case Error.ERROR_NOT_FOUND:
                    case Error.ERROR_LOG_START_OF_LOG:
                    case Error.ERROR_LOG_BLOCK_INVALID:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.Argument_InvalidStartSequenceNumber));

                    case Error.ERROR_LOG_BLOCK_INCOMPLETE:
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_IO_DEVICE:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }

            return true;
        }

        public static bool ReadNextLogRecordSync(SafeReadContext pvReadContext,
                                                 out byte* ppvReadBuffer,
                                                 out int pcbReadBuffer,
                                                 ref byte peRecordType,
                                                 out ulong plsnUndoNext,
                                                 out ulong plsnPrevious,
                                                 out ulong plsnRecord)
        {
            if (!_ReadNextLogRecord(pvReadContext,
                                    out ppvReadBuffer,
                                    out pcbReadBuffer,
                                    ref peRecordType,
                                    IntPtr.Zero,
                                    out plsnUndoNext,
                                    out plsnPrevious,
                                    out plsnRecord,
                                    null))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                    // This means that CLFS has a bug, so it is not safe to continue processing.
                    DiagnosticUtility.FailFast("No async in ReadNextLogRecordSync");
                }
                switch (errorCode)
                {
                    case Error.ERROR_HANDLE_EOF:
                    case Error.ERROR_LOG_START_OF_LOG:
                        return false;

                    case Error.ERROR_LOG_BLOCK_INCOMPLETE:
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_IO_DEVICE:
                    case Error.ERROR_LOG_READ_CONTEXT_INVALID:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }

            return true;
        }

        public static uint ReserveAndAppendLog(
            SafeMarshalContext pvMarshal,
            CLFS_WRITE_ENTRY[] rgWriteEntries,
            int cWriteEntries,
            [In] ref ulong plsnUndoNext,
            [In] ref ulong plsnPrevious,
            int cReserveRecords,
            long[] rgcbReservation,
            int fFlags,
            IntPtr plsn,
            NativeOverlapped* pOverlapped)
        {
            uint errorCode = Error.ERROR_SUCCESS;
            if (!_ReserveAndAppendLog(pvMarshal,
                                      rgWriteEntries,
                                      cWriteEntries,
                                      ref plsnUndoNext,
                                      ref plsnPrevious,
                                      cReserveRecords,
                                      rgcbReservation,
                                      fFlags,
                                      plsn,
                                      pOverlapped))
            {
                errorCode = (uint)Marshal.GetLastWin32Error();
            }

            return errorCode;
        }

        public static Exception ReserveAndAppendLogFilter(uint errorCode)
        {
            if (errorCode == Error.ERROR_IO_PENDING)
            {
                // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                // This means that CLFS has a bug, so it is not safe to continue processing.
                DiagnosticUtility.FailFast("Async must be handled elsewhere");
            }
            switch (errorCode)
            {
                case Error.ERROR_NO_SYSTEM_RESOURCES:
                case Error.ERROR_LOG_FULL:
                case Error.ERROR_INVALID_HANDLE:
                case Error.ERROR_ACCESS_DENIED:
                case Error.ERROR_INVALID_PARAMETER:
                case Error.ERROR_IO_DEVICE:
                    return DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                default:
                    return DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
            }
        }

        public static uint WriteLogRestartArea(SafeMarshalContext pvMarshal,
                                               byte* pvRestartBuffer,
                                               int cbRestartBuffer,
                                               [In] ref ulong plsnBase,
                                               int fFlags,
                                               IntPtr pcbWritten, // int*
                                               IntPtr plsnRestart, // ulong*
                                               NativeOverlapped* pOverlapped)
        {
            uint errorCode = Error.ERROR_SUCCESS;
            if (!_WriteLogRestartArea(pvMarshal,
                                      pvRestartBuffer,
                                      cbRestartBuffer,
                                      ref plsnBase,
                                      fFlags,
                                      pcbWritten,
                                      plsnRestart,
                                      pOverlapped))
            {
                errorCode = (uint)Marshal.GetLastWin32Error();
            }

            return errorCode;
        }

        public static uint WriteLogRestartAreaNoBase(
            SafeMarshalContext pvMarshal,
            byte* pvRestartBuffer,
            int cbRestartBuffer,
            int fFlags,
            IntPtr pcbWritten, // int*
            IntPtr plsnRestart, // ulong*
            NativeOverlapped* pOverlapped)
        {
            uint errorCode = Error.ERROR_SUCCESS;
            if (!_WriteLogRestartAreaNoBase(pvMarshal,
                                            pvRestartBuffer,
                                            cbRestartBuffer,
                                            IntPtr.Zero,
                                            fFlags,
                                            pcbWritten,
                                            plsnRestart,
                                            pOverlapped))
            {
                errorCode = (uint)Marshal.GetLastWin32Error();
            }

            return errorCode;
        }

        public static Exception WriteLogRestartAreaFilter(uint errorCode)
        {
            if (errorCode == Error.ERROR_IO_PENDING)
            {
                // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                // This means that CLFS has a bug, so it is not safe to continue processing.
                DiagnosticUtility.FailFast("Async must be handled elsewhere");
            }
            switch (errorCode)
            {
                case Error.ERROR_NO_SYSTEM_RESOURCES:
                case Error.ERROR_LOG_FULL:
                case Error.ERROR_INVALID_HANDLE:
                case Error.ERROR_ACCESS_DENIED:
                case Error.ERROR_IO_DEVICE:
                case Error.ERROR_INVALID_PARAMETER:
                case Error.ERROR_LOG_TAIL_INVALID:
                    return DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                default:
                    return DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
            }
        }

        public static void AddLogContainerSync(SafeFileHandle hLog,
                                               ref ulong pcbContainer,
                                               string pwszContainerPath)
        {
            if (!_AddLogContainer(hLog,
                                  ref pcbContainer,
                                  pwszContainerPath,
                                  IntPtr.Zero))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                    // This means that CLFS has a bug, so it is not safe to continue processing.
                    DiagnosticUtility.FailFast("No async in AddLogContainerSync");
                }
                switch (errorCode)
                {
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_CANT_RESOLVE_FILENAME:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_INVALID_NAME:
                    case Error.ERROR_ALREADY_EXISTS:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    case Error.ERROR_DISK_FULL:
                    case Error.ERROR_MEDIUM_FULL:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(Error.LOGSTORE_ERROR_DISK_FULL));

                    case Error.ERROR_DUP_NAME:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.InvalidOperation(SR.LogStore_DuplicateExtent));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }

        public static void AddLogContainerNoSizeSync(
            SafeFileHandle hLog,
            string pwszContainerPath)
        {
            if (!_AddLogContainerNoSize(hLog,
                                        IntPtr.Zero,
                                        pwszContainerPath,
                                        IntPtr.Zero))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                    // This means that CLFS has a bug, so it is not safe to continue processing.
                    DiagnosticUtility.FailFast("No async in AddLogContainerNSSync");
                }
                switch (errorCode)
                {
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_CANT_RESOLVE_FILENAME:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_INVALID_NAME:
                    case Error.ERROR_ALREADY_EXISTS:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    case Error.ERROR_DISK_FULL:
                    case Error.ERROR_MEDIUM_FULL:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(Error.LOGSTORE_ERROR_DISK_FULL));

                    case Error.ERROR_DUP_NAME:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.InvalidOperation(SR.LogStore_DuplicateExtent));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }


        public static void RemoveLogContainerSync(SafeFileHandle hLog,
                                                  string pszContainerPath,
                                                  bool fForce)
        {
            if (!_RemoveLogContainer(hLog,
                                     pszContainerPath,
                                     fForce,
                                     null))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                    // This means that CLFS has a bug, so it is not safe to continue processing.
                    DiagnosticUtility.FailFast("No async in RemoveLogContainerSync");
                }
                switch (errorCode)
                {
                    case Error.ERROR_ACCESS_DENIED:
                        // Special case here-- ACCESS_DENIED could mean
                        // that the thing is still in use. Gotta love
                        // those Win32 semantics. So we use a special
                        // exception here.
                        //
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(
                                        Error.InvalidOperation(SR.LogStore_CannotRemoveExtent));

                    case Error.ERROR_NOT_FOUND:
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_CANT_RESOLVE_FILENAME:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_LOG_CANT_DELETE:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }


        public static void CreateLogContainerScanContextSync(
            SafeFileHandle hLog,
            uint cFromContainer,
            uint cContainers,
            CLFS_SCAN_MODE eScanMode,
            ref CLFS_SCAN_CONTEXT pcxScan)
        {
            if (!_CreateLogContainerScanContext(hLog,
                                                cFromContainer,
                                                cContainers,
                                                eScanMode,
                                                ref pcxScan,
                                                null))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                    // This means that CLFS has a bug, so it is not safe to continue processing.
                    DiagnosticUtility.FailFast("No async in CreateLCScanSync");
                }
                switch (errorCode)
                {
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_OUTOFMEMORY:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }

        public static bool ScanLogContainersSyncClose(
            ref CLFS_SCAN_CONTEXT pcxScan)
        {
#pragma warning suppress 56523
            bool ret = _ScanLogContainers(ref pcxScan,
                                          CLFS_SCAN_MODE.CLFS_SCAN_CLOSE,
                                          IntPtr.Zero);
            if (!ret)
            {
                // The CLFS API was called in close handle mode. If it fails, either CLFS has a bug or IO.Log does
                // Either way, it is not safe to continue processing.
                DiagnosticUtility.FailFast("Closing scan context cannot fail");
            }

            return true;
        }

        public static void SetLogArchiveTailSync(SafeFileHandle hLog,
                                                 ref ulong plsnArchiveTail)
        {
            if (!_SetLogArchiveTail(hLog, ref plsnArchiveTail, null))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                    // This means that CLFS has a bug, so it is not safe to continue processing.
                    DiagnosticUtility.FailFast("No async in SetLogArchiveTailSync");
                }
                switch (errorCode)
                {
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }

        public static void SetEndOfLogSync(SafeFileHandle hLog,
                                           ref ulong plsnEnd)
        {
            if (!_SetEndOfLog(hLog, ref plsnEnd, null))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                    // This means that CLFS has a bug, so it is not safe to continue processing.
                    DiagnosticUtility.FailFast("No async in SetEndOfLogSync");
                }
                switch (errorCode)
                {
                    case Error.ERROR_LOG_START_OF_LOG:
                    case Error.ERROR_LOG_BLOCK_INVALID:
                    case Error.ERROR_LOG_SECTOR_PARITY_INVALID:
                    case Error.ERROR_HANDLE_EOF:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.Argument_InvalidSequenceNumber));

                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_LOG_INVALID_RANGE:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }

        public static void TruncateLogSync(SafeMarshalContext pvMarshal,
                                           ref ulong plsnEnd)
        {
            if (!_TruncateLog(pvMarshal, ref plsnEnd, null))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                    // This means that CLFS has a bug, so it is not safe to continue processing.
                    DiagnosticUtility.FailFast("No async in TruncateLogSync");
                }
                switch (errorCode)
                {
                    case Error.ERROR_LOG_START_OF_LOG:
                    case Error.ERROR_LOG_BLOCK_INVALID:
                    case Error.ERROR_LOG_SECTOR_PARITY_INVALID:
                    case Error.ERROR_HANDLE_EOF:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.Argument_InvalidSequenceNumber));

                    case Error.ERROR_LOG_METADATA_CORRUPT:
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_LOG_INVALID_RANGE:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }

        public static bool PrepareLogArchive(
            SafeFileHandle hLog,
            StringBuilder pszBaseLogFileName,
            int cLen,
            ref ulong pLsnLow,
            ref ulong pLsnHigh,
            out int pcActualLength,
            out ulong poffBaseLogFileData,
            out ulong pcbBaseLogFileLength,
            out ulong plsnBase,
            out ulong plsnLast,
            out ulong plsnCurrentArchiveTail,
            out SafeArchiveContext ppvArchiveContext)
        {
            if (!_PrepareLogArchive(hLog,
                                    pszBaseLogFileName,
                                    cLen,
                                    ref pLsnLow,
                                    ref pLsnHigh,
                                    out pcActualLength,
                                    out poffBaseLogFileData,
                                    out pcbBaseLogFileLength,
                                    out plsnBase,
                                    out plsnLast,
                                    out plsnCurrentArchiveTail,
                                    out ppvArchiveContext))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                Utility.CloseInvalidOutSafeHandle(ppvArchiveContext);
                ppvArchiveContext = null;
                switch (errorCode)
                {
                    case Error.ERROR_BUFFER_OVERFLOW:
                        return false;

                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_OUTOFMEMORY:
                    case Error.ERROR_NOT_SUPPORTED:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_INVALID_OPERATION:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }

            return true;
        }

        public static void ReadLogArchiveMetadata(
            SafeArchiveContext pvArchiveContext,
            int cbOffset,
            int cbBytesToRead,
            byte* pbReadBuffer,
            out uint pcbBytesRead)
        {
            if (!_ReadLogArchiveMetadata(pvArchiveContext,
                                         cbOffset,
                                         cbBytesToRead,
                                         pbReadBuffer,
                                         out pcbBytesRead))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                switch (errorCode)
                {
                    case Error.ERROR_LOG_READ_CONTEXT_INVALID:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_INVALID_STATE:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }

        public static bool GetNextLogArchiveExtentSingle(
            SafeArchiveContext pvArchiveContext,
            ref CLFS_ARCHIVE_DESCRIPTOR rgadExtent,
            out int pcDescriptorsReturned)
        {
            if (!_GetNextLogArchiveExtent(pvArchiveContext,
                                          ref rgadExtent,
                                          1,
                                          out pcDescriptorsReturned))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                switch (errorCode)
                {
                    case Error.ERROR_NO_MORE_ITEMS:
                        return false;

                    case Error.ERROR_LOG_READ_CONTEXT_INVALID:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_INVALID_STATE:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }

            return true;
        }

        public static void GetLogContainerName(
            SafeFileHandle hLog,
            int cidLogicalContainer,
            StringBuilder pwstrContainerName,
            int cLenContainerName,
            out int pcActualLenContainerName)
        {
            if (!_GetLogContainerName(hLog,
                                      cidLogicalContainer,
                                      pwstrContainerName,
                                      cLenContainerName,
                                      out pcActualLenContainerName))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                switch (errorCode)
                {
                    case Error.ERROR_MORE_DATA:
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }

        public static void SetLogArchiveMode(SafeFileHandle hLog,
                                             CLFS_LOG_ARCHIVE_MODE eNewMode)
        {
            if (!_SetLogArchiveMode(hLog, eNewMode))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                switch (errorCode)
                {
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_INVALID_PARAMETER:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }

        public static void QueryLogPolicy(
            SafeFileHandle hLog,
            out CLFS_MGMT_POLICY_MAXIMUMSIZE buffer)
        {
            buffer = new CLFS_MGMT_POLICY_MAXIMUMSIZE(0);
            if (!_QueryLogPolicy(
                    hLog,
                    buffer.Common.PolicyType,
                    ref buffer,
                    ref buffer.Common.LengthInBytes))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleQueryPolicyFailure(errorCode);
            }
        }

        public static void QueryLogPolicy(
            SafeFileHandle hLog,
            out CLFS_MGMT_POLICY_MINIMUMSIZE buffer)
        {
            buffer = new CLFS_MGMT_POLICY_MINIMUMSIZE(0);
            if (!_QueryLogPolicy(
                    hLog,
                    buffer.Common.PolicyType,
                    ref buffer,
                    ref buffer.Common.LengthInBytes))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleQueryPolicyFailure(errorCode);
            }
        }

        public static void QueryLogPolicy(
            SafeFileHandle hLog,
            out CLFS_MGMT_POLICY_GROWTHRATE buffer)
        {
            buffer = new CLFS_MGMT_POLICY_GROWTHRATE(0);
            if (!_QueryLogPolicy(
                    hLog,
                    buffer.Common.PolicyType,
                    ref buffer,
                    ref buffer.Common.LengthInBytes))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleQueryPolicyFailure(errorCode);
            }
        }

        public static void QueryLogPolicy(
            SafeFileHandle hLog,
            out CLFS_MGMT_POLICY_LOGTAIL buffer)
        {
            buffer = new CLFS_MGMT_POLICY_LOGTAIL(0);
            if (!_QueryLogPolicy(
                    hLog,
                    buffer.Common.PolicyType,
                    ref buffer,
                    ref buffer.Common.LengthInBytes))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleQueryPolicyFailure(errorCode);
            }
        }

        public static void QueryLogPolicy(
            SafeFileHandle hLog,
            out CLFS_MGMT_POLICY_AUTOSHRINK buffer)
        {
            buffer = new CLFS_MGMT_POLICY_AUTOSHRINK(0);
            if (!_QueryLogPolicy(
                    hLog,
                    buffer.Common.PolicyType,
                    ref buffer,
                    ref buffer.Common.LengthInBytes))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleQueryPolicyFailure(errorCode);
            }
        }

        public static void QueryLogPolicy(
            SafeFileHandle hLog,
            out CLFS_MGMT_POLICY_AUTOGROW buffer)
        {
            buffer = new CLFS_MGMT_POLICY_AUTOGROW(0);
            if (!_QueryLogPolicy(
                    hLog,
                    buffer.Common.PolicyType,
                    ref buffer,
                    ref buffer.Common.LengthInBytes))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleQueryPolicyFailure(errorCode);
            }
        }

        public static void QueryLogPolicy(
            SafeFileHandle hLog,
            out CLFS_MGMT_POLICY_NEWCONTAINERPREFIX buffer)
        {
            buffer = new CLFS_MGMT_POLICY_NEWCONTAINERPREFIX(0);
            if (!_QueryLogPolicy(
                    hLog,
                    buffer.Common.PolicyType,
                    ref buffer,
                    ref buffer.Common.LengthInBytes))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleQueryPolicyFailure(errorCode);
            }
        }

        public static void QueryLogPolicy(
            SafeFileHandle hLog,
            out CLFS_MGMT_POLICY_NEXTCONTAINERSUFFIX buffer)
        {
            buffer = new CLFS_MGMT_POLICY_NEXTCONTAINERSUFFIX(0);
            if (!_QueryLogPolicy(
                    hLog,
                    buffer.Common.PolicyType,
                    ref buffer,
                    ref buffer.Common.LengthInBytes))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleQueryPolicyFailure(errorCode);
            }
        }

        public static void InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_MAXIMUMSIZE policy)
        {
            if (!_InstallLogPolicy(hLog, ref policy))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleInstallPolicyFailure(errorCode);
            }
        }

        public static void InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_MINIMUMSIZE policy)
        {
            if (!_InstallLogPolicy(hLog, ref policy))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleInstallPolicyFailure(errorCode);
            }
        }

        public static void InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_GROWTHRATE policy)
        {
            if (!_InstallLogPolicy(hLog, ref policy))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleInstallPolicyFailure(errorCode);
            }
        }

        public static void InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_LOGTAIL policy)
        {
            if (!_InstallLogPolicy(hLog, ref policy))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleInstallPolicyFailure(errorCode);
            }
        }

        public static void InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_AUTOSHRINK policy)
        {
            if (!_InstallLogPolicy(hLog, ref policy))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleInstallPolicyFailure(errorCode);
            }
        }

        public static void InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_AUTOGROW policy)
        {
            if (!_InstallLogPolicy(hLog, ref policy))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleInstallPolicyFailure(errorCode);
            }
        }

        public static void InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_NEWCONTAINERPREFIX policy)
        {
            if (!_InstallLogPolicy(hLog, ref policy))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleInstallPolicyFailure(errorCode);
            }
        }

        public static void InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_NEXTCONTAINERSUFFIX policy)
        {
            if (!_InstallLogPolicy(hLog, ref policy))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                HandleInstallPolicyFailure(errorCode);
            }
        }

        public static uint HandleLogFull(SafeFileHandle hLog)
        {
            // Spec says this function must be async, if it's gonna
            // work at all. So we should never get true from this.
            //
            bool ret = _HandleLogFull(hLog);
            uint errorCode = (uint)Marshal.GetLastWin32Error();

            if (ret)
            {
                // This CLFS API is documented to always fail with ERROR_IO_PENDING.
                DiagnosticUtility.FailFast("HandleLogFull returned true");
            }

            return errorCode;
        }

        public static Exception HandleLogFullFilter(uint errorCode)
        {
            if (errorCode == Error.ERROR_IO_PENDING)
            {
                // The CLFS API was called with a NULL overlapped, so the operation should not be asynchronous.
                // This means that CLFS has a bug, so it is not safe to continue processing.
                DiagnosticUtility.FailFast("Async must be handled elsewhere");
            }
            switch (errorCode)
            {
                case Error.ERROR_LOG_FULL:
                case Error.ERROR_LOG_FULL_HANDLER_IN_PROGRESS:
                    return DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));
                default:
                    return DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
            }
        }

        public static bool ReadLogNotification(
            SafeFileHandle hLog,
            CLFS_MGMT_NOTIFICATION pNotification,
            NativeOverlapped* lpOverlapped)
        {
            if (!_ReadLogNotification(hLog, pNotification, lpOverlapped))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                switch (errorCode)
                {
                    case Error.ERROR_IO_PENDING:
                        return false;

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }

            return true;
        }

        public static void RegisterManageableLogClient(
            SafeFileHandle hLog,
            IntPtr pCallbacks) /* LOG_MANAGEMENT_CALLBACKS pCallbacks */
        {
            if (!_RegisterManageableLogClient(hLog, pCallbacks))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                switch (errorCode)
                {
                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }

        public static void AdvanceLogBaseSync(
            SafeMarshalContext pvMarshal,
            [In] ref ulong plsnBase,
            int fFlags)
        {
            if (!_AdvanceLogBase(pvMarshal, ref plsnBase, fFlags, null))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                switch (errorCode)
                {
                    case Error.ERROR_NO_SYSTEM_RESOURCES:
                    case Error.ERROR_LOG_FULL:
                    case Error.ERROR_INVALID_HANDLE:
                    case Error.ERROR_ACCESS_DENIED:
                    case Error.ERROR_IO_DEVICE:
                    case Error.ERROR_INVALID_PARAMETER:
                    case Error.ERROR_LOG_TAIL_INVALID:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                    default:
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
                }
            }
        }


        public static void LogTailAdvanceFailure(
            SafeFileHandle hLog,
            int reason)
        {
            if (!_LogTailAdvanceFailure(hLog, reason))
            {
                uint errorCode = (uint)Marshal.GetLastWin32Error();
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
            }

            return;
        }

        static void HandleInstallPolicyFailure(uint errorCode)
        {
            switch (errorCode)
            {
                // Generic handling for this error would be misleading.
                // This condition occurs when the NewExtentPrefix property is too long.
                case Error.ERROR_PATH_NOT_FOUND:
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new PathTooLongException(SR.GetString(SR.Policy_NewExtentPrefixInvalid)));
                case Error.ERROR_NOT_SUPPORTED:
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotSupported(SR.NotSupported_Policy));
                case Error.ERROR_ACCESS_DENIED:
                case Error.ERROR_INVALID_HANDLE:
                case Error.ERROR_OUTOFMEMORY:
                case Error.ERROR_NO_SYSTEM_RESOURCES:
                case Error.ERROR_LOG_POLICY_INVALID:
                case Error.ERROR_LOG_POLICY_CONFLICT:
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                default:
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
            }
        }

        static void HandleQueryPolicyFailure(uint errorCode)
        {
            switch (errorCode)
            {
                case Error.ERROR_LOG_POLICY_NOT_INSTALLED:
                    // NOTE: This is all right in our queries, as we just
                    // want the default back. The constructors for the
                    // buffers fill in the defaults, so this should be all
                    // right. It would be best if CLFS would tell us the
                    // defaults, and then we wouldn't get this error, but
                    // oh well.
                    //
                    break;
                case Error.ERROR_NOT_SUPPORTED:
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotSupported(SR.NotSupported_Policy));
                case Error.ERROR_ACCESS_DENIED:
                case Error.ERROR_INVALID_HANDLE:
                case Error.ERROR_OUTOFMEMORY:
                case Error.ERROR_NO_SYSTEM_RESOURCES:
                case Error.ERROR_LOG_POLICY_INVALID:
                case Error.ERROR_LOG_POLICY_CONFLICT:
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForKnownCode(errorCode));

                default:
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ExceptionForUnknownCode(errorCode));
            }
        }

        //=================================================================
        // Actual p/invoke signatures
        //=================================================================

        [DllImport(CLFSW32,
                   EntryPoint = "CreateLogFile",
                   SetLastError = true,
                   CharSet = CharSet.Unicode)]
        [ResourceExposure(ResourceScope.Machine)]
        [ResourceConsumption(ResourceScope.Machine)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static SafeFileHandle _CreateLogFile(
            string pszLogFileName,
            int fDesiredAccess,
            System.IO.FileShare dwShareMode,
            SECURITY_ATTRIBUTES psaLogFile,
            System.IO.FileMode fCreateDisposition,
            int fFlagsAndAttributes);

        [DllImport(CLFSW32,
                   EntryPoint = "DeleteLogFile",
                   SetLastError = true,
                   CharSet = CharSet.Unicode)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _DeleteLogFile(
            string pszLogFileName,
            SECURITY_ATTRIBUTES psaSecurityAttributes);

        [DllImport(CLFSW32,
                   EntryPoint = "GetLogFileInformation",
                   SetLastError = true,
                   CharSet = CharSet.Unicode)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _GetLogFileInformation(
            SafeFileHandle hLog,
            ref CLFS_INFORMATION pinfoBuffer,
            ref int cbBuffer);

        [DllImport(CLFSW32,
                   EntryPoint = "FlushLogToLsn",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _FlushLogToLsnSync(
            SafeMarshalContext pvMarshalContext,
            [In] ref ulong plsnFlush,
            out ulong plsnLastFlushed,
            NativeOverlapped* overlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "FlushLogToLsn",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _FlushLogToLsnAsync(
            SafeMarshalContext pvMarshalContext,
            [In] ref ulong plsnFlush,
            IntPtr plsnLastFlushed,
            NativeOverlapped* overlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "CreateLogMarshallingArea",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _CreateLogMarshallingArea(
            SafeFileHandle hLog,
            IntPtr pfnAllocBuffer,
            IntPtr pfnFreeBuffer,
            IntPtr pvBlockAllocContext,
            int cbMarshallingBlock,
            int cMaxWriteBlocks,
            int cMaxReadBlocks,
            out SafeMarshalContext ppvMarshal);

        [DllImport(CLFSW32,
                   EntryPoint = "AlignReservedLog",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _AlignReservedLogSingle(
            SafeMarshalContext pvMarshal,
            int cReservedRecords,
            ref long rgcbReservation,
            out long pcbAlignReservation);

        [DllImport(CLFSW32,
                   EntryPoint = "AllocReservedLog",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _AllocReservedLog(
            SafeMarshalContext pvMarshal,
            int cReservedRecords,
            [In] ref long pcbAdjustment);

        [DllImport(CLFSW32,
                   EntryPoint = "FreeReservedLog",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _FreeReservedLog(
            SafeMarshalContext pvMarshal,
            uint cReservedRecords,
            [In] ref long pcbAdjustment);

        [DllImport(CLFSW32,
                   EntryPoint = "ReadLogRestartArea",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _ReadLogRestartArea(
            SafeMarshalContext pvMarshal,
            out byte* ppvRestartBuffer,
            out int pcbRestartBuffer,
            out ulong plsn,
            out SafeReadContext ppvContext,
            NativeOverlapped* overlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "ReadPreviousLogRestartArea",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _ReadPreviousLogRestartArea(
            SafeReadContext pvReadContext,
            out byte* ppvRestartBuffer,
            out int pcbRestartBuffer,
            out ulong plsnRestart,
            NativeOverlapped* pOverlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "ReadLogRecord",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _ReadLogRecord(
            SafeMarshalContext pvMarshal,
            [In] ref ulong plsnFirst,
            CLFS_CONTEXT_MODE ecxMode,
            out byte* ppvReadBuffer,
            out int pcbReadBuffer,
            out byte peRecordType,
            out ulong plsnUndoNext,
            out ulong plsnPrevious,
            out SafeReadContext ppvReadContext,
            NativeOverlapped* pOverlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "ReadNextLogRecord",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _ReadNextLogRecord(
            SafeReadContext pvReadContext,
            out byte* ppvReadBuffer,
            out int pcbReadBuffer,
            ref byte peRecordType,
            IntPtr plsnUser, // NOTE: Not used: [In] ref ulong
            out ulong plsnUndoNext,
            out ulong plsnPrevious,
            out ulong plsnRecord,
            NativeOverlapped* pOverlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "ReserveAndAppendLog",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _ReserveAndAppendLog(
            SafeMarshalContext pvMarshal,
            CLFS_WRITE_ENTRY[] rgWriteEntries,
            int cWriteEntries,
            [In] ref ulong plsnUndoNext,
            [In] ref ulong plsnPrevious,
            int cReserveRecords,
            long[] rgcbReservation,
            int fFlags,
            IntPtr plsn,
            NativeOverlapped* pOverlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "WriteLogRestartArea",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _WriteLogRestartArea(
            SafeMarshalContext pvMarshal,
            byte* pvRestartBuffer,
            int cbRestartBuffer,
            [In] ref ulong plsnBase,
            int fFlags,
            IntPtr pcbWritten, // int*
            IntPtr plsnRestart, // ulong*
            NativeOverlapped* pOverlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "WriteLogRestartArea",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _WriteLogRestartAreaNoBase(
            SafeMarshalContext pvMarshal,
            byte* pvRestartBuffer,
            int cbRestartBuffer,
            IntPtr mustBeZero,
            int fFlags,
            IntPtr pcbWritten, // int*
            IntPtr plsnRestart, // ulong*
            NativeOverlapped* pOverlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "AddLogContainer",
                   SetLastError = true,
                   CharSet = CharSet.Unicode)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _AddLogContainer(SafeFileHandle hLog,
                                            [In] ref ulong pcbContainer,
                                            string pwszContainerPath,
                                            IntPtr pReserved);

        [DllImport(CLFSW32,
                   EntryPoint = "AddLogContainer",
                   SetLastError = true,
                   CharSet = CharSet.Unicode)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _AddLogContainerNoSize(
            SafeFileHandle hLog,
            IntPtr mbz,
            string pwszContainerPath,
            IntPtr pReserved);

        [DllImport(CLFSW32,
                   EntryPoint = "RemoveLogContainer",
                   SetLastError = true,
                   CharSet = CharSet.Unicode)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _RemoveLogContainer(SafeFileHandle hLog,
                                               string pszContainerPath,
                                               bool fForce,
                                               NativeOverlapped* pOverlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "CreateLogContainerScanContext",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _CreateLogContainerScanContext(
            SafeFileHandle hLog,
            uint cFromContainer,
            uint cContainers,
            CLFS_SCAN_MODE eScanMode,
            ref CLFS_SCAN_CONTEXT pcxScan,
            NativeOverlapped* pOverlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "ScanLogContainers",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _ScanLogContainers(ref CLFS_SCAN_CONTEXT pcxScan,
                                              CLFS_SCAN_MODE eScanMode,
                                              IntPtr pReserved);

        [DllImport(CLFSW32,
                   EntryPoint = "SetLogArchiveTail",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _SetLogArchiveTail(SafeFileHandle hLog,
                                              ref ulong plsnArchiveTail,
                                              NativeOverlapped* pverlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "SetEndOfLog",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _SetEndOfLog(SafeFileHandle hLog,
                                        [In] ref ulong plsnEnd,
                                        NativeOverlapped* lpOverlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "TruncateLog",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _TruncateLog(SafeMarshalContext pvMarshal,
                                        [In] ref ulong plsnEnd,
                                        NativeOverlapped* lpOverlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "PrepareLogArchive",
                   SetLastError = true,
                   CharSet = CharSet.Unicode)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _PrepareLogArchive(
            SafeFileHandle hLog,
            [Out] StringBuilder pszBaseLogFileName,
            int cLen,
            [In] ref ulong pLsnLow,
            [In] ref ulong pLsnHigh,
            out int pcActualLength,
            out ulong poffBaseLogFileData,
            out ulong pcbBaseLogFileLength,
            out ulong plsnBase,
            out ulong plsnLast,
            out ulong plsnCurrentArchiveTail,
            out SafeArchiveContext ppvArchiveContext);

        [DllImport(CLFSW32,
                   EntryPoint = "ReadLogArchiveMetadata",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _ReadLogArchiveMetadata(
            SafeArchiveContext pvArchiveContext,
            int cbOffset,
            int cbBytesToRead,
            byte* pbReadBuffer,
            out uint pcbBytesRead);

        [DllImport(CLFSW32,
                   EntryPoint = "GetNextLogArchiveExtent",
                   SetLastError = true,
                   CharSet = CharSet.Unicode)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _GetNextLogArchiveExtent(
            SafeArchiveContext pvArchiveContext,
            ref CLFS_ARCHIVE_DESCRIPTOR rgadExtent,
            int cDescriptors, // Must be 1
            out int pcDescriptorsReturned);

        [DllImport(CLFSW32,
                   EntryPoint = "GetLogContainerName",
                   SetLastError = true,
                   CharSet = CharSet.Unicode)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _GetLogContainerName(
            SafeFileHandle hLog,
            int cidLogicalContainer,
            StringBuilder pwstrContainerName,
            int cLenContainerName,
            out int pcActualLenContainerName);

        [DllImport(CLFSW32,
                   EntryPoint = "SetLogArchiveMode",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _SetLogArchiveMode(
            SafeFileHandle hLog,
            CLFS_LOG_ARCHIVE_MODE eNewMode);

        //--------------------------------------------------------------------
        // These log policy things are so duplicated, because
        // marshalling a union is complicated.
        //--------------------------------------------------------------------

        [DllImport(CLFSW32,
                   EntryPoint = "QueryLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _QueryLogPolicy(
            SafeFileHandle hLog,
            CLFS_MGMT_POLICY_TYPE ePolicyType,
            ref CLFS_MGMT_POLICY_MAXIMUMSIZE buffer,
            ref uint pcbPolicyBuffer);

        [DllImport(CLFSW32,
                   EntryPoint = "QueryLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _QueryLogPolicy(
            SafeFileHandle hLog,
            CLFS_MGMT_POLICY_TYPE ePolicyType,
            ref CLFS_MGMT_POLICY_MINIMUMSIZE buffer,
            ref uint pcbPolicyBuffer);

        [DllImport(CLFSW32,
                   EntryPoint = "QueryLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _QueryLogPolicy(
            SafeFileHandle hLog,
            CLFS_MGMT_POLICY_TYPE ePolicyType,
            ref CLFS_MGMT_POLICY_GROWTHRATE buffer,
            ref uint pcbPolicyBuffer);

        [DllImport(CLFSW32,
                   EntryPoint = "QueryLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _QueryLogPolicy(
            SafeFileHandle hLog,
            CLFS_MGMT_POLICY_TYPE ePolicyType,
            ref CLFS_MGMT_POLICY_LOGTAIL buffer,
            ref uint pcbPolicyBuffer);

        [DllImport(CLFSW32,
                   EntryPoint = "QueryLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _QueryLogPolicy(
            SafeFileHandle hLog,
            CLFS_MGMT_POLICY_TYPE ePolicyType,
            ref CLFS_MGMT_POLICY_AUTOSHRINK buffer,
            ref uint pcbPolicyBuffer);

        [DllImport(CLFSW32,
                   EntryPoint = "QueryLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _QueryLogPolicy(
            SafeFileHandle hLog,
            CLFS_MGMT_POLICY_TYPE ePolicyType,
            ref CLFS_MGMT_POLICY_AUTOGROW buffer,
            ref uint pcbPolicyBuffer);

        [DllImport(CLFSW32,
                   EntryPoint = "QueryLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _QueryLogPolicy(
            SafeFileHandle hLog,
            CLFS_MGMT_POLICY_TYPE ePolicyType,
            ref CLFS_MGMT_POLICY_NEWCONTAINERPREFIX buffer,
            ref uint pcbPolicyBuffer);

        [DllImport(CLFSW32,
                   EntryPoint = "QueryLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _QueryLogPolicy(
            SafeFileHandle hLog,
            CLFS_MGMT_POLICY_TYPE ePolicyType,
            ref CLFS_MGMT_POLICY_NEXTCONTAINERSUFFIX buffer,
            ref uint pcbPolicyBuffer);

        [DllImport(CLFSW32,
                   EntryPoint = "InstallLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_MAXIMUMSIZE buffer);

        [DllImport(CLFSW32,
                   EntryPoint = "InstallLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_MINIMUMSIZE buffer);

        [DllImport(CLFSW32,
                   EntryPoint = "InstallLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_GROWTHRATE buffer);

        [DllImport(CLFSW32,
                   EntryPoint = "InstallLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_LOGTAIL buffer);

        [DllImport(CLFSW32,
                   EntryPoint = "InstallLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_AUTOSHRINK buffer);

        [DllImport(CLFSW32,
                   EntryPoint = "InstallLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_AUTOGROW buffer);

        [DllImport(CLFSW32,
                   EntryPoint = "InstallLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_NEWCONTAINERPREFIX buffer);

        [DllImport(CLFSW32,
                   EntryPoint = "InstallLogPolicy",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _InstallLogPolicy(
            SafeFileHandle hLog,
            ref CLFS_MGMT_POLICY_NEXTCONTAINERSUFFIX buffer);

        //--------------------------------------------------------------------
        // End policy thingies
        //--------------------------------------------------------------------

        [DllImport(CLFSW32,
                   EntryPoint = "HandleLogFull",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _HandleLogFull(SafeFileHandle hLog);

        [DllImport(CLFSW32,
                   EntryPoint = "ReadLogNotification",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _ReadLogNotification(
            SafeFileHandle hLog,
            CLFS_MGMT_NOTIFICATION pNotification,
            NativeOverlapped* pOverlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "RegisterManageableLogClient",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _RegisterManageableLogClient(
            SafeFileHandle hLog,
            IntPtr pCallbacks); /* LOG_MANAGEMENT_CALLBACKS */

        [DllImport(CLFSW32,
                   EntryPoint = "AdvanceLogBase",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _AdvanceLogBase(
            SafeMarshalContext pvMarshal,
            [In] ref ulong plsnBase,
            int fFlags,
            NativeOverlapped* pOverlapped);

        [DllImport(CLFSW32,
                   EntryPoint = "LogTailAdvanceFailure",
                   SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        [SuppressMessage(FxCop.Category.Security, FxCop.Rule.ReviewSuppressUnmanagedCodeSecurityUsage, Justification = "Explicit security permission is demanded during log file create, open and delete operations in LogStore")]
        extern static bool _LogTailAdvanceFailure(
            SafeFileHandle hLog,
            int reason);

    }
}
