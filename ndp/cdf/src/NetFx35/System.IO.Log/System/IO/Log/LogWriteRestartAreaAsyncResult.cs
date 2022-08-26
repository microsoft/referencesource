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

    sealed class LogWriteRestartAreaAsyncResult : OverlappedAsyncResult
    {
        LogWriteRestartAreaState state;

        internal LogWriteRestartAreaAsyncResult(LogRecordSequence recordSequence,
                                                AsyncCallback callback,
                                                object state)
            : base(callback, state)
        {
            this.state = new LogWriteRestartAreaState();
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

        internal ulong NewBaseLsn
        {
            /* get { return this.state.NewBaseLsn; } */
            set { this.state.NewBaseLsn = value; }
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
