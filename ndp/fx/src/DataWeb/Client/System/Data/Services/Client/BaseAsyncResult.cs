//---------------------------------------------------------------------
// <copyright file="BaseAsyncResult.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// query object
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;
    using System.Diagnostics;

#if !ASTORIA_LIGHT // Data.Services http stack
    using System.Net;
#else
    using System.Data.Services.Http;
#endif

    /// <summary>
    /// Implementation of IAsyncResult
    /// </summary>
    internal abstract class BaseAsyncResult : IAsyncResult
    {
        /// <summary>Originating object, used to validate End*</summary>
        internal readonly object Source;

        /// <summary>Originating method on source, to differentiate between different methods from same source</summary>
        internal readonly string Method;

        /// <summary>User callback passed to Begin*</summary>
        private readonly AsyncCallback userCallback;

        /// <summary>User state passed to Begin*</summary>
        private readonly object userState;

        /// <summary>wait handle for user to wait until done, we only use this within lock of asyncWaitDisposeLock.</summary>
        private System.Threading.ManualResetEvent asyncWait;

        /// <summary>Holding exception to throw as a nested exception during to End*</summary>
        private Exception failure;

        /// <summary>Abortable request</summary>
        private WebRequest abortable;

        /// <summary>true unless something completes asynchronously</summary>
        private bool completedSynchronously = true;

        /// <summary>true when really completed for the user</summary>
        private bool userCompleted;

        /// <summary>true when no more changes are pending, 0 false, 1 completed, 2 aborted</summary>
        private int completed;

        /// <summary>verify we only invoke the user callback once, 0 false, 1 true</summary>
        private int userNotified;

        /// <summary>non-zero after End*, 0 false, 1, true</summary>
        private int done;

        /// <summary>true if the AsyncWaitHandle has already been disposed.</summary>
        private bool asyncWaitDisposed;

        /// <summary>delay created object to lock to prevent using disposed asyncWait handle.</summary>
        private object asyncWaitDisposeLock;

        /// <summary>
        /// ctor
        /// </summary>
        /// <param name="source">source object of async request</param>
        /// <param name="method">async method name on source object</param>
        /// <param name="callback">user callback to invoke when complete</param>
        /// <param name="state">user state</param>
        internal BaseAsyncResult(object source, string method, AsyncCallback callback, object state)
        {
            Debug.Assert(null != source, "null source");
            this.Source = source;
            this.Method = method;
            this.userCallback = callback;
            this.userState = state;
        }

#if ASTORIA_LIGHT
        /// <summary>
        /// Generic function delegate that is declared in .Net Framework 4.0, but not .Net Framework 3.5 or Silverlight
        /// </summary>
        /// <typeparam name="T1">type of parameter 1</typeparam>
        /// <typeparam name="T2">type of parameter 2</typeparam>
        /// <typeparam name="T3">type of parameter 3</typeparam>
        /// <typeparam name="T4">type of parameter 4</typeparam>
        /// <typeparam name="T5">type of parameter 5</typeparam>
        /// <typeparam name="TResult">type of result</typeparam>
        /// <param name="arg1">generic parameter 1</param>
        /// <param name="arg2">generic parameter 2</param>
        /// <param name="arg3">generic parameter 3</param>
        /// <param name="arg4">generic parameter 4</param>
        /// <param name="arg5">generic parameter 5</param>
        /// <returns>generic result</returns>
        internal delegate TResult Func<in T1, in T2, in T3, in T4, in T5, out TResult>(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5);
#endif

        #region IAsyncResult implmentation - AsyncState, AsyncWaitHandle, CompletedSynchronously, IsCompleted

        /// <summary>user state object parameter</summary>
        public object AsyncState
        {
            get { return this.userState; }
        }

        /// <summary>wait handle for when waiting is required</summary>
        /// <remarks>if displayed by debugger, it undesirable to create the WaitHandle</remarks>
        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        public System.Threading.WaitHandle AsyncWaitHandle
        {
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "WaitHandle is disposed during EndXXX(IAsyncResult) method")]
            get
            {
                if (null == this.asyncWait)
                {   // delay create the wait handle since the user may never use it
                    // like asyncWait which will be GC'd, the losers in creating the asyncWait will also be GC'd
                    System.Threading.Interlocked.CompareExchange(ref this.asyncWait, new System.Threading.ManualResetEvent(this.IsCompleted), null);

                    // multi-thread condition
                    // 1) thread 1 returned IAsyncResult and !IsCompleted so AsyncWaitHandle.WaitOne()
                    // 2) thread 2 signals complete, however thread 1 has retrieved this.completed but not assigned asyncWait
                    if (this.IsCompleted)
                    {   // yes, Set may be called multiple times - but user would have to assume ManualResetEvent and call Reset
                        //
                        // There is a very small window for race condition between setting the wait handle here and disposing
                        // the wait handle inside EndExecute(). Say thread1 calls EndExecute() and IsCompleted is already true we won't
                        // create asyncWait from inside thread1, and thread2 wakes up right before thread1 tries to dispose the handle and
                        // thread2 calls AsyncWaitHandle which creates a new asyncWait handle, if thread1 wakes up right before the Set event
                        // here and disposes the asyncWait handle we just created here, Set() will throw ObjectDisposedException.
                        // SetAsyncWaitHandle() will protect this scenario with a critical section.
                        this.SetAsyncWaitHandle();
                    }
                }

                // Note that if the first time AsyncWaitHandle gets called is after EndExecute() is completed, we don't dispose the
                // newly created handle, it'll just get GC'd.
                return this.asyncWait;
            }
        }

        /// <summary>did the result complete synchronously?</summary>
        public bool CompletedSynchronously
        {
            get { return this.completedSynchronously; }
            internal set { this.completedSynchronously = value; }
        }

        /// <summary>is the result complete?</summary>
        public bool IsCompleted
        {
            get { return this.userCompleted; }
        }

        /// <summary>is the result complete?</summary>
        internal bool IsCompletedInternally
        {
            get { return (0 != this.completed); }
        }

        /// <summary>abort the result</summary>
        internal bool IsAborted
        {
            get { return (2 == this.completed); }
        }

        #endregion

        /// <summary>
        /// WebRequest available for DataServiceContext.CancelRequest
        /// </summary>
        internal WebRequest Abortable
        {
            get
            {
                return this.abortable;
            }

            set
            {
                this.abortable = value;
                if ((null != value) && this.IsAborted)
                {   // if the value hadn't been set yet, but aborting then propogate the abort
                    value.Abort();
                }
            }
        }

        /// <summary>first exception that happened</summary>
        internal Exception Failure
        {
            get { return this.failure; }
        }

        /// <summary>
        /// common handler for EndExecuteBatch &amp; EndSaveChanges
        /// </summary>
        /// <typeparam name="T">derived type of the AsyncResult</typeparam>
        /// <param name="source">source object of async request</param>
        /// <param name="method">async method name on source object</param>
        /// <param name="asyncResult">the asyncResult being ended</param>
        /// <returns>data service response for batch</returns>
        internal static T EndExecute<T>(object source, string method, IAsyncResult asyncResult) where T : BaseAsyncResult
        {
            Util.CheckArgumentNull(asyncResult, "asyncResult");

            T result = (asyncResult as T);
            if ((null == result) || (source != result.Source) || (result.Method != method))
            {
                throw Error.Argument(Strings.Context_DidNotOriginateAsync, "asyncResult");
            }

            Debug.Assert((result.CompletedSynchronously && result.IsCompleted) || !result.CompletedSynchronously, "CompletedSynchronously && !IsCompleted");

            if (!result.IsCompleted)
            {   // if the user doesn't want to wait forever, they should explictly wait on the handle with a timeout
                result.AsyncWaitHandle.WaitOne();

                Debug.Assert(result.IsCompleted, "not completed after waiting");
            }

            // Prevent EndExecute from being called more than once.
            if (System.Threading.Interlocked.Exchange(ref result.done, 1) != 0)
            {
                throw Error.Argument(Strings.Context_AsyncAlreadyDone, "asyncResult");
            }

            // Dispose the wait handle.
            if (null != result.asyncWait)
            {
                System.Threading.Interlocked.CompareExchange(ref result.asyncWaitDisposeLock, new object(), null);
                lock (result.asyncWaitDisposeLock)
                {
                    result.asyncWaitDisposed = true;
                    Util.Dispose(result.asyncWait);
                }
            }

            if (result.IsAborted)
            {
                throw Error.InvalidOperation(Strings.Context_OperationCanceled);
            }

            if (null != result.Failure)
            {
                if (Util.IsKnownClientExcption(result.Failure))
                {
                    throw result.Failure;
                }

                throw Error.InvalidOperation(Strings.DataServiceException_GeneralError, result.Failure);
            }

            return result;
        }

        /// <summary>
        /// Due to the unexpected behaviors of IAsyncResult.CompletedSynchronously in the System.Net networking stack, we have to make
        /// async calls to their APIs using the specific pattern they've prescribed. This method runs in the caller thread and invokes
        /// the BeginXXX methods.  It then checks IAsyncResult.CompletedSynchronously and if it is true, we invoke the callback in the
        /// caller thread.
        /// </summary>
        /// <param name="asyncAction">
        /// This is the action that invokes the BeginXXX method. Note we MUST use our special callback from GetDataServiceAsyncCallback()
        /// when invoking the async call.
        /// </param>
        /// <param name="callback">async callback to be called when the operation is complete</param>
        /// <param name="state">A user-provided object that distinguishes this particular asynchronous request from other requests.</param>
        /// <returns>Returns the async result from the BeginXXX method.</returns>
        /// <remarks>
        /// CompletedSynchronously (for System.Net networking stack) means "was the operation completed before the first time
        /// that somebody asked if it was completed synchronously"? They do this because some of their asynchronous operations
        /// (particularly those in the Socket class) will avoid the cost of capturing and transferring the ExecutionContext
        /// to the callback thread by checking CompletedSynchronously, and calling the callback from within BeginXxx instead of
        /// on the completion port thread if the native winsock call completes quickly.
        /// 
        /// For other operations however (notably those in HttpWebRequest), they use the same underlying IAsyncResult implementation,
        /// but do NOT check CompletedSynchronously before returning from BeginXxx.  That means that CompletedSynchronously will
        /// be false if and only if you checked it from the thread which called BeginXxx BEFORE the operation completed.  It will
        /// then continue to be false even after IsCompleted becomes true.
        /// 
        /// Note that CompletedSynchronously == true does not guarantee anything about how much of your callback has executed.
        /// 
        /// The usual pattern for handling synchronous completion is that both the caller and callback should check CompletedSynchronously.
        /// If its true, the callback should do nothing and the caller should call EndRead and process the result.
        /// This guarantees that the caller and callback are not accessing the stream or buffer concurrently without the need
        /// for explicit synchronization between the two.
        /// </remarks>
        internal static IAsyncResult InvokeAsync(Func<AsyncCallback, object, IAsyncResult> asyncAction, AsyncCallback callback, object state)
        {
            IAsyncResult asyncResult = asyncAction(BaseAsyncResult.GetDataServiceAsyncCallback(callback), state);
            return PostInvokeAsync(asyncResult, callback);
        }

        /// <summary>
        /// Due to the unexpected behaviors of IAsyncResult.CompletedSynchronously in the System.Net networking stack, we have to make
        /// async calls to their APIs using the specific pattern they've prescribed. This method runs in the caller thread and invokes
        /// the BeginXXX methods.  It then checks IAsyncResult.CompletedSynchronously and if it is true, we invoke the callback in the
        /// caller thread.
        /// </summary>
        /// <param name="asyncAction">
        /// This is the action that invokes the BeginXXX method. Note we MUST use our special callback from GetDataServiceAsyncCallback()
        /// when invoking the async call.
        /// </param>
        /// <param name="buffer">buffer to transfer the data</param>
        /// <param name="offset">byte offset in buffer</param>
        /// <param name="length">max number of bytes in the buffer</param>
        /// <param name="callback">async callback to be called when the operation is complete</param>
        /// <param name="state">A user-provided object that distinguishes this particular asynchronous request from other requests.</param>
        /// <returns>An IAsyncResult that represents the asynchronous operation, which could still be pending</returns>
        /// <remarks>Please see remarks on the other InvokeAsync() overload.</remarks>
        internal static IAsyncResult InvokeAsync(Func<byte[], int, int, AsyncCallback, object, IAsyncResult> asyncAction, byte[] buffer, int offset, int length, AsyncCallback callback, object state)
        {
            IAsyncResult asyncResult = asyncAction(buffer, offset, length, BaseAsyncResult.GetDataServiceAsyncCallback(callback), state);
            return PostInvokeAsync(asyncResult, callback);
        }

        /// <summary>Set the AsyncWait and invoke the user callback.</summary>
        /// <remarks>
        /// If the background thread gets a ThreadAbort, the userCallback will never be invoked.
        /// This is why it's generally important to never wait forever, but to have more specific
        /// time limit.  Also then cancel the operation, to make sure its stopped, to avoid 
        /// multi-threading if your wait time limit was just too short.
        /// </remarks>
        internal void HandleCompleted()
        {
            // Dev10 Bug #524145: even if background thread of async operation encounters
            // an "uncatchable" exception, do the minimum to unblock the async result.
            if (this.IsCompletedInternally && (System.Threading.Interlocked.Exchange(ref this.userNotified, 1) == 0))
            {
                this.abortable = null; // reset abort via CancelRequest
                try
                {
                    // avoid additional work when aborting for exceptional reasons
                    if (!Util.DoNotHandleException(this.Failure))
                    {
                        // the CompleteRequest may do additional work which is why
                        // it is important not to signal the user via either the
                        // IAsyncResult.IsCompleted, IAsyncResult.WaitHandle or the callback
                        this.CompletedRequest();
                    }
                }
                catch (Exception ex)
                {
                    if (this.HandleFailure(ex))
                    {
                        throw;
                    }
                }
                finally
                {
                    // 1. set IAsyncResult.IsCompleted, otherwise user was
                    // signalled on another thread, but the property may not be true.
                    this.userCompleted = true;

                    // 2. signal the wait handle because it can't be first nor can it be last.
                    //
                    // There is a very small window for race condition between setting the wait handle here and disposing
                    // the wait handle inside EndExecute(). Say thread1 is the async thread that executes up till this point, i.e. right
                    // after userCompleted is set to true and before the asyncWait is signaled; thread2 wakes up and calls EndExecute() till
                    // right before we try to dispose the wait handle; thread3 wakes up and calls AsyncWaitHandle which creates a new instance
                    // for this.asyncWait; thread2 then resumes to dispose this.asyncWait and if at this point thread1 sets this.asyncWait,
                    // we'll get an ObjectDisposedException on thread1.  SetAsyncWaitHandle() will protect this scenario with a critical section.
                    this.SetAsyncWaitHandle();

                    // 3. invoke the callback because user may throw an exception and stop any further processing
                    if ((null != this.userCallback) && !(this.Failure is System.Threading.ThreadAbortException) && !(this.Failure is System.StackOverflowException))
                    {   // any exception thrown by user should be "unhandled"
                        // it's possible callback will be invoked while another creates and sets the asyncWait
                        this.userCallback(this);
                    }
                }
            }
        }

        /// <summary>Cache the exception that happened on the background thread for the caller of EndSaveChanges.</summary>
        /// <param name="e">exception object from background thread</param>
        /// <returns>true if the exception (like StackOverflow or ThreadAbort) should be rethrown</returns>
        internal bool HandleFailure(Exception e)
        {
            System.Threading.Interlocked.CompareExchange(ref this.failure, e, null);
            this.SetCompleted();
            return Util.DoNotHandleException(e);
        }

        /// <summary>Set the async result as completed and aborted.</summary>
        internal void SetAborted()
        {
            System.Threading.Interlocked.Exchange(ref this.completed, 2);
        }

        /// <summary>Set the async result as completed.</summary>
        internal void SetCompleted()
        {
            System.Threading.Interlocked.CompareExchange(ref this.completed, 1, 0);
        }

        /// <summary>invoked for derived classes to cleanup before callback is invoked</summary>
        protected abstract void CompletedRequest();

        /// <summary>
        /// Due to the unexpected behaviors of IAsyncResult.CompletedSynchronously in the System.Net networking stack, we have to make
        /// async calls to their APIs using the specific pattern they've prescribed. This method runs in the caller thread after the
        /// BeginXXX method returns.  It checks IAsyncResult.CompletedSynchronously and if it is true, we invoke the callback in the
        /// caller thread.
        /// </summary>
        /// <param name="asyncResult">The IAsyncResult that represents the asynchronous operation we just called, which could still be pending</param>
        /// <param name="callback">Callback to be invoked when IAsyncResult.CompletedSynchronously is true.</param>
        /// <returns>Returns an IAsyncResult that represents the asynchronous operation we just called, which could still be pending</returns>
        /// <remarks>Please see remarks on BaseAsyncResult.InvokeAsync().</remarks>
        private static IAsyncResult PostInvokeAsync(IAsyncResult asyncResult, AsyncCallback callback)
        {
            Debug.Assert(asyncResult != null, "asyncResult != null");
            if (asyncResult.CompletedSynchronously)
            {
                Debug.Assert(asyncResult.IsCompleted, "asyncResult.IsCompleted");
                callback(asyncResult);
            }

            return asyncResult;
        }

        /// <summary>
        /// Due to the unexpected behaviors of IAsyncResult.CompletedSynchronously in the System.Net networking stack, we have to make
        /// async calls to their APIs using the specific pattern they've prescribed. This method returns an AsyncCallback which we can pass
        /// to the BeginXXX methods in the caller thread.  The returned callback will only run the wrapped callback if
        /// IAsyncResult.CompletedSynchronously is false, otherwise it returns immediately.
        /// </summary>
        /// <param name="callback">callback to be wrapped</param>
        /// <returns>Returnes a callback which will only run the wrapped callback if IAsyncResult.CompletedSynchronously is false, otherwise it returns immediately.</returns>
        /// <remarks>Please see remarks on BaseAsyncResult.InvokeAsync().</remarks>
        private static AsyncCallback GetDataServiceAsyncCallback(AsyncCallback callback)
        {
            return (asyncResult) =>
            {
                Debug.Assert(asyncResult != null && asyncResult.IsCompleted, "asyncResult != null && asyncResult.IsCompleted");
                if (asyncResult.CompletedSynchronously)
                {
                    return;
                }

                callback(asyncResult);
            };
        }

        /// <summary>
        /// Sets the async wait handle
        /// </summary>
        private void SetAsyncWaitHandle()
        {
            if (null != this.asyncWait)
            {
                System.Threading.Interlocked.CompareExchange(ref this.asyncWaitDisposeLock, new object(), null);
                lock (this.asyncWaitDisposeLock)
                {
                    if (!this.asyncWaitDisposed)
                    {
                        this.asyncWait.Set();
                    }
                }
            }
        }
    }
}
