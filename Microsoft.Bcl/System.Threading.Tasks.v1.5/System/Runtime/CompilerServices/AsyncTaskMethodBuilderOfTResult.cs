using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.Contracts;
using System.Linq;
using System.Security;
using System.Text;
using System.Threading.Tasks;

namespace System.Runtime.CompilerServices
{

    /// <summary>
    /// Provides a builder for asynchronous methods that return <see cref="System.Threading.Tasks.Task{TResult}"/>.
    /// This type is intended for compiler use only.
    /// </summary>
    /// <remarks>
    /// AsyncTaskMethodBuilder{TResult} is a value type, and thus it is copied by value.
    /// Prior to being copied, one of its Task, SetResult, or SetException members must be accessed,
    /// or else the copies may end up building distinct Task instances.
    /// </remarks>
    public struct AsyncTaskMethodBuilder<TResult> : IAsyncMethodBuilder
    {
        /// <summary>A cached task for default(TResult).</summary>
        internal readonly static TaskCompletionSource<TResult> s_defaultResultTask = AsyncMethodTaskCache<TResult>.CreateCompleted(default(TResult));

        // WARNING: For performance reasons, the m_task field is lazily initialized.
        //          For correct results, the struct AsyncTaskMethodBuilder<TResult> must 
        //          always be used from the same location/copy, at least until m_task is 
        //          initialized.  If that guarantee is broken, the field could end up being 
        //          initialized on the wrong copy.

        /// <summary>State related to the IAsyncStateMachine.</summary>
        private AsyncMethodBuilderCore m_coreState; // mutable struct: must not be readonly
        /// <summary>The lazily-initialized task.</summary>
        /// <remarks>Must be named m_task for debugger step-over to work correctly.</remarks>
        private Task<TResult> m_task; // lazily-initialized: must not be readonly
        /// <summary>The lazily-initialized task completion source.</summary>
        private TaskCompletionSource<TResult> m_taskCompletionSource; // lazily-initialized: must not be readonly

        /// <summary>Temporary support for disabling crashing if tasks go unobserved.</summary>
        static AsyncTaskMethodBuilder()
        {
            try { AsyncVoidMethodBuilder.PreventUnobservedTaskExceptions(); }
            catch { }
        }

        /// <summary>Initializes a new <see cref="AsyncTaskMethodBuilder"/>.</summary>
        /// <returns>The initialized <see cref="AsyncTaskMethodBuilder"/>.</returns>
        public static AsyncTaskMethodBuilder<TResult> Create()
        {
            return default(AsyncTaskMethodBuilder<TResult>);
            // NOTE:  If this method is ever updated to perform more initialization,
            //        ATMB.Create must also be updated to call this Create method.
        }

        /// <summary>Initiates the builder's execution with the associated state machine.</summary>
        /// <typeparam name="TStateMachine">Specifies the type of the state machine.</typeparam>
        /// <param name="stateMachine">The state machine instance, passed by reference.</param>
        [DebuggerStepThrough]
        public void Start<TStateMachine>(ref TStateMachine stateMachine) where TStateMachine : IAsyncStateMachine
        {
            m_coreState.Start(ref stateMachine); // argument validation handled by AsyncMethodBuilderCore
        }

        /// <summary>Associates the builder with the state machine it represents.</summary>
        /// <param name="stateMachine">The heap-allocated state machine object.</param>
        /// <exception cref="System.ArgumentNullException">The <paramref name="stateMachine"/> argument was null (Nothing in Visual Basic).</exception>
        /// <exception cref="System.InvalidOperationException">The builder is incorrectly initialized.</exception>
        public void SetStateMachine(IAsyncStateMachine stateMachine)
        {
            m_coreState.SetStateMachine(stateMachine); // argument validation handled by AsyncMethodBuilderCore
        }

        /// <summary>Perform any initialization necessary prior to lifting the builder to the heap.</summary>
        void IAsyncMethodBuilder.PreBoxInitialization()
        {
            // Force the Task to be initialized prior to the first suspending await so 
            // that the original stack-based builder has a reference to the right Task.
            var ignored = this.Task;
        }

