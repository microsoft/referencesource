//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.IO;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.ComTypes;
    using System.Runtime.CompilerServices;
    using System.Runtime.ConstrainedExecution;
    using Microsoft.Win32;
    using Microsoft.Win32.SafeHandles;

    static class ETWWsatTraceSession
    {
        const string WsatTraceSessionName = "WsatTraceSession";
        const string WsatTraceSessionGuid = "7f3fe630-462b-47c5-ab07-67ca84934abd";
        const string WsatProviderGuid = "7f3fe630-462b-47c5-ab07-67ca84934abd";
        const string DafaultLogDirectory = @"MsDtc\Trace";
        const string DefaultLogFileName = "WsatTrace.etl";
        internal const int DefaultLogFileSize = 10; // In MB
        const int DefaultBuffers = 25; // In MB
        internal const int MaxLogFileSize = 1024; // In MB
        internal const int MaxLogBuffers = 999; // Max. log buffers is between 1-999
        const int BackupAttemptNum = 100;

        internal static void FlushData()
        {
            EVENT_TRACE_PROPERTIES properties = new EVENT_TRACE_PROPERTIES(String.Empty, String.Empty);
            uint err = SafeNativeMethods.FlushTrace(0, WsatTraceSessionName, ref properties);

            if (err != SafeNativeMethods.ERROR_SUCCESS)
            {
                if (err == SafeNativeMethods.ERROR_ACCESS_DENIED)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_ACCESS_DENIED,
                                                 SR.GetString(SR.ErrorSessionFlushDataAccessDenied));
                }
                else
                {
                    throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_ERROR,
                                                 SR.GetString(SR.ErrorSessionFlushData, err));
                }
            }
        }

        internal static void StopSession()
        {
            ulong sessionHandle = 0;

            GetSessionHandle(ref sessionHandle);

            // session not exists
            if (sessionHandle == 0)
            {
                return;
            }

            // disable provider first
            Guid providerGuid = new Guid(WsatProviderGuid);
            uint err = SafeNativeMethods.EnableTrace(0, 0, 0, ref providerGuid, sessionHandle);

            if (err == SafeNativeMethods.ERROR_SUCCESS)
            {
                EVENT_TRACE_PROPERTIES properties = new EVENT_TRACE_PROPERTIES(String.Empty, String.Empty);
                err = SafeNativeMethods.StopTrace(0, WsatTraceSessionName, ref properties);
            }

            // still check for SessionExist as the StopTrace API always treats us
            if (err != SafeNativeMethods.ERROR_SUCCESS || IsSessionExist())
            {
                if (err == SafeNativeMethods.ERROR_ACCESS_DENIED)
                {
                    // throw access denied
                    throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_ACCESS_DENIED,
                                                 SR.GetString(SR.ErrorSessionStopAccessDenied));
                }
                else
                {
                    // throw general error message
                    throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_ERROR,
                                                 SR.GetString(SR.ErrorSessionStop, err));
                }
            }
        }

        static void GetSessionHandle(ref ulong sessionHandle)
        {
            EVENT_TRACE_PROPERTIES properties = new EVENT_TRACE_PROPERTIES(String.Empty, String.Empty);

            uint err = SafeNativeMethods.QueryTrace(0, WsatTraceSessionName, ref properties);

            if (err == SafeNativeMethods.ERROR_SUCCESS)
            {
                sessionHandle = properties.Head.Wnode.HistoricalContext;
            }
            else
            {
                sessionHandle = 0;
            }
        }

        internal static bool IsSessionExist()
        {
            ulong sessionHandle = 0;

            GetSessionHandle(ref sessionHandle);

            return (sessionHandle != 0);
        }

        // For UI to get MaxFileSize saved in registry
        internal static uint GetMaxTraceFileSizeFromReg()
        {
            uint fileSize = 0;

            RegistryConfigurationProvider wsatTraceProvider = new RegistryConfigurationProvider(RegistryHive.LocalMachine, WsatKeys.WsatRegKey, null);
            using (wsatTraceProvider)
            {
                fileSize = wsatTraceProvider.ReadUInt32(WsatKeys.MaxTraceSizeKey, DefaultLogFileSize);
            }

            return fileSize;
        }

        internal static void SaveMaxTraceFileSizeToReg(uint fileSize)
        {
            RegistryConfigurationProvider wsatTraceProvider = new RegistryConfigurationProvider(RegistryHive.LocalMachine, WsatKeys.WsatRegKey, null);
            using (wsatTraceProvider)
            {
                wsatTraceProvider.WriteUInt32(WsatKeys.MaxTraceSizeKey, fileSize);
            }
        }

        static void ReadParamFromReg(out string logFileName, out uint maxLogBuffers)
        {
            // Read log file location from registry if exists
            string logFileNameInRegistry = null;

            RegistryConfigurationProvider wsatTraceProvider = new RegistryConfigurationProvider(RegistryHive.LocalMachine, WsatKeys.WsatRegKey, null);
            using (wsatTraceProvider)
            {
                logFileNameInRegistry = wsatTraceProvider.ReadString(WsatKeys.TraceFileDiectoryKey, null);
                maxLogBuffers = wsatTraceProvider.ReadUInt32(WsatKeys.MaxTraceBuffersKey, DefaultBuffers);

                if (maxLogBuffers == 0 || maxLogBuffers > MaxLogBuffers)
                {
                    maxLogBuffers = DefaultBuffers;
                    wsatTraceProvider.WriteUInt32(WsatKeys.MaxTraceBuffersKey, DefaultBuffers);
                }
            }

            String errLogFileName = String.Empty;
            try
            {
                if (!String.IsNullOrEmpty(logFileNameInRegistry))
                {
                    logFileNameInRegistry = logFileNameInRegistry.Trim();
                }

                if (!String.IsNullOrEmpty(logFileNameInRegistry))
                {
                    errLogFileName = logFileNameInRegistry;
                    logFileName = Path.Combine(logFileNameInRegistry, DefaultLogFileName);
                }
                else
                {
                    errLogFileName = Environment.SystemDirectory;
                    logFileName = Path.Combine(Environment.SystemDirectory, DafaultLogDirectory);
                    errLogFileName = logFileName;
                    logFileName = Path.Combine(logFileName, DefaultLogFileName);
                }
            }
            catch (ArgumentException)
            {
                throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_INVALID_LOGFILE_NAME,
                                             SR.GetString(SR.ErrorSessionInvalidLogFileName, errLogFileName));
            }

            if (logFileName.Length >= SafeNativeMethods.MaxTraceFileNameLen)
            {
                throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_TOO_LONG_LOGFILE_NAME,
                                             SR.GetString(SR.ErrorSessionTooLongLogFileName, logFileName));
            }
        }

        static void BackUpLogFile(string logFileName)
        {
            if (!File.Exists(logFileName))
            {
                return;
            }

            // backup log file
            DateTime localtime = DateTime.Now;

            string timeStamp = String.Format(System.Globalization.CultureInfo.InvariantCulture,
                                             "{0:0000}-{1:00}-{2:00}-{3:00}-{4:00}-{5:00}-{6:0000}",
                                             localtime.Year, localtime.Month, localtime.Day, localtime.Hour,
                                             localtime.Minute, localtime.Second, localtime.Millisecond);

            string backupFile = logFileName + '.' + timeStamp;

            int attemptNum;
            for (attemptNum = 0; attemptNum < BackupAttemptNum; attemptNum++)
            {
                string backupFile2 = backupFile + "-" + String.Format(System.Globalization.CultureInfo.InvariantCulture, "{0:00}", attemptNum);
                if (!File.Exists(backupFile2))
                {
                    try
                    {
                        File.Move(logFileName, backupFile2);
                        break;
                    }
#pragma warning suppress 56500
                    catch (Exception e)
                    {
                        throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_BACKUPFILE_ERROR,
                                                     SR.GetString(SR.ErrorSessionBackupFileRename, logFileName), e);
                    }
                }
            }

            if (attemptNum >= BackupAttemptNum)
            {
                throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_BACKUPFILE_ERROR,
                                                 SR.GetString(SR.ErrorSessionBackupFileAttempt, logFileName));
            }
        }

        internal static void StartSession(uint logFileSize)
        {
            if (IsSessionExist())
                return;

            string logFileName = String.Empty;
            uint logBuffers = 0;

            ReadParamFromReg(out logFileName, out logBuffers);
            BackUpLogFile(logFileName);

            EVENT_TRACE_PROPERTIES properties = new EVENT_TRACE_PROPERTIES(WsatTraceSessionName, logFileName);
            ulong sessionHandle = 0;

            properties.Head.Wnode.Flags = SafeNativeMethods.WNODE_FLAG_TRACED_GUID;
            properties.Head.Wnode.Guid = new Guid(WsatTraceSessionGuid);

            properties.Head.Wnode.ClientContext = 1; //QPC clock resolution
            properties.Head.LogFileMode = SafeNativeMethods.EVENT_TRACE_FILE_MODE_CIRCULAR | SafeNativeMethods.EVENT_TRACE_USE_PAGED_MEMORY;
            properties.Head.MaximumBuffers = logBuffers;
            properties.Head.MaximumFileSize = logFileSize;

            uint err = SafeNativeMethods.StartTrace(out sessionHandle, WsatTraceSessionName, ref properties);

            if (err != SafeNativeMethods.ERROR_SUCCESS)
            {
                switch (err) 
                {
                    case SafeNativeMethods.ERROR_BAD_PATHNAME:
                        throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_BAD_PATHNAME,
                                                     SR.GetString(SR.ErrorSessionStartBadPathname));
                    case SafeNativeMethods.ERROR_DISK_FULL:
                        throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_DISK_FULL,
                                                     SR.GetString(SR.ErrorSessionStartDiskFull));
                    case SafeNativeMethods.ERROR_FILE_NOT_FOUND:
                        throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_FILE_NOT_FOUND,
                                                     SR.GetString(SR.ErrorSessionStartFileNotFound, logFileName));
                    case SafeNativeMethods.ERROR_PATH_NOT_FOUND:
                        throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_PATH_NOT_FOUND,
                                                     SR.GetString(SR.ErrorSessionStartPathNotFound, logFileName));
                    default:
                        throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_ERROR,
                                                     SR.GetString(SR.ErrorSessionStart, err));
                }
            }

            Guid providerGuid = new Guid(WsatProviderGuid);
            err = SafeNativeMethods.EnableTrace(1, 0, 0, ref providerGuid, sessionHandle);

            if (err != SafeNativeMethods.ERROR_SUCCESS)
            {
                // stop trace session
                EVENT_TRACE_PROPERTIES properties2 = new EVENT_TRACE_PROPERTIES(String.Empty, String.Empty);
                SafeNativeMethods.StopTrace(0, WsatTraceSessionName, ref properties2);

                if (err == SafeNativeMethods.ERROR_ACCESS_DENIED)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_ACCESS_DENIED,
                                                 SR.GetString(SR.ErrorSessionEnableProviderAccessDenied));
                }
                else
                {
                    throw new WsatAdminException(WsatAdminErrorCode.ETW_SESSION_ERROR,
                                                 SR.GetString(SR.ErrorSessionEnableProviderError, err));
                }
            }

            SaveMaxTraceFileSizeToReg(logFileSize);
        }
    }
}
