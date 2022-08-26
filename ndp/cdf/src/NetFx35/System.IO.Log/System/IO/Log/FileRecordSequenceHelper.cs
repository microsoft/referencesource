//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.IO.Log
{
    using System;
    using System.Runtime;
    using System.Threading;
    using System.Collections.Generic;

    // This class contains all the workarounds for simple file log.
    class FileRecordSequenceHelper
    {
        SimpleFileLog log;
        SequenceNumber newBaseSeqNum;
        SequenceNumber lastRestartArea;

        // We take a reader lock during append and a writer lock during WriteRestartArea and Truncate.
        ReaderWriterLock appendLock;

        // We need to handle truncate failure during Recover otherwise the user will not be able to open the log.
        // If truncate fails, all operations except 'Read' retry trucate.  
        bool truncateFailed;

        internal FileRecordSequenceHelper(SimpleFileLog log)
        {
            this.log = log;
            this.appendLock = new ReaderWriterLock();
            this.truncateFailed = false;
            this.lastRestartArea = SequenceNumber.Invalid;
            this.newBaseSeqNum = SequenceNumber.Invalid;

            Recover();
        }

        internal SequenceNumber BaseSequenceNumber
        {
            get
            {
                SequenceNumber first, last;
                this.log.GetLogLimits(out first, out last);
                return first;
            }
        }

        internal SequenceNumber LastSequenceNumber
        {
            get
            {
                SequenceNumber first, last;
                log.GetLogLimits(out first, out last);
                if (last != SequenceNumber.Invalid)
                {
                    if (last < first)
                    {
                        // Special condition - log empty.
                        // When the last sequence number returned by simple file log is less than the first, 
                        // the log is empty.
                        // For IO.Log OM, if log is empty then Base lsn == Last lsn. 
                        return new SequenceNumber(first.High, 0);
                    }
                    else
                    {
                        // The low part is 1 because the last sequence number should be greater than
                        // the sequence number of the last record in the log. 
                        // Last sequence number is a valid input only to WriteRestartArea.
                        return new SequenceNumber(last.High, 1);
                    }
                }
                else
                {
                    return last;
                }
            }
        }

        internal SequenceNumber RestartSequenceNumber
        {
            get { return this.lastRestartArea; }
        }

        // During WriteRestarArea, we truncate all records before the new base sequence number.
        // The new base sequence number is recorded in the restart-area record.
        // If truncate failed during WriteRestartArea, then the records before the new base seq number 
        // will still be present in the log.  During recovery, we will cleanup the log by removing these records.

        // Recovery steps -
        // Scan the log backwards
        // Stop when the last restart area is found
        // Truncate the log if needed

        private void Recover()
        {
            SequenceNumber first;
            SequenceNumber last;
            this.log.GetLogLimits(out first, out last);

            // Internal knowledge - if last < first, log is empty
            if (last < first)
                return;

            SequenceNumber sn = last;
            while (sn != SequenceNumber.Invalid && first <= sn && sn <= last)
            {
                FileLogRecordStream stream = new FileLogRecordStream(log, sn);
                if (stream.Header.IsRestartArea)
                {
                    this.lastRestartArea = stream.RecordSequenceNumber;

                    // if the base sequence number is different from
                    // the next undo lsn, then we crashed during or
                    // before truncate.  Perform the truncate now.
                    if (first < stream.Header.NextUndoLsn)
                    {

                        if (stream.Header.NextUndoLsn == SequenceNumber.Invalid)
                        {
                            // WriteRestartArea was called with LastSequenceNumber

                            if (first != stream.RecordSequenceNumber)
                            {
                                this.newBaseSeqNum = stream.RecordSequenceNumber;
                            }
                        }
                        else
                        {
                            this.newBaseSeqNum = stream.Header.NextUndoLsn;
                        }

                        // This method is called from the constructor.  So no need to take a write lock.
                        if (this.newBaseSeqNum != SequenceNumber.Invalid)
                        {
                            try
                            {
                                log.TruncatePrefix(this.newBaseSeqNum);
                                this.newBaseSeqNum = SequenceNumber.Invalid;
                            }
#pragma warning suppress 56500
                            catch (Exception exception)
                            {
                                // Truncate failed again.  We were unable to cleanup the log.  
                                this.truncateFailed = true;

                                if (Fx.IsFatal(exception)) throw;
                            }
                        }
                    }

                    break;
                }

                sn = stream.PrevLsn;
            }
        }

        internal void ValidateSequenceNumber(SequenceNumber sequenceNumber)
        {
            if (sequenceNumber == SequenceNumber.Invalid || sequenceNumber.Low != 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.SequenceNumberInvalid());
            }

            if (sequenceNumber < this.BaseSequenceNumber || sequenceNumber > this.LastSequenceNumber)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.SequenceNumberNotActive("sequenceNumber"));
            }
        }

        internal void AdvanceBaseSequeceNumber(SequenceNumber newBaseSequenceNumber)
        {
            TruncateIfNecessary();

            bool lockHeld = false;
            try
            {
                try { }
                finally
                {
                    appendLock.AcquireWriterLock(-1);
                    lockHeld = true;
                }
                ValidateSequenceNumber(newBaseSequenceNumber);
                this.log.TruncatePrefix(newBaseSequenceNumber);
            }
            finally
            {
                if (lockHeld)
                {
                    appendLock.ReleaseWriterLock();
                }
            }
        }

        internal SequenceNumber Append(IList<ArraySegment<byte>> data,
                                        SequenceNumber nextUndoRecord,
                                        SequenceNumber previousRecord,
                                        bool forceFlush)
        {
            TruncateIfNecessary();

            bool lockHeld = false;
            try
            {
                try { }
                finally
                {
                    appendLock.AcquireReaderLock(-1);
                    lockHeld = true;
                }
                using (AppendHelper helper = new AppendHelper(
                                                    data,
                                                    previousRecord,
                                                    nextUndoRecord,
                                                    false))
                {

                    return log.AppendRecord(helper.Blobs, forceFlush);
                }
            }
            finally
            {
                if (lockHeld)
                {
                    appendLock.ReleaseReaderLock();
                }
            }
        }

        internal SequenceNumber Flush(SequenceNumber sequenceNumber)
        {
            TruncateIfNecessary();

            if (sequenceNumber == SequenceNumber.Invalid)
            {
                // Re-interpret... SimpleFileLog uses 0 to mean "flush
                // entire log", not SequenceNumber.Invalid.
                //
                sequenceNumber = new SequenceNumber(0);
            }
            else
            {
                ValidateSequenceNumber(sequenceNumber);
            }

            this.log.Force(sequenceNumber);

            return sequenceNumber;
        }

        internal SequenceNumber WriteRestartAreaInternal(
            IList<ArraySegment<byte>> data,
            SequenceNumber newBaseSeqNum)
        {

            if (data == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("data"));
            }

            TruncateIfNecessary();

            bool lockHeld = false;
            try
            {
                try { }
                finally
                {
                    appendLock.AcquireWriterLock(-1);
                    lockHeld = true;
                }

                SequenceNumber sn;
                AppendHelper helper;
                bool lastSeqNum, firstSeqNum;

                lastSeqNum = newBaseSeqNum == this.LastSequenceNumber;
                firstSeqNum = newBaseSeqNum == this.BaseSequenceNumber;

                if (lastSeqNum)
                {
                    // We dont know the new base sequence number.
                    // It will be the sequence number we get from append.
                    helper = new AppendHelper(data,
                                            this.lastRestartArea,
                                            SequenceNumber.Invalid,
                                            true);
                }
                else
                {
                    if (!firstSeqNum)
                    {
                        ValidateSequenceNumber(newBaseSeqNum);

                        // Validate newBaseSequenceNumber by reading the corresponding record.
                        int cbData = 1;
                        byte[] record;
                        int recordSize;
                        SequenceNumber prev, next;

                        // Sequence number validation checks if the Seq Num > BSN and Seq Num < LSN. 
                        // Now we validate if Seq Number is a valid number in this sequence.
                        try
                        {
                            this.log.ReadRecordPrefix(newBaseSeqNum, out record, ref cbData, out recordSize, out prev, out next);
                        }
                        catch (ArgumentException exception)
                        {
                            throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange(SR.Argument_TailInvalid, exception));
                        }
                    }

                    helper = new AppendHelper(data,
                                            this.lastRestartArea,
                                            newBaseSeqNum,
                                            true);
                }

                // If there is a failure betweeen append and truncate,
                // then the log will be in an inconsistent state.  To
                // solve this, we write the new base sequence number in
                // the header.  During recovery from failure, we will find
                // the restart area and truncate the log.

                using (helper)
                {
                    // No need for a reader appendLock, since WriteRestartAreaInternal is under a lock.
                    sn = this.log.AppendRecord(helper.Blobs, true);
                }

                this.lastRestartArea = sn;

                if (firstSeqNum)
                {
                    return sn;
                }

                if (lastSeqNum)
                {
                    newBaseSeqNum = sn;
                }

                try
                {
                    log.TruncatePrefix(newBaseSeqNum);
                }
#pragma warning suppress 56500
                catch (Exception exception)
                {
                    this.newBaseSeqNum = newBaseSeqNum;
                    this.truncateFailed = true;

                    if (Fx.IsFatal(exception)) throw;
                }

                return sn;
            }
            finally
            {
                if (lockHeld)
                {
                    appendLock.ReleaseWriterLock();
                }
            }
        }

        private void TruncateIfNecessary()
        {
            // Retry trucate if it failed during Recover or WriteRestartArea.
            if (this.truncateFailed)
            {
                bool lockHeld = false;
                try
                {
                    try { }
                    finally
                    {
                        appendLock.AcquireWriterLock(-1);
                        lockHeld = true;
                    }
                    if (this.truncateFailed)
                    {
                        this.log.TruncatePrefix(this.newBaseSeqNum);
                        this.truncateFailed = false;
                    }
                }
                finally
                {
                    if (lockHeld)
                    {
                        appendLock.ReleaseWriterLock();
                    }
                }
            }
        }
    }
}
