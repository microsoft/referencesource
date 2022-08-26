//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.IO;
    using System.Security.AccessControl;
    using System.Runtime;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Diagnostics;
    using System.Threading;

    // FileRecordSequence uses Simple file log to write records to a file.
    public sealed class FileRecordSequence : IRecordSequence
    {
        SimpleFileLog log;
        bool retryAppend;
        object syncTailPinned = new object();
        bool tailPinnedCalled = false;
        FileAccess access;
        FileRecordSequenceHelper frsHelper;
        long reservedBytes;
        object syncReservedBytes = new object();
        int tailPinnedThreadID = -1;

        public FileRecordSequence(string path) : this(path, FileAccess.ReadWrite, 0) { }

        public FileRecordSequence(string path, FileAccess access) : this(path, access, 0) { }

        [PermissionSetAttribute(SecurityAction.Demand, Unrestricted = true)]
        public FileRecordSequence(string path, FileAccess access, int size)
        {
            if (size < 0)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("size"));

            this.access = access;
            this.log = new SimpleFileLog(Path.GetFullPath(path), size);
            this.frsHelper = new FileRecordSequenceHelper(this.log);
            this.reservedBytes = 0;
        }

        public SequenceNumber BaseSequenceNumber
        {
            get { return this.frsHelper.BaseSequenceNumber; }
        }

        public SequenceNumber LastSequenceNumber
        {
            get { return this.frsHelper.LastSequenceNumber; }
        }

        public long MaximumRecordLength
        {
            get
            {
                return Int32.MaxValue;
            }
        }

        public long ReservedBytes
        {
            get
            {
                return this.reservedBytes;
            }
        }


        public bool RetryAppend
        {
            get { return this.retryAppend; }
            set { this.retryAppend = value; }
        }


        public SequenceNumber RestartSequenceNumber
        {
            get { return this.frsHelper.RestartSequenceNumber; }
        }

        public event EventHandler<TailPinnedEventArgs> TailPinned;

        public void AdvanceBaseSequenceNumber(
            SequenceNumber newBaseSequenceNumber)
        {
            this.frsHelper.AdvanceBaseSequeceNumber(newBaseSequenceNumber);
        }

        internal void AddReservation(long reservation)
        {
            lock (this.syncReservedBytes)
            {
                this.reservedBytes += reservation;
            }
        }

        internal void RemoveReservation(long reservation)
        {
            lock (this.syncReservedBytes)
            {
                this.reservedBytes -= reservation;
            }
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
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions)
        {
            if (data == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("data"));
            }

            if ((this.access & FileAccess.Write) == 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotSupported(SR.NotSupported_ReadOnly));
            }

            if (recordAppendOptions > (RecordAppendOptions.ForceAppend | RecordAppendOptions.ForceFlush) ||
               recordAppendOptions < RecordAppendOptions.None)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("recordAppendOptions"));
            }

            SequenceNumber sn;
            bool forceFlush = (recordAppendOptions & RecordAppendOptions.ForceFlush) != 0;
            try
            {
                sn = frsHelper.Append(data, nextUndoRecord, previousRecord, forceFlush);
            }
            catch (SequenceFullException)
            {
                RaiseTailPinnedEvent();

                if (this.RetryAppend)
                {
                    sn = frsHelper.Append(data, nextUndoRecord, previousRecord, forceFlush);
                }
                else
                {
                    throw;
                }
            }

            return sn;
        }


        public SequenceNumber Append(
            IList<ArraySegment<byte>> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservations)
        {
            int size = 0;

            if (reservations == null)
            {
                return Append(data, nextUndoRecord, previousRecord, recordAppendOptions);
            }

            if (data == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("data"));
            }

            FileReservationCollection reservationCollection = reservations as FileReservationCollection;

            if (reservationCollection == null || !reservationCollection.IsMyCollection(this))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.LogRecSeq_InvalidReservationCollection));
            }

            for (int i = 0; i < data.Count; i++)
            {
                size += data[i].Count;
            }

            long reservation = reservationCollection.GetBestMatchingReservation(size);

            if (reservation < 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ReservationNotFound());
            }

            bool throwing = true;
            try
            {
                SequenceNumber returnValue = this.Append(data, nextUndoRecord, previousRecord, recordAppendOptions);
                throwing = false;
                return returnValue;
            }
            finally
            {
                if (throwing)
                {
                    reservationCollection.Add(reservation);
                }
            }
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
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            AsyncCallback callback,
            object state)
        {
            SequenceNumber result = Append(data, nextUndoRecord, previousRecord, recordAppendOptions);
            return new FileRecordSequenceCompletedAsyncResult(result, callback, state, Work.Append);
        }


        public IAsyncResult BeginAppend(
            IList<ArraySegment<byte>> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservations,
            AsyncCallback callback,
            object state)
        {
            SequenceNumber result = Append(data, nextUndoRecord, previousRecord, recordAppendOptions, reservations);
            return new FileRecordSequenceCompletedAsyncResult(result, callback, state, Work.Append);
        }

        public IAsyncResult BeginFlush(
            SequenceNumber sequenceNumber,
            AsyncCallback callback,
            object state)
        {
            SequenceNumber result = this.Flush(sequenceNumber);
            return new FileRecordSequenceCompletedAsyncResult(result, callback, state, Work.Flush);
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
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservationCollection,
            long[] reservations,
            AsyncCallback callback,
            object state)
        {
            SequenceNumber result = ReserveAndAppend(data,
                                                      nextUndoRecord,
                                                      previousRecord,
                                                      recordAppendOptions,
                                                      reservationCollection,
                                                      reservations);

            return new FileRecordSequenceCompletedAsyncResult(result, callback, state, Work.ReserveAndAppend);
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
            ReservationCollection reservation,
            AsyncCallback callback,
            object state)
        {
            SequenceNumber result = WriteRestartArea(data, newBaseSeqNum, reservation);
            return new FileRecordSequenceCompletedAsyncResult(result, callback, state, Work.WriteRestartArea);
        }


        public ReservationCollection CreateReservationCollection()
        {
            FileReservationCollection collection = new FileReservationCollection(this);
            return collection;
        }


        public void Dispose()
        {
            this.log.Close();
        }


        public SequenceNumber EndAppend(
            IAsyncResult result)
        {
            FileRecordSequenceCompletedAsyncResult asyncResult = result as FileRecordSequenceCompletedAsyncResult;

            if (asyncResult == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.AsyncResult_Invalid));
            }

            if (asyncResult.CompletedWork != Work.Append)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.AsyncResult_Invalid));
            }

            return asyncResult.End();
        }


        public SequenceNumber EndFlush(
            IAsyncResult result)
        {
            FileRecordSequenceCompletedAsyncResult asyncResult = result as FileRecordSequenceCompletedAsyncResult;

            if (asyncResult == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.AsyncResult_Invalid));
            }

            if (asyncResult.CompletedWork != Work.Flush)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.AsyncResult_Invalid));
            }

            return asyncResult.End();
        }

        public SequenceNumber EndReserveAndAppend(
            IAsyncResult result)
        {
            FileRecordSequenceCompletedAsyncResult asyncResult = result as FileRecordSequenceCompletedAsyncResult;

            if (asyncResult == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.AsyncResult_Invalid));
            }

            if (asyncResult.CompletedWork != Work.ReserveAndAppend)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.AsyncResult_Invalid));
            }

            return asyncResult.End();
        }

        public SequenceNumber EndWriteRestartArea(
            IAsyncResult result)
        {
            FileRecordSequenceCompletedAsyncResult asyncResult = result as FileRecordSequenceCompletedAsyncResult;

            if (asyncResult == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.AsyncResult_Invalid));
            }

            if (asyncResult.CompletedWork != Work.WriteRestartArea)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.AsyncResult_Invalid));
            }

            return asyncResult.End();
        }


        public SequenceNumber Flush()
        {
            return this.Flush(SequenceNumber.Invalid);
        }


        public SequenceNumber Flush(SequenceNumber sequenceNumber)
        {
            return this.frsHelper.Flush(sequenceNumber);
        }

        // The first thread raises the tail pinned event.  
        // All other threads wait until first thread finishes.
        private void RaiseTailPinnedEvent()
        {
            bool raiseEvent;
            lock (this.syncTailPinned)
            {
                if (this.tailPinnedCalled)
                {
                    if (this.tailPinnedThreadID == Thread.CurrentThread.ManagedThreadId)
                    {
                        // This is the same thread where RaiseTailPinnedEvent() was called and user
                        // tried an Append or WriteRestartArea in the TailPinned callback. 
                        // Let the first call complete...
                        // Trying to prevent deadlock...
                        return;
                    }
                    else
                    {
                        // This is not the first thread.  Wait until the first thread has handled tail pinned
                        raiseEvent = false;
                        Monitor.Wait(this.syncTailPinned);
                    }
                }
                else
                {
                    // First thread.  Raise the event.
                    this.tailPinnedCalled = true;
                    raiseEvent = true;
                    this.tailPinnedThreadID = Thread.CurrentThread.ManagedThreadId;
                }
            }

            if (raiseEvent)
            {
                // First thread raises the event.  Signals other waiting threads when done.
                try
                {
                    EventHandler<TailPinnedEventArgs> handler;
                    handler = this.TailPinned;
                    if (handler != null)
                    {
                        try
                        {
                            handler(this, new TailPinnedEventArgs(this.LastSequenceNumber));
                        }
#pragma warning suppress 56500 // This is a callback exception
                        catch (Exception exception)
                        {
                            if (Fx.IsFatal(exception))
                                throw;

                            throw DiagnosticUtility.ExceptionUtility.ThrowHelperCallback(exception);
                        }
                    }
                }
                finally
                {
                    lock (this.syncTailPinned)
                    {
                        this.tailPinnedCalled = false;
                        this.tailPinnedThreadID = -1;
                        // Signal waiting threads
                        Monitor.PulseAll(this.syncTailPinned);
                    }
                }
            }
        }

        public IEnumerable<LogRecord> ReadLogRecords(
            SequenceNumber start,
            LogRecordEnumeratorType logRecordEnum)
        {
            if ((this.access & FileAccess.Read) == 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotSupported(SR.NotSupported_WriteOnly));
            }

            if (logRecordEnum < LogRecordEnumeratorType.User || logRecordEnum > LogRecordEnumeratorType.Next)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("logRecordEnum"));
            }

            return new FileLogRecordEnumerable(this.log, start, logRecordEnum, false);
        }


        public IEnumerable<LogRecord> ReadRestartAreas()
        {
            if ((this.access & FileAccess.Read) == 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotSupported(SR.NotSupported_WriteOnly));
            }

            return new FileLogRecordEnumerable(this.log,
                                                this.frsHelper.RestartSequenceNumber,
                                                LogRecordEnumeratorType.Previous,
                                                true);
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
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservationCollection,
            params long[] reservations)
        {

            if (reservationCollection == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("reservationCollection"));
            }

            if (reservations == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("reservations"));
            }

            FileReservationCollection fileResCollection = null;

            fileResCollection = reservationCollection as FileReservationCollection;
            if (fileResCollection == null || !fileResCollection.IsMyCollection(this))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.LogRecSeq_InvalidReservationCollection));
            }

            foreach (long reservationSize in reservations)
            {
                if (reservationSize < 0)
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("reservations"));
            }

            foreach (long reservationSize in reservations)
            {
                fileResCollection.Add(reservationSize);
            }

            bool throwing = true;
            try
            {
                SequenceNumber returnValue = Append(data, nextUndoRecord, previousRecord, recordAppendOptions);
                throwing = false;
                return returnValue;
            }
            finally
            {
                if (throwing && fileResCollection != null)
                {
                    foreach (long reservationSize in reservations)
                    {
                        fileResCollection.Remove(reservationSize);
                    }
                }
            }
        }

        public SequenceNumber WriteRestartArea(
            ArraySegment<byte> data)
        {
            return WriteRestartArea(new ArraySegment<byte>[] { data });
        }

        public SequenceNumber WriteRestartArea(
            ArraySegment<byte> data,
            SequenceNumber newBaseSeqNum)
        {
            return WriteRestartArea(new ArraySegment<byte>[] { data },
                                    newBaseSeqNum);
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
            IList<ArraySegment<byte>> data)
        {
            return WriteRestartArea(data, this.BaseSequenceNumber);
        }

        public SequenceNumber WriteRestartArea(
            IList<ArraySegment<byte>> data,
            SequenceNumber newBaseSeqNum)
        {

            if (newBaseSeqNum == SequenceNumber.Invalid)
            {
                newBaseSeqNum = this.BaseSequenceNumber;
            }

            try
            {
                return this.frsHelper.WriteRestartAreaInternal(data, newBaseSeqNum);
            }
            catch (SequenceFullException)
            {
                RaiseTailPinnedEvent();

                if (this.RetryAppend)
                {
                    return this.frsHelper.WriteRestartAreaInternal(data, newBaseSeqNum);
                }
                else
                {
                    throw;
                }
            }
        }


        public SequenceNumber WriteRestartArea(
            IList<ArraySegment<byte>> data,
            SequenceNumber newBaseSeqNum,
            ReservationCollection reservations)
        {
            long size = 0;

            if (data == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("data"));
            }

            if (reservations == null)
                return WriteRestartArea(data, newBaseSeqNum);

            FileReservationCollection reservationCollection = reservations as FileReservationCollection;

            if (reservationCollection == null || !reservationCollection.IsMyCollection(this))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.LogRecSeq_InvalidReservationCollection));
            }

            for (int i = 0; i < data.Count; i++)
            {
                size = checked(size + data[i].Count);
            }
            long reservation = reservationCollection.GetBestMatchingReservation(size);

            if (reservation < 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ReservationNotFound());
            }

            bool throwing = true;
            try
            {
                SequenceNumber returnValue = WriteRestartArea(data, newBaseSeqNum);
                throwing = false;
                return returnValue;
            }
            finally
            {
                if (throwing)
                {
                    reservationCollection.Add(reservation);
                }
            }
        }
    }
}

