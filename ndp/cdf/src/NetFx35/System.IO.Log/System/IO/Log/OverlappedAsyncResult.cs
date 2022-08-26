//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Diagnostics;
    using System.Runtime;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Threading;
    using System.ServiceModel.Diagnostics;

    using Microsoft.Win32.SafeHandles;

    // How to use:
    //   1. Subclass, implement IOCompleted.
    //   2. Call Pack() each time before invoking native method.
    //   3. Use NativeOverlapped pointer in native method
    //   4. If call completes synchronously, Free() immediately
    //   5. If call completes asynchronously, Free() is called for you
    //
    unsafe abstract class OverlappedAsyncResult : IAsyncResult
    {
        // Boring stuff needed to implement IAsyncResult
        // correctly.
        //
        object userState;
        AsyncCallback callback;

        ManualResetEvent waitHandle;
        object syncRoot;

        bool isCompleted;
        bool completedSynchronously;
        bool endCalled;

        Exception storedException;

        // More exciting stuff...
        //
        NativeOverlapped* nativeOverlapped;

        protected OverlappedAsyncResult(AsyncCallback callback,
                                        object state)
        {
            this.callback = callback;
            this.userState = state;
            this.syncRoot = new Object();

            GC.SuppressFinalize(this);
        }

        ~OverlappedAsyncResult()
        {
            // Avoid leaking the NativeOverlapped in case things go wrong 
            if (!Environment.HasShutdownStarted &&
                !AppDomain.CurrentDomain.IsFinalizingForUnload())
            {
                Free();
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
                    if (this.waitHandle == null)
                    {
                        this.waitHandle = new ManualResetEvent(this.isCompleted);
                    }
                }

                return this.waitHandle;
            }
        }

        public bool CompletedSynchronously
        {
            get { return this.completedSynchronously; }
        }

        public bool IsCompleted
        {
            get { return this.isCompleted; }
        }

        internal NativeOverlapped* NativeOverlapped
        {
            get { return this.nativeOverlapped; }
        }

        internal void Complete(bool completedSynchronously, Exception exception)
        {
            lock (this.syncRoot)
            {
                this.storedException = exception;
                this.completedSynchronously = completedSynchronously;
                this.isCompleted = true;

                if (this.waitHandle != null)
                    this.waitHandle.Set();

                Monitor.PulseAll(this.syncRoot);
            }

            if (this.callback != null)
            {
                if (!completedSynchronously ||
                    !ThreadPool.QueueUserWorkItem(InvokeUserCallback, this))
                {
                    InvokeUserCallbackFunction(this);
                }
            }
        }

        public void End()
        {
            lock (this.syncRoot)
            {
                while (!this.IsCompleted)
                {
                    Monitor.Wait(this.syncRoot);
                }

                if (this.endCalled)
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.DuplicateEnd());
                this.endCalled = true;

                if (this.storedException != null)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(this.storedException);
                }
            }
        }

        internal void Free()
        {
            if (this.nativeOverlapped != null)
            {
                GC.SuppressFinalize(this);
                Overlapped.Free(this.nativeOverlapped);
                this.nativeOverlapped = null;
            }
        }

        internal void Pack(object pinnedObjects)
        {
            if (this.nativeOverlapped != null)
            {
                // If we reach this condition, we have a bug in a derived AsyncResult class.
                // It attempted to initiate a new async I/O operation before the previous
                // one completed. This is a fatal condition, so we cannot continue execution.
                DiagnosticUtility.FailFast("Must allow previous I/O to complete before packing");
            }

            GC.ReRegisterForFinalize(this);

            Overlapped overlapped = new Overlapped(0, 0, IntPtr.Zero, this);
            if (this.callback == null)
            {
                this.nativeOverlapped = overlapped.UnsafePack(
                    IOCompletionCallback,
                    pinnedObjects);
            }
            else
            {
                this.nativeOverlapped = overlapped.Pack(
                    IOCompletionCallback,
                    pinnedObjects);
            }
        }

        internal abstract void IOCompleted(uint errorCode);

        static IOCompletionCallback IOCompletionCallback = Fx.ThunkCallback(new IOCompletionCallback(CompletionCallback));

        static void CompletionCallback(uint errorCode, uint numBytes, NativeOverlapped* nativeOverlapped)
        {
            try
            {
                Overlapped overlapped = Overlapped.Unpack(nativeOverlapped);

                OverlappedAsyncResult result;
                result = (OverlappedAsyncResult)overlapped.AsyncResult;
                result.Free();

                result.IOCompleted(errorCode);
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

        static WaitCallback InvokeUserCallback = Fx.ThunkCallback(new WaitCallback(InvokeUserCallbackFunction));

        static void InvokeUserCallbackFunction(
            object data)
        {
            try
            {
                OverlappedAsyncResult result = (OverlappedAsyncResult)data;
                result.callback(result);
            }
#pragma warning suppress 56500 // We will be terminating the process with any exception in this call
            catch (Exception e)
            {
                if (Fx.IsFatal(e))
                    throw;

                throw DiagnosticUtility.ExceptionUtility.ThrowHelperCallback(e);
            }
        }
    }
}
