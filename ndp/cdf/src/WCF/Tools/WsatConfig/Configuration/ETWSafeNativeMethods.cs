//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.ComTypes;
    using System.Runtime.CompilerServices;
    using System.Runtime.ConstrainedExecution;
    using Microsoft.Win32.SafeHandles;

    [StructLayout(LayoutKind.Explicit, CharSet = CharSet.Unicode, Size = 48)]
    struct WNODE_HEADER
    {
        [FieldOffset(0)]
        internal uint BufferSize;
        [FieldOffset(4)]
        internal uint ProviderId;

        [FieldOffset(8)]
        internal ulong HistoricalContext;
        [FieldOffset(8)]
        internal uint Version;
        [FieldOffset(12)]
        internal uint Linkage;

        [FieldOffset(16)]
        internal uint KernelHandle;
        [FieldOffset(16)]
        internal Int64 TimeStamp;

        [FieldOffset(24)]
        internal System.Guid Guid;
        [FieldOffset(40)]
        internal uint ClientContext;
        [FieldOffset(44)]
        internal uint Flags;
    };

    // the size of this structure is 120 in both 32-bits and 64-bits system
    // In 32-bits system, there is a 4 bytes padding at the end of structure
    // the difference is caused by the size of LoggerThreadId, it is 4 bytes in 32-bits
    // while 8 bytes in 64 bits
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Size = 120)]
    struct EVENT_TRACE_PROPERTIES_HEAD
    {
        internal WNODE_HEADER Wnode;
        internal uint BufferSize;
        internal uint MinimumBuffers;
        internal uint MaximumBuffers;
        internal uint MaximumFileSize;
        internal uint LogFileMode;
        internal uint FlushTimer;
        internal uint EnableFlags;
        internal int AgeLimit;
        internal uint NumberOfBuffers;
        internal uint FreeBuffers;
        internal uint EventsLost;
        internal uint BuffersWritten;
        internal uint LogBuffersLost;
        internal uint RealTimeBuffersLost;
        internal IntPtr LoggerThreadId;
        internal uint LogFileNameOffset;
        internal uint LoggerNameOffset;
    };

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Size = 4096)]
    struct EVENT_TRACE_PROPERTIES_TAIL
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = SafeNativeMethods.MaxTraceLoggerNameLen)]
        internal char[] LoggerName;   // max len 1024 is used as vary lens of logger names can't be implemented
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = SafeNativeMethods.MaxTraceFileNameLen)]
        internal char[] LogFileName;  // max len 1024 is used as vary lens of log file name can't be implemented
    };

    // this structure is divided into two parts(head and tail) as the size must be 120 bytes in
    // 32-bits and 64-bits system
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Size = 4216)]
    struct EVENT_TRACE_PROPERTIES
    {
        internal EVENT_TRACE_PROPERTIES_HEAD Head;
        internal EVENT_TRACE_PROPERTIES_TAIL Tail;

        internal EVENT_TRACE_PROPERTIES(string loggerName, string logFileName)
        {

            Head.Wnode.BufferSize        = 0; Head.Wnode.ProviderId = 0;
            Head.Wnode.HistoricalContext = 0; Head.Wnode.Version = 0;
            Head.Wnode.Linkage           = 0; Head.Wnode.KernelHandle = 0;
            Head.Wnode.TimeStamp         = 0; Head.Wnode.ClientContext = 0;
            Head.Wnode.Flags             = 0;

            Head.Wnode.Guid = new System.Guid();

            Head.BufferSize       = 0; Head.MinimumBuffers = 0;
            Head.MaximumBuffers   = 0; Head.MaximumFileSize = 0;
            Head.LogFileMode      = 0; Head.FlushTimer = 0;
            Head.EnableFlags      = 0; Head.AgeLimit = 0;
            Head.NumberOfBuffers  = 0; Head.FreeBuffers = 0;
            Head.EventsLost       = 0; Head.LogFileNameOffset = 0;
            Head.LogBuffersLost   = 0; Head.RealTimeBuffersLost = 0;
            Head.BuffersWritten   = 0; Head.LoggerThreadId = IntPtr.Zero;
            Head.LoggerNameOffset = 0;

            Tail.LoggerName = new char[SafeNativeMethods.MaxTraceLoggerNameLen];
            Tail.LogFileName = new char[SafeNativeMethods.MaxTraceFileNameLen];

            if (!string.IsNullOrEmpty(loggerName))
            {
                char[] loggerNameChars = loggerName.ToCharArray();
                if (loggerNameChars.Length < SafeNativeMethods.MaxTraceLoggerNameLen)
                {
                    Array.Copy(loggerNameChars, 0, Tail.LoggerName, 0, loggerNameChars.Length);
                }
                else
                {
                    Array.Copy(loggerNameChars, 0, Tail.LoggerName, 0, SafeNativeMethods.MaxTraceLoggerNameLen - 1);
                }
            }

            if (!string.IsNullOrEmpty(logFileName))
            {
                char[] logFileNameChars = logFileName.ToCharArray();
                if (logFileNameChars.Length < SafeNativeMethods.MaxTraceFileNameLen)
                {
                    Array.Copy(logFileNameChars, 0, Tail.LogFileName, 0, logFileNameChars.Length);
                }
                else
                {
                    Array.Copy(logFileNameChars, 0, Tail.LogFileName, 0, SafeNativeMethods.MaxTraceFileNameLen - 1);
                }
            }

            Head.Wnode.BufferSize = (uint)(Marshal.SizeOf(this));
            Head.LogFileNameOffset = (uint)(Marshal.SizeOf(this) - Tail.LogFileName.Length * sizeof(char));
            Head.LoggerNameOffset = (uint)(Head.LogFileNameOffset - Tail.LoggerName.Length * sizeof(char));
        }
    };

    static partial class SafeNativeMethods
    {
        internal const uint ERROR_ACCESS_DENIED = 5;
        internal const uint ERROR_BAD_PATHNAME = 161;
        internal const uint ERROR_DISK_FULL = 112;
        internal const uint ERROR_ALREADY_EXISTS = 183;
        internal const uint ERROR_PATH_NOT_FOUND = 3;

        internal const uint WNODE_FLAG_TRACED_GUID = 0x01000000;
        internal const uint EVENT_TRACE_FILE_MODE_CIRCULAR = 0x00000002;
        internal const uint EVENT_TRACE_USE_PAGED_MEMORY = 0x01000000;

        internal const int MaxTraceFileNameLen = 1024;
        internal const int MaxTraceLoggerNameLen = 1024;

        [SuppressUnmanagedCodeSecurity]
        [DllImport(AdvApi32, CharSet = CharSet.Unicode)]
        internal static extern uint StartTrace(
            [Out] out ulong SessionHandle,
            [In] string SessionName,
            [In, Out] ref EVENT_TRACE_PROPERTIES Properties);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(AdvApi32, CharSet = CharSet.Unicode)]
        internal static extern uint StopTrace(
            [In] ulong SessionHandle,
            [In] string SessionName,
            [In, Out] ref EVENT_TRACE_PROPERTIES Properties);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(AdvApi32, CharSet = CharSet.Unicode)]
        internal static extern uint EnableTrace(
            [In] uint Enable,
            [In] uint EnableFlag,
            [In] uint EnableLevel,
            [In] ref Guid ControlGuid,
            [In] ulong SessionHandle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(AdvApi32, CharSet = CharSet.Unicode)]
        internal static extern uint FlushTrace(
            [In] ulong SessionHandle,
            [In] string SessionName,
            [In, Out] ref EVENT_TRACE_PROPERTIES Properties);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(AdvApi32, CharSet = CharSet.Unicode)]
        internal static extern uint QueryTrace(
            [In] ulong SessionHandle,
            [In] string SessionName,
            [In, Out] ref EVENT_TRACE_PROPERTIES Properties);
    }
}
