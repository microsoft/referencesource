//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.ComTypes;

    internal class SimpleFileLog
    {
        SmuggledIUnknown smuggledLog;

        internal SimpleFileLog(string absolutePath, int size)
        {
            // Open the log.
            // First, we try to open an existing log-- if that doesn't
            // work, we create a new one.
            //
            bool loaded = false;
            ILog log = (ILog)new SimpleFileBasedLog();
            try
            {
                if (File.Exists(absolutePath))
                {
                    try
                    {
                        IPersistFile ipf = (IPersistFile)log;
                        ipf.Load(absolutePath, (int)STGM.ReadWrite);
                        loaded = true;
                    }
                    catch (FileNotFoundException)
                    {
                        // File not found, create a new one.
                    }
                }

                if (!loaded)
                {
                    // File does not exist.  Attempt to create a new one,
                    // with the specified size.
                    //
                    IFileBasedLogInit logInit = (IFileBasedLogInit)log;
                    logInit.InitNew(absolutePath, size);
                }

                log.SetAccessPolicyHint(RECORD_READING_POLICY.RANDOM);
                this.smuggledLog = new SmuggledIUnknown(log);
            }
            catch (COMException exception)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ConvertKnownException(exception));
            }
            catch (FileNotFoundException)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentException(SR.GetString(SR.Argument_FileNameInvalid)));
            }
            finally
            {
                SafeRelease(log);
            }
        }

        static void SafeRelease(object obj)
        {
            if (obj != null)
            {
                Marshal.FinalReleaseComObject(obj);
            }
        }

        internal void Close()
        {

            if (!this.smuggledLog.IsInvalid)
            {
                ILog log = (ILog)(this.smuggledLog.Smuggle());
                try
                {
                    this.smuggledLog.Close();
                }
                catch (COMException exception)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ConvertKnownException(exception));
                }
                finally
                {
                    SafeRelease(log);
                }
            }
        }

        internal void TruncatePrefix(SequenceNumber seqNumber)
        {
            if (this.smuggledLog.IsInvalid)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
            }

            ILog log = (ILog)(this.smuggledLog.Smuggle());
            try
            {
                log.TruncatePrefix(seqNumber.High);
            }
            catch (COMException exception)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ConvertKnownException(exception));
            }
            finally
            {
                SafeRelease(log);
            }
        }

        internal void Force(SequenceNumber seqNumber)
        {
            if (this.smuggledLog.IsInvalid)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
            }

            ILog log = (ILog)(this.smuggledLog.Smuggle());
            try
            {
                log.Force(seqNumber.High);
            }
            catch (COMException exception)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ConvertKnownException(exception));
            }
            finally
            {
                SafeRelease(log);
            }
        }

        internal void GetLogLimits(out SequenceNumber first, out SequenceNumber last)
        {
            ulong lsnfirst = 0;
            ulong lsnlast = 0;

            if (this.smuggledLog.IsInvalid)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
            }

            ILog log = (ILog)(this.smuggledLog.Smuggle());
            try
            {
                log.GetLogLimits(out lsnfirst, out lsnlast);
            }
            catch (COMException exception)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ConvertKnownException(exception));
            }
            finally
            {
                SafeRelease(log);
            }

            first = new SequenceNumber(lsnfirst);
            last = new SequenceNumber(lsnlast);
        }

        internal SequenceNumber AppendRecord(UnmanagedBlob[] data, bool fForceNow)
        {
            ulong recordLsn = 0;

            if (this.smuggledLog.IsInvalid)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
            }

            ILog log = (ILog)(this.smuggledLog.Smuggle());
            try
            {
                // Due to WinSE 184096 bug, we always do force flush as a work around. Ignoring fForceNow value... 
                // If force flush is not done, the user will be able to read stale records after calling TruncatePrefix and 
                // the base lsn will not be updated to reflect the new lsn in TruncatePrefix call.
                // This is a temporary work around fix and if txflog issue is fixed at a later date, we need to have a QFE
                // to use the user passed in fForceNow flag value to support buffering of log records.
                // Related Bugs:
                //          WinSE: 184096
                //          Windows OS: 1729140
                //          MB: 57586 52216

                log.AppendRecord(data,
                                 data.Length,
                                 true,
                                 out recordLsn);
            }
            catch (COMException exception)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ConvertKnownException(exception));
            }
            finally
            {
                SafeRelease(log);
            }

            return new SequenceNumber(recordLsn);
        }

        internal void ReadRecord(SequenceNumber seqNumtoRead, out byte[] record, out int recordSize, out SequenceNumber previous, out SequenceNumber next)
        {
            ulong prevLsn = 0;
            ulong nextLsn = 0;
            record = null;
            recordSize = 0;

            if (this.smuggledLog.IsInvalid)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
            }

            ILog log = (ILog)(this.smuggledLog.Smuggle());
            try
            {
                CoTaskMemHandle pData = null;
                try
                {
                    log.ReadRecord(seqNumtoRead.High,
                                   out prevLsn,
                                   out nextLsn,
                                   out pData,
                                   out recordSize);

                    record = new byte[recordSize];

                    Marshal.Copy(pData.DangerousGetHandle(),
                                 record,
                                 0,
                                 recordSize);
                }
                catch (COMException exception)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ConvertKnownException(exception));
                }
                finally
                {
                    if (pData != null && !pData.IsInvalid) pData.Close();
                }
            }
            finally
            {
                SafeRelease(log);
            }

            previous = new SequenceNumber(prevLsn);
            next = new SequenceNumber(nextLsn);
        }

        internal void ReadRecordPrefix(SequenceNumber seqNumtoRead, out byte[] record, ref int cbData, out int recordSize, out SequenceNumber previous, out SequenceNumber next)
        {
            ulong prevLsn = 0;
            ulong nextLsn = 0;
            record = null;
            recordSize = 0;

            if (this.smuggledLog.IsInvalid)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
            }

            ILog log = (ILog)(this.smuggledLog.Smuggle());
            try
            {
                record = new byte[cbData];
                log.ReadRecordPrefix(seqNumtoRead.High,
                                out prevLsn,
                                out nextLsn,
                                record,
                                ref cbData,
                                out recordSize);

            }
            catch (COMException exception)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ConvertKnownException(exception));
            }
            finally
            {
                SafeRelease(log);
            }

            previous = new SequenceNumber(prevLsn);
            next = new SequenceNumber(nextLsn);
        }
    }
}