        /// <summary>
        /// Schedules the specified state machine to be pushed forward when the specified awaiter completes.
        /// </summary>
        /// <typeparam name="TAwaiter">Specifies the type of the awaiter.</typeparam>
        /// <typeparam name="TStateMachine">Specifies the type of the state machine.</typeparam>
        /// <param name="awaiter">The awaiter.</param>
        /// <param name="stateMachine">The state machine.</param>
        public void AwaitOnCompleted<TAwaiter, TStateMachine>(
            ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : INotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            try
            {
                var continuation = m_coreState.GetCompletionAction(ref this, ref stateMachine);
                Contract.Assert(continuation != null, "GetCompletionAction should always return a valid action.");
                awaiter.OnCompleted(continuation);
            }
            catch (Exception e)
            {
                AsyncServices.ThrowAsync(e, targetContext: null);
            }
        }

        /// <summary>
        /// Schedules the specified state machine to be pushed forward when the specified awaiter completes.
        /// </summary>
        /// <typeparam name="TAwaiter">Specifies the type of the awaiter.</typeparam>
        /// <typeparam name="TStateMachine">Specifies the type of the state machine.</typeparam>
        /// <param name="awaiter">The awaiter.</param>
        /// <param name="stateMachine">The state machine.</param>
#if !SILVERLIGHT
        // [SecuritySafeCritical]
#endif
        public void AwaitUnsafeOnCompleted<TAwaiter, TStateMachine>(
            ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : ICriticalNotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            try
            {
                var continuation = m_coreState.GetCompletionAction(ref this, ref stateMachine);
                Contract.Assert(continuation != null, "GetCompletionAction should always return a valid action.");
                awaiter.UnsafeOnCompleted(continuation);
            }
            catch (Exception e)
            {
                AsyncServices.ThrowAsync(e, targetContext: null);
            }
        }

        /// <summary>Gets the lazily-initialized TaskCompletionSource.</summary>
        internal TaskCompletionSource<TResult> CompletionSource
        {
            get
            {
                // Get and return the task. If there isn't one, first create one and store it.
                var tcs = m_taskCompletionSource;
                if (tcs == null)
                {
                    Contract.Assert(m_task == null, "Task should be null if TCS is null");
                    m_taskCompletionSource = tcs = new TaskCompletionSource<TResult>();
                    m_task = tcs.Task;
                }
                return tcs;
            }
        }

        /// <summary>Gets the <see cref="System.Threading.Tasks.Task{TResult}"/> for this builder.</summary>
        /// <returns>The <see cref="System.Threading.Tasks.Task{TResult}"/> representing the builder's asynchronous operation.</returns>
        public Task<TResult> Task
        {
            get
            {
                var tcs = CompletionSource;
                Contract.Assert(tcs != null && m_task != null, "Task should have been initialized.");
                return tcs.Task;
            }
        }

        /// <summary>
        /// Completes the <see cref="System.Threading.Tasks.Task{TResult}"/> in the 
        /// <see cref="System.Threading.Tasks.TaskStatus">RanToCompletion</see> state with the specified result.
        /// </summary>
        /// <param name="result">The result to use to complete the task.</param>
        /// <exception cref="System.InvalidOperationException">The task has already completed.</exception>
        public void SetResult(TResult result)
        {
            // Get the currently stored task, which will be non-null if get_Task has already been accessed.
            // If there isn't one, get a task and store it.
            var tcs = m_taskCompletionSource;
            if (tcs == null)
            {
                Contract.Assert(m_task == null, "Task should be null if TCS is null");
                m_taskCompletionSource = GetTaskForResult(result);
                Contract.Assert(m_taskCompletionSource != null, "GetTaskForResult should never return null");
                m_task = m_taskCompletionSource.Task;
            }
            // Slow path: complete the existing task.
            else if (!tcs.TrySetResult(result))
            {
                throw new InvalidOperationException("The Task was already completed.");
            }
        }

