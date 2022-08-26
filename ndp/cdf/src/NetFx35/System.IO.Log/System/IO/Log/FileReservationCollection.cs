//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.IO.Log
{
    using System;
    using System.Collections.Generic;
    using System.Text;

    internal class FileReservationCollection : ReservationCollection
    {
        FileRecordSequence sequence;

        internal FileReservationCollection(FileRecordSequence sequence) 
        {
            this.sequence = sequence;
        }

        internal bool IsMyCollection(FileRecordSequence sequence)
        {
            return this.sequence == sequence;
        }

        protected override void MakeReservation(long reservationSize)
        {
            this.sequence.AddReservation(reservationSize);
        }

        protected override void FreeReservation(long reservationSize)
        {
            this.sequence.RemoveReservation(reservationSize);
        }

        internal new long GetBestMatchingReservation(long size)
        {
            long reservation = base.GetBestMatchingReservation(size);
            this.sequence.RemoveReservation(reservation);
            return reservation;
        }
    }
}
