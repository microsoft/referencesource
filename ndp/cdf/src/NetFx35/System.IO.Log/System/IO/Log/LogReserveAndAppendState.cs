//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Threading;


    internal struct LogReserveAndAppendState
    {
        enum PinIndex
        {
            ReturnLsn = 0,
            Reservations = 1,
            Header = 2,
            Padding = 3,
            FirstData = 4,
        }

        enum State
        {
            Unprepared = 0,
            Prepared,
            AwaitCompletion,
            AwaitPolicyCompletion,
            AwaitSecondCompletion,
            Completed
        }

        //============================================================
        // Inputs
        //============================================================
        IList<ArraySegment<byte>> data;
        long totalRecordSize;
        ulong userLsn;
        ulong previousLsn;
        LogReservationCollection reservationCollection;
        long[] reservations;
        RecordAppendOptions recordAppendOptions;
        LogRecordSequence recordSequence;
        LogAppendAsyncResult asyncResult;

        //============================================================
        // Internal State
        //============================================================
        State currentState;
        Exception exceptionResult;

        long[] alignedReservations;
        object boxedResultLsn;
        byte[] headerBits;
        long reservationSize;
        int flags;

        //============================================================
        // Pinned data
        //============================================================
        object[] pinnedObjects;
        GCHandle[] handles;
        CLFS_WRITE_ENTRY[] writeEntries;

        //============================================================
        // Parameters
        //============================================================        
        public LogAppendAsyncResult AsyncResult
        {
            /* get { return this.asyncResult; } */
            set { this.asyncResult = value; }
        }

        public IList<ArraySegment<byte>> Data
        {
            /* get { return this.data; } */
            set { this.data = value; }
        }

        public ulong PreviousLsn
        {
            /* get { return this.previousLsn; } */
            set { this.previousLsn = value; }
        }

        public long[] Reservations
        {
            get { return this.reservations; }
            set { this.reservations = value; }
        }

        public LogRecordSequence RecordSequence
        {
            /* get { return this.recordSequence; } */
            set { this.recordSequence = value; }
        }

        public LogReservationCollection ReservationCollection
        {
            /* get { return this.reservationCollection; } */
            set { this.reservationCollection = value; }
        }

        public long TotalRecordSize
        {
            /* get { return this.totalRecordSize; } */
            set { this.totalRecordSize = value; }
        }

        public ulong UserLsn
        {
            /* get { return this.userLsn; } */
            set { this.userLsn = value; }
        }

        public RecordAppendOptions RecordAppendOptions
        {
            /* get { return this.recordAppendOptions; } */
            set { this.recordAppendOptions = value; }
        }

        //============================================================
        // Results
        //============================================================        
        public ulong ResultLsn
        {
            get { return (ulong)(this.boxedResultLsn); }
        }

        //================================================================
        // Execution: State Machine
        //================================================================
        public void Start()
        {
            if (this.currentState != State.Unprepared)
            {
                // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                // log records, we failfast the process.
                DiagnosticUtility.FailFast("Calling Start twice (reserve n' append)");
            }
            Prepare();

            bool complete = false;
            try
            {
                // We use our boxed LSN as a syncRoot, since nobody else
                // is ever going to see it.
                //
                lock (this.boxedResultLsn)
                {
                    Pin();

                    this.currentState = State.AwaitCompletion;
                    uint errorCode = ReserveAndAppend();
                    if (errorCode == Error.ERROR_IO_PENDING)
                    {
                        return;
                    }
                    else
                    {
                        AwaitCompletion_Complete(errorCode);
                    }

                    // Make sure to check for completion BEFORE
                    // leaving the lock, or there's a ---- with IO
                    // completion that may cause double-completes.
                    //
                    complete = (this.currentState == State.Completed);
                }
            }
#pragma warning suppress 56500 // We will be terminating the process with any exception in this call
            catch (Exception e)
            {
                // The code in the try block should not throw any exceptions.
                // If an exception is caught here, IO.Log may be in an unknown state.
                // We prefer to failfast instead of risking the possibility of log corruption.
                // Any client code using IO.Log must have a recovery model that can deal 
                // with appdomain and process failures.
                DiagnosticUtility.InvokeFinalHandler(e);
            }

            if (complete)
            {
                if (this.asyncResult != null)
                {
                    this.asyncResult.Complete(true, this.exceptionResult);
                }
                else if (this.exceptionResult != null)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(this.exceptionResult);
                }
            }
            else
            {
                if (this.asyncResult == null)
                {
                    // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                    // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                    // log records, we failfast the process.
                    DiagnosticUtility.FailFast("What am I doing returning early without an AsyncResult?");
                }
            }
        }

        public void IOComplete(uint errorCode)
        {
            if (this.asyncResult == null)
            {
                // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                // log records, we failfast the process.
                DiagnosticUtility.FailFast("What am I doing in IO completion without an AsyncResult?");
            }

            try
            {
                bool complete = false;

                // We use our boxed LSN as a syncRoot, since nobody
                // else is ever going to see it.
                //
                lock (this.boxedResultLsn)
                {
                    switch (this.currentState)
                    {
                        case State.AwaitCompletion:
                            AwaitCompletion_Complete(errorCode);
                            break;

                        case State.AwaitPolicyCompletion:
                            AwaitPolicyCompletion_Complete(errorCode);
                            break;

                        case State.AwaitSecondCompletion:
                            AwaitSecondCompletion_Complete(errorCode);
                            break;

                        default:

                            // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                            // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                            // log records, we failfast the process.
                            DiagnosticUtility.FailFast("Invalid state for IO completion");
                            break;
                    }

                    // Make sure to check for completion BEFORE
                    // leaving the lock, or there's a ---- with IO
                    // completion that may cause double-completes.
                    //
                    complete = (this.currentState == State.Completed);
                }

                if (complete)
                {
                    this.asyncResult.Complete(false, this.exceptionResult);
                }
            }
#pragma warning suppress 56500 // We will be terminating the process with any exception in this call
            catch (Exception e)
            {
                // The code in the try block should not throw any exceptions.
                // If an exception is caught here, IO.Log may be in an unknown state.
                // We prefer to failfast instead of risking the possibility of log corruption.
                // Any client code using IO.Log must have a recovery model that can deal 
                // with appdomain and process failures.
                DiagnosticUtility.InvokeFinalHandler(e);
            }
        }

        void AwaitCompletion_Complete(uint errorCode)
        {
            Unpin();

            if (errorCode == Error.ERROR_SUCCESS)
            {
                Complete(null);
            }
            else if (((errorCode == Error.ERROR_NO_SYSTEM_RESOURCES)
                      || (errorCode == Error.ERROR_LOG_FULL))
                     && (this.recordSequence.RetryAppend))
            {
                this.currentState = State.AwaitPolicyCompletion;
                errorCode = InvokePolicyEngine();
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    return;
                }

                AwaitPolicyCompletion_Complete(errorCode);
            }
            else
            {
                Complete(
                    UnsafeNativeMethods.ReserveAndAppendLogFilter(errorCode));
            }
        }

        void AwaitPolicyCompletion_Complete(uint errorCode)
        {
            if (errorCode == Error.ERROR_SUCCESS)
            {
                Pin();

                this.currentState = State.AwaitSecondCompletion;
                errorCode = ReserveAndAppend();
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    return;
                }

                AwaitSecondCompletion_Complete(errorCode);
            }
            else
            {
                Complete(
                    UnsafeNativeMethods.HandleLogFullFilter(errorCode));
            }
        }

        void AwaitSecondCompletion_Complete(uint errorCode)
        {
            Unpin();

            if (errorCode == Error.ERROR_SUCCESS)
            {
                Complete(null);
            }
            else
            {
                Complete(
                    UnsafeNativeMethods.ReserveAndAppendLogFilter(errorCode));
            }
        }

        // Get everything ready to run. Only call this once, unlike Pin.
        //
        void Prepare()
        {
            if (this.currentState != State.Unprepared)
            {
                // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                // log records, we failfast the process.
                DiagnosticUtility.FailFast("Calling Prepare Twice");
            }

            // Box the result LSN.
            //
            ulong resultLsn = 0;
            this.boxedResultLsn = (object)(resultLsn);

            int paddingSize = 0;
            if (this.reservationCollection != null)
            {
                // Determine if we are writing from reservation.
                // Get the best matching reservation if so. Allocate padding
                // if necessary.
                //
                if (this.reservations == null)
                {
                    this.reservationSize = this.reservationCollection.GetMatchingReservation(this.totalRecordSize);
                    if (this.reservationSize < this.totalRecordSize)
                    {
                        // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                        // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                        // log records, we failfast the process.
                        DiagnosticUtility.FailFast("Somehow got a smaller reservation");
                    }
                    if (this.reservationSize <= 0)
                    {
                        // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                        // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                        // log records, we failfast the process.
                        DiagnosticUtility.FailFast("Reservation size must be bigger than zero");
                    }

                    paddingSize = checked((int)(this.reservationSize -
                                                this.totalRecordSize));
                }
                else
                {
                    // Otherwise, we are making new reservations
                    // (ReserveAndAppend). The reservations we make
                    // must be adjusted by the header size.
                    //
                    long[] adjustedReservations = new long[this.reservations.Length];
                    for (int i = 0; i < adjustedReservations.Length; i++)
                    {
                        adjustedReservations[i] = (this.reservations[i] +
                                                   LogLogRecordHeader.Size);
                    }
                    this.reservations = adjustedReservations;
                    this.alignedReservations = (long[])this.reservations.Clone();
                }
            }

            // Allocate a new LogLogRecordHeader and set it up
            // correctly. 
            //
            this.headerBits = new byte[LogLogRecordHeader.Size + paddingSize];
            LogLogRecordHeader header = new LogLogRecordHeader(this.headerBits);
            header.MajorVersion = LogLogRecordHeader.CurrentMajorVersion;
            header.MinorVersion = LogLogRecordHeader.CurrentMinorVersion;
            header.Padding = (paddingSize != 0);
            if (paddingSize != 0)
            {
                LogLogRecordHeader.EncodePaddingSize(this.headerBits,
                                                     LogLogRecordHeader.Size,
                                                     paddingSize);
            }

            // Translate write flags to CLFS flags. 
            //
            this.flags = 0;
            if ((this.recordAppendOptions & RecordAppendOptions.ForceAppend) != 0)
            {
                this.flags |= Const.CLFS_FLAG_FORCE_APPEND;
            }
            if ((this.recordAppendOptions & RecordAppendOptions.ForceFlush) != 0)
            {
                this.flags |= Const.CLFS_FLAG_FORCE_FLUSH;
            }
            if (this.reservationSize > 0)
            {
                this.flags |= Const.CLFS_FLAG_USE_RESERVATION;
            }

            this.currentState = State.Prepared;
        }

        // Allocate the pinned object array, fill it out, pin the
        // objects, and set up the CLFS_WRITE_ENTRIES all at the same
        // time.
        //
        // (After this function has been called, the GC is not happy.)
        //
        void Pin()
        {
            if (this.handles != null)
            {
                // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                // log records, we failfast the process.
                DiagnosticUtility.FailFast("Already pinned");
            }

            int pinnedObjectCount = (int)PinIndex.FirstData;
            pinnedObjectCount += this.data.Count;
            this.pinnedObjects = new object[pinnedObjectCount];
            this.handles = new GCHandle[pinnedObjectCount];

            int writeEntryCount = this.data.Count + 1; // Header
            this.writeEntries = new CLFS_WRITE_ENTRY[writeEntryCount];

            // Pin objects, and optionally fill in write entries.
            //
            int entryIndex = 0;

            // Return LSN
            //
            this.pinnedObjects[(int)PinIndex.ReturnLsn] = this.boxedResultLsn;
            this.handles[(int)PinIndex.ReturnLsn] = GCHandle.Alloc(this.boxedResultLsn, GCHandleType.Pinned);

            // Reservations
            //
            if (this.alignedReservations != null)
            {
                this.pinnedObjects[(int)PinIndex.Reservations] = this.alignedReservations;
                this.handles[(int)PinIndex.Reservations] = GCHandle.Alloc(this.alignedReservations, GCHandleType.Pinned);
            }

            // Header
            //
            this.pinnedObjects[(int)PinIndex.Header] = this.headerBits;
            this.handles[(int)PinIndex.Header] = GCHandle.Alloc(this.headerBits, GCHandleType.Pinned);
            this.writeEntries[entryIndex].Buffer = Marshal.UnsafeAddrOfPinnedArrayElement(this.headerBits, 0);
            this.writeEntries[entryIndex].ByteLength = this.headerBits.Length;
            entryIndex++;

            // Data bits
            //
            for (int i = 0; i < this.data.Count; i++)
            {
                ArraySegment<byte> segment = this.data[i];

                this.pinnedObjects[(int)PinIndex.FirstData + i] = segment.Array;
                this.handles[(int)PinIndex.FirstData + i] = GCHandle.Alloc(segment.Array, GCHandleType.Pinned);
                this.writeEntries[entryIndex].Buffer = Marshal.UnsafeAddrOfPinnedArrayElement(segment.Array, segment.Offset);
                this.writeEntries[entryIndex].ByteLength = segment.Count;
                entryIndex++;
            }

            if (entryIndex != this.writeEntries.Length)
            {
                // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                // log records, we failfast the process.
                DiagnosticUtility.FailFast("Entry counts do not agree");
            }
        }

        // Unpin all the objects, free the GC handles, clear out
        // arrays, etc. Generally remove our hands from around the
        // GC's throat.
        //
        void Unpin()
        {
            if (this.handles != null)
            {
                for (int i = 0; i < this.handles.Length; i++)
                {
                    if (this.handles[i].IsAllocated)
                        this.handles[i].Free();
                }

                this.handles = null;
            }

            this.pinnedObjects = null;
            this.writeEntries = null;
        }

        // Finish up. Synchronize the reservation collection, in the
        // case of failure and success, capture the error (if any),
        // and change state to completed.
        //
        void Complete(Exception error)
        {
            if (this.handles != null)
            {
                // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                // log records, we failfast the process.
                DiagnosticUtility.FailFast("Must unpin before completing");
            }
            if (this.currentState == State.Completed)
            {
                // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                // log records, we failfast the process.
                DiagnosticUtility.FailFast("Called complete twice");
            }

            if (error == null)
            {
                if (this.reservations != null)
                {
                    if (this.reservationCollection == null)
                    {
                        // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                        // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                        // log records, we failfast the process.
                        DiagnosticUtility.FailFast("How did I get reservations sans reservationCollection?");
                    }

                    foreach (long size in this.reservations)
                    {
                        this.reservationCollection.InternalAddReservation(size);
                    }
                }
            }
            else
            {
                if (this.reservationSize > 0)
                {
                    this.reservationCollection.InternalAddReservation(this.reservationSize);
                }
            }

            this.exceptionResult = error;
            this.currentState = State.Completed;
        }

        uint ReserveAndAppend()
        {
            uint errorCode = Error.ERROR_SUCCESS;

            // Ensure we are packed now
            //
            if (this.asyncResult != null)
            {
                this.asyncResult.Pack(this.pinnedObjects);
            }

            try
            {
                unsafe
                {
                    NativeOverlapped* overlapped = null;
                    if (this.asyncResult != null)
                    {
                        overlapped = this.asyncResult.NativeOverlapped;
                        if (overlapped == null)
                        {
                            // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                            // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                            // log records, we failfast the process.
                            DiagnosticUtility.FailFast("Should have packed the async result already");
                        }
                    }
                    else
                    {
                        overlapped = null;
                    }

                    int reservationCount = 0;
                    if (this.alignedReservations != null)
                        reservationCount = this.alignedReservations.Length;

                    errorCode = UnsafeNativeMethods.ReserveAndAppendLog(
                        this.recordSequence.MarshalContext,
                        this.writeEntries,
                        this.writeEntries.Length,
                        ref this.userLsn,
                        ref this.previousLsn,
                        reservationCount,
                        this.alignedReservations,
                        this.flags,
                        this.handles[(int)PinIndex.ReturnLsn].AddrOfPinnedObject(),
                        overlapped);
                }
            }
            finally
            {
                if (this.asyncResult != null &&
                    errorCode != Error.ERROR_IO_PENDING)
                {
                    this.asyncResult.Free();
                }
            }

            return errorCode;
        }

        uint InvokePolicyEngine()
        {
            HandleLogFullCallback callback = null;
            if (this.asyncResult != null)
            {
                callback = new HandleLogFullCallback(this.asyncResult.IOCompleted);
            }

            LogManagementAsyncResult logManagement;
            logManagement = this.recordSequence.LogStore.LogManagement;

            return logManagement.HandleLogFull(callback);
        }
    }
}
