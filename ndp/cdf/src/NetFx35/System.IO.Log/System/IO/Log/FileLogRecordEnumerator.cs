//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Collections;
    using System.Collections.Generic;

    internal class FileLogRecordEnumerable : IEnumerable<LogRecord>
    {
        LogRecordEnumeratorType logRecordEnum;
        SimpleFileLog log;
        SequenceNumber start;
        bool enumRestartAreas;

        internal FileLogRecordEnumerable(
            SimpleFileLog log, 
            SequenceNumber start, 
            LogRecordEnumeratorType logRecordEnum,
            bool enumRestartAreas)
        {
            this.log = log;
            this.start = start;
            this.logRecordEnum = logRecordEnum;
            this.enumRestartAreas = enumRestartAreas;
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }
        
        public IEnumerator<LogRecord> GetEnumerator()
        {
            return new FileLogRecordEnumerator(
                                        log, 
                                        start, 
                                        logRecordEnum, 
                                        enumRestartAreas);
        }
    }

    internal class FileLogRecordEnumerator : IEnumerator<LogRecord>
    {
        FileLogRecordStream stream = null;
        FileLogRecord record = null;
        bool enumStarted = false;
        SequenceNumber start;
        SequenceNumber current;
        LogRecordEnumeratorType logRecordEnum;
        SimpleFileLog log;
        bool disposed = false;
        bool enumRestartAreas;

        internal FileLogRecordEnumerator(
            SimpleFileLog log,
            SequenceNumber start,
            LogRecordEnumeratorType logRecordEnum,
            bool enumRestartAreas)
        {
            this.log = log;
            this.start = start;
            this.current = start;
            this.logRecordEnum = logRecordEnum;
            this.enumRestartAreas = enumRestartAreas;
        }

        object IEnumerator.Current
        {
            get 
            {
                return this.Current;
            }
        }
        
        public LogRecord Current
        {
            get 
            {
                if (this.disposed)
#pragma warning suppress 56503
                     throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());

// IEnumerable interface contract for "current" member can throw InvalidOperationException. Suppressing this warning.  

                if (!this.enumStarted)
#pragma warning suppress 56503
                   throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.EnumNotStarted());

                if (this.record == null)
#pragma warning suppress 56503
                     throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.EnumEnded());

                return this.record;
            }
        }


        public bool MoveNext()
        {
            if (this.disposed)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());

            if (this.current == SequenceNumber.Invalid)
                return false;
            
            if (!this.enumStarted)
            {
                this.enumStarted = true;
            }
            else
            {
                switch (this.logRecordEnum)
                {
                    case LogRecordEnumeratorType.Next:
                        this.current = this.stream.NextLsn;
                        break;
                    case LogRecordEnumeratorType.Previous:
                        this.current = this.stream.Header.PreviousLsn;
                        break;
                    case LogRecordEnumeratorType.User:
                        this.current = this.stream.Header.NextUndoLsn;
                        break;
                }
            }

            SequenceNumber first;
            SequenceNumber last;
            log.GetLogLimits(out first, out last);
            if (this.current < first 
                || last < this.current
                || this.current == SequenceNumber.Invalid)
            {
                this.record = null;
                return false;
            }

            this.stream = new FileLogRecordStream(this.log, this.current);

            if (!this.enumRestartAreas && this.stream.Header.IsRestartArea)
            {
                if (this.logRecordEnum == LogRecordEnumeratorType.Next)
                {
                    // Move to the next record after restart area.
                    return MoveNext();
                }
                else
                {
                    // We have hit a restart area.  
                    // Restart areas have special values for prev and next undo in the header.
                    // Cannot enumerate further.
                    this.record = null;
                    return false;
                }
            }

            this.record = new FileLogRecord(this.stream);
            return true;
        }

        public void Reset()
        {
            if (this.disposed)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());

            this.enumStarted = false;
            this.current = this.start;
            this.record = null;
        }

        public void Dispose()
        {
            this.disposed = true;
        }
    }
}