        /// <summary>
        /// Completes the builder by using either the supplied completed task, or by completing
        /// the builder's previously accessed task using default(TResult).
        /// </summary>
        /// <param name="completedTask">A task already completed with the value default(TResult).</param>
        /// <exception cref="System.InvalidOperationException">The task has already completed.</exception>
        internal void SetResult(TaskCompletionSource<TResult> completedTask)
        {
            Contract.Requires(completedTask != null, "Expected non-null task");
            Contract.Requires(completedTask.Task.Status == TaskStatus.RanToCompletion, "Expected a successfully completed task");

            // Get the currently stored task, which will be non-null if get_Task has already been accessed.
            // If there isn't one, store the supplied completed task.
            var tcs = m_taskCompletionSource;
            if (tcs == null)
            {
                Contract.Assert(m_task == null, "Task should be null if TCS is null");
                m_taskCompletionSource = completedTask;
                m_task = m_taskCompletionSource.Task;
            }
            // Otherwise, complete the task that's there.
            else
            {
                SetResult(default(TResult));
            }
        }

        /// <summary>
        /// Completes the <see cref="System.Threading.Tasks.Task{TResult}"/> in the 
        /// <see cref="System.Threading.Tasks.TaskStatus">Faulted</see> state with the specified exception.
        /// </summary>
        /// <param name="exception">The <see cref="System.Exception"/> to use to fault the task.</param>
        /// <exception cref="System.ArgumentNullException">The <paramref name="exception"/> argument is null (Nothing in Visual Basic).</exception>
        /// <exception cref="System.InvalidOperationException">The task has already completed.</exception>
        public void SetException(Exception exception)
        {
            if (exception == null) throw new ArgumentNullException("exception");
            Contract.EndContractBlock();

            // Get the task, forcing initialization if it hasn't already been initialized.
            var task = this.CompletionSource;

            // If the exception represents cancellation, cancel the task.  Otherwise, fault the task.
            var oce = exception as OperationCanceledException;
            bool successfullySet = oce != null ?
                task.TrySetCanceled() :
                task.TrySetException(exception);

            // Unlike with TaskCompletionSource, we do not need to spin here until m_task is completed,
            // since AsyncTaskMethodBuilder.SetException should not be immediately followed by any code
            // that depends on the task having completely completed.  Moreover, with correct usage, 
            // SetResult or SetException should only be called once, so the Try* methods should always
            // return true, so no spinning would be necessary anyway (the spinning in TCS is only relevant
            // if another thread won the race to complete the task).

            if (!successfullySet)
            {
                throw new InvalidOperationException("The Task was already completed.");
            }
        }

        /// <summary>
        /// Called by the debugger to request notification when the first wait operation
        /// (await, Wait, Result, etc.) on this builder's task completes.
        /// </summary>
        /// <param name="enabled">
        /// true to enable notification; false to disable a previously set notification.
        /// </param>
        /// <remarks>
        /// This should only be invoked from within an asynchronous method,
        /// and only by the debugger.
        /// </remarks>
        internal void SetNotificationForWaitCompletion(bool enabled)
        {
            // Nop in the compat lib
        }

        /// <summary>
        /// Gets an object that may be used to uniquely identify this builder to the debugger.
        /// </summary>
        /// <remarks>
        /// This property lazily instantiates the ID in a non-thread-safe manner.  
        /// It must only be used by the debugger, and only in a single-threaded manner
        /// when no other threads are in the middle of accessing this property or this.Task.
        /// </remarks>
        private object ObjectIdForDebugger { get { return this.Task; } }

        /// <summary>
        /// Gets a task for the specified result.  This will either
        /// be a cached or new task, never null.
        /// </summary>
        /// <param name="result">The result for which we need a task.</param>
        /// <returns>The completed task containing the result.</returns>
        private TaskCompletionSource<TResult> GetTaskForResult(TResult result)
        {
            //Contract.Ensures(
            //   EqualityComparer<TResult>.Default.Equals(result, Contract.Result<Task<TResult>>().Result),
            //    "The returned task's Result must return the same value as the specified result value.");
            var cache = AsyncMethodTaskCache<TResult>.Singleton;
            return cache != null ?
                cache.FromResult(result) :
                AsyncMethodTaskCache<TResult>.CreateCompleted(result);
        }
    }
}
