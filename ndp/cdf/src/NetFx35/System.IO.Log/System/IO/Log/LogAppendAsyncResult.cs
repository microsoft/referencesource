//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Threading;

    using Microsoft.Win32.SafeHandles;

    sealed class LogAppendAsyncResult : OverlappedAsyncResult
    {
        LogReserveAndAppendState state;

        internal LogAppendAsyncResult(LogRecordSequence recordSequence,
                                      AsyncCallback callback,
                                      object state)
            : base(callback, state)
        {
            this.state = new LogReserveAndAppendState();
            this.state.RecordSequence = recordSequence;
            this.state.AsyncResult = this;
        }
        
        //============================================================
        // Parameters
        //============================================================        
        internal IList<ArraySegment<byte>> Data
        {
            /* get { return this.state.Data; } */
            set { this.state.Data = value; }
        }

        internal ulong PreviousLsn
        {
            /* get { return this.state.PreviousLsn; } */
            set { this.state.PreviousLsn = value; }
        }
        
        internal long[] Reservations
        {
            get { return this.state.Reservations; }
            set { this.state.Reservations = value; }
        }

        internal LogReservationCollection ReservationCollection
        {
            /* get { return this.state.ReservationCollection; } */
            set { this.state.ReservationCollection = value; }
        }

        internal long TotalRecordSize
        {
            /* get { return this.state.TotalRecordSize; } */
            set { this.state.TotalRecordSize = value; }
        }

        internal ulong UserLsn
        {
            /* get { return this.state.UserLsn; } */
            set { this.state.UserLsn = value; }
        }
        
        internal RecordAppendOptions RecordAppendOptions
        {
            /* get { return this.state.RecordAppendOptions; } */
            set { this.state.RecordAppendOptions = value; }
        }

        //============================================================
        // Result
        //============================================================        
        internal ulong ResultLsn
        {
            get { return this.state.ResultLsn; }
        }
        
        internal void Start()
        {
            this.state.Start();
        }
        
        internal override void IOCompleted(uint errorCode)
        {
            this.state.IOComplete(errorCode);
        }
    }
}
