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

    sealed class LogFlushAsyncResult : OverlappedAsyncResult
    {
        LogRecordSequence recordSequence;
        ulong sequenceNumber;
        object boxedResultLsn;
        GCHandle pinnedResultLsn;

        internal LogFlushAsyncResult(LogRecordSequence recordSequence,
                                     AsyncCallback callback,
                                     object state)
            : base(callback, state)
        {
            this.recordSequence = recordSequence;
        }

        //============================================================
        // Parameters
        //============================================================        
        internal ulong SequenceNumber
        {
            /* get { return this.sequenceNumber; } */
            set { this.sequenceNumber = value; }
        }

        //============================================================
        // Result
        //============================================================        
        internal ulong ResultLsn
        {
            get { return (ulong)this.boxedResultLsn; }
        }

        internal void Start()
        {
            try
            {
                ulong resultLsn = 0;

                this.boxedResultLsn = (object)(resultLsn);
                this.pinnedResultLsn = GCHandle.Alloc(this.boxedResultLsn,
                                                      GCHandleType.Pinned);

                Pack(this.boxedResultLsn);

                uint errorCode;
                unsafe
                {
                    errorCode = UnsafeNativeMethods.FlushLogToLsnAsync(
                        this.recordSequence.MarshalContext,
                        ref this.sequenceNumber,
                        this.pinnedResultLsn.AddrOfPinnedObject(),
                        this.NativeOverlapped);
                }

                if (errorCode != Error.ERROR_IO_PENDING)
                {
                    // We don't need to call Free() in a finally block, 
                    // because any exception will failfast the process
                    Free();

                    this.pinnedResultLsn.Free();

                    if (errorCode == Error.ERROR_SUCCESS)
                    {
                        Complete(true, null);
                    }
                    else
                    {
                        Complete(
                            true,
                            UnsafeNativeMethods.FlushLogToLsnFilter(errorCode));
                    }
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

        internal override void IOCompleted(uint errorCode)
        {
            this.pinnedResultLsn.Free();
            if (errorCode == Error.ERROR_SUCCESS)
            {
                Complete(false, null);
            }
            else
            {
                Complete(false,
                         UnsafeNativeMethods.FlushLogToLsnFilter(errorCode));
            }
        }
    }
}
