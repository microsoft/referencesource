//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;


    // See the notes on LogLogRecordEnumerator for why this is
    // thread-safe: the same arguments apply here.
    //    
    class LogRestartAreaEnumerator : IEnumerator<LogRecord>
    {
        enum State
        {
            BeforeFirst,
            Valid,
            AfterLast,
            Disposed
        }

        LogRecordSequence recordSequence;

        State state;
        object syncRoot;
        SafeReadContext readContext;
        LogLogRecord current;

        internal LogRestartAreaEnumerator(LogRecordSequence recordSequence)
        {
            this.recordSequence = recordSequence;

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
                    return ReadRestartArea();
                }
                else
                {
                    if (this.current != null)
                        this.current.Detach();

                    return ReadPreviousRestartArea();
                }
            }
        }

        bool ReadRestartArea()
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
                ulong lsnRecord;

                if (!UnsafeNativeMethods.ReadLogRestartAreaSync(
                        this.recordSequence.MarshalContext,
                        out readBuffer,
                        out bufferLength,
                        out lsnRecord,
                        out this.readContext))
                {
                    this.state = State.AfterLast;
                    return false;
                }

                this.current = new LogLogRecord(
                    new SequenceNumber(lsnRecord),
                    SequenceNumber.Invalid,
                    SequenceNumber.Invalid,
                    readBuffer,
                    bufferLength);

                this.state = State.Valid;
                return true;
            }
        }

        bool ReadPreviousRestartArea()
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
                ulong lsnRecord;

                if (!UnsafeNativeMethods.ReadPreviousLogRestartAreaSync(
                        this.readContext,
                        out readBuffer,
                        out bufferLength,
                        out lsnRecord))
                {
                    this.state = State.AfterLast;
                    return false;
                }

                this.current = new LogLogRecord(
                    new SequenceNumber(lsnRecord),
                    SequenceNumber.Invalid,
                    SequenceNumber.Invalid,
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
