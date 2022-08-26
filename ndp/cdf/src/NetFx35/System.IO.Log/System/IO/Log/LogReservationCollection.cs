//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.IO.Log;

    // NOTE: The reservations in the collection do not account for
    //       header sizes, out of necessity. The reservations made
    //       with the record sequence *do* account for header sizes.
    //
    //       In order to maintain sanity, the record sequence only
    //       deals with reservations that include the header
    //       size. i.e., if it goes into CLFS, it has the header size
    //       appended to it.
    //
    //       So that means: If we receive a size from the collection,
    //       add the header size to it before returning it. If we
    //       receive a size from our code, subtract the header size
    //       before giving it to the collection.
    //
    //       Keep this straight, or else.
    //
    sealed class LogReservationCollection : ReservationCollection
    {
        LogRecordSequence recordSequence;

        internal LogReservationCollection(LogRecordSequence sequence)
        {
            this.recordSequence = sequence;
        }

        internal LogRecordSequence RecordSequence
        {
            get { return this.recordSequence; }
        }

        internal long GetMatchingReservation(long size)
        {
            // Reservation coming from CLFS, subtract record header
            // size.
            //
            size -= LogLogRecordHeader.Size;
            size = GetBestMatchingReservation(size);
            if (size == -1)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ReservationNotFound());

            // Reservation coming from the collection, add record
            // header size.
            //
            size += LogLogRecordHeader.Size;
            return size;
        }

        internal void InternalAddReservation(long size)
        {
            // Reservation coming from CLFS, remove record header
            // size.
            //
            size -= LogLogRecordHeader.Size;
            ReservationMade(size);
        }

        protected override void MakeReservation(long size)
        {
            if (size < 0)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("size"));

            // Reservation coming from collection, add record header
            // size.
            //
            size += LogLogRecordHeader.Size;

            long aligned;
            UnsafeNativeMethods.AlignReservedLogSingle(
                this.recordSequence.MarshalContext,
                size,
                out aligned);

            UnsafeNativeMethods.AllocReservedLog(
                this.recordSequence.MarshalContext,
                1,
                ref aligned);
        }

        protected override void FreeReservation(long size)
        {
            if (size < 0)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("size"));

            lock (this.recordSequence.LogStore.SyncRoot)
            {
                SafeMarshalContext context = this.recordSequence.InternalMarshalContext;

                if (context == null || context.IsInvalid)
                {
                    return;
                }

                // Reservation coming from collection, add record header
                // size.
                //
                size += LogLogRecordHeader.Size;

                long aligned;
                UnsafeNativeMethods.AlignReservedLogSingle(
                    context,
                    size,
                    out aligned);

                // Adjustment must be negative, otherwise it's considered
                // a "set".  (Yuck.)
                //
                aligned = -aligned;
                UnsafeNativeMethods.FreeReservedLog(
                    context,
                    1,
                    ref aligned);
            }
        }
    }
}
