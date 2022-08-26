//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;


    // Although we would normally not make an enumerator like this
    // thread-safe, we do so because we are dealing in unmanaged
    // memory. Specifically, we construct an UnmanagedMemoryStream
    // around a pointer that comes back from ReadLogRecord and
    // ReadNextLogRecord (in LogLogRecord). This is fine, so long as
    // we know that memory is valid. That memory is valid as long as
    // the read context is valid. When the read context moves on, or
    // becomes invalid, we must "detach" the current record, copying
    // the data into a managed array.
    //
    // Of necessity, we allocate the read context before the record,
    // and detach the record before freeing or moving the read
    // context. What if some malicious person engineered a ----
    // condition between the allocate/move and the detach? In this
    // case, a log record might wind up un-detached, but pointing at
    // invalid memory. This is NOT GOOD. Hence the thread-safety.
    //    
    class LogLogRecordEnumerator : IEnumerator<LogRecord>
    {
        enum State
        {
            BeforeFirst,
            Valid,
            AfterLast,
            Disposed
        }

        LogRecordSequence recordSequence;
        CLFS_CONTEXT_MODE mode;
        ulong startLsn;

        State state;
        object syncRoot;
        SafeReadContext readContext;
        LogLogRecord current;

        internal LogLogRecordEnumerator(LogRecordSequence recordSequence,
                                        CLFS_CONTEXT_MODE mode,
                                        ulong startLsn)
        {
            this.recordSequence = recordSequence;
            this.mode = mode;
            this.startLsn = startLsn;

            this.syncRoot = new object();
            this.state = State.BeforeFirst;
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
                lock (this.syncRoot)
                {
                    if (this.state == State.Disposed)
#pragma warning suppress 56503
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());

                    // IEnumerable interface contract for "current" member can throw InvalidOperationException. Suppressing this warning.  

                    if (this.state == State.BeforeFirst)
#pragma warning suppress 56503
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.EnumNotStarted());
                    if (this.state == State.AfterLast)
#pragma warning suppress 56503
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.EnumEnded());

                    return this.current;
                }
            }
        }

        public void Dispose()
        {
            lock (this.syncRoot)
            {
                if (this.current != null)
                {
                    this.current.Detach();
                    this.current = null;
                }

                if ((this.readContext != null) &&
                    (!this.readContext.IsInvalid))
                {
                    this.readContext.Close();
                }

                this.state = State.Disposed;
            }
        }

        public bool MoveNext()
        {
            lock (this.syncRoot)
            {
                if (this.state == State.Disposed)
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
                if (this.state == State.AfterLast)
                    return false;

                if (this.readContext == null)
                {
                    return ReadLogRecord();
                }
                else
                {
                    if (this.current != null)
                        this.current.Detach();

                    return ReadNextLogRecord();
                }
            }
        }

        bool ReadLogRecord()
        {
            if (!((this.readContext == null || this.readContext.IsInvalid) && (this.current == null)))
            {
                // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                // log records, we failfast the process.
                DiagnosticUtility.FailFast("Should only call this for first record!");
            }
            unsafe
            {
                byte* readBuffer;
                int bufferLength;
                byte recordType;
                ulong lsnUser;
                ulong lsnPrevious;

                if (!UnsafeNativeMethods.ReadLogRecordSync(
                        this.recordSequence.MarshalContext,
                        ref this.startLsn,
                        this.mode,
                        out readBuffer,
                        out bufferLength,
                        out recordType,
                        out lsnUser,
                        out lsnPrevious,
                        out this.readContext))
                {
                    this.state = State.AfterLast;
                    return false;
                }

                if ((recordType & Const.ClfsDataRecord) != 0)
                {
                    this.current = new LogLogRecord(
                        new SequenceNumber(this.startLsn),
                        new SequenceNumber(lsnUser),
                        new SequenceNumber(lsnPrevious),
                        readBuffer,
                        bufferLength);

                    this.state = State.Valid;
                    return true;
                }
                else
                {
                    return ReadNextLogRecord();
                }
            }
        }

        bool ReadNextLogRecord()
        {
            if (this.readContext == null || this.readContext.IsInvalid)
            {
                // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                // log records, we failfast the process.
                DiagnosticUtility.FailFast("Should only be called for records after the first!");
            }
            unsafe
            {
                byte* readBuffer;
                int bufferLength;
                byte recordType = Const.ClfsDataRecord;
                ulong lsnUser;
                ulong lsnPrevious;
                ulong lsnRecord;

                if (!UnsafeNativeMethods.ReadNextLogRecordSync(
                        this.readContext,
                        out readBuffer,
                        out bufferLength,
                        ref recordType,
                        out lsnUser,
                        out lsnPrevious,
                        out lsnRecord))
                {
                    this.state = State.AfterLast;
                    return false;
                }

                this.current = new LogLogRecord(
                    new SequenceNumber(lsnRecord),
                    new SequenceNumber(lsnUser),
                    new SequenceNumber(lsnPrevious),
                    readBuffer,
                    bufferLength);

                this.state = State.Valid;
                return true;
            }
        }

        public void Reset()
        {
            lock (this.syncRoot)
            {
                if (this.state == State.Disposed)
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());

                if (this.current != null)
                {
                    this.current.Detach();
                    this.current = null;
                }

                if ((this.readContext != null) &&
                    (!this.readContext.IsInvalid))
                {
                    this.readContext.Close();
                    this.readContext = null;
                }

                this.state = State.BeforeFirst;
            }
        }
    }
}
