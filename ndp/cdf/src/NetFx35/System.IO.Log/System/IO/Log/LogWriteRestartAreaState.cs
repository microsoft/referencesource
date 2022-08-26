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


    internal struct LogWriteRestartAreaState
    {
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
        ulong newBaseLsn;
        LogReservationCollection reservationCollection;
        LogRecordSequence recordSequence;
        LogWriteRestartAreaAsyncResult asyncResult;

        //============================================================
        // Internal State
        //============================================================
        State currentState;
        Exception exceptionResult;

        object boxedResultLsn;
        byte[] restartArea;
        long reservationSize;
        int flags;

        //============================================================
        // Pinned State
        //============================================================
        GCHandle pinnedResultHandle;

        //============================================================
        // Parameters
        //============================================================        
        public LogWriteRestartAreaAsyncResult AsyncResult
        {
            /* get { return this.asyncResult; } */
            set { this.asyncResult = value; }
        }

        public IList<ArraySegment<byte>> Data
        {
            /* get { return this.data; } */
            set { this.data = value; }
        }

        public ulong NewBaseLsn
        {
            /* get { return this.newBaseLsn; } */
            set { this.newBaseLsn = value; }
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
                DiagnosticUtility.FailFast("Calling Start twice (write restart area)");
            }
            Prepare();

            bool complete = false;
            try
            {
                // We use our boxed LSN as a syncRoot, since nobody else
                // is ever going to see it. We take the lock here to
                // avoid ----s with IO completion.
                //
                lock (this.boxedResultLsn)
                {
                    Pin();

                    this.currentState = State.AwaitCompletion;
                    uint errorCode = WriteLogRestartArea();
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
            else if ((errorCode == Error.ERROR_NO_SYSTEM_RESOURCES) && (this.recordSequence.RetryAppend))
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
                Complete(UnsafeNativeMethods.WriteLogRestartAreaFilter(errorCode));
            }
        }

        void AwaitPolicyCompletion_Complete(uint errorCode)
        {
            if (errorCode == Error.ERROR_SUCCESS)
            {
                Pin();

                this.currentState = State.AwaitSecondCompletion;
                errorCode = WriteLogRestartArea();
                if (errorCode == Error.ERROR_IO_PENDING)
                {
                    return;
                }

                AwaitSecondCompletion_Complete(errorCode);
            }
            else
            {
                Complete(UnsafeNativeMethods.WriteLogRestartAreaFilter(errorCode));
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
                Complete(UnsafeNativeMethods.WriteLogRestartAreaFilter(errorCode));
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

            // Determine if we are writing from reservation.
            // Get the best matching reservation if so. Allocate padding
            // if necessary.
            //
            int paddingSize = 0;
            if (this.reservationCollection != null)
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
                this.flags = Const.CLFS_FLAG_USE_RESERVATION;
            }

            // Allocate a new LogLogRecordHeader and set it up
            // correctly. 
            //
            byte[] headerBytes = new byte[LogLogRecordHeader.Size + paddingSize];
            LogLogRecordHeader header = new LogLogRecordHeader(headerBytes);
            header.MajorVersion = LogLogRecordHeader.CurrentMajorVersion;
            header.MinorVersion = LogLogRecordHeader.CurrentMinorVersion;
            header.Padding = (paddingSize != 0);
            if (paddingSize != 0)
            {
                LogLogRecordHeader.EncodePaddingSize(headerBytes,
                                                     LogLogRecordHeader.Size,
                                                     paddingSize);
            }

            // Now we write-gather here, since WriteLogRestartArea does
            // not support write-gather.
            //
            // (NOTE: totalRecordSize includes the size of the header,
            //        for consistency with reservations...)
            //
            long restartAreaSize = this.totalRecordSize + paddingSize;
            this.restartArea = new byte[restartAreaSize];
            restartAreaSize = 0;

            Array.Copy(header.Bits,
                       0,
                       this.restartArea,
                       restartAreaSize,
                       header.Bits.Length);
            restartAreaSize += header.Bits.Length;

            for (int i = 0; i < this.data.Count; i++)
            {
                ArraySegment<byte> segment = this.data[i];

                Array.Copy(segment.Array,
                           segment.Offset,
                           this.restartArea,
                           restartAreaSize,
                           segment.Count);
                restartAreaSize += segment.Count;
            }

            if (restartAreaSize != this.restartArea.Length)
            {
                // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                // log records, we failfast the process.
                DiagnosticUtility.FailFast("Did not do restartArea gather correctly");
            }

            this.currentState = State.Prepared;
        }

        void Pin()
        {
            if (this.pinnedResultHandle.IsAllocated)
            {
                // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                // log records, we failfast the process.
                DiagnosticUtility.FailFast("Already pinned");
            }

            this.pinnedResultHandle = GCHandle.Alloc(this.boxedResultLsn,
                                                     GCHandleType.Pinned);
        }

        void Unpin()
        {
            if (!this.pinnedResultHandle.IsAllocated)
            {
                // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                // log records, we failfast the process.
                DiagnosticUtility.FailFast("Already unpinned");
            }

            this.pinnedResultHandle.Free();
        }

        // Finish up. Synchronize the reservation collection, in the
        // case of failure and success, capture the error (if any),
        // and change state to completed.
        //
        void Complete(Exception error)
        {
            if (this.pinnedResultHandle.IsAllocated)
            {
                // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                // log records, we failfast the process.
                DiagnosticUtility.FailFast("Must unpin before completing");
            }

            if (error != null)
            {
                // If we were writing from reservation, then we need
                // to put the reservation back...
                //
                if (this.reservationCollection != null)
                {
                    this.reservationCollection.InternalAddReservation(this.reservationSize);
                    this.reservationSize = 0;
                }
            }

            this.exceptionResult = error;
            this.currentState = State.Completed;
        }

        uint WriteLogRestartArea()
        {
            uint errorCode = Error.ERROR_SUCCESS;

            // Ensure we are packed now
            //
            if (this.asyncResult != null)
            {
                this.asyncResult.Pack(this.boxedResultLsn);
            }

            try
            {
                unsafe
                {
                    NativeOverlapped* overlapped;
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

                    fixed (byte* pRestartArea = this.restartArea)
                    {
                        if (this.newBaseLsn != UInt64.MaxValue)
                        {
                            errorCode = UnsafeNativeMethods.WriteLogRestartArea(
                                this.recordSequence.MarshalContext,
                                pRestartArea,
                                this.restartArea.Length,
                                ref this.newBaseLsn,
                                this.flags,
                                IntPtr.Zero,
                                this.pinnedResultHandle.AddrOfPinnedObject(),
                                overlapped);
                        }
                        else
                        {
                            errorCode = UnsafeNativeMethods.WriteLogRestartAreaNoBase(
                                this.recordSequence.MarshalContext,
                                pRestartArea,
                                this.restartArea.Length,
                                this.flags,
                                IntPtr.Zero,
                                this.pinnedResultHandle.AddrOfPinnedObject(),
                                overlapped);
                        }
                    }
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
