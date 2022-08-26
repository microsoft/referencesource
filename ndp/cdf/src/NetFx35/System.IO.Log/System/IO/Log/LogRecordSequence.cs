//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security.AccessControl;
    using System.Security.Permissions;
    using System.Threading;


    public sealed class LogRecordSequence : IRecordSequence
    {
        const uint INFINITE = 0xFFFFFFFF;

        const int DefaultWriteBufferCount = 10;
        const int DefaultReadBufferCount = unchecked((int)INFINITE);
        const int DefaultBufferSize = 64 * 1024;

        // This is just a common-sense buffer size, to prevent
        // accidental integer overflow and such.
        //
        const int MaxBufferSize = 0x40000000;

        const int CLFS_SECTOR_SIZE = 0x00000200;
        const int CLFS_SECTOR_ALIGN = (CLFS_SECTOR_SIZE - 1);

        LogStore store;
        int bufferSize;
        int writeBufferCount;
        bool ownStore;
        bool retryAppend;

        SafeMarshalContext marshalContext;

        // The LogStore constructor will demand full trust, so we don't have to.
        public LogRecordSequence(
            string path,
            FileMode mode)
        {
            this.store = new LogStore(path, mode);
            this.ownStore = true;

            this.bufferSize = GetBufferSize(DefaultBufferSize);
            this.writeBufferCount = DefaultWriteBufferCount;
        }

        // The LogStore constructor will demand full trust, so we don't have to.
        public LogRecordSequence(
            string path,
            FileMode mode,
            FileAccess access)
        {
            this.store = new LogStore(path, mode, access);
            this.ownStore = true;

            this.bufferSize = GetBufferSize(DefaultBufferSize);
            this.writeBufferCount = DefaultWriteBufferCount;
        }

        // The LogStore constructor will demand full trust, so we don't have to.
        public LogRecordSequence(
            string path,
            FileMode mode,
            FileAccess access,
            FileShare share)
        {
            this.store = new LogStore(path, mode, access, share);
            this.ownStore = true;

            this.bufferSize = GetBufferSize(DefaultBufferSize);
            this.writeBufferCount = DefaultWriteBufferCount;
        }

        // The LogStore constructor will demand full trust, so we don't have to.
        public LogRecordSequence(
            string path,
            FileMode mode,
            FileAccess access,
            FileShare share,
            int bufferSize,
            int bufferCount)
        {
            if (bufferSize <= 0)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("bufferSize"));
            if (bufferCount <= 0)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("bufferCount"));

            this.store = new LogStore(path, mode, access, share);
            this.ownStore = true;

            this.bufferSize = GetBufferSize(bufferSize);
            this.writeBufferCount = bufferCount;
        }

        // The LogStore constructor will demand full trust, so we don't have to.
        public LogRecordSequence(
            string path,
            FileMode mode,
            FileAccess access,
            FileShare share,
            int bufferSize,
            int bufferCount,
            FileSecurity fileSecurity)
        {
            if (bufferSize <= 0)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("bufferSize"));
            if (bufferCount <= 0)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("bufferCount"));

            this.store = new LogStore(path,
                                      mode,
                                      access,
                                      share,
                                      fileSecurity);
            this.ownStore = true;

            this.bufferSize = GetBufferSize(bufferSize);
            this.writeBufferCount = bufferCount;
        }

        [PermissionSetAttribute(SecurityAction.Demand, Unrestricted = true)]
        public LogRecordSequence(LogStore logStore)
        {
            if (logStore == null)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("logStore"));

            this.store = logStore;
            this.ownStore = false;

            this.bufferSize = GetBufferSize(DefaultBufferSize);
            this.writeBufferCount = DefaultWriteBufferCount;
        }

        [PermissionSetAttribute(SecurityAction.Demand, Unrestricted = true)]
        public LogRecordSequence(
            LogStore logStore,
            int bufferSize,
            int bufferCount)
        {
            if (logStore == null)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("logStore"));

            if (bufferSize <= 0)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("bufferSize"));
            if (bufferCount <= 0)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("bufferCount"));

            this.store = logStore;
            this.ownStore = false;

            this.bufferSize = GetBufferSize(bufferSize);
            this.writeBufferCount = bufferCount;
        }

        public SequenceNumber BaseSequenceNumber
        {
            get
            {
                return this.store.BaseSequenceNumber;
            }
        }

        public SequenceNumber LastSequenceNumber
        {
            get
            {
                return this.store.LastSequenceNumber;
            }
        }

        public LogStore LogStore
        {
            get
            {
                return this.store;
            }
        }

        internal SafeMarshalContext MarshalContext
        {
            get
            {
                EnsureMarshalContext();
                return this.marshalContext;
            }
        }

        internal SafeMarshalContext InternalMarshalContext
        {
            get
            {
                return this.marshalContext;
            }
        }

        public long MaximumRecordLength
        {
            get
            {
                return this.bufferSize - LogLogRecordHeader.Size;
            }
        }

        public long ReservedBytes
        {
            get
            {
                CLFS_INFORMATION info;
                this.store.GetLogFileInformation(out info);

                return checked((long)(info.TotalUndoCommitment));
            }
        }

        public bool RetryAppend
        {
            get
            {
                return this.retryAppend;
            }

            set
            {
                this.retryAppend = value;
            }
        }

        public SequenceNumber RestartSequenceNumber
        {
            get
            {
                CLFS_INFORMATION info;
                this.store.GetLogFileInformation(out info);

                if (info.RestartLsn == Const.CLFS_LSN_INVALID)
                    return SequenceNumber.Invalid;

                return new SequenceNumber(info.RestartLsn);
            }
        }

        public event EventHandler<TailPinnedEventArgs> TailPinned
        {
            add
            {
                this.store.LogManagement.TailPinned += value;
            }

            remove
            {
                this.store.LogManagement.TailPinned -= value;
            }
        }

        public void AdvanceBaseSequenceNumber(
            SequenceNumber newBaseSequenceNumber)
        {
            this.store.ValidateSequenceNumber(
                ref newBaseSequenceNumber,
                SequenceNumberConstraint.Arbitrary,
                "newBaseSequenceNumber");

            ulong newBaseLsn = newBaseSequenceNumber.High;

            EnsureMarshalContext();

            UnsafeNativeMethods.AdvanceLogBaseSync(this.marshalContext,
                                                   ref newBaseLsn,
                                                   0);
        }

        public SequenceNumber Append(
            ArraySegment<byte> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions)
        {
            return Append(new ArraySegment<byte>[] { data },
                          nextUndoRecord,
                          previousRecord,
                          recordAppendOptions);
        }


        public SequenceNumber Append(
            ArraySegment<byte> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservations)
        {
            return Append(new ArraySegment<byte>[] { data },
                          nextUndoRecord,
                          previousRecord,
                          recordAppendOptions,
                          reservations);
        }

        public SequenceNumber Append(
            IList<ArraySegment<byte>> data,
            SequenceNumber userRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions)
        {
            return Append(
                data,
                userRecord,
                previousRecord,
                recordAppendOptions,
                null);
        }

        public SequenceNumber Append(
            IList<ArraySegment<byte>> data,
            SequenceNumber userRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservations)
        {
            long totalRecordSize;
            LogReservationCollection lrc;

            if ((this.store.Access & FileAccess.Write) == 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotSupported(SR.NotSupported_ReadOnly));
            }

            if (recordAppendOptions > (RecordAppendOptions.ForceAppend | RecordAppendOptions.ForceFlush) ||
               recordAppendOptions < RecordAppendOptions.None)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("recordAppendOptions"));
            }

            this.store.ValidateSequenceNumber(
                ref userRecord,
                SequenceNumberConstraint.Arbitrary,
                "userRecord");
            this.store.ValidateSequenceNumber(
                ref previousRecord,
                SequenceNumberConstraint.Arbitrary,
                "previousRecord");

            totalRecordSize = ValidateData(data);
            lrc = ValidateReservationCollection(reservations);

            EnsureMarshalContext();

            LogReserveAndAppendState state = new LogReserveAndAppendState();

            state.RecordSequence = this;
            state.Data = data;
            state.TotalRecordSize = totalRecordSize;
            state.UserLsn = userRecord.High;
            state.PreviousLsn = previousRecord.High;
            state.RecordAppendOptions = recordAppendOptions;
            state.ReservationCollection = lrc;

            state.Start();

            return new SequenceNumber(state.ResultLsn);
        }

        public IAsyncResult BeginAppend(
            ArraySegment<byte> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            AsyncCallback callback,
            object state)
        {
            return BeginAppend(new ArraySegment<byte>[] { data },
                               nextUndoRecord,
                               previousRecord,
                               recordAppendOptions,
                               callback,
                               state);
        }

        public IAsyncResult BeginAppend(
            ArraySegment<byte> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservations,
            AsyncCallback callback,
            object state)
        {
            return BeginAppend(new ArraySegment<byte>[] { data },
                               nextUndoRecord,
                               previousRecord,
                               recordAppendOptions,
                               reservations,
                               callback,
                               state);
        }

        public IAsyncResult BeginAppend(
            IList<ArraySegment<byte>> data,
            SequenceNumber userRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            AsyncCallback callback,
            object state)
        {
            return BeginAppend(
                data,
                userRecord,
                previousRecord,
                recordAppendOptions,
                null,
                callback,
                state);
        }

        public IAsyncResult BeginAppend(
            IList<ArraySegment<byte>> data,
            SequenceNumber userRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservations,
            AsyncCallback callback,
            object state)
        {
            long totalRecordSize;
            LogReservationCollection lrc;

            if ((this.store.Access & FileAccess.Write) == 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotSupported(SR.NotSupported_ReadOnly));
            }

            if (recordAppendOptions > (RecordAppendOptions.ForceAppend | RecordAppendOptions.ForceFlush) ||
               recordAppendOptions < RecordAppendOptions.None)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("recordAppendOptions"));
            }

            this.store.ValidateSequenceNumber(
                ref userRecord,
                SequenceNumberConstraint.Arbitrary,
                "userRecord");
            this.store.ValidateSequenceNumber(
                ref previousRecord,
                SequenceNumberConstraint.Arbitrary,
                "previousRecord");

            totalRecordSize = ValidateData(data);
            lrc = ValidateReservationCollection(reservations);

            EnsureMarshalContext();

            LogAppendAsyncResult result = new LogAppendAsyncResult(this,
                                                                   callback,
                                                                   state);
            result.Data = data;
            result.TotalRecordSize = totalRecordSize;
            result.UserLsn = userRecord.High;
            result.PreviousLsn = previousRecord.High;
            result.RecordAppendOptions = recordAppendOptions;
            result.ReservationCollection = lrc;

            result.Start();

            return result;
        }

        public IAsyncResult BeginFlush(
            SequenceNumber sequenceNumber,
            AsyncCallback callback,
            object state)
        {
            this.store.ValidateSequenceNumber(
                ref sequenceNumber,
                SequenceNumberConstraint.Arbitrary,
                "sequenceNumber");

            EnsureMarshalContext();

            LogFlushAsyncResult flushResult = new LogFlushAsyncResult(this,
                                                                      callback,
                                                                      state);
            if (sequenceNumber == SequenceNumber.Invalid)
            {
                flushResult.SequenceNumber = 0;
            }
            else
            {
                flushResult.SequenceNumber = sequenceNumber.High;
            }

            flushResult.Start();
            return flushResult;
        }

        public IAsyncResult BeginReserveAndAppend(
            ArraySegment<byte> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservationCollection,
            long[] reservations,
            AsyncCallback callback,
            object state)
        {
            return BeginReserveAndAppend(new ArraySegment<byte>[] { data },
                                         nextUndoRecord,
                                         previousRecord,
                                         recordAppendOptions,
                                         reservationCollection,
                                         reservations,
                                         callback,
                                         state);
        }

        public IAsyncResult BeginReserveAndAppend(
            IList<ArraySegment<byte>> data,
            SequenceNumber userRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservationCollection,
            long[] reservations,
            AsyncCallback callback,
            object state)
        {
            long totalRecordSize;
            LogReservationCollection lrc;

            this.store.ValidateSequenceNumber(
                ref userRecord,
                SequenceNumberConstraint.Arbitrary,
                "userRecord");
            this.store.ValidateSequenceNumber(
                ref previousRecord,
                SequenceNumberConstraint.Arbitrary,
                "previousRecord");

            if (reservationCollection == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("reservationCollection"));
            }
            if (reservations == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("reservations"));
            }
            ValidateReservations(reservations);

            totalRecordSize = ValidateData(data);
            lrc = ValidateReservationCollection(reservationCollection);

            EnsureMarshalContext();

            LogAppendAsyncResult result = new LogAppendAsyncResult(this,
                                                                   callback,
                                                                   state);
            result.Data = data;
            result.TotalRecordSize = totalRecordSize;
            result.UserLsn = userRecord.High;
            result.PreviousLsn = previousRecord.High;
            result.RecordAppendOptions = recordAppendOptions;
            result.ReservationCollection = lrc;
            result.Reservations = reservations;

            result.Start();

            return result;
        }

        public IAsyncResult BeginWriteRestartArea(
            ArraySegment<byte> data,
            SequenceNumber newBaseSeqNum,
            ReservationCollection reservation,
            AsyncCallback callback,
            object state)
        {
            return BeginWriteRestartArea(new ArraySegment<byte>[] { data },
                                         newBaseSeqNum,
                                         reservation,
                                         callback,
                                         state);
        }

        public IAsyncResult BeginWriteRestartArea(
            IList<ArraySegment<byte>> data,
            SequenceNumber newBaseSeqNum,
            ReservationCollection reservationCollection,
            AsyncCallback callback,
            object state)
        {
            long totalRecordSize;
            LogReservationCollection lrc;

            this.store.ValidateSequenceNumber(
                ref newBaseSeqNum,
                SequenceNumberConstraint.Arbitrary,
                "newBaseSeqNum");

            totalRecordSize = ValidateData(data);
            lrc = ValidateReservationCollection(reservationCollection);

            EnsureMarshalContext();

            LogWriteRestartAreaAsyncResult result;
            result = new LogWriteRestartAreaAsyncResult(this,
                                                        callback,
                                                        state);
            result.Data = data;
            result.TotalRecordSize = totalRecordSize;
            result.NewBaseLsn = newBaseSeqNum.High;
            result.ReservationCollection = lrc;

            result.Start();

            return result;
        }

        public ReservationCollection CreateReservationCollection()
        {
            return new LogReservationCollection(this);
        }

        public void Dispose()
        {
            if ((this.marshalContext != null) &&
                (!this.marshalContext.IsInvalid))
            {
                this.marshalContext.Close();
            }

            if (this.ownStore)
                this.store.Dispose();
        }

        public SequenceNumber EndAppend(
            IAsyncResult result)
        {
            LogAppendAsyncResult lar = result as LogAppendAsyncResult;
            if (lar == null)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.InvalidAsyncResult());

            // Having some reservations to make is a requirement of
            // calling BeginReserveAndAppend, so if we have some, we
            // know this is not our AsyncResult.
            //
            if (lar.Reservations != null)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.InvalidAsyncResult());

            lar.End();

            return new SequenceNumber(lar.ResultLsn);
        }

        public SequenceNumber EndFlush(
            IAsyncResult result)
        {
            LogFlushAsyncResult lar = result as LogFlushAsyncResult;
            if (lar == null)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.InvalidAsyncResult());

            lar.End();

            return new SequenceNumber(lar.ResultLsn);
        }

        public SequenceNumber EndReserveAndAppend(
            IAsyncResult result)
        {
            LogAppendAsyncResult lar = result as LogAppendAsyncResult;
            if (lar == null)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.InvalidAsyncResult());

            // Having some reservations to make is a requirement of
            // calling BeginReserveAndAppend, so if we have none, we
            // know this is an AsyncResult for some OTHER method.
            //
            if (lar.Reservations == null)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.InvalidAsyncResult());

            lar.End();

            return new SequenceNumber(lar.ResultLsn);
        }

        public SequenceNumber EndWriteRestartArea(
            IAsyncResult result)
        {
            LogWriteRestartAreaAsyncResult lar = result as LogWriteRestartAreaAsyncResult;
            if (lar == null)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.InvalidAsyncResult());

            lar.End();

            return new SequenceNumber(lar.ResultLsn);
        }

        public SequenceNumber Flush()
        {
            return Flush(SequenceNumber.Invalid);
        }

        public SequenceNumber Flush(SequenceNumber sequenceNumber)
        {
            this.store.ValidateSequenceNumber(
                ref sequenceNumber,
                SequenceNumberConstraint.Arbitrary,
                "sequenceNumber");

            ulong resultLsn;
            ulong lsn;

            if (sequenceNumber == SequenceNumber.Invalid)
            {
                lsn = 0;
            }
            else
            {
                lsn = sequenceNumber.High;
            }

            EnsureMarshalContext();
            UnsafeNativeMethods.FlushLogToLsnSync(this.marshalContext,
                                                  ref lsn,
                                                  out resultLsn);

            return new SequenceNumber(resultLsn);
        }

        public IEnumerable<LogRecord> ReadLogRecords(
            SequenceNumber start,
            LogRecordEnumeratorType logRecordEnum)
        {
            this.store.ValidateSequenceNumber(
                ref start,
                SequenceNumberConstraint.Arbitrary,
                "start");

            CLFS_CONTEXT_MODE mode;
            switch (logRecordEnum)
            {
                case LogRecordEnumeratorType.User:
                    mode = CLFS_CONTEXT_MODE.ClfsContextUndoNext;
                    break;

                case LogRecordEnumeratorType.Previous:
                    mode = CLFS_CONTEXT_MODE.ClfsContextPrevious;
                    break;

                case LogRecordEnumeratorType.Next:
                    mode = CLFS_CONTEXT_MODE.ClfsContextForward;
                    break;

                default:
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("logRecordEnum"));
            }

            return new RecordEnumerable(this, mode, start.High);
        }

        public IEnumerable<LogRecord> ReadRestartAreas()
        {
            return new RestartAreaEnumerable(this);
        }

        public SequenceNumber ReserveAndAppend(
            ArraySegment<byte> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservationCollection,
            params long[] reservations)
        {
            return ReserveAndAppend(new ArraySegment<byte>[] { data },
                                    nextUndoRecord,
                                    previousRecord,
                                    recordAppendOptions,
                                    reservationCollection,
                                    reservations);
        }

        public SequenceNumber ReserveAndAppend(
            IList<ArraySegment<byte>> data,
            SequenceNumber userRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservationCollection,
            params long[] reservations)
        {
            long totalRecordSize;
            LogReservationCollection lrc;

            this.store.ValidateSequenceNumber(
                ref userRecord,
                SequenceNumberConstraint.Arbitrary,
                "userRecord");
            this.store.ValidateSequenceNumber(
                ref previousRecord,
                SequenceNumberConstraint.Arbitrary,
                "previousRecord");

            totalRecordSize = ValidateData(data);
            lrc = ValidateReservationCollection(reservationCollection);
            if (lrc == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("reservationCollection"));
            }
            if (reservations == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("reservations"));
            }
            ValidateReservations(reservations);

            EnsureMarshalContext();

            LogReserveAndAppendState state = new LogReserveAndAppendState();

            state.RecordSequence = this;
            state.Data = data;
            state.TotalRecordSize = totalRecordSize;
            state.UserLsn = userRecord.High;
            state.PreviousLsn = previousRecord.High;
            state.RecordAppendOptions = recordAppendOptions;
            state.ReservationCollection = lrc;
            state.Reservations = reservations;

            state.Start();

            return new SequenceNumber(state.ResultLsn);
        }

        public void SetLastRecord(
            SequenceNumber sequenceNumber)
        {
            this.store.ValidateSequenceNumber(
                ref sequenceNumber,
                SequenceNumberConstraint.Arbitrary,
                "sequenceNumber");

            ulong lsn = sequenceNumber.High;

            try
            {
                EnsureMarshalContext();
                UnsafeNativeMethods.TruncateLogSync(this.marshalContext,
                                                                        ref lsn);
            }
            catch (EntryPointNotFoundException)
            {
                // CLFS API for SetEndOfLog doesn;t update the client marshaling area and directly
                // uses the log handle. The client marshalling and kernal stats becomes out of sync 
                // and will cause subsequent appends to fail.
                // Work around - Invalidate the client marshall area. Call SetEndOfLog.
                // Subsequent Append calls will create a new marshalling context.

                InvalidateMarshalContext();
                UnsafeNativeMethods.SetEndOfLogSync(this.store.Handle,
                                                    ref lsn);
            }
        }

        public SequenceNumber WriteRestartArea(
            ArraySegment<byte> data)
        {
            return WriteRestartArea(new ArraySegment<byte>[] { data });
        }

        public SequenceNumber WriteRestartArea(
            IList<ArraySegment<byte>> data)
        {
            return WriteRestartArea(data, SequenceNumber.Invalid);
        }

        public SequenceNumber WriteRestartArea(
            ArraySegment<byte> data,
            SequenceNumber newBaseSeqNum)
        {
            return WriteRestartArea(new ArraySegment<byte>[] { data },
                                    newBaseSeqNum);
        }

        public SequenceNumber WriteRestartArea(
            IList<ArraySegment<byte>> data,
            SequenceNumber newBaseSeqNum)
        {
            return WriteRestartArea(data, newBaseSeqNum, null);
        }

        public SequenceNumber WriteRestartArea(
            ArraySegment<byte> data,
            SequenceNumber newBaseSeqNum,
            ReservationCollection reservations)
        {
            return WriteRestartArea(new ArraySegment<byte>[] { data },
                                    newBaseSeqNum,
                                    reservations);
        }

        public SequenceNumber WriteRestartArea(
            IList<ArraySegment<byte>> data,
            SequenceNumber newBaseSeqNum,
            ReservationCollection reservationCollection)
        {
            long totalRecordSize;
            LogReservationCollection lrc;

            this.store.ValidateSequenceNumber(
                ref newBaseSeqNum,
                SequenceNumberConstraint.Arbitrary,
                "newBaseSeqNum");

            totalRecordSize = ValidateData(data);
            lrc = ValidateReservationCollection(reservationCollection);

            EnsureMarshalContext();

            LogWriteRestartAreaState state = new LogWriteRestartAreaState();

            state.RecordSequence = this;
            state.Data = data;
            state.TotalRecordSize = totalRecordSize;
            state.ReservationCollection = lrc;
            state.NewBaseLsn = newBaseSeqNum.High;

            state.Start();

            return new SequenceNumber(state.ResultLsn);
        }

        void EnsureMarshalContext()
        {
            // NOTE: Simple synchronization here may cause problems. I
            //       am counting on the unfairness of the lock object to
            //       make this fast enough.
            //
            lock (this.store.SyncRoot)
            {
                if ((this.marshalContext != null) &&
                    (!this.marshalContext.IsInvalid))
                {
                    return;
                }

                try
                {
                    this.marshalContext =
                        UnsafeNativeMethods.CreateLogMarshallingArea(
                            this.store.Handle,
                            IntPtr.Zero,
                            IntPtr.Zero,
                            IntPtr.Zero,
                            this.bufferSize,
                            this.writeBufferCount,
                            DefaultReadBufferCount);
                }
                catch (InvalidOperationException)
                {
                    if (this.store.Extents.Count == 0)
                    {
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(
                                        Error.InvalidOperation(SR.InvalidOperation_MustHaveExtents));
                    }
                    else
                    {
                        throw;
                    }
                }
            }
        }

        void InvalidateMarshalContext()
        {
            lock (this.store.SyncRoot)
            {
                if (this.marshalContext != null)
                {
                    this.marshalContext.Close();
                    this.marshalContext = null;
                }
            }
        }

        int GetBufferSize(int bufferSize)
        {
            if (bufferSize > MaxBufferSize)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("bufferSize"));
            }

            bufferSize += LogLogRecordHeader.Size;
            bufferSize = (bufferSize + CLFS_SECTOR_ALIGN) & ~CLFS_SECTOR_ALIGN;

            return bufferSize;
        }

        long ValidateData(IList<ArraySegment<byte>> data)
        {
            if (data == null)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("data"));

            long dataLength = 0;
            for (int i = 0; i < data.Count; i++)
            {
                ArraySegment<byte> segment = data[i];

                dataLength = checked(dataLength + segment.Count);
                if (dataLength > MaximumRecordLength)
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.LogRecSeq_Append_TooBig));
            }

            return dataLength + LogLogRecordHeader.Size;
        }

        LogReservationCollection ValidateReservationCollection(
            ReservationCollection reservations)
        {
            LogReservationCollection lrc = null;
            if (reservations != null)
            {
                lrc = reservations as LogReservationCollection;
                if (lrc == null)
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(
                                        Error.ArgumentInvalid(SR.LogRecSeq_InvalidReservationCollection));

                if (lrc.RecordSequence != this)
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(
                                        Error.ArgumentInvalid(SR.LogRecSeq_InvalidReservationCollection));
            }

            return lrc;
        }

        void ValidateReservations(long[] reservations)
        {
            foreach (long reservation in reservations)
            {
                if ((reservation < 0) || (reservation > MaximumRecordLength))
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("reservations"));
            }
        }

        class RecordEnumerable : IEnumerable<LogRecord>
        {
            LogRecordSequence recordSequence;
            CLFS_CONTEXT_MODE mode;
            ulong startLsn;

            internal RecordEnumerable(LogRecordSequence recordSequence,
                                      CLFS_CONTEXT_MODE mode,
                                      ulong startLsn)
            {
                this.recordSequence = recordSequence;
                this.mode = mode;
                this.startLsn = startLsn;
            }

            IEnumerator IEnumerable.GetEnumerator()
            {
                return this.GetEnumerator();
            }

            public IEnumerator<LogRecord> GetEnumerator()
            {
                return new LogLogRecordEnumerator(this.recordSequence,
                                                  this.mode,
                                                  this.startLsn);
            }
        }

        class RestartAreaEnumerable : IEnumerable<LogRecord>
        {
            LogRecordSequence recordSequence;

            internal RestartAreaEnumerable(LogRecordSequence recordSequence)
            {
                this.recordSequence = recordSequence;
            }

            IEnumerator IEnumerable.GetEnumerator()
            {
                return this.GetEnumerator();
            }

            public IEnumerator<LogRecord> GetEnumerator()
            {
                return new LogRestartAreaEnumerator(this.recordSequence);
            }
        }
    }
}
