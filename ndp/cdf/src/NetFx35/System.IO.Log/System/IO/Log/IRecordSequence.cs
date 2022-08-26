//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System.IO;
    using System.Collections.Generic;

    public interface IRecordSequence : IDisposable
    {
        SequenceNumber BaseSequenceNumber { get; }        
        SequenceNumber LastSequenceNumber { get; }        
        long MaximumRecordLength { get; }
        long ReservedBytes { get; }
        bool RetryAppend { get; set; }
        SequenceNumber RestartSequenceNumber { get; }
        
        event EventHandler<TailPinnedEventArgs> TailPinned;

        void AdvanceBaseSequenceNumber(
            SequenceNumber newBaseSequenceNumber);
        
        SequenceNumber Append(
            ArraySegment<byte> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions);            
        SequenceNumber Append(
            ArraySegment<byte> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservations);
        SequenceNumber Append(
            IList<ArraySegment<byte>> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions);
        SequenceNumber Append(
            IList<ArraySegment<byte>> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservations);
        
        IAsyncResult BeginAppend(
            ArraySegment<byte> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            AsyncCallback callback,
            object state);
        IAsyncResult BeginAppend(
            ArraySegment<byte> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservations,
            AsyncCallback callback,
            object state);
        IAsyncResult BeginAppend(
            IList<ArraySegment<byte>> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            AsyncCallback callback,
            object state);
        IAsyncResult BeginAppend(
            IList<ArraySegment<byte>> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousUndoRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservations,
            AsyncCallback callback,
            object state);
        
        IAsyncResult BeginFlush(
            SequenceNumber sequenceNumber,
            AsyncCallback callback,
            object state);

        IAsyncResult BeginReserveAndAppend(
            ArraySegment<byte> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservationCollection,
            long[] reservations,
            AsyncCallback callback,
            object state);
        IAsyncResult BeginReserveAndAppend(
            IList<ArraySegment<byte>> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservationCollection,
            long[] reservations,
            AsyncCallback callback,
            object state);

        IAsyncResult BeginWriteRestartArea(
            ArraySegment<byte> data,
            SequenceNumber newBaseSequenceNumber,
            ReservationCollection reservation, 
            AsyncCallback callback,
            object state);
        IAsyncResult BeginWriteRestartArea(
            IList<ArraySegment<byte>> data,
            SequenceNumber newBaseSequenceNumber,
            ReservationCollection reservation, 
            AsyncCallback callback,
            object state);

        ReservationCollection CreateReservationCollection();

        SequenceNumber EndAppend(
            IAsyncResult result);
        
        SequenceNumber EndFlush(
            IAsyncResult result);

        SequenceNumber EndReserveAndAppend(
            IAsyncResult result);

        SequenceNumber EndWriteRestartArea(
            IAsyncResult result);

        SequenceNumber Flush();

        SequenceNumber Flush(
            SequenceNumber sequenceNumber);

        IEnumerable<LogRecord> ReadLogRecords(
            SequenceNumber start,
            LogRecordEnumeratorType logRecordEnum);

        IEnumerable<LogRecord> ReadRestartAreas();

        SequenceNumber ReserveAndAppend(
            ArraySegment<byte> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservationCollection,
            params long[] reservations);
        SequenceNumber ReserveAndAppend(
            IList<ArraySegment<byte>> data,
            SequenceNumber nextUndoRecord,
            SequenceNumber previousRecord,
            RecordAppendOptions recordAppendOptions,
            ReservationCollection reservationCollection,
            params long[] reservations);

        SequenceNumber WriteRestartArea(
            ArraySegment<byte> data);
        SequenceNumber WriteRestartArea(
            IList<ArraySegment<byte>> data);
        SequenceNumber WriteRestartArea(
            ArraySegment<byte> data,
            SequenceNumber newBaseSequenceNumber);
        SequenceNumber WriteRestartArea(
            IList<ArraySegment<byte>> data,
            SequenceNumber newBaseSequenceNumber);
        SequenceNumber WriteRestartArea(
            ArraySegment<byte> data,
            SequenceNumber newBaseSequenceNumber,
            ReservationCollection reservation);
        SequenceNumber WriteRestartArea(
            IList<ArraySegment<byte>> data,
            SequenceNumber newBaseSequenceNumber,
            ReservationCollection reservation);
    }
}
