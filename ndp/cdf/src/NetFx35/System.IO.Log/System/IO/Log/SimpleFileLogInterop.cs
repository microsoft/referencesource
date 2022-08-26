//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
using System;
using System.Runtime.InteropServices;
using System.Security;

namespace System.IO.Log
{
    internal struct UnmanagedBlob
    {
        public uint cbSize;
        public IntPtr pBlobData;
    }

    internal enum STGM
    {
        Read = 0,
        Write = 1,
        ReadWrite = 2
    }

    internal enum RECORD_READING_POLICY
    {
        FORWARD = 1,
        BACKWARD = 2,
        RANDOM = 3
    }

    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("FF222117-0C6C-11d2-B89A-00C04FB9618A")]
    [SuppressUnmanagedCodeSecurity]
    internal interface ILog
    {
        void Force(ulong lsnMinToForce);

        void AppendRecord([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1)] UnmanagedBlob[] rgBlob,
                          int cBlob,
                          [MarshalAs(UnmanagedType.Bool)] bool fForceNow,
                          out ulong lsn);

        void ReadRecord(ulong lsnToRead,
                        out ulong plsnPrev,
                        out ulong plsnNext,
                        out CoTaskMemHandle data,
                        out int cbData);

        void ReadRecordPrefix(
                                ulong lsnToRead,
                                out ulong plsnPrev,
                                out ulong plsnNext,
                                [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 0)] byte[] pbData,
                                ref int cbData,
                                out int chRecord
                                   );

        void GetLogLimits(out ulong lsnFirst,
                          out ulong lsnLast);

        void TruncatePrefix(ulong lsnFirstToKeep);

        void SetAccessPolicyHint(RECORD_READING_POLICY policy);
    }

    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("00951E8C-1294-11d1-97E4-00C04FB9618A")]
    [SuppressUnmanagedCodeSecurity]
    internal interface IFileBasedLogInit
    {
        void InitNew([MarshalAs(UnmanagedType.LPWStr)] string filename,
                     int cbCapacityHint);
    }


    [ComImport]
    [Guid("E16C0593-128F-11d1-97E4-00C04FB9618A")]
    [SuppressUnmanagedCodeSecurity]
    internal class SimpleFileBasedLog
    {
    }
}
