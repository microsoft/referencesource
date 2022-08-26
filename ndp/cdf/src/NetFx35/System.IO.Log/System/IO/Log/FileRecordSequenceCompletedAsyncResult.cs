//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.IO.Log
{
    using System;
    using System.Diagnostics;
    using System.Runtime;
    using System.Threading;

    enum Work
    {
        Append,
        Flush,
        ReserveAndAppend,
        WriteRestartArea
    }

    sealed class FileRecordSequenceCompletedAsyncResult : IAsyncResult
    {
        SequenceNumber result;
        object userState;
        AsyncCallback callback;
        bool endCalled;
        Work work;
        object syncRoot;

        ManualResetEvent waitHandle;

        public FileRecordSequenceCompletedAsyncResult(
            SequenceNumber result,
            AsyncCallback callback,
            object userState,
            Work work)
        {
            this.result = result;
            this.callback = callback;
            this.userState = userState;
            this.work = work;

            this.syncRoot = new object();

            if (this.callback != null)
            {
                try
                {
                    this.callback(this);
                }
#pragma warning suppress 56500 // This is a callback exception
                catch (Exception e)
                {
                    if (Fx.IsFatal(e))
                        throw;

                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperCallback(e);
                }
            }
        }

        public Object AsyncState
        {
            get { return this.userState; }
        }

        public WaitHandle AsyncWaitHandle
        {
            get
            {
                lock (this.syncRoot)
                {
                    // We won't ever close it (it must be GC'd instead), but try
                    // not to be too excessive in allocations.
                    //
                    if (this.waitHandle == null)
                        this.waitHandle = new ManualResetEvent(true);
                }

                return this.waitHandle;
            }
        }

        public bool CompletedSynchronously
        {
            get { return true; }
        }

        public Work CompletedWork
        {
            get { return this.work; }
        }

        public bool IsCompleted
        {
            get { return true; }
        }

        internal SequenceNumber End()
        {
            if (this.endCalled)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.DuplicateEnd());
            }
            this.endCalled = true;

            return this.result;
        }
    }
}
